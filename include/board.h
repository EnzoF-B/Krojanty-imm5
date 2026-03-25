/**
 * @file board.h
 * @brief Interface pour la gestion du plateau de jeu Krojanty et ses opérations fondamentales.
 *
 * @author marcelin.trag@uha.fr
 * @date 2025-09-16
 * @version 1.0.0
 *
 * Ce module fournit toutes les fonctions nécessaires à la manipulation du plateau de jeu :
 *   - Initialisation d’un plateau vide ou de test
 *   - Lecture et écriture de pièces à des coordonnées données
 *   - Vérification de la validité d’un chemin (absence d’obstacles en ligne ou colonne)
 *   - Affichage lisible du plateau en mode texte (ASCII)
 *
 * Structure du plateau (type Board) :
 *   - Plateau carré de taille BOARD_SIZE x BOARD_SIZE (typiquement 9x9)
 *   - Chaque case contient une pièce (type Piece) : EMPTY, BLUE_SOLDIER, BLUE_KING, RED_SOLDIER, RED_KING
 *   - Le plateau mémorise aussi le joueur courant (Player) et le numéro du tour
 *
 * Utilisation typique :
 * @code
 * Board b;
 * board_init(&b, PLAYER_BLUE);
 * board_set(&b, (Coord){4,4}, BLUE_KING);
 * if (board_path_clear(&b, (Coord){4,4}, (Coord){4,8})) { ... }
 * board_print_ascii(&b);
 * @endcode
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include "const.h"
#include "coord.h"

/**
 * @brief Réinitialise le plateau de jeu : toutes les cases sont vidées et le joueur courant est fixé.
 *
 * Toutes les cases du plateau sont remplies avec la pièce vide (`EMPTY`).
 * Le joueur courant est positionné selon le paramètre `starts` (PLAYER_BLUE ou PLAYER_RED).
 * Le compteur de tours est remis à zéro.
 *
 * @param b      Pointeur vers le plateau à initialiser.
 * @param starts Joueur qui commence la partie.
 */
void board_init(Board* b, Player starts);

/**
 * @brief Lit la pièce présente à la coordonnée spécifiée.
 *
 * Si la coordonnée est hors du plateau, retourne `EMPTY`.
 *
 * @param b Plateau source.
 * @param c Coordonnée (x,y) à lire.
 * @return La pièce trouvée à la coordonnée, ou `EMPTY` si hors limites.
 */
Piece board_get(const Board* b, Coord c);

/**
 * @brief Place une pièce sur une case du plateau.
 *
 * Si la coordonnée est hors du plateau, l’opération est ignorée.
 *
 * @param b Plateau cible.
 * @param c Coordonnée (x,y) où placer la pièce.
 * @param p Pièce à placer (`BLUE_SOLDIER`, `RED_SOLDIER`, `BLUE_KING`, `RED_KING`, `EMPTY`).
 */
void board_set(Board* b, Coord c, Piece p);

/**
 * @brief Vérifie si le chemin entre deux cases (en ligne droite) est libre d’obstacles.
 *
 * Le chemin doit être strictement horizontal ou vertical (pas de diagonale).
 * La case d’arrivée peut être occupée (seul le couloir entre les deux est vérifié).
 * Retourne `false` si une pièce est rencontrée sur le chemin (hors case d’arrivée),
 * ou si les coordonnées sont hors bornes ou identiques.
 *
 * @param b    Plateau à analyser.
 * @param from Case de départ.
 * @param to   Case d’arrivée.
 * @return true si le chemin est libre, false sinon.
 */
bool board_path_clear(const Board* b, Coord from, Coord to);

/**
 * @brief Affiche le plateau en mode texte (ASCII), avec lettres pour les colonnes (A..I) et chiffres pour les rangs (9..1).
 *
 * Permet de visualiser rapidement l’état du jeu dans un terminal (pour débogage ou mode CLI).
 * Exemple de sortie :
 * @code
 *      A B C D E F G H I
 *    9  . . S . . . . . .  9
 *    8  . . . . . . . . .  8
 *    ...
 *      A B C D E F G H I
 * @endcode
 *
 * @param b Plateau à afficher.
 */
void board_print_ascii(const Board* b);

/**
 * @brief Place une configuration de test prédéfinie sur le plateau, utile pour valider rapidement les règles du jeu.
 *
 * Contenu de la configuration :
 *   - Roi bleu (BLUE_KING) en A9 (coin supérieur gauche)
 *   - Deux soldats bleus (BLUE_SOLDIER) sur la première rangée (C9 et E9)
 *   - Roi rouge (RED_KING) en I1 (coin inférieur droit)
 *   - Deux soldats rouges (RED_SOLDIER) sur la dernière rangée (G1 et E1)
 *   - Le joueur courant est BLEU (PLAYER_BLUE)
 *
 * Utilité : permet de tester rapidement les mouvements, l’affichage ou les captures sans avoir à saisir une configuration à la main.
 *
 * @param b Plateau à préparer.
 */
void board_place_test_setup(Board* b);

#endif /* BOARD_H */