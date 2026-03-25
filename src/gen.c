/**
 * @file gen.c
 * @brief Génération de tous les coups légaux possibles pour un joueur.
 *
 * Ici on fabrique la liste de tous les coups qu’un joueur peut jouer
 * à partir d’un plateau donné :
 *   - On parcourt toutes les pièces alliées.
 *   - On regarde dans les 4 directions (haut, bas, gauche, droite).
 *   - On ajoute chaque coup possible si c’est légal.
 *
 * Cette partie est cruciale pour l’IA (Minimax) et pour
 * vérifier les coups d’un joueur humain.
 */

#include "gen.h"

/**
 * @brief Ajoute un coup s’il est légal, dans la liste @p out.
 *
 * Vérifie deux choses :
 *   - La liste n’est pas pleine (on respecte @p max_out).
 *   - Le coup est bien autorisé selon les règles de base.
 *
 * @param b       Plateau courant.
 * @param m       Coup candidat (from → to).
 * @param out     Tableau de coups de sortie.
 * @param max_out Taille max du tableau @p out.
 * @param n       Compteur de coups déjà placés (incrémenté si succès).
 */
static void push_if_legal(const Board* b, Move m, Move* out, int max_out, int* n){
    if(*n>=max_out) return;
    if(move_is_legal_basic(b,m)){ out[*n]=m; (*n)++; }
}

/**
 * @brief Génère tous les coups légaux du joueur courant.
 *
 * Méthode :
 *   - On identifie le joueur courant (@ref Board.current_player).
 *   - Pour chaque case du plateau :
 *       - Si c’est une de mes pièces → je la considère comme point de départ.
 *       - Je teste 4 directions : droite, gauche, bas, haut.
 *       - Je “avance” pas à pas :
 *           - Si la case est vide → coup possible, je continue.
 *           - Si c’est un ennemi → coup possible, mais je m’arrête après.
 *           - Si c’est un allié → on bloque, pas de coup.
 *
 * @param b       Plateau à explorer.
 * @param out     Tableau où écrire les coups trouvés.
 * @param max_out Taille max du tableau @p out.
 * @return Nombre total de coups légaux trouvés.
 */
int gen_legal_moves(const Board* b, Move* out, int max_out){
    int n=0; if(max_out<=0) return 0;
    Player me=b->current_player;

    for(int y=0;y<BOARD_SIZE;++y) for(int x=0;x<BOARD_SIZE;++x){
        Coord from={x,y}; 
        Piece p=board_get(b,from);

        if(p==EMPTY || !piece_is_ally(p,me)) continue; // ignorer vide ou ennemi

        int dirs[4][2]={{1,0},{-1,0},{0,1},{0,-1}}; // droite, gauche, bas, haut
        for(int d=0;d<4;++d){
            int dx=dirs[d][0], dy=dirs[d][1];
            Coord cur=from;

            while(1){
                cur.x+=dx; cur.y+=dy;
                if(!coord_in_bounds(cur)) break; // hors plateau -> stop

                Piece q=board_get(b,cur);
                Move m=(Move){from,cur};

                if(q==EMPTY){ 
                    // case vide : on peut s’y déplacer et continuer à avancer
                    push_if_legal(b,m,out,max_out,&n); 
                    continue;
                }
                if(piece_is_enemy(q,me)){
                    // case occupée par un ennemi : coup possible mais arrêt
                    push_if_legal(b,m,out,max_out,&n);
                }
                break; // allié ou ennemi = stop
            }
        }
    }
    return n;
}