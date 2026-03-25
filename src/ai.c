/**
 * \file ai.c
 * \brief Implémentation de l’Intelligence Artificielle pour le jeu Krojanty.
 *
 * Ici, on a programmé l’IA en utilisant l’algorithme Minimax avec allégation alpha-Beta,
 *
 * Étapes principales :
 *  - On évalue une position (fonction eval_board).
 *  - On explore récursivement les coups possibles (search_rec).
 *  - On choisit le meilleur coup pour l’IA (ai_choose_move).
 */

#include "game.h"
#include "ai.h"
#include <limits.h>
#include <string.h>
#include "memoization.h"

/**
 * \brief Donne une valeur aux situations terminales (victoire/défaite/nul).
 *
 * Quand la partie est finie, pas besoin de continuer à chercher :
 * on retourne une grande valeur positive si "me" a gagné,
 * et une grande valeur négative si "me" a perdu.
 *
 * \param gr Résultat de la partie (victoire BLEU, ROUGE ou match nul).
 * \param me Le joueur IA.
 * \return Une valeur entière très grande (victoire/défaite) ou 0 (nul).
 */
static int terminal_utility(GameResult gr, Player me){
    if (gr == GAME_WIN_BLUE) return (me==PLAYER_BLUE)?  10000000 : -10000000;
    if (gr == GAME_WIN_RED)  return (me==PLAYER_RED)?   10000000 : -10000000;
    return 0; /* match nul */
}

//Passer au joueur suivant en interne
static inline Player other(Player p) {
    return (p == PLAYER_BLUE) ? PLAYER_RED : PLAYER_BLUE;
}



/**
 * \brief Évaluation statique optimisée du plateau NON terminal.
 *        (identique en logique, mais plus rapide que l’ancienne version)
 */
static int eval_board(const Board* b, const GameState* gs, Player me){
    Player opp = (me == PLAYER_BLUE) ? PLAYER_RED : PLAYER_BLUE;

    /* --- Score de base : soldats et roi --- */
    int blue_alive = 9 - gs->blue_soldiers_captured;
    int red_alive  = 9 - gs->red_soldiers_captured;
    int ctrl_blue = gs->blue_points - blue_alive;
    int ctrl_red = gs->red_points - red_alive;
    int blue_score =  blue_alive * 100 + ctrl_blue * 20;
    int red_score  =  red_alive  * 100 + ctrl_red * 20;

    int score_for_blue = blue_score - red_score;
    int s = (me == PLAYER_BLUE) ? score_for_blue : -score_for_blue;

    GameResult gr = check_victory(b, gs);
    if (gr != GAME_ONGOING) return terminal_utility(gr, me);

    return s;
}



/**
 * \brief Recherche récursive (Minimax avec ou sans Alpha-Beta).
 *
 * Cette fonction explore les coups possibles à une certaine profondeur.
 * Elle choisit la meilleure évaluation selon que l’IA maximise ou minimise.
 *
 * \param b     Plateau actuel.
 * \param gs    État courant de la partie.
 * \param me    Joueur pour lequel l’IA calcule.
 * \param depth Profondeur restante à explorer.
 * \param alpha Borne alpha pour l’élagage (si Alpha-Beta activé).
 * \param beta  Borne beta pour l’élagage (si Alpha-Beta activé).
 * \param cfg   Configuration de l’IA (profondeur, alpha-beta activé ou non).
 * \return La valeur de la position (score calculé).
 */
static int search_rec(const Board* b, const GameState* gs, Player me,
                       int alpha, int beta){
                        GameResult gr = check_victory(b, gs);
    if (gr != GAME_ONGOING) {
        return terminal_utility(gr, me);
    }
    if (gs->Depth <= 0){
        return eval_board(b, gs, me);
    }
    int ttScore;
    if(tt_lookup(gs->zobrist_key, gs->Depth, &ttScore, &alpha, &beta)){
        return ttScore; // Si on a déjà évalué cette position, on return
    }
    Move mv[256];
    int n = gen_legal_moves(b, mv, 256);
    if (n <= 0){
        int score = eval_board(b, gs, me);
        tt_store(gs->zobrist_key, score, gs->Depth, TT_EXACT); // On stocke la position calculée
        return score;
    }

    int maximizing = (b->current_player == me);
    int best = maximizing ? INT_MIN/2 : INT_MAX/2;
    
    for(int i=0; i<n; ++i){
            Board nb = *b; GameState ng = *gs; GameResult ngr=GAME_ONGOING;
            if(!apply_move(&nb, &ng, mv[i], &ngr)) continue;
            ng.Depth = gs->Depth - 1;
            nb.current_player = other(b->current_player);  // joueur suivant
            int val = search_rec(&nb, &ng, me, alpha, beta);
        if (maximizing){
            if (val > best) best = val;
            if (best > alpha) alpha = best;
        }
        else{
            if (val < best) best = val;
            if (best < beta) beta = best;
        }
        if (alpha >= beta) break; 
    } 
    int flag = TT_EXACT;
    if (best <= alpha) flag = TT_UPPERBOUND;
    else if (best >= beta) flag = TT_LOWERBOUND;
    tt_store(gs->zobrist_key, best, gs->Depth, flag ); // On stocke la position calculée
    return best;
}


/**
 * \brief Choisir le meilleur coup pour l’IA.
 *
 * Cette fonction est appelée par le reste du programme quand c’est à l’IA de jouer.
 * Elle génère tous les coups possibles, appelle la recherche récursive,
 * et choisit celui qui maximise les chances de victoire.
 *
 * \param b         Plateau actuel.
 * \param gs        État courant de la partie.
 * \param me        Joueur IA (BLEU ou ROUGE).
 * \param out_move  (sortie) Coup choisi par l’IA.
 * \param out_score (sortie) Valeur numérique du coup choisi.
 * \return 1 si un coup a été trouvé, 0 sinon.
 */
int ai_choose_move(const Board* b, const GameState* gs, Player me,
                    Move* out_move, int* out_score){
    Move mv[256];
    int n = gen_legal_moves(b, mv, 256);
    if (n <= 0) return 0;

    int best_val = (b->current_player==me)? INT_MIN/2 : INT_MAX/2;
    int best_idx = -1;

    for(int i=0;i<n;++i){
        Board nb = *b; GameState ng = *gs; GameResult ngr=GAME_ONGOING;
        if(!apply_move(&nb, &ng, mv[i], &ngr)) continue;
        ng.Depth = gs->Depth - 1;
        nb.current_player = other(b->current_player);
        int v = search_rec(&nb, &ng, me, INT_MIN/2, INT_MAX/2);

        if ((b->current_player==me && v > best_val) || 
            (b->current_player!=me && v < best_val) ){
             best_val = v; best_idx = i; }
    }

    if (best_idx < 0) return 0;
    if (out_move)  *out_move  = mv[best_idx];
    if (out_score) *out_score = best_val;
    return 1;
}