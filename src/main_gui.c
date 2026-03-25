/**
 * @file main_gui.c
 * @brief Lance l’interface GTK de Krojanty (local ou réseau, avec IA optionnelle).
 *
 * Idée générale (simple) :
 *  - On lit les options de la ligne de commande (local / serveur / client, IA, profondeur).
 *  - On prépare l’état réseau (si besoin) et l’état du jeu (plateau + GameState).
 *  - On démarre la fenêtre GTK avec la bonne UI :
 *      - @ref ui_run_gui() pour le local (2 joueurs au même PC),
 *      - @ref ui_run_gui_network() pour le réseau (client/serveur) avec IA locale optionnelle.
 *
 * Pourquoi cette structure ?
 *  - Séparer le parsing des arguments (propre et lisible).
 *  - Séparer la configuration réseau (host/port, qui commence).
 *  - Laisser toute la logique d’affichage/interaction à ui_gtk.c.
 */

#include <gtk/gtk.h>
#include "ui_gtk.h"
#include "game.h"
#include "net.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ai.h"

/**
 * @brief Placement initial identique au mode CLI.
 *
 * On garde un layout “lisible” en ASCII pour tester rapidement.
 * Si ce parsing échoue, on repasse au placement par défaut via
 * @ref board_place_initial().
 */
static const char* LAYOUT_IMAGE[9] = {
    ". . S S . . . . .",
    ". K S S . . . . .",
    "S S S . . . . . .",
    "S S . . . . . . .",
    ". . . . . . . . .",
    ". . . . . . . s s",
    ". . . . . . s s s",
    ". . . . . s s k .",
    ". . . . . s s . ."
};

/* ====================================================================== */
/*                      1) Variables globales (CLI)                       */
/* ====================================================================== */

/**
 * @brief Mode choisi (local, serveur, client).
 *
 * Par défaut : LOCAL (plus simple).
 */
static GameMode  g_mode       = GAME_MODE_LOCAL;

/**
 * @brief Hôte serveur pour le mode client (défaut 127.0.0.1).
 */
static char      g_host[256]  = "127.0.0.1";

/**
 * @brief Port réseau (défaut 8080).
 */
static uint16_t  g_port       = 8080;

/**
 * @brief Active l’IA locale côté GUI réseau (0 = non, 1 = oui).
 *
 * Remarque : en mode LOCAL, on ne pilote pas par IA via ce fichier :
 * l’IA locale est gérée surtout dans la version “réseau” de l’UI
 * (@ref ui_run_gui_network) car on simule un “côté local piloté par IA”.
 */
static int       g_ai_enabled = 0;

/**
 * @brief Profondeur de recherche IA (défaut 3).
 *
 * Plus grand = plus “fort/long”. On reste raisonnable pour garder la GUI réactive.
 */
static int       g_ai_depth   = 3;

/**
 * @brief Affiche l’aide en ligne (simple et directe).
 * @param prog Nom du programme.
 */
static void usage(const char* prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s                    # GUI avec sélection de mode\n"
        "  %s -l                 # Local (2 joueurs)\n"
        "  %s -s [port]          # Serveur (défaut 8080)\n"
        "  %s -c <host:port>     # Client\n"
        "  %s -s -ia [port]      # Serveur avec IA locale\n"
        "  %s -c -ia <host:port> # Client avec IA locale\n"
        "  %s -ia-depth N        # (optionnel) profondeur IA (défaut 3)\n",
        prog, prog, prog, prog, prog, prog, prog);
}

/* ====================================================================== */
/*                   2) Petits helpers pour le parsing                     */
/* ====================================================================== */

/**
 * @brief Vérifie qu’une chaîne est un entier 16 bits non signé.
 * @param s chaîne (ex: "8080")
 * @return 1 si OK, 0 sinon.
 *
 * Pourquoi ? Pour ne pas planter si l’utilisateur tape n’importe quoi.
 */
static int is_uint16(const char* s){
    if(!s || !*s) return 0;
    for(const char* p=s; *p; ++p) if(*p<'0'||*p>'9') return 0;
    long v = strtol(s, NULL, 10);
    return v>0 && v<=65535;
}

