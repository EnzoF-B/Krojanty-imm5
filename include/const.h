/**
 * @file const.h
 * @brief Définitions des constantes et structures de base du jeu.
 *
 * Ce fichier regroupe :
 * - la taille du plateau,
 * - les types pour représenter les joueurs et les pièces,
 * - la structure `Coord` pour les coordonnées,
 * - la structure `Board` pour représenter l’état du plateau.
 */

#ifndef CONST_H
#define CONST_H

/** @brief Taille du plateau (9x9 cases). */
#define BOARD_SIZE 9

/**
 * @enum Player
 * @brief Représente les deux joueurs.
 */
typedef enum {
    PLAYER_BLUE = 0,  /**< Joueur Bleu (commence en haut). */
    PLAYER_RED  = 1   /**< Joueur Rouge (commence en bas). */
} Player;

/**
 * @enum Piece
 * @brief Différents types de pièces sur le plateau.
 */
typedef enum {
    EMPTY = 0,       /**< Case vide. */
    BLUE_SOLDIER,    /**< Soldat bleu. */
    BLUE_KING,       /**< Roi bleu. */
    RED_SOLDIER,     /**< Soldat rouge. */
    RED_KING         /**< Roi rouge. */
} Piece;

/**
 * @struct Coord
 * @brief Coordonnée d’une case du plateau.
 *
 * - Axe X : colonnes de A à I (0 à 8)
 * - Axe Y : lignes de 9 à 1 (0 à 8)
 * Exemple : A9 = (0,0)
 */
typedef struct {
    int x; /**< Colonne (0 = A, 8 = I). */
    int y; /**< Ligne (0 = rang 9, 8 = rang 1). */
} Coord;

/**
 * @struct Board
 * @brief Représente l’état complet du plateau.
 */
typedef struct {
    Piece  cells[BOARD_SIZE][BOARD_SIZE]; /**< Grille des pièces (9x9). */
    Player current_player;                /**< Joueur dont c’est le tour. */
    int    turn_number;                   /**< Numéro du tour en cours. */
} Board;

#endif /* CONST_H */