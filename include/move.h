/**
 * @file move.h
 * @brief Gestion des coups (représentation et conversion texte).
 *
 * Ce fichier définit la structure d’un coup et fournit
 * des fonctions pour convertir un coup en texte ou depuis
 * une chaîne de caractères.
 *
 * @see move.c pour l'implémentation
 */

#ifndef MOVE_H
#define MOVE_H
#include <stdbool.h>
#include "coord.h"

/**
 * @struct Move
 * @brief Représente un coup avec une case de départ et une case d’arrivée.
 *
 * @note Un coup est considéré comme valide si:
 *  -Les coordonnées `from` et `to` sont différentes
 *  -Les coordonnées sont dans les limites du plateau
 */
typedef struct { 
    Coord from; /**< Case de départ */
    Coord to;   /**< Case d’arrivée */
} Move;

/**
 * @brief Convertit une chaîne de texte en coup.
 *
 * Exemple : `"A9 A5"` sera transformé en un coup de A9 vers A5.
 *
 *
 * @warning La chaine doit etre exactement du format `"Xn Ym"`
 * (longueur 5 avec un espace au milieu)
 *
 * @param s Chaîne contenant le coup (format `"A9 A5"`).
 * @param mv Pointeur vers la structure Move à remplir.
 * @return true si la conversion a réussi, false sinon.
 */
bool move_from_str(const char* s, Move* mv);

/**
 * @brief Convertit un coup en texte.
 *
 * Exemple : un coup de A9 vers A5 donnera `"A9 A5"`.
 *
 * @param mv Coup à convertir.
 * @param out Tableau de 6 caractères pour stocker la chaîne (ex: `"A9 A5"`).
 * @return true si la conversion a réussi, false sinon.
 */
bool move_to_str(Move mv, char out[6]);

#endif /* MOVE_H */