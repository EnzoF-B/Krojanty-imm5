/**
 * @file net.h
 * @brief Interface pour la gestion du réseau (connexion TCP, envoi/réception de données) pour le jeu Krojanty.
 *
 * @author marcelin.trag@uha.fr
 * @date 2025-09-16
 * @version 1.0.0
 *
 * Ce fichier déclare l'ensemble des fonctions nécessaires à la communication réseau du jeu :
 *   - Création et gestion d’un serveur TCP (écoute, acceptation de connexions entrantes)
 *   - Connexion à un serveur distant en tant que client
 *   - Envoi et réception fiables de données sur des sockets TCP, avec gestion des interruptions
 *   - Primitives spécialisées pour l’échange de messages de 4 octets, adaptés au protocole du jeu (ex : transmission des coups sous forme "A1A3")
 *
 * Le protocole réseau utilisé est volontairement simple : chaque coup ou commande est transmis sous forme d’un message de 4 octets, sans terminaison nulle.
 * Toutes les fonctions sont conçues pour garantir l’envoi ou la réception complète des données demandées, même en cas de lectures/écritures partielles ou d’interruptions système.
 *
 * Ce module vise à masquer la complexité des appels système POSIX et à fournir une API claire et robuste pour la couche réseau du projet.
 */

#ifndef NET_H
#define NET_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>   // ssize_t

/**
 * @struct NetConn
 * @brief Structure d'encapsulation pour une connexion TCP.
 *
 * Cette structure sert à représenter une connexion réseau active dans le jeu Krojanty.
 * Elle encapsule le descripteur de socket associé à la connexion TCP, permettant ainsi
 * de manipuler les connexions de façon plus abstraite et sûre dans le code applicatif.
 *
 * @var NetConn::fd
 * Descripteur de socket TCP (valeur >= 0 si valide, -1 si non initialisé ou fermé).
 * Ce champ doit être géré avec attention : il doit être fermé explicitement lors de la
 * libération de la connexion pour éviter toute fuite de ressources.
 *
 * Exemple d'utilisation :
 * @code
 * NetConn conn;
 * conn.fd = net_client_connect("127.0.0.1", 5555);
 * if (conn.fd >= 0) {
 *     // Utiliser conn.fd pour envoyer/recevoir des données
 * }
 * @endcode
 */
typedef struct {
    int fd; /**< Descripteur de socket TCP (>=0 si valide, -1 sinon). */
} NetConn;

/* ---- serveur ---- */

/**
 * @brief Ouvre un socket TCP serveur et commence à écouter sur le port spécifié.
 *
 * @param port Port TCP à écouter (ex : 5555).
 * @return Descripteur d’écoute (>=0) en cas de succès, -1 en cas d’erreur.
 */
int net_server_listen(uint16_t port);

/**
 * @brief Accepte une connexion TCP entrante sur un socket serveur.
 *
 * Cette fonction attend (de façon bloquante) qu’un client tente de se connecter
 * au socket d’écoute spécifié. Lorsqu’une connexion est acceptée, elle retourne
 * un nouveau descripteur de socket dédié à la communication avec ce client.
 *
 * @param listen_fd Descripteur de socket d’écoute (retourné par net_server_listen()).
 *        Ce socket reste ouvert pour accepter d’autres connexions.
 * @return Descripteur de socket client (>=0) en cas de succès, -1 en cas d’erreur.
 *
 * @note Le descripteur retourné doit être fermé explicitement (close()) après usage.
 * @note En cas d’erreur (ex : interruption système, ressources insuffisantes), -1 est retourné.
 *
 * Exemple d’utilisation :
 * @code
 * int listen_fd = net_server_listen(5555);
 * if (listen_fd >= 0) {
 *     int client_fd = net_server_accept(listen_fd);
 *     if (client_fd >= 0) {
 *         // Communiquer avec le client via client_fd
 *         close(client_fd);
 *     }
 *     close(listen_fd);
 * }
 * @endcode
 */
int net_server_accept(int listen_fd);

/* ---- client ---- */

/**
* @brief Se connecte à un serveur TCP.
*
* @param host Adresse IP ou nom DNS (ex: "127.0.0.1").
* @param port Numéro de port TCP.
* @return Descripteur de socket client (>=0) en cas de succès, -1 en cas d’erreur.
*
* @note Le descripteur retourné doit être fermé explicitement (close()) après usage.
* @note En cas d’erreur (ex : interruption système, ressources insuffisantes), -1 est retourné.
*
* Exemple d’utilisation :
* @code
* int client_fd = net_client_connect("127.0.0.1", 5555);
* if (client_fd >= 0) {
*     // Communiquer avec le serveur via client_fd
*     close(client_fd);
* }
* @endcode
*/
int net_client_connect(const char* host, uint16_t port);

/* ---- I/O brutes ---- */

/**
 * @brief Envoie exactement `len` octets.
 *
 * Boucle tant que tous les octets ne sont pas envoyés.
 *
 * @param fd Socket TCP.
 * @param buf Données à envoyer.
 * @param len Taille des données.
 * @return Nombre d’octets envoyés ou -1 si erreur.
 */
ssize_t net_send_all(int fd, const void* buf, size_t len);

/**
 * @brief Reçoit exactement `len` octets.
 *
 * Boucle tant que tous les octets ne sont pas lus.
 *
 * @param fd Socket TCP.
 * @param buf Tampon de réception.
 * @param len Taille attendue.
 * @return Nombre d’octets lus, 0 si EOF, -1 si erreur.
 */
ssize_t net_recv_all(int fd, void* buf, size_t len);

/* ---- I/O protocole 4 octets ---- */

/**
 * @brief Envoie un message de 4 octets (protocole du jeu).
 *
 * @param fd Socket TCP.
 * @param four Tableau de 4 octets à envoyer.
 * @return 0 si succès, -1 si erreur.
 */
int net_send4(int fd, const char four[4]);

/**
 * @brief Reçoit un message de 4 octets (protocole du jeu).
 *
 * @param fd Socket TCP.
 * @param out Tableau de 4 octets à remplir.
 * @return 1 si succès, 0 si EOF propre, -1 si erreur.
 */
int net_recv4(int fd, char out[4]);

/* ---- utils ---- */

/**
 * @brief Ferme un socket proprement.
 *
 * @param fd Descripteur du socket à fermer.
 */
void net_close(int fd);

#endif /* NET_H */