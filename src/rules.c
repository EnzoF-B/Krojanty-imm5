/**
 * @file rules.c
 * @brief Règles de base du jeu Krojanty.
 *
 * Ce fichier contient les fonctions qui gèrent :
 *   - l’appartenance des pièces à un joueur,
 *   - la détection d’alliés ou d’ennemis,
 *   - la légalité des coups (déplacements autorisés),
 *   - l’application d’un coup basique sur le plateau.
 *
 * ⚠️ Ici on ne traite pas encore les captures spéciales
 * (c’est fait dans game.c), seulement les règles générales.
 */

#include "rules.h"

/**
 * @brief Détermine à quel joueur appartient une pièce.
 *
 * @param p  La pièce (roi ou soldat bleu/rouge, ou vide).
 * @param ok Si non NULL, renvoie true si la pièce est valide.
 * @return PLAYER_BLUE, PLAYER_RED ou arbitrairement PLAYER_BLUE si vide.
 */
Player owner_of(Piece p, bool* ok){
    if(ok) *ok=true;
    switch(p){
        case BLUE_SOLDIER:
        case BLUE_KING: return PLAYER_BLUE;
        case RED_SOLDIER:
        case RED_KING:  return PLAYER_RED;
        case EMPTY: default: if(ok) *ok=false; return PLAYER_BLUE;
    }
}

/**
 * @brief Vérifie si une pièce appartient au joueur donné.
 *
 * @param p Pièce à tester.
 * @param j Joueur (BLEU ou ROUGE).
 * @return true si la pièce appartient au joueur, false sinon.
 */
bool piece_is_ally(Piece p, Player j){
    bool ok=false; Player o=owner_of(p,&ok);
    return ok && o==j;
}

/**
 * @brief Vérifie si une pièce est ennemie du joueur donné.
 *
 * @param p Pièce à tester.
 * @param j Joueur.
 * @return true si la pièce est une pièce adverse, false sinon.
 */
bool piece_is_enemy(Piece p, Player j){
    bool ok=false; Player o=owner_of(p,&ok);
    return ok && o!=j;
}

/**
 * @brief Vérifie si un mouvement est légal (version basique).
 *
 * Règles appliquées :
 *   - Les coordonnées doivent être dans le plateau.
 *   - La case d’arrivée doit être différente de la case de départ.
 *   - La case de départ doit contenir une pièce alliée.
 *   - La case d’arrivée doit être vide.
 *   - Le chemin entre départ et arrivée doit être libre (pas d’obstacles).
 *
 * @param b Plateau actuel.
 * @param m Mouvement (case de départ -> case d’arrivée).
 * @return true si le coup est légal, false sinon.
 */
bool move_is_legal_basic(const Board* b, Move m){
    if(!coord_in_bounds(m.from)||!coord_in_bounds(m.to)) return false;
    if(m.from.x==m.to.x && m.from.y==m.to.y) return false;

    Piece pf = board_get(b, m.from);
    if(pf==EMPTY) return false;
    if(!piece_is_ally(pf, b->current_player)) return false;

    Piece pt = board_get(b, m.to);
    if(pt != EMPTY) return false;               // arrivée doit être vide

    if(!board_path_clear(b, m.from, m.to)) return false;
    return true;
}

/**
 * @brief Applique un coup basique sur le plateau.
 *
 * Étapes :
 *   - Déplace la pièce de la case départ vers la case arrivée.
 *   - Vide la case de départ.
 *   - Change le joueur courant (tour suivant).
 *   - Incrémente le compteur de tours.
 *
 * @param b Plateau de jeu.
 * @param m Mouvement à jouer.
 */
void apply_move_basic(Board* b, Move m){
    Piece pf=board_get(b,m.from);
    board_set(b,m.to,pf);
    board_set(b,m.from,EMPTY);
    b->current_player = other_player(b->current_player);
    b->turn_number += 1;
}