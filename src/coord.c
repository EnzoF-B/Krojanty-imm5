/**
 * @file coord.c
 * @brief Fonctions utilitaires pour la gestion des coordonnées du plateau.
 *
 * Ce fichier contient toutes les fonctions nécessaires pour convertir
 * entre les coordonnées internes (x,y) utilisées par le programme
 * et la représentation textuelle (par exemple "A9").
 *
 * Le plateau est une grille 9x9 :
 *   - Les colonnes sont notées de 'A' à 'I'.
 *   - Les lignes sont notées de '9' (haut) à '1' (bas).
 *
 * Exemples :
 *   - "A9" correspond à la case interne (0,0).
 *   - "I1" correspond à la case interne (8,8).
 */

#include <ctype.h>
#include <string.h>
#include "coord.h"

/**
 * @brief Convertit une lettre de colonne ('A'–'I' ou 'a'–'i') en abscisse interne.
 *
 * @param col Lettre de la colonne.
 * @return Index x (0 à 8), ou -1 si invalide.
 */
int col_char_to_x(char col) {
    if (col >= 'A' && col <= 'I') return col - 'A';
    if (col >= 'a' && col <= 'i') return col - 'a';
    return -1;
}

/**
 * @brief Convertit une abscisse interne (x) en caractère de colonne.
 *
 * @param x Index de colonne (0 à 8).
 * @return Lettre de colonne ('A'–'I'), ou '?' si invalide.
 */
char x_to_col_char(int x) { 
    return (x>=0 && x<BOARD_SIZE) ? (char)('A'+x) : '?'; 
}

/**
 * @brief Convertit un caractère de ligne ('1'–'9') en ordonnée interne.
 *
 * @param row Caractère de ligne.
 * @return Index y (0 à 8), ou -1 si invalide.
 *
 * Exemple :
 *   - '9' devient 0 (haut du plateau).
 *   - '1' devient 8 (bas du plateau).
 */
int row_char_to_y(char row) {
    if (row < '1' || row > '9') return -1;
    int shown = row - '0';  // 1..9
    int y = 9 - shown;      // 9->0 ... 1->8
    return (y>=0 && y<BOARD_SIZE) ? y : -1;
}

/**
 * @brief Convertit une ordonnée interne (y) en caractère de ligne.
 *
 * @param y Index de ligne (0 à 8).
 * @return Caractère '1'–'9', ou '?' si invalide.
 */
char y_to_row_char(int y) {
    if (y<0 || y>=BOARD_SIZE) return '?';
    int shown = 9 - y;
    return (char)('0' + shown);
}

/**
 * @brief Convertit une chaîne comme "A9" en coordonnées internes.
 *
 * @param s Chaîne de 2 caractères (ex: "A9").
 * @param out Pointeur vers la structure Coord à remplir.
 * @return true si la conversion est valide, false sinon.
 */
bool coord_from_str(const char* s, Coord* out) {
    if (!s || !out) return false;
    if (strlen(s) != 2) return false;
    int x = col_char_to_x(s[0]);
    int y = row_char_to_y(s[1]);
    if (x<0 || y<0) return false;
    out->x = x; out->y = y; return true;
}

/**
 * @brief Convertit une coordonnée interne en chaîne comme "A9".
 *
 * @param c Coordonnée interne.
 * @param out Tableau de 3 caractères minimum pour stocker la chaîne (ex: "A9\0").
 * @return true si la conversion réussit, false sinon.
 */
bool coord_to_str(Coord c, char out[3]) {
    if (!out || !coord_in_bounds(c)) return false;
    out[0] = x_to_col_char(c.x);
    out[1] = y_to_row_char(c.y);
    out[2] = '\0';
    return (out[0] != '?' && out[1] != '\0');
}