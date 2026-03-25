/**
 * @file net.c
 * @brief Gestion du réseau (TCP) pour le jeu Krojanty.
 * 
 * @author marcelin.trag@uha.fr
 * @date 2025-09-16
 * @version 1.0.2
 *
 * Ce fichier definit les fonctions qui permettent la gestion du reseau:
 *   - de creer un serveur qui attend une connexion,
 *   - de se connecter a un serveur comme client,
 *   - d'envoyer et de recevoir des donnees via TCP,
 *   - de fermer proprement la connexion.
 *
 * On utilise ici des sockets POSIX (standard Linux/Unix).
 * Les echanges de coups se font avec un protocole fixe defini etre les equipes.
 *   - Nous utilisons UTF-8 pour les caracteres.
 *   - Les coups sont envoyes sous la forme de 4 octects (ex: "A1A3" deplacement de A1 vers A3).
 *   - Nous attendons 1 seul message par tour.
 *   - Le J1 est le client et le J2 est le serveur.
 */

#include "net.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

/* ===== serveur ===== */

/**
 * @brief Crée et démarre un socket serveur TCP sur le port spécifié.
 *
 * Cette fonction initialise un socket IPv4 (AF_INET) en mode TCP (SOCK_STREAM),
 * configure l’option SO_REUSEADDR pour permettre la réutilisation rapide du port,
 * lie le socket à toutes les interfaces réseau locales (INADDR_ANY) et
 * place le socket en mode écoute pour accepter des connexions entrantes.
 *
 * @details
 * Exemple d’utilisation :
 * @code
 *   int listen_fd = net_server_listen(8080);
 *   if (listen_fd < 0) {
 *       // Gestion d’erreur
 *   }
 *   // Attendre une connexion : int client_fd = net_server_accept(listen_fd);
 * @endcode
 *
 * @param port Numéro de port TCP sur lequel écouter (ex : 8080).
 * @return
 *   - Descripteur de socket d’écoute (>=0) en cas de succès.
 *   - -1 en cas d’erreur (échec de création, de liaison ou d’écoute).
 *
 * @note
 *   - Le socket retourné doit être fermé avec @c close() après utilisation.
 *   - Utilisez @ref net_server_accept pour accepter une connexion entrante.
 *   - Cette fonction ne gère que les sockets IPv4.
 */
int net_server_listen(uint16_t port){
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd<0) return -1;

    int yes=1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in sin;
    memset(&sin,0,sizeof sin);
    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port        = htons(port);

    if(bind(fd,(struct sockaddr*)&sin,sizeof sin)<0){ close(fd); return -1; }
    if(listen(fd, 5)<0){ close(fd); return -1; }
    return fd;
}

/**
 * @brief Accepte une connexion entrante sur un socket serveur TCP.
 *
 * Cette fonction attend (bloque) jusqu'à ce qu'un client tente de se connecter
 * au socket serveur spécifié par @p listen_fd. Lorsqu'une connexion entrante
 * est détectée, elle est acceptée et un nouveau descripteur de socket est
 * retourné, permettant la communication avec ce client.
 *
 * @details
 * - Le socket @p listen_fd doit avoir été créé et mis en écoute via
 *   @ref net_server_listen.
 * - Cette fonction est bloquante tant qu'aucun client ne se connecte.
 * - Le descripteur retourné doit être fermé avec @c close() après utilisation.
 * - Les informations sur le client distant ne sont pas récupérées ici
 *   (NULL passé à @c accept).
 *
 * @param listen_fd Descripteur du socket serveur en écoute (retourné par @ref net_server_listen).
 * @return
 *   - Descripteur du socket connecté (>=0) en cas de succès.
 *   - -1 en cas d’erreur (échec d’acceptation ou @p listen_fd invalide).
 *
 * @code
 *   int listen_fd = net_server_listen(8080);
 *   int client_fd = net_server_accept(listen_fd);
 *   if (client_fd < 0) {
 *       // Gestion d'erreur
 *   }
 *   // Utiliser client_fd pour communiquer avec le client
 * @endcode
 */
int net_server_accept(int listen_fd){
    return accept(listen_fd, NULL, NULL);
}

