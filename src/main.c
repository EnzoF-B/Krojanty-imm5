/**
 * @file main.c
 * @brief Programme principal en mode console pour Krojanty (local, réseau, IA).
 *
 * Ce fichier lance le jeu en console, soit :
 *  - en local (2 humains ou 1 humain + IA),
 *  - en réseau (client/serveur) avec protocole simple sur 4 octets par coup,
 *  - avec IA (Minimax/Alpha-Beta) quand on passe l’option -ia.
 *
 * Idée simple :
 *  - On prépare un plateau de départ.
 *  - On affiche l’état (tour, joueur, points, captures, rois).
 *  - À chaque tour : soit l’humain saisit un coup, soit l’IA choisit un coup.
 *  - En réseau : on envoie/reçoit le coup au format 4 octets (ex: "B8B5").
 *
 * Sections dans ce fichier :
 *  1) Helpers I/O et affichage de l’état.
 *  2) Helpers de protocole réseau (4 caractères par coup).
 *  3) Un tour local (humain/IA) + envoi réseau.
 *  4) Un tour distant (réception réseau).
 *  5) Modes d’exécution : local / serveur / client.
 *  6) Parsing des arguments et main().
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include "board.h"
#include "move.h"
#include "rules.h"
#include "game.h"
#include "gen.h"
#include "net.h"
#include "ai.h"

/**
 * @brief Placement initial “image” (rang 9 vers 1).
 *
 * On définit ici un plateau de départ lisible (facultatif).
 * Si jamais le parsing échoue, on prendra le placement par défaut
 * via @ref board_place_initial().
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

/**
 * @brief Supprime les espaces de fin de chaîne (petit utilitaire).
 * @param s Chaîne modifiable. On marche depuis la fin et on coupe.
 */
static void trim(char* s){ int n=(int)strlen(s); while(n>0 && isspace((unsigned char)s[n-1])) s[--n]=0; }

/* ------------------------------------------------------------------------- */
/* 1) I/O console : afficher l’état complet                                   */
/* ------------------------------------------------------------------------- */

/**
 * @brief Affiche le plateau et les infos de partie d’une manière compacte.
 *
 * On montre :
 *  - le plateau ASCII,
 *  - le numéro de tour et le joueur qui doit jouer,
 *  - les points, les captures, la vie des rois,
 *  - et si la partie est finie (gagnant ou nul).
 *
 * @param b  Plateau courant.
 * @param gs État courant (scores, captures…).
 * @param gr Résultat actuel (ongoing, win bleu, win rouge, nul).
 */
static void print_status(const Board* b, const GameState* gs, GameResult gr){
    board_print_ascii(b);
    printf("Tour:%d — Joueur:%s | Pts(B:%d,R:%d) | Capt(B:%d,R:%d) | Rois(B:%d,R:%d)\n",
        b->turn_number, (b->current_player==PLAYER_BLUE?"BLEU":"ROUGE"),
        gs->blue_points, gs->red_points,
        gs->blue_soldiers_captured, gs->red_soldiers_captured,
        gs->blue_king_alive, gs->red_king_alive);
    if(gr==GAME_WIN_BLUE) puts("Je (bleu) gagne");
    else if(gr==GAME_WIN_RED) puts("Je (rouge) gagne");
    else if(gr==GAME_DRAW) puts("Match nul");
}

/* ------------------------------------------------------------------------- */
/* 2) Protocole réseau 4 octets : "A1A3"                                     */
/* ------------------------------------------------------------------------- */

/**
 * @brief Convertit un @ref Move vers 4 caractères (ex: "B8B5").
 * @param m   Coup (from->to).
 * @param out Buffer de 4 chars (pas de '\0' ajouté).
 *
 * Format :
 *   - out[0] = colonne A..I du départ
 *   - out[1] = rang 1..9 du départ (9 = haut visuel)
 *   - out[2] = colonne A..I d’arrivée
 *   - out[3] = rang 1..9 d’arrivée
 */
static void move_to_4chars(Move m, char out[4]){
    out[0] = (char)('A' + m.from.x);
    out[1] = (char)('0' + (9 - m.from.y));
    out[2] = (char)('A' + m.to.x);
    out[3] = (char)('0' + (9 - m.to.y));
}

/**
 * @brief Parse 4 caractères "A1A3" en @ref Move.
 * @param in  4 caractères (pas besoin de '\0').
 * @param out Décode le @ref Move si OK.
 * @return 1 si OK, 0 sinon.
 */
static int move_from_4chars(const char in[4], Move* out){
    if(in[0]<'A' || in[0]>'I') return 0;
    if(in[2]<'A' || in[2]>'I') return 0;
    if(in[1]<'1' || in[1]>'9') return 0;
    if(in[3]<'1' || in[3]>'9') return 0;
    char tmp[8];
    snprintf(tmp,sizeof tmp,"%c%c %c%c", in[0], in[1], in[2], in[3]);
    return move_from_str(tmp, out);
}

