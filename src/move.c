/**
 * @file move.c
 * @brief Conversion des coups entre texte (ex: "B8 B5") et structures Move.
 *
 * Ce fichier sert uniquement à transformer :
 *   - une chaîne écrite par un joueur ("A1 B3") → en un coup utilisable par le programme.
 *   - un coup interne (Move) → en texte affichable.
 *
 * Très utile pour :
 *   - lire les coups tapés dans le terminal,
 *   - afficher les coups choisis par l’IA,
 *   - envoyer les coups sur le réseau (toujours sous forme texte).
 */

#include <string.h>
#include "move.h"

/**
 * @brief Transforme une chaîne en coup Move.
 *
 * Exemple d’entrée : `"B8 B5"`.
 *
 * Vérifications effectuées :
 *   - La chaîne existe et n’est pas vide.
 *   - Sa longueur est exactement 5 caractères (`Xn Ym`).
 *   - Le caractère du milieu est bien un espace.
 *   - Les coordonnées de départ et d’arrivée sont valides.
 *
 * @warning Si une de ces conditions n'est pas respectée,
 * la fonction returne `false` sans modifier `mv`.
 * @param s   La chaîne en entrée ("A1 B3").
 * @param mv  Pointeur vers la structure Move à remplir.
 * @return true si la conversion a réussi, false sinon.
 */
bool move_from_str(const char* s, Move* mv) {
    if (!s || !mv) return false;
    if (strlen(s) != 5) return false;
    if (s[2] != ' ') return false;

    Coord a,b;
    char A[3] = {s[0], s[1], 0};
    char B[3] = {s[3], s[4], 0};

    if (!coord_from_str(A,&a) || !coord_from_str(B,&b)) return false;

    mv->from = a; 
    mv->to = b; 
    return true;
}

/**
 * @brief Transforme un coup Move en texte.
 *
 * Exemple :
 *   - Move (A1 → B3) devient `"A1 B3"`.
 *
 * Vérifications effectuées :
 *   - Le buffer de sortie existe.
 *   - Les coordonnées sont valides.
 *
 * @param mv   Le coup à convertir.
 * @param out  Buffer de sortie (taille ≥ 6, car format "A1 B2\0").
 * @return true si la conversion a réussi, false sinon.
 */
bool move_to_str(Move mv, char out[6]) {
    if (!out) return false;

    char A[3],B[3];
    if (!coord_to_str(mv.from,A) || !coord_to_str(mv.to,B)) return false;

    out[0]=A[0];
    out[1]=A[1];
    out[2]=' ';
    out[3]=B[0];
    out[4]=B[1];
    out[5]=0;
    
    return true;
}