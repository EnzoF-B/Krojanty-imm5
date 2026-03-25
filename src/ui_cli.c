/**
 * @file ui_cli.c
 * @brief Interface console (mode texte) pour Krojanty.
 *
 * Ce fichier fournit une petite interface en mode texte
 * permettant d’afficher le plateau et de tester des coups.
 *
 * ⚠️ Ce n’est pas le moteur complet du jeu, mais juste
 * une démo pour voir le plateau et vérifier certains mouvements.
 */

#include <stdio.h>
#include "board.h"
#include "move.h"

/**
 * @brief Affiche le plateau de jeu dans la console.
 *
 * Utilise la fonction interne `board_print_ascii` pour dessiner
 * une grille avec les pièces et les coordonnées (A–I, 1–9).
 *
 * @param b Pointeur vers le plateau actuel.
 */
void cli_demo_print(Board* b) {
    printf("=== Plateau ===\n");
    board_print_ascii(b);
    printf("\n");
}

/**
 * @brief Démonstration de mouvement en mode console.
 *
 * Vérifie si un mouvement donné (au format "B8 B5")
 * correspond à un chemin rectiligne libre.
 *
 * Étapes :
 *   - Conversion de la chaîne en structure Move.
 *   - Vérifie qu’il y a bien une pièce sur la case de départ.
 *   - Vérifie si le chemin entre départ et arrivée est libre.
 *   - Affiche un message explicatif.
 *
 * ⚠️ Ce n’est pas une vérification complète de légalité
 * (ne teste pas les règles de capture ou autres conditions).
 *
 * @param b         Plateau de jeu.
 * @param move_str  Chaîne représentant le coup (ex: "B8 B5").
 */
void cli_demo_try_move(Board* b, const char* move_str) {
    Move m;
    if (!move_from_str(move_str, &m)) {
        printf("Coup invalide (format) : %s\n", move_str);
        return;
    }
    Piece p = board_get(b, m.from);
    if (p == EMPTY) {
        printf("Aucune pièce à %s\n", move_str);
        return;
    }
    if (board_path_clear(b, m.from, m.to)) {
        printf("Chemin libre pour %s\n", move_str);
    } else {
        printf("Chemin bloqué ou non rectiligne pour %s\n", move_str);
    }
}