/**
 * @brief Parse une chaîne "host:port" en host + port.
 * @param hp     Ex: "127.0.0.1:8080"
 * @param host   Buffer de sortie pour l’hôte.
 * @param hostsz Taille du buffer hôte.
 * @param port   Sortie du port.
 * @return 1 si OK, 0 sinon.
 *
 * Pourquoi ? Pour accepter des paramètres compacts côté client.
 */
static int parse_host_port(const char* hp, char* host, size_t hostsz, uint16_t* port){
    const char* c = strrchr(hp, ':'); if(!c) return 0;
    size_t hl = (size_t)(c-hp); if(hl==0 || hl>=hostsz) return 0;
    memcpy(host, hp, hl); host[hl]='\0';
    if(!is_uint16(c+1)) return 0;
    *port = (uint16_t)atoi(c+1);
    return 1;
}

/* ====================================================================== */
/*                 3) Parsing robuste de la ligne de commande              */
/* ====================================================================== */

/**
 * @brief Parse les arguments (accepte -s -ia 5555 ou -c -ia host:port).
 *
 * Idée :
 *  - On passe une fois sur argv.
 *  - On mémorise les flags (-l / -s / -c / -ia / -ia-depth).
 *  - Pour les arguments positionnels (port ou host:port), on se base sur
 *    le mode courant pour savoir comment les lire.
 *
 * @return 0 si OK, 1 si erreur (et on affiche l’usage).
 */
static int parse_argv(int argc, char** argv){
    int have_server_port = 0;
    int have_client_hp   = 0;

    for (int i=1;i<argc;i++){
        const char* a = argv[i];

        if (!strcmp(a,"-h") || !strcmp(a,"--help")) { usage(argv[0]); return 1; }
        else if (!strcmp(a,"-ia")) { g_ai_enabled = 1; }
        else if (!strcmp(a,"-ia-depth")) {
            if (i+1>=argc){ fprintf(stderr,"-ia-depth N\n"); return 1; }
            g_ai_depth = atoi(argv[++i]); if(g_ai_depth<1) g_ai_depth=1;
        }
        else if (!strcmp(a,"-l")) { g_mode = GAME_MODE_LOCAL; }
        else if (!strcmp(a,"-s")) { g_mode = GAME_MODE_SERVER; }
        else if (!strcmp(a,"-c")) { g_mode = GAME_MODE_CLIENT; }
        else if (a[0] != '-') {
            /* Argument positionnel : dépend du mode courant */
            if (g_mode == GAME_MODE_SERVER && !have_server_port && is_uint16(a)) {
                g_port = (uint16_t)atoi(a);
                have_server_port = 1;
            } else if (g_mode == GAME_MODE_CLIENT && !have_client_hp) {
                if (!parse_host_port(a, g_host, sizeof g_host, &g_port)) {
                    fprintf(stderr,"Format invalide (host:port) : %s\n", a);
                    return 1;
                }
                have_client_hp = 1;
            } else {
                fprintf(stderr,"Option inconnue: %s\n", a);
                usage(argv[0]);
                return 1;
            }
        }
        else {
            fprintf(stderr,"Option inconnue: %s\n", a);
            usage(argv[0]);
            return 1;
        }
    }

    /* Valeurs par défaut si nécessaires */
    if (g_mode == GAME_MODE_SERVER && !have_server_port) g_port = 8080;
    if (g_mode == GAME_MODE_CLIENT && !have_client_hp) {
        /* Autoriser -c sans host:port explicite -> on prend localhost:8080 */
        strncpy(g_host, "127.0.0.1", sizeof g_host - 1);
        g_port = 8080;
    }
    return 0;
}

/* ====================================================================== */
/*                    4) Lancement de l’UI selon le mode                  */
/* ====================================================================== */