/* ------------------------------------------------------------------------- */
/* 3) Un tour local : humain ou IA, puis envoi réseau éventuel               */
/* ------------------------------------------------------------------------- */

/**
 * @brief Joue un tour local (humain ou IA). Envoie les 4 octets si réseau.
 *
 * Logique :
 *  - Si IA activée : on appelle @ref ai_choose_move(). Sinon, on lit la saisie.
 *  - On applique le coup avec @ref apply_move().
 *  - Si connexion réseau : envoi des 4 octets via @ref net_send4().
 *  - On affiche l’état et on gère la fin de partie (envoie aussi "QUIT").
 *
 * @param b           Plateau (modifié si coup légal).
 * @param gs          État de partie (modifié).
 * @param netfd       Descripteur réseau ou -1 si pas de réseau.
 * @param ai_enabled  1 = IA joue ce côté, 0 = humain.
 * @param my_side     Le camp qui joue (BLEU/ROUGE).
 * @param aicfg       Paramètres de l’IA (profondeur, alpha-beta).
 * @return 0 si un coup a été joué et la partie continue,
 *         1 si la partie s’arrête,
 *        -1 si erreur (réseau ou coup IA illégal).
 */
static int do_local_turn_send4(Board* b, GameState* gs, int netfd,
                               Player my_side){
    while(1){
        Move m; char s[6];
        if (gs->UseAI){
            int score=0;
            if(!ai_choose_move(b, gs, my_side, &m, &score)){
                puts("IA: pas de coup légal — arrêt");
                return -1;
            }
            move_to_str(m, s);
            if (my_side==PLAYER_BLUE){
                printf("Je (bleu) joue : %s\n", s);
                printf("Je (bleu) envoie : %s\n", s);
            }else{
                printf("Je (rouge) joue : %s\n", s);
                printf("Je (rouge) envoie : %s\n", s);
            }
        } else {
            printf("Votre coup (%s): ", (b->current_player==PLAYER_BLUE?"BLEU":"ROUGE"));
            fflush(stdout);
            char line[128];
            if(!fgets(line,sizeof line, stdin)) return -1;
            trim(line); if(!line[0]) continue;
            if(!strcmp(line,"board")){ GameResult t=check_victory(b,gs); print_status(b,gs,t); continue; }
            if(!move_from_str(line,&m)){ puts("Format invalide. Ex: B8 B5"); continue; }
            move_to_str(m, s);
            if (my_side==PLAYER_BLUE){
                printf("Je (bleu) joue : %s\n", s);
                printf("Je (bleu) envoie : %s\n", s);
            }else{
                printf("Je (rouge) joue : %s\n", s);
                printf("Je (rouge) envoie : %s\n", s);
            }
        }

        GameResult gr=GAME_ONGOING;
        if(!apply_move(b,gs,m,&gr)){
            puts("Coup illégal.");
            if(gs->UseAI) return -1;
            continue; /* re-saisie humain */
        }
        char four[4]; move_to_4chars(m, four);
        if(netfd>=0 && net_send4(netfd, four)<0){ puts("Réseau: erreur envoi."); return -1; }

        GameResult cur=gr; print_status(b,gs,cur);
        if(cur!=GAME_ONGOING){
            if(netfd>=0){
                const char quit[4] = {'Q','U','I','T'};
                net_send4(netfd, quit);
            }
            return 1; // fin
        }
        return 0; // coup OK
    }
}

/* ------------------------------------------------------------------------- */
/* 4) Un tour distant : on reçoit 4 octets et on applique                    */
/* ------------------------------------------------------------------------- */

/**
 * @brief Reçoit le coup de l’adversaire (4 octets) et l’applique.
 *
 * Logique :
 *  - Lecture avec @ref net_recv4().
 *  - Gestion du message spécial "QUIT" (fin volontaire).
 *  - Parsing du coup et @ref apply_move().
 *  - Affichage de l’état et détection de fin.
 *
 * @param b        Plateau (modifié).
 * @param gs       État de partie (modifié).
 * @param netfd    Descripteur réseau du pair.
 * @param opp_side Le camp de l’adversaire (BLEU/ROUGE), juste pour les logs.
 * @return 0 si la partie continue, 1 si fin (victoire/nul/QUIT), -1 si erreur.
 */