/* ===== client ===== */

/**
 * @brief Crée un socket client et établit une connexion TCP vers un serveur distant.
 *
 * Cette fonction permet à un client de se connecter à un serveur TCP en utilisant
 * soit une adresse IP, soit un nom d’hôte (résolution DNS automatique si nécessaire).
 *
 * ### Détail des étapes :
 *   1. Création d’un socket TCP (AF_INET, SOCK_STREAM).
 *   2. Préparation de la structure sockaddr_in avec la famille d’adresses, le port (en réseau),
 *      et l’adresse IP du serveur.
 *   3. Si @p host est une adresse IP (ex: "192.168.1.10"), conversion directe avec inet_pton().
 *      Sinon, tentative de résolution DNS via gethostbyname().
 *   4. Connexion au serveur avec connect().
 *   5. En cas d’échec à n’importe quelle étape, le socket est fermé et -1 est retourné.
 *
 * @param host Nom d’hôte (ex: "localhost", "serveur.exemple.com") ou adresse IPv4 (ex: "192.168.1.10").
 * @param port Numéro de port TCP du serveur (ex: 8080).
 * @return
 *   - Descripteur de socket connecté (>=0) en cas de succès.
 *   - -1 en cas d’erreur (échec de création, résolution ou connexion).
 *
 * @note
 *   - Le descripteur retourné doit être fermé avec @c close() après utilisation.
 *   - Cette fonction ne gère que l’IPv4.
 *   - La résolution DNS utilise gethostbyname() (non thread-safe, mais suffisant ici).
 *
 * @code
 *   int fd = net_client_connect("127.0.0.1", 8080);
 *   if (fd < 0) {
 *       // Gestion d’erreur
 *   } else {
 *       // Utiliser fd pour communiquer avec le serveur
 *   }
 * @endcode
 */
int net_client_connect(const char* host, uint16_t port){
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd<0) return -1;

    struct sockaddr_in sin;
    memset(&sin,0,sizeof sin);
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(port);

    // Tente de convertir l’hôte en adresse IP directe (ex: "192.168.1.10")
    if (inet_pton(AF_INET, host, &sin.sin_addr) <= 0){
        // Si ce n’est pas une IP, on tente une résolution DNS
        struct hostent* he = gethostbyname(host);
        if(!he){ close(fd); return -1; }
        memcpy(&sin.sin_addr, he->h_addr_list[0], he->h_length);
    }

    // Tente la connexion au serveur
    if(connect(fd,(struct sockaddr*)&sin,sizeof sin)<0){ close(fd); return -1; }
    return fd;
}

/* ===== I/O brutes ===== */

/**
 * @brief Envoie exactement len octets sur le socket fd.
 *
 * Utilise send en boucle pour garantir l’envoi complet,
 * même si send n’envoie qu’une partie à chaque appel.
 *
 * @param fd  Socket TCP déjà connecté.
 * @param buf Pointeur vers les données à envoyer.
 * @param len Nombre d’octets à envoyer.
 * @return len si tout a été envoyé, -1 en cas d’erreur.
 */
ssize_t net_send_all(int fd, const void* buf, size_t len){
    const char* p = (const char*)buf;
    size_t left = len;
    while(left > 0){
        ssize_t n = send(fd, p, left, 0);
        if(n < 0){
            if(errno == EINTR) continue;
            return -1;
        }
        if(n == 0) break;
        p += n;
        left -= n;
    }
    return (ssize_t)len;
}

/**
 * @brief Reçoit exactement `len` octets depuis un socket TCP, sauf en cas de fermeture ou d’erreur.
 *
 * Cette fonction garantit la lecture de la totalité des `len` octets demandés, même si `recv`
 * ne lit qu’une partie des données à chaque appel (ce qui est courant en réseau).
 * Elle effectue plusieurs appels à `recv` si nécessaire, jusqu’à ce que :
 *   - tous les octets aient été reçus (succès),
 *   - la connexion soit fermée proprement par l’autre extrémité (retourne 0),
 *   - ou qu’une erreur irrécupérable survienne (retourne -1).
 *
 * Si la connexion est interrompue par un signal (`EINTR`), la lecture reprend automatiquement.
 *
 * @param fd  Descripteur du socket connecté (doit être valide).
 * @param buf Pointeur vers le buffer où stocker les données reçues (doit être alloué pour au moins `len` octets).
 * @param len Nombre d’octets à recevoir (doit être > 0).
 * @return
 *   - `len` si tous les octets ont été reçus avec succès,
 *   - 0 si la connexion a été fermée proprement (EOF) avant d’avoir tout reçu,
 *   - -1 en cas d’erreur (voir errno pour le détail).
 *
 * @note Cette fonction est utile pour les protocoles où la taille des messages est fixe ou connue à l’avance.
 */
ssize_t net_recv_all(int fd, void* buf, size_t len){
    char* p = (char*)buf;
    size_t left = len;
    while(left > 0){
        ssize_t n = recv(fd, p, left, 0);
        if(n < 0){
            if(errno == EINTR) continue; // Interruption par signal, on recommence
            return -1;                   // Autre erreur
        }
        if(n == 0) return 0; // Connexion fermée (EOF)
        p += n;
        left -= n;
    }
    return (ssize_t)len;
}

/* ===== Protocole 4 octets ===== */

/**
 * @brief Envoie exactement 4 octets sur le socket (ex: "A1A3").
 *
 * Cette fonction garantit l’envoi complet d’un message de 4 caractères,
 * typiquement utilisé pour transmettre un coup ou une commande courte.
 * Elle vérifie que le pointeur n’est pas NULL et que le socket est valide.
 *
 * @param fd   Descripteur du socket connecté.
 * @param four Tableau de 4 caractères à envoyer (non NULL).
 * @return 0 en cas de succès, -1 en cas d’erreur.
 */
int net_send4(int fd, const char four[4]){
    if(fd < 0 || !four) return -1;
    ssize_t sent = net_send_all(fd, four, 4);
    return (sent == 4) ? 0 : -1;
}

/**
 * @brief Reçoit exactement 4 octets depuis un socket connecté.
 *
 * Cette fonction lit 4 octets sur le socket spécifié, ce qui correspond à un coup du jeu
 * (ex: "A1A3" pour un déplacement) ou à une commande spéciale de 4 caractères
 * (ex: "QUIT", "RSET"). Elle garantit que le buffer de sortie est rempli uniquement si
 * la lecture a réussi pour les 4 octets demandés.
 *
 * @param fd  Descripteur du socket connecté (doit être valide).
 * @param out Buffer de sortie (tableau de 4 caractères, non NULL) où seront stockés les octets reçus.
 * @return
 *   - 1 si les 4 octets ont été reçus avec succès,
 *   - 0 si la connexion a été fermée proprement avant la réception complète (EOF),
 *   - -1 en cas d’erreur (voir errno pour le détail).
 *
 * @note Cette fonction ne termine pas la chaîne reçue par un caractère nul ('\0').
 *       Si vous souhaitez manipuler le résultat comme une chaîne C, pensez à ajouter un '\0' après.
 */
int net_recv4(int fd, char out[4]){
    ssize_t n = net_recv_all(fd, out, 4);
    if(n < 0) return -1;
    if(n == 0) return 0; // Connexion fermée (EOF)
    return 1;            // Succès
}

/* ===== utils ===== */

/**
 * @brief Ferme proprement un socket réseau (TCP).
 *
 * Cette fonction encapsule l’appel système @c close() pour fermer un descripteur de socket,
 * en vérifiant que le descripteur est valide (>=0) avant de procéder.
 *
 * @details
 * - Si @p fd est négatif, la fonction ne fait rien.
 * - Si @p fd est valide, la connexion réseau associée est proprement fermée et le descripteur libéré.
 * - Après appel, le descripteur ne doit plus être utilisé.
 *
 * @param fd Descripteur du socket à fermer (doit être >= 0 pour être effectif).
 *
 * @note
 *   - Fermer un socket libère les ressources associées côté système.
 *   - Il est recommandé d’appeler cette fonction pour tout socket ouvert (client ou serveur)
 *     dès qu’il n’est plus utilisé.
 */
void net_close(int fd){
    if(fd>=0) close(fd);
}