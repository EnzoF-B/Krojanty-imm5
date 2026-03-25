/**
 * @file coord.h
 * @brief Fonctions utilitaires pour gérer les coordonnées du plateau.
 *
 * Ce fichier regroupe toutes les fonctions permettant de :
 * - convertir une lettre de colonne (A..I) en indice numérique,
 * - convertir un chiffre de ligne (1..9) en indice numérique,
 * - vérifier si une coordonnée est dans les limites du plateau,
 * - traduire une chaîne ("A9") en coordonnées `(x,y)` et inversement.
 */

#ifndef COORD_H
#define COORD_H
#include <stdbool.h>
#include "const.h"

/**
 * @brief Convertit une lettre de colonne en indice X.
 * @param col Lettre (A..I ou a..i).
 * @return indice X (0..8) ou -1 si invalide.
 */
int  col_char_to_x(char col);

/**
 * @brief Convertit un indice X en lettre de colonne.
 * @param x Indice (0..8).
 * @return Lettre correspondante (A..I) ou '?' si invalide.
 */
char x_to_col_char(int x);

/**
 * @brief Convertit un caractère de ligne en indice Y.
 * @param row Chiffre ('1'..'9').
 * @return Indice Y (0..8) ou -1 si invalide.
 *
 * Exemple : '9' → 0, '1' → 8.
 */
int  row_char_to_y(char row);

/**
 * @brief Convertit un indice Y en caractère de ligne.
 * @param y Indice (0..8).
 * @return Chiffre ('1'..'9') ou '?' si invalide.
 */
char y_to_row_char(int y);

/**
 * @brief Vérifie si une coordonnée est bien dans le plateau.
 * @param c Coordonnée à tester.
 * @return true si 0 <= x,y < BOARD_SIZE.
 */
static inline bool coord_in_bounds(Coord c) {
    return (c.x >= 0 && c.x < BOARD_SIZE && c.y >= 0 && c.y < BOARD_SIZE);
}

/**
 * @brief Convertit une chaîne ("A9") en coordonnées.
 * @param s Chaîne d’entrée (2 caractères).
 * @param out Pointeur où stocker le résultat.
 * @return true si conversion réussie.
 */
bool coord_from_str(const char* s, Coord* out);

/**
 * @brief Convertit une coordonnée en chaîne ("A9").
 * @param c Coordonnée.
 * @param out Chaîne de sortie (3 cases, incluant '\0').
 * @return true si conversion réussie.
 */
bool coord_to_str(Coord c, char out[3]);

#endif /* COORD_H */