static int do_remote_turn_recv4(Board* b, GameState* gs, int netfd, Player opp_side){
    char four[4];
    int rr = net_recv4(netfd, four);
    if(rr<=0){ puts("Réseau : connexion fermée"); return -1; }
    if(four[0]=='Q' && four[1]=='U' && four[2]=='I' && four[3]=='T'){
        puts("Fin de partie (pair).");
        return 1;
    }
    Move m;
    if(!move_from_4chars(four, &m)){
        printf("L'adversaire (%s) a joué un message invalide - arrêt\n",
               (opp_side==PLAYER_BLUE?"bleu":"rouge"));
        return -1;
    }
    char s[6]; move_to_str(m, s);
    if (opp_side==PLAYER_BLUE)
        printf("L'adversaire (bleu) propose : %s\n", s);
    else
        printf("L'adversaire (rouge) propose : %s\n", s);

    GameResult gr=GAME_ONGOING;
    if(!apply_move(b,gs,m,&gr)){
        printf("La proposition %s est illégale - arrêt\n",
               (opp_side==PLAYER_BLUE?"bleu":"rouge"));
        return -1;
    }
    if (opp_side==PLAYER_BLUE) puts("La proposition bleu est validée");
    else                        puts("La proposition rouge est validée");

    GameResult cur=gr; print_status(b,gs,cur);
    if(cur!=GAME_ONGOING) return 1;
    return 0;
}

/* ------------------------------------------------------------------------- */
/* 5) Modes d’exécution : local / serveur / client                           */
/* ------------------------------------------------------------------------- */

/**
 * @brief Mode local : 2 joueurs même clavier (ou IA si demandé).
 * @param ai_enabled 1 = l’IA joue à la place du joueur courant.
 *
 * Boucle simple : tant que la partie n’est pas finie, on fait des tours locaux.
 */
static void run_local(int ai_enabled){
    Board b; GameState gs; 
    if(!board_place_from_ascii(&b, LAYOUT_IMAGE)) board_place_initial(&b);
    if(ai_enabled){tt_init(); zobrist_init();};
    game_init_state(&gs,64, &b, ai_enabled);
    if(gs.UseAI == 1){gs.zobrist_key = zobrist_hash(b);}
    else{gs.zobrist_key = 0;}
    GameResult gr=check_victory(&b,&gs);
    print_status(&b,&gs,gr);
    puts("Mode local. Entrez des coups (ex: B8 B5).");

    while(gr==GAME_ONGOING){
        int r = do_local_turn_send4(&b,&gs,-1, b.current_player);
        if(r!=0) break;
        gr = check_victory(&b,&gs);
    }
}

/**
 * @brief Mode SERVEUR : on écoute, on accepte un client. On joue ROUGE (2ème).
 * @param port       Port TCP d’écoute.
 * @param ai_enabled 1 = l’IA joue le côté ROUGE.
 *
 * Boucle de jeu :
 *   1) Le client (BLEU) joue d’abord -> @ref do_remote_turn_recv4().
 *   2) Puis le serveur (ROUGE) joue -> @ref do_local_turn_send4().
 */
static void run_server(uint16_t port, int ai_enabled){
    int lfd = net_server_listen(port);
    if(lfd<0){ fprintf(stderr,"[srv] listen %u failed\n",port); return; }
    printf("[srv] écoute sur %u, en attente du client…\n", port);
    int cfd = net_server_accept(lfd);
    net_close(lfd);
    if(cfd<0){ fprintf(stderr,"[srv] accept failed\n"); return; }
    puts("[srv] client connecté.");

    Board b; GameState gs; 
    if(!board_place_from_ascii(&b, LAYOUT_IMAGE)) board_place_initial(&b);
    if(ai_enabled){tt_init(); zobrist_init();};
    game_init_state(&gs,64, &b, ai_enabled);
    if(gs.UseAI == 1){gs.zobrist_key = zobrist_hash(b);}
    else{gs.zobrist_key = 0;}
    GameResult gr=check_victory(&b,&gs);
    print_status(&b,&gs,gr);
    puts("Mode réseau SERVEUR (J2 ROUGE, vous jouez 2ème).");

    while(1){
        /* 1) J1 (client/bleu) joue d’abord */
        int r = do_remote_turn_recv4(&b,&gs,cfd, PLAYER_BLUE);
        if(r!=0){ if(r<0) puts("[srv] erreur."); break; }

        /* 2) À vous (ROUGE) - humain ou IA selon -ia */
        r = do_local_turn_send4(&b,&gs,cfd, PLAYER_RED);
        if(r!=0){ if(r<0) puts("[srv] erreur."); break; }
    }
    net_close(cfd);
}

/**
 * @brief Mode CLIENT : on se connecte au serveur. On joue BLEU (1er).
 * @param host       Adresse du serveur.
 * @param port       Port TCP du serveur.
 *
 * Boucle de jeu :
 *   1) Le client (BLEU) joue d’abord -> @ref do_local_turn_send4().
 *   2) Puis le serveur (ROUGE) répond -> @ref do_remote_turn_recv4().
 */
static void run_client(const char* host, uint16_t port, int ai_enabled){
    int fd = net_client_connect(host, port);
    if(fd<0){ fprintf(stderr,"[cli] connect %s:%u failed\n",host,port); return; }
    printf("[cli] connecté à %s:%u\n", host, port);

    Board b; GameState gs; 
    if(!board_place_from_ascii(&b, LAYOUT_IMAGE)) board_place_initial(&b);
    if(ai_enabled){tt_init(); zobrist_init();};
    game_init_state(&gs,64, &b, ai_enabled);
    if(gs.UseAI == 1){gs.zobrist_key = zobrist_hash(b);}
    else{gs.zobrist_key = 0;}
    GameResult gr=check_victory(&b,&gs);
    print_status(&b,&gs,gr);
    puts("Mode réseau CLIENT (J1 BLEU, vous jouez 1er).");

    while(1){
        /* 1) À vous (BLEU) d’abord — humain ou IA */
        int r = do_local_turn_send4(&b,&gs,fd, PLAYER_BLUE);
        if(r!=0){ if(r<0) puts("[cli] erreur."); break; }

        /* 2) Puis le serveur (ROUGE) */
        r = do_remote_turn_recv4(&b,&gs,fd, PLAYER_RED);
        if(r!=0){ if(r<0) puts("[cli] erreur."); break; }
    }
    net_close(fd);
}

/* ------------------------------------------------------------------------- */
/* 6) Parsing CLI et main()                                                  */
/* ------------------------------------------------------------------------- */

/**
 * @brief Affiche l’usage attendu, dans l’ordre imposé par le sujet.
 * @param prog Nom de l’exécutable.
 */
static void usage(const char* prog){
    fprintf(stderr,
        "Usage (ordre et écriture impératifs) :\n"
        "  %s -l                         # local sans IA\n"
        "  %s -s PPP                     # serveur sans IA\n"
        "  %s -c AAAA:PPP                # client sans IA\n"
        "  %s -s -ia PPP                 # serveur avec IA\n"
        "  %s -c -ia AAAA:PPP            # client avec IA\n",
        prog, prog, prog, prog, prog);
}

/**
 * @brief Point d’entrée : lit les arguments et lance le bon mode.
 *
 * Cas pris en charge (format strict) :
 *  - `-l` : local sans IA.
 *  - `-s PPP` : serveur sans IA.
 *  - `-c AAAA:PPP` : client sans IA.
 *  - `-s -ia PPP` : serveur avec IA.
 *  - `-c -ia AAAA:PPP` : client avec IA.
 *
 * @return 0 si OK, 1 si erreur d’arguments.
 */
int main(int argc, char** argv){
    if (argc == 2 && strcmp(argv[1], "-l")==0){
        run_local(0);
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "-s")==0){
        uint16_t port = (uint16_t)atoi(argv[2]);
        if(!port){ usage(argv[0]); return 1; }
        run_server(port, 0);
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "-c")==0){
        char host[128]; int port=0;
        const char* hp = argv[2];
        const char* colon = strrchr(hp, ':');
        if(!colon){ usage(argv[0]); return 1; }
        size_t hl = (size_t)(colon - hp);
        if(hl>=sizeof host){ fprintf(stderr,"host trop long\n"); return 1; }
        memcpy(host, hp, hl); host[hl]='\0';
        port = atoi(colon+1);
        if(port<=0 || port>65535){ usage(argv[0]); return 1; }
        run_client(host, (uint16_t)port, 0);
        return 0;
    }
    if (argc == 4 && strcmp(argv[1], "-s")==0 && strcmp(argv[2], "-ia")==0){
        uint16_t port = (uint16_t)atoi(argv[3]);
        if(!port){ usage(argv[0]); return 1; }
        puts("./game -s -ia PPP : exécution en réseau avec IA");
        run_server(port, 1);
        return 0;
    }
    if (argc == 4 && strcmp(argv[1], "-c")==0 && strcmp(argv[2], "-ia")==0){
        char host[128]; int port=0;
        const char* hp = argv[3];
        const char* colon = strrchr(hp, ':');
        if(!colon){ usage(argv[0]); return 1; }
        size_t hl = (size_t)(colon - hp);
        if(hl>=sizeof host){ fprintf(stderr,"host trop long\n"); return 1; }
        memcpy(host, hp, hl); host[hl]='\0';
        port = atoi(colon+1);
        if(port<=0 || port>65535){ usage(argv[0]); return 1; }
        printf("./game -c -ia %s : exécution en réseau avec IA\n", hp);
        run_client(host, (uint16_t)port, 1);
        return 0;
    }

    usage(argv[0]);
    return 1;
}