/**
 * @brief Configure le réseau + état du jeu, puis lance la bonne UI GTK.
 *
 * Détails réseau (simple) :
 *  - Serveur (ROUGE) joue 2e -> is_my_turn = 0.
 *  - Client (BLEU) joue 1er -> is_my_turn = 1.
 *  - En local, on n’a pas de vraie connexion, mais on marque connected=1.
 *
 * IA locale :
 *  - Si `-ia` est passé, on autorise la GUI réseau à faire jouer l’IA
 *    côté local (client ou serveur) avec profondeur `g_ai_depth`.
 *
 * @param app       Instance GTK.
 * @param user_data Non utilisé.
 */
static void launch_mode(GtkApplication *app, gpointer user_data) {
    (void)user_data;

    /* 1) Préparer l’état réseau en fonction du mode choisi */
    NetworkState *net_state = g_new0(NetworkState, 1);
    net_state->mode = g_mode;

    if (g_mode == GAME_MODE_SERVER){
        net_state->port = g_port;
        net_state->is_my_turn = 0;     /* Serveur = ROUGE = 2e */
        net_state->connected = 0;      /* en attente du client */
        fprintf(stderr, "./game -s%s %u : exécution en réseau%s\n",
                g_ai_enabled? " -ia":"", (unsigned)g_port,
                g_ai_enabled? " avec IA":" sans IA");
    } else if (g_mode == GAME_MODE_CLIENT){
        strncpy(net_state->host, g_host, sizeof(net_state->host)-1);
        net_state->host[sizeof(net_state->host)-1]='\0';
        net_state->port = g_port;
        net_state->is_my_turn = 1;     /* Client = BLEU = 1er */
        net_state->connected = 0;      /* connecté plus tard */
        fprintf(stderr, "./game -c%s %s:%u : exécution en réseau%s\n",
                g_ai_enabled? " -ia":"", g_host, (unsigned)g_port,
                g_ai_enabled? " avec IA":" sans IA");
    } else {
        /* Mode local : pas de vrai réseau, mais on force “connecté” */
        net_state->connected = 1;
        net_state->is_my_turn = 1;
    }

    /* 2) Préparer l’état du jeu (GameState + Board) */
    Board b; 
    GameState gs; 
    game_init_state(&gs, 64, &b, g_ai_enabled);   /* 64 tours max */
    
    // Placement initial
    if (!board_place_from_ascii(&b, &gs, LAYOUT_IMAGE)) {
        board_place_initial(&b, &gs);
    }

    // Marquer toutes les cases contenant un pion comme contrôlées dès le départ
    game_init_control(&b, &gs);

    /* 3) Lancer l’UI adaptée */
    if (g_mode == GAME_MODE_LOCAL){
        ui_run_gui(app, &b, &gs);
    } else {
        ui_run_gui_network(app, &b, &gs, net_state, g_ai_enabled, g_ai_depth);
    }
}


/**
 * @brief Callback “activate” de GTK : on lance simplement notre mode.
 *
 * Pourquoi ce wrapper ? Parce que GTK déclenche “activate” après init,
 * donc on sépare la logique de lancement du parsing des arguments.
 */
static void activate(GtkApplication *app, gpointer user_data){
    (void)user_data;
    launch_mode(app, NULL);
}

/* ====================================================================== */
/*                              5) main()                                  */
/* ====================================================================== */

/**
 * @brief Point d’entrée GUI : parse les options puis lance GTK.
 *
 * Détail technique : on passe seulement argv[0] à GTK (le reste est déjà
 * consommé par notre parseur), pour éviter que GTK essaye de lire nos
 * options `-s`, `-c`, etc.
 */
int main(int argc, char** argv){
    if (parse_argv(argc, argv)) return 1;

    /* Laisser GTK consommer uniquement argv[0] (évite l’interception de nos options) */
    char* gtk_argv[2] = { argv[0], NULL };
    GtkApplication *app = gtk_application_new("org.krojanty.app", G_APPLICATION_NON_UNIQUE);

    g_signal_connect(app, "activate", G_CALLBACK(launch_mode), NULL);
    int status = g_application_run(G_APPLICATION(app), 1, gtk_argv);
    g_object_unref(app);
    return status;
}