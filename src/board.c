/**
 * @file board.c
 * @brief Fonctions de base pour la gestion du plateau : initialisation, accès/écriture de cases,
 *        vérification de chemin libre, affichage ASCII, et configuration de scénarios de test.
 * @author marcelin.trag@uha.fr
 * @date 2025-09-16
 * @version 1.0.0
 *
 * Structure du plateau :
 *  - Le plateau est représenté par un tableau 9x9 de pièces (type Piece).
 *  - Accès et modification des cases via des coordonnées (type Coord).
 *  - Vérification de la validité des déplacements (chemin libre en ligne/colonne).
 *  - Affichage lisible du plateau pour la console (mode texte).
 *  - Fonction utilitaire pour placer rapidement une configuration de test.
 */

#include <stdio.h>
#include <string.h>
#include "board.h"

/**
 * @brief Convertit une pièce en caractère lisible pour l'affichage ASCII.
 *
 * Correspondances :
 *  - 'S' : soldat bleu (BLUE_SOLDIER)
 *  - 'K' : roi bleu (BLUE_KING)
 *  - 's' : soldat rouge (RED_SOLDIER)
 *  - 'k' : roi rouge (RED_KING)
 *  - '.' : case vide (EMPTY)
 *
 * @param p La pièce à convertir.
 * @return Le caractère associé à la pièce pour l'affichage.
 */
static char piece_char(Piece p){
    switch(p){
        case BLUE_SOLDIER: return 'S';
        case BLUE_KING:    return 'K';
        case RED_SOLDIER:  return 's';
        case RED_KING:     return 'k';
        case EMPTY: default: return '.';
    }
}

/**
 * @brief Réinitialise le plateau : toutes les cases sont vidées et le joueur courant est fixé.
 *
 * Utilité :
 *  - Permet de repartir d’un plateau vierge pour une nouvelle partie.
 *  - Le compteur de tours est remis à zéro.
 *
 * @param b      Pointeur vers le plateau à initialiser.
 * @param starts Joueur qui commence la partie (PLAYER_BLUE ou PLAYER_RED).
 */
void board_init(Board* b, Player starts){
    for(int y=0;y<BOARD_SIZE;++y)
        for(int x=0;x<BOARD_SIZE;++x)
            b->cells[y][x]=EMPTY;
    b->current_player=starts;
    b->turn_number=0;
}

/**
 * @brief Lit la pièce présente à la coordonnée spécifiée.
 *
 * Sécurité :
 *  - Si la coordonnée est hors du plateau, retourne @c EMPTY (case vide).
 *
 * @param b Plateau source.
 * @param c Coordonnée (x,y) à lire.
 * @return La pièce trouvée à la coordonnée, ou @c EMPTY si hors limites.
 */
Piece board_get(const Board* b, Coord c){
    if(!coord_in_bounds(c)) return EMPTY;
    return b->cells[c.y][c.x];
}

/**
 * @brief Place une pièce sur une case du plateau.
 *
 * Sécurité :
 *  - Si la coordonnée est hors du plateau, l’opération est ignorée.
 *
 * @param b Plateau cible.
 * @param c Coordonnée (x,y) où placer la pièce.
 * @param p Pièce à placer.
 */
void  board_set(Board* b, Coord c, Piece p){
    if(!coord_in_bounds(c)) return;
    b->cells[c.y][c.x]=p;
}

/**
 * @brief Vérifie si le chemin entre deux cases (en ligne droite) est libre d’obstacles.
 *
 * Comment un déplacmeent est considéré comme légal :
 *  - Le déplacement doit être strictement horizontal ou vertical (pas de diagonale).
 *  - La case d’arrivée peut être occupée (seul le couloir entre les deux est vérifié).
 *  - Si une pièce est rencontrée sur le chemin (hors case d’arrivée), retourne false.
 *  - Retourne false si les coordonnées sont hors bornes ou identiques.
 *
 * @param b    Plateau à analyser.
 * @param from Case de départ.
 * @param to   Case d’arrivée.
 * @return true si le chemin est libre, false sinon.
 */
bool board_path_clear(const Board* b, Coord from, Coord to){
    if(!coord_in_bounds(from)||!coord_in_bounds(to)) return false;
    if(from.x==to.x && from.y==to.y) return false;
    int dx=0,dy=0;
    if(from.x==to.x) dy=(to.y>from.y)?1:-1;
    else if(from.y==to.y) dx=(to.x>from.x)?1:-1;
    else return false;
    Coord cur=from;
    while(1){
        cur.x+=dx; cur.y+=dy;
        if(cur.x==to.x && cur.y==to.y) return true;
        if(board_get(b,cur)!=EMPTY) return false;
    }
}

/**
 * @brief Affiche le plateau en mode texte (ASCII), avec lettres pour les colonnes et chiffres pour les rangs.
 *
 * Objectif :
 *  - Permet de visualiser rapidement l’état du jeu dans un terminal (pour débogage ou mode CLI).
 *  - Affichage lisible : lettres en haut/bas, numéros de rang à gauche/droite.
 *
 * Exemple de sortie :
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
void board_print_ascii(const Board* b){
    printf("    "); for(int x=0;x<BOARD_SIZE;++x) printf(" %c",'A'+x); printf("\n");
    for(int y=0;y<BOARD_SIZE;++y){
        char row = y_to_row_char(y);
        printf("  %c ",row);
        for(int x=0;x<BOARD_SIZE;++x){
            printf(" %c", piece_char(board_get(b,(Coord){x,y})));
        }
        printf("  %c\n",row);
    }
    printf("    "); for(int x=0;x<BOARD_SIZE;++x) printf(" %c",'A'+x); printf("\n");
}

/**
 * @brief Place une configuration de test prédéfinie sur le plateau, utile pour valider rapidement les règles du jeu.
 *
 * Contenu de la configuration :
 *  - Roi bleu (BLUE_KING) en A9 (coin supérieur gauche)
 *  - Deux soldats bleus (BLUE_SOLDIER) sur la première rangée (C9 et E9)
 *  - Roi rouge (RED_KING) en I1 (coin inférieur droit)
 *  - Deux soldats rouges (RED_SOLDIER) sur la dernière rangée (G1 et E1)
 *  - Le joueur courant est BLEU (PLAYER_BLUE)
 *
 * Utilité :
 *  - Permet de tester rapidement les mouvements, l’affichage ou les captures sans avoir à saisir une configuration à la main.
 *
 * @param b Plateau à préparer.
 */
void board_place_test_setup(Board* b){
    board_init(b,PLAYER_BLUE);
    board_set(b,(Coord){0,0},BLUE_KING);
    board_set(b,(Coord){2,0},BLUE_SOLDIER);
    board_set(b,(Coord){4,0},BLUE_SOLDIER);
    board_set(b,(Coord){8,8},RED_KING);
    board_set(b,(Coord){6,8},RED_SOLDIER);
    board_set(b,(Coord){4,8},RED_SOLDIER);
}