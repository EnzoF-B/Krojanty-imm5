/**
 * @file game.h
 * @brief Structures et fonctions principales pour gérer l'état de la partie.
 *
 * Ce fichier définit les règles générales du jeu :
 * - suivi des soldats capturés et des rois encore en vie,
 * - suivi des points et des zones contrôlées,
 * - conditions de victoire,
 * - initialisation du plateau et application des coups.
 */

#ifndef GAME_H
#define GAME_H
#include <stdbool.h>
#include <stdint.h>
#include "board.h"
#include "rules.h"
#include "move.h"


/**
 * @brief Coordonnée de la cité bleue (case objectif pour le roi rouge).
 * A9 correspond à (0,0).
 */
static const Coord CITY_BLUE = {0,0};

/**
 * @brief Coordonnée de la cité rouge (case objectif pour le roi bleu).
 * I1 correspond à (8,8).
 */
static const Coord CITY_RED  = {8,8};

/**
 * @brief Propriétaire d’une case du plateau.
 * Utilisé pour attribuer les points de contrôle.
 */
typedef enum {
    CTRL_NONE = 0, /**< Case neutre */
    CTRL_BLUE,     /**< Case contrôlée par le joueur bleu */
    CTRL_RED       /**< Case contrôlée par le joueur rouge */
} ControlOwner;

/**
 * @brief État complet de la partie.
 *
 * Cette structure garde toutes les informations nécessaires :
 * - capturés (soldats bleus/rouges),
 * - statut des rois (vivant ou non),
 * - nombre maximum de tours,
 * - tableau de contrôle des cases,
 * - points cumulés par chaque joueur.
 */
typedef struct {
    int  blue_soldiers_captured; /**< Soldats bleus capturés */
    int  red_soldiers_captured;  /**< Soldats rouges capturés */
    bool blue_king_alive;        /**< Roi bleu encore en vie */
    bool red_king_alive;         /**< Roi rouge encore en vie */
    int  max_turns;              /**< Limite de tours (exemple : 64) */

    ControlOwner control[BOARD_SIZE][BOARD_SIZE]; /**< Zone contrôlée */
    int blue_points; /**< Points joueur bleu */
    int red_points;  /**< Points joueur rouge */

    int UseAI;  
    int Depth; // Profondeur de l'IA
    uint64_t zobrist_key; // Clé unique associée à la game

    // Marquage "déjà compté" par équipe (évite de regagner +1 en repassant)
    bool visited_blue[BOARD_SIZE][BOARD_SIZE];
    bool visited_red [BOARD_SIZE][BOARD_SIZE];

} GameState;

// Inclusion pas avant GameState sinon cassé
#include "ai.h"

/**
 * @brief Résultat de la partie.
 */
typedef enum {
    GAME_ONGOING = 0, /**< Partie en cours */
    GAME_WIN_BLUE,    /**< Victoire du joueur bleu */
    GAME_WIN_RED,     /**< Victoire du joueur rouge */
    GAME_DRAW         /**< Match nul */
} GameResult;

/**
 * @brief Initialise l’état d’une partie.
 * @param gs Pointeur vers l’état du jeu.
 * @param max_turns Nombre maximum de tours.
 * @param cfg Configuration de l'IA
 * @param b Plateau de jeu
 */
void game_init_state(GameState* gs, int max_turns, const Board* b, int ai_enabled);

/**
 * @brief Place les pièces selon la disposition initiale standard.
 * @param b Plateau à initialiser.
 */
void board_place_initial(Board* b, GameState* gs);

/**
 * @brief Place les pièces depuis une représentation ASCII (9 lignes).
 * @param b Plateau à remplir.
 * @param rows Tableau de 9 chaînes (une par ligne).
 * @return true si placement réussi, false sinon.
 */
bool board_place_from_ascii(Board* b, GameState* gs, const char* rows[9]);

/**
 * @brief Met à jour le score lorsqu’une pièce quitte une case.
 * @param gs État du jeu.
 * @param mover Joueur qui vient de jouer.
 * @param from Coordonnée de la case quittée.
 */
void score_leave_cell(GameState* gs, Player mover, Coord from);

/**
 * @brief Vérifie les conditions de victoire.
 * @param b Plateau actuel.
 * @param gs État actuel du jeu.
 * @return Résultat (victoire bleu/rouge, nul, ou en cours).
 */
GameResult check_victory(const Board* b, const GameState* gs);

/**
 * @brief Applique un coup complet sur le plateau.
 * @param b Plateau.
 * @param gs État du jeu.
 * @param m Coup à jouer.
 * @param out_result Résultat de la partie après le coup.
 * @return true si le coup est valide et appliqué.
 */
bool apply_move(Board* b, GameState* gs, Move m, GameResult* out_result);

/* ----------------------------------------------------------------
 *  Ajout : recalcul “source de vérité” du score
 *  Recompte : points = (#soldats vivants) + (#cases contrôlées)
 *  Appelé au démarrage et après chaque coup pour garder la synchro.
 * ---------------------------------------------------------------- */
void game_recompute_scores(GameState* gs, const Board* b);

/**
 * @brief Initialise le contrôle et les points en fonction des pièces placées.
 * @param b  Plateau (positions des pièces).
 * @param gs État du jeu (points et contrôle mis à jour).
 */
void game_init_control(Board* b, GameState* gs);

/* Initialisation complémentaire après placement des pièces :
 * - marque les contrôles initiaux sur les cases occupées,
 * - empêche le double comptage au premier départ,
 * - synchronise les scores pour démarrer à 20/20. */
void game_post_setup_seed(GameState* gs, const Board* b);

#endif /* GAME_H */