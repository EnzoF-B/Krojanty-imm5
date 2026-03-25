/**
 * @file rules.h
 * @brief Règles de base du jeu Krojanty.
 *
 * Ce fichier définit les règles fondamentales :
 * - relation entre pièces (allié / ennemi),
 * - légalité d’un coup de base,
 * - application minimale d’un coup,
 * - changement de joueur.
 */

#ifndef RULES_H
#define RULES_H
#include <stdbool.h>
#include "board.h"
#include "move.h"

/**
 * @brief Vérifie si une pièce appartient au joueur donné.
 *
 * @param p Pièce (soldat/roi bleu ou rouge, ou vide).
 * @param j Joueur (bleu ou rouge).
 * @return true si la pièce appartient au joueur, false sinon.
 */
bool piece_is_ally(Piece p, Player j);

/**
 * @brief Vérifie si une pièce est ennemie pour un joueur donné.
 *
 * @param p Pièce à tester.
 * @param j Joueur de référence.
 * @return true si la pièce est ennemie, false sinon.
 */
bool piece_is_enemy(Piece p, Player j);

/**
 * @brief Vérifie si un coup est légal au niveau "de base".
 *
 * Conditions testées :
 * - départ et arrivée dans les limites du plateau,
 * - départ ≠ arrivée,
 * - la case de départ contient une pièce du joueur courant,
 * - la case d’arrivée est vide,
 * - le chemin est rectiligne et libre.
 *
 * @param b Plateau de jeu.
 * @param m Coup (coordonnées départ et arrivée).
 * @return true si le coup est légal, false sinon.
 */
bool move_is_legal_basic(const Board* b, Move m);

/**
 * @brief Applique un coup de manière basique (sans captures spéciales).
 *
 * Déplace une pièce d’une case à une autre, vide la case de départ,
 * puis change le joueur courant et incrémente le compteur de tours.
 *
 * @param b Plateau de jeu.
 * @param m Coup à appliquer.
 */
void apply_move_basic(Board* b, Move m);

/**
 * @brief Renvoie l’autre joueur.
 *
 * @param p Joueur actuel.
 * @return PLAYER_RED si p==PLAYER_BLUE, sinon PLAYER_BLUE.
 */
static inline Player other_player(Player p) {
    return (p == PLAYER_BLUE) ? PLAYER_RED : PLAYER_BLUE;
}

#endif /* RULES_H */