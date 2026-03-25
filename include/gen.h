/**
 * @file gen.h
 * @brief Génération des coups légaux possibles.
 *
 * Ce fichier fournit les fonctions qui permettent
 * de générer la liste des coups valides qu’un joueur
 * peut jouer à partir d’un plateau donné.
 */

#ifndef GEN_H
#define GEN_H
#include "move.h"
#include "rules.h"

/**
 * @brief Génère tous les coups légaux pour le joueur courant.
 *
 * La fonction parcourt le plateau et cherche tous les coups
 * possibles en ligne droite (haut, bas, gauche, droite),
 * en respectant les règles de légalité de base.
 *
 * @param b Plateau actuel.
 * @param out Tableau où stocker les coups trouvés.
 * @param max_out Taille maximale du tableau @p out.
 * @return Nombre de coups légaux trouvés (0 si aucun).
 */
int gen_legal_moves(const Board* b, Move* out, int max_out);

#endif /* GEN_H */