/**
 * @file ai.h
 * @brief Interface pour l'intelligence artificielle (IA) du jeu.
 *
 * Ce module fournit la configuration et la fonction principale
 * permettant de calculer le meilleur coup pour l'IA.
 *
 * L'IA utilise l'algorithme **Minimax**, avec ou sans optimisation
 * par l'élagage **Alpha-Beta**.
 */

#ifndef AI_H
#define AI_H

#include <stdint.h>
#include "board.h"
#include "move.h"
#include "game.h"
#include "gen.h"

/**
 * @brief Choisit le meilleur coup à jouer pour l’IA.
 *
 * Cette fonction explore l’arbre des coups possibles jusqu’à une certaine profondeur
 * (définie dans `AiConfig`) et renvoie le coup estimé comme le plus avantageux.
 *
 * @param b Pointeur vers le plateau actuel.
 * @param gs Pointeur vers l’état de la partie (scores, captures, etc.).
 * @param me Joueur contrôlé par l’IA (`PLAYER_BLUE` ou `PLAYER_RED`).
 * @param cfg Configuration de l’IA (profondeur, alpha-beta).
 * @param out_move Pointeur où sera stocké le coup choisi.
 * @param out_score Pointeur où sera stocké le score estimé du coup choisi.
 * @return 1 si un coup a été trouvé, 0 si aucun coup légal n’existe.
 *
 * Exemple d’utilisation :
 * @code
 * AiConfig cfg = { .depth = 3, .use_ab = 1 };
 * Move best;
 * int score;
 * if (ai_choose_move(&board, &state, PLAYER_BLUE, &cfg, &best, &score)) {
 *     printf("Coup choisi : ");
 *     move_to_str(best, buf);
 *     puts(buf);
 * }
 * @endcode
 */
int ai_choose_move(const Board* b, const GameState* gs, Player me,
                    Move* out_move, int* out_score);

#endif /* AI_H */