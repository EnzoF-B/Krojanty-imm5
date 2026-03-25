/**
 * @file ui_gtk.h
 * @brief Interface graphique GTK pour le jeu Krojanty.
 *
 * Ce fichier définit :
 * - les modes de jeu (local, serveur, client),
 * - l’état réseau nécessaire pour la synchronisation,
 * - les fonctions pour lancer l’interface graphique avec ou sans réseau.
 */

#ifndef UI_GTK_H
#define UI_GTK_H
#include <gtk/gtk.h>
#include "game.h"
#include "net.h"
#include "ai.h"

/**
 * @enum GameMode
 * @brief Les différents modes de jeu.
 *
 * - GAME_MODE_LOCAL  : Partie locale, deux joueurs sur le même ordinateur.
 * - GAME_MODE_SERVER : Partie réseau, ce joueur héberge (joueur rouge, 2ème).
 * - GAME_MODE_CLIENT : Partie réseau, ce joueur rejoint (joueur bleu, 1er).
 */
typedef enum {
    GAME_MODE_LOCAL,
    GAME_MODE_SERVER,
    GAME_MODE_CLIENT
} GameMode;

/**
 * @struct NetworkState
 * @brief Représente l’état du réseau pour une partie en ligne.
 *
 * Contient les informations nécessaires à la communication réseau :
 * - mode (serveur ou client),
 * - adresse IP/nom d’hôte,
 * - port TCP,
 * - socket réseau,
 * - statut de tour (si c’est à moi de jouer),
 * - état de connexion.
 */
typedef struct {
    GameMode mode;       /**< Mode de jeu (local, serveur ou client). */
    char host[256];      /**< Nom d’hôte ou adresse IP (pour le client). */
    uint16_t port;       /**< Port TCP utilisé. */
    int net_fd;          /**< Descripteur de socket réseau. */
    bool is_my_turn;     /**< Indique si c’est au tour de ce joueur. */
    bool connected;      /**< Indique si la connexion est établie. */
} NetworkState;

/**
 * @brief Lance l’interface graphique pour une partie locale (sans réseau).
 *
 * @param app Application GTK en cours.
 * @param initial_board Plateau de jeu initial.
 * @param initial_state État du jeu initial.
 */
void ui_run_gui(GtkApplication* app, Board* initial_board, GameState* initial_state);

/**
 * @brief Lance l’interface graphique pour une partie en réseau (client ou serveur).
 *
 * Permet aussi d’activer l’IA côté local si demandé.
 *
 * @param app Application GTK en cours.
 * @param initial_board Plateau de jeu initial.
 * @param initial_state État du jeu initial.
 * @param net_state Structure contenant l’état réseau.
 * @param ai_enabled 1 si l’IA doit être activée côté local, 0 sinon.
 * @param ai_depth Profondeur de recherche de l’IA (si activée).
 */
void ui_run_gui_network(GtkApplication* app, Board* initial_board, GameState* initial_state,
                        NetworkState* net_state, int ai_enabled, int ai_depth);

#endif /* UI_GTK_H */