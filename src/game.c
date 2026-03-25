/**
 * @file game.c
 * @brief Règle le cœur de la partie : initialisation, placement, points, victoires, captures spéciales et application d’un coup.
 *
 * Idée simple :
 *  - On prépare l’état du jeu (@ref game_init_state).
 *  - On place les pièces de départ (auto ou via un petit ASCII lisible).
 *  - À chaque coup : on marque la case quittée (points), on déplace la pièce,
 *    on applique les captures spéciales (Linca / Seultou), puis on teste la victoire.
 *
 * On garde tout très clair et séquencé pour ne pas se perdre.
 */

#include <stdio.h>
#include <string.h>
#include "game.h"
#include "ai.h"
#include "memoization.h"

/* ====== INIT & PLACEMENT ====== */

/**
 * @brief Initialise les compteurs et le tableau de contrôle des cases.
 *
 * Pourquoi ?
 *  - Remettre la partie à zéro : captures, rois vivants, scores, nombre max de tours.
 *  - Nettoyer les informations de contrôle de chaque case (personne ne “possède” encore les cases).
 *
 * @param gs       État du jeu à remplir.
 * @param max_turns Nombre maximum de tours autorisés (ex: 64).
 */
void game_init_state(GameState* gs, int max_turns, const Board* b, int ai_enabled){
    gs->blue_soldiers_captured=0;
    gs->red_soldiers_captured =0;
    gs->blue_king_alive=true;
    gs->red_king_alive =true;
    gs->max_turns=max_turns;
    gs->blue_points = 0;
    gs->red_points  = 0;
    gs->UseAI = ai_enabled;
    gs->Depth = 3; // constante
    for(int y=0;y<BOARD_SIZE;++y){
        for(int x=0;x<BOARD_SIZE;++x){
            gs->control[y][x] = CTRL_NONE;
            gs->visited_blue[y][x] = false;
            gs->visited_red [y][x] = false;
        }
    }
}

/** 
 * @brief Cette fonction sert à savoir quelle case est controlé par quel joueur à l'aide d'un tableau qui garde en mémoire quelle case appartient a qui
 * Comment ça marche :
 * - On commence par déterminer le camp pour savoir à qui on doit attribuer la case
 * - On effectue ensuite une rapide vérification pour voir si l'abscice et l'ordonner sont dans le tableau
 * - On donne ensuite la case au camp du pion qui est sur la case
 * - Puis on marque cette case comme appartenant au joueur afin qu'il ne récupère pas de nouveau points si il y retourne
 * 
 * @param owner sert a savoir a qui va appartenir cette case
 */
static void mark_control_and_visited(GameState* gs, Player owner,
                                     const Coord* list, int n){
    ControlOwner mine = (owner==PLAYER_BLUE? CTRL_BLUE:CTRL_RED);
    for(int i=0;i<n;i++){
        Coord c = list[i];
        if(c.x<0||c.x>=BOARD_SIZE||c.y<0||c.y>=BOARD_SIZE) continue;
        gs->control[c.y][c.x] = mine;
        if(owner==PLAYER_BLUE) gs->visited_blue[c.y][c.x] = true;
        else                   gs->visited_red [c.y][c.x] = true;
    }
}

/**
 * @brief Cette fonction vérifie que chaque mouvement ne creer aucune incohérence
 * Comment ça marche :
 *  - On commence par initialiser les points, les soldats des bleus et des rouges à 0 (toujours avec short)
 *  - On parcour une première fois le plateau pour regarder le nombre de soldat en vie puis on ajoute les points en fonction
 *  - Puis on effectue une deuxième boucle pour savoir quelle case appartient a quel camp et on ajoute les points en fonction
 *  - Puis à la fin on ajoute les points
*/
void game_recompute_scores(GameState* gs, const Board* b){
    int ctrlB=0, ctrlR=0;
    int soldiersB=0, soldiersR=0;

    /* Option : les cases occupées imposent au minimum leur contrôle à leur propriétaire. */
    for(int y=0;y<BOARD_SIZE;y++){
        for(int x=0;x<BOARD_SIZE;x++){
            Piece p = b->cells[y][x];
            if(p==BLUE_SOLDIER) soldiersB++;
            else if(p==RED_SOLDIER) soldiersR++;

            if(p==BLUE_SOLDIER || p==BLUE_KING){
                gs->control[y][x] = CTRL_BLUE;
            } else if(p==RED_SOLDIER || p==RED_KING){
                gs->control[y][x] = CTRL_RED;
            }
        }
    }

    /* Compte des cases contrôlées */
    for(int y=0;y<BOARD_SIZE;y++){
        for(int x=0;x<BOARD_SIZE;x++){
            if(gs->control[y][x]==CTRL_BLUE) ctrlB++;
            else if(gs->control[y][x]==CTRL_RED) ctrlR++;
        }
    }

    /* Score = soldats vivants + cases contrôlées */
    gs->blue_points = soldiersB + ctrlB;
    gs->red_points  = soldiersR + ctrlR;
}

/**
 * @brief Initialise le contrôle et les points de départ en fonction des pièces placées.
 *
 * Chaque case occupée par un pion/roi devient immédiatement contrôlée par son camp.
 * Les soldats vivants valent 1 point chacun.
 * Les rois ne valent pas de points, mais la case qu'ils occupent vaut 1 point.
 *
 * @param b  Plateau (positions des pièces).
 * @param gs État du jeu (points et contrôle mis à jour).
 */
void game_init_control(Board* b, GameState* gs){
    gs->blue_points = 0;
    gs->red_points  = 0;

    for(int y=0; y<BOARD_SIZE; ++y){
        for(int x=0; x<BOARD_SIZE; ++x){
            Coord c = {x, y};
            Piece p = board_get(b, c);

            if(p == BLUE_SOLDIER){
                gs->control[y][x] = CTRL_BLUE;
                gs->visited_blue[y][x] = true;
                gs->blue_points += 1; // soldat vivant = 1 point
            }
            else if(p == BLUE_KING){
                gs->control[y][x] = CTRL_BLUE;
                gs->visited_blue[y][x] = true;
                gs->blue_points += 1; // roi ne vaut pas mais sa case = 1
            }
            else if(p == RED_SOLDIER){
                gs->control[y][x] = CTRL_RED;
                gs->visited_red[y][x] = true;
                gs->red_points += 1;  // soldat vivant = 1 point
            }
            else if(p == RED_KING){
                gs->control[y][x] = CTRL_RED;
                gs->visited_red[y][x] = true;
                gs->red_points += 1;  // roi ne vaut pas mais sa case = 1
            }
            else {
                gs->control[y][x] = CTRL_NONE;
                gs->visited_blue[y][x] = false;
                gs->visited_red[y][x]  = false;
            }
        }
    }
}

/**
 * @brief Place un plateau “classique” par défaut.
 *
 * Idée simple :
 *  - On vide tout.
 *  - Roi bleu en A9, ligne de soldats bleus au rang 8.
 *  - Roi rouge en I1, ligne de soldats rouges au rang 2.
 *  - Le joueur courant est BLEU (dans @ref board_init).
 */
void board_place_initial(Board* b, GameState* gs){
    board_init(b,PLAYER_BLUE);
    for(int y=0;y<BOARD_SIZE;++y)
        for(int x=0;x<BOARD_SIZE;++x)
            board_set(b,(Coord){x,y},EMPTY);

    /* Placement des pièces */
    board_set(b,(Coord){0,0},BLUE_KING);                  // A9
    for(int x=0;x<BOARD_SIZE;++x) board_set(b,(Coord){x,1},BLUE_SOLDIER); // rang 8

    board_set(b,(Coord){8,8},RED_KING);                   // I1
    for(int x=0;x<BOARD_SIZE;++x) board_set(b,(Coord){x,7},RED_SOLDIER);  // rang 2

    // contrôle initial
    game_init_control(b, gs);
}

/**
 * @brief Place un plateau depuis 9 lignes ASCII (lisibles à l’œil).
 *
 * Format :
 *  - 9 lignes, de la rangée 9 (index 0) à la rangée 1 (index 8).
 *  - Symboles autorisés : 'K' (roi bleu), 'S' (soldat bleu), 'k' (roi rouge), 's' (soldat rouge), '.' (vide).
 *  - Les espaces sont tolérés pour aérer.
 *
 * Choix de conception :
 *  - On vérifie que chaque ligne a bien 9 cellules utiles.
 *  - On exige qu’il y ait un roi bleu et un roi rouge (sinon false).
 *  - On met le joueur courant à BLEU et le numéro de tour à 0.
 *
 * @param b    Plateau à remplir.
 * @param rows Pointeurs vers 9 chaînes (les 9 rangées).
 * @return true si placement OK, false sinon.
 */
bool board_place_from_ascii(Board* b, GameState* gs, const char* rows[9]){
    if(!rows) return false;
    board_init(b,PLAYER_BLUE);
    for(int y=0;y<BOARD_SIZE;++y)
        for(int x=0;x<BOARD_SIZE;++x)
            board_set(b,(Coord){x,y},EMPTY);

    bool blueK=false, redK=false;
    for(int y=0;y<BOARD_SIZE;++y){
        const char* line=rows[y]; if(!line) return false;
        int x=0;
        for(int i=0; line[i] && x<BOARD_SIZE; ++i){
            char c=line[i]; if(c==' ') continue;
            Piece p=EMPTY;
            if(c=='K'){ p=BLUE_KING; blueK=true; }
            else if(c=='S'){ p=BLUE_SOLDIER; }
            else if(c=='k'){ p=RED_KING; redK=true; }
            else if(c=='s'){ p=RED_SOLDIER; }
            else if(c=='.'){ p=EMPTY; }
            else return false;
            board_set(b,(Coord){x,y},p); x++;
        }
        if(x!=BOARD_SIZE) return false;
    }
    b->current_player=PLAYER_BLUE; 
    b->turn_number=0;
    if(!blueK || !redK) return false;

    // contrôle initial
    game_init_control(b, gs);

    return true;
}

/**
 * @brief Marque la case d'arrivé comme contrôlée par le joueur qui vient de jouer, et met à jour les points.
 *
 * Règle simple :
 *  - Quand j'arrive sur une case, elle devient à moi (CTRL_BLUE/CTRL_RED).
 *  - Si la case était neutre : +1 pour moi.
 *  - Si elle appartenait à l’autre : +1 pour moi et -1 pour l’autre (je lui reprends).
 *  - Si elle était déjà à moi : rien ne change.
 *
 * @param gs    État des scores et contrôles de cases.
 * @param mover Joueur qui se déplace (BLEU/ROUGE).
 * @param from  Case quittée.
 */
void score_enter_cell(GameState* gs, Player mover, Coord to){
    ControlOwner co   = gs->control[to.y][to.x];
    ControlOwner mine = (mover==PLAYER_BLUE? CTRL_BLUE : CTRL_RED);
    bool* already = (mover==PLAYER_BLUE)
                    ? &gs->visited_blue[to.y][to.x]
                    : &gs->visited_red [to.y][to.x];

    // Si cette équipe a déjà "validé" cette case une fois dans la partie : pas de nouveau +1
    if (*already){
        if (co != mine) gs->control[to.y][to.x] = mine;
        return;
    }

    if (co == CTRL_NONE){
        if (mover==PLAYER_BLUE) gs->blue_points += 1;
        else                    gs->red_points  += 1;
    } else if (co != mine){
        if (mover==PLAYER_BLUE){ gs->blue_points += 1; gs->red_points  -= 1; }
        else                    { gs->red_points  += 1; gs->blue_points -= 1; }
    }

    *already = true;
    gs->control[to.y][to.x] = mine;
}


/**
 * @brief Marque une case comme libérée, retire le contrôle et ajuste les points.
 *
 * Utilisé lors de captures pour enlever les points associés à une pièce supprimée.
 *
 * @param gs État du jeu (points et contrôle modifiés).
 * @param c  Coordonnée de la case à libérer.
 */

static void release_cell(GameState* gs, Coord c){
    ControlOwner co = gs->control[c.y][c.x];
    if(co == CTRL_BLUE){
        gs->blue_points -= 1;
    } else if(co == CTRL_RED){
        gs->red_points -= 1;
    }
    gs->control[c.y][c.x] = CTRL_NONE;
}

/* ====== VICTOIRES ====== */

/**
 * @brief Vérifie si la partie est gagnée/perdue/nulle/continue.
 *
 * Ordre logique (raisonnement simple) :
 *  1) Si un roi est mort : victoire de l’autre.
 *  2) Si un roi entre dans la cité adverse : il gagne.
 *  3) Si 8 soldats d’une couleur ont été capturés : l’autre gagne.
 *  4) Si on a atteint le nombre max de tours : on compare les points.
 *  5) Sinon la partie continue.
 *
 * @param b  Plateau (positions).
 * @param gs État (rois vivants, captures, points…).
 * @return Résultat (win bleu, win rouge, draw, ongoing).
 */
GameResult check_victory(const Board* b, const GameState* gs){
    if(!gs->blue_king_alive) return GAME_WIN_RED;
    if(!gs->red_king_alive)  return GAME_WIN_BLUE;

    // Conquête de cité par le roi adverse
    for(int y=0;y<BOARD_SIZE;++y) for(int x=0;x<BOARD_SIZE;++x){
        Coord c={x,y}; Piece pc=board_get(b,c);
        if(pc==BLUE_KING && c.x==CITY_RED.x && c.y==CITY_RED.y)  return GAME_WIN_BLUE;
        if(pc==RED_KING  && c.x==CITY_BLUE.x && c.y==CITY_BLUE.y) return GAME_WIN_RED;
    }

    if(gs->red_soldiers_captured>=8) return GAME_WIN_BLUE;
    if(gs->blue_soldiers_captured>=8) return GAME_WIN_RED;

    if(b->turn_number>=gs->max_turns){
        if(gs->blue_points > gs->red_points) return GAME_WIN_BLUE;
        if(gs->red_points > gs->blue_points) return GAME_WIN_RED;
        return GAME_DRAW;
    }
    return GAME_ONGOING;
}

/* ====== CAPTURES SPÉCIALES ====== */

/**
 * @brief Règle “Linca” : capture par sandwich autour de la case d’arrivée.
 *
 * Idée :
 *  - Après mon déplacement en sur une case, je regarde les 4 directions (haut/bas/gauche/droite).
 *  - Si juste à côté il y a un ennemi, et *derrière lui* (dans la même direction) un de mes alliés,
 *    alors l’ennemi est pris en sandwich ⇒ capturé.
 *  - La case capturée devient neutre en contrôle.
 *
 * @param b     Plateau (modifié si capture).
 * @param gs    État (maj captures/points/rois vivants).
 * @param to    Case d’arrivée de mon coup.
 * @param mover Joueur qui vient de bouger.
 * @return Nombre de pièces capturées par Linca.
 */
static int apply_linca_captures(Board* b, GameState* gs, Coord to, Player mover){
    int taken=0; 
    int dirs[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
    for(int d=0;d<4;++d){
        int dx=dirs[d][0], dy=dirs[d][1];
        Coord adj={to.x+dx,to.y+dy}, back={to.x+2*dx,to.y+2*dy};
        if(!coord_in_bounds(adj)||!coord_in_bounds(back)) continue;
        Piece p_adj=board_get(b,adj), p_back=board_get(b,back);
        if(p_adj!=EMPTY && piece_is_enemy(p_adj,mover) && piece_is_ally(p_back,mover)){
            // capture
            if(p_adj==BLUE_KING) gs->blue_king_alive=false;
            else if(p_adj==RED_KING) gs->red_king_alive=false;
            else if(p_adj==BLUE_SOLDIER) gs->blue_soldiers_captured++;
            else if(p_adj==RED_SOLDIER)  gs->red_soldiers_captured++;
            zobrist_remove_piece(gs, adj, p_adj);  // Enlever la pièce capturée dans le hashage Zobrist
            board_set(b,adj,EMPTY);
            release_cell(gs, adj);   // enlève aussi le point de contrôle
            taken++;
        }
    }
    return taken;
}

/**
 * @brief Règle “Seultou” : capture directionnelle juste *devant* l’arrivée, sauf si l’ennemi est protégé par un allié à lui derrière.
 *
 * Résumé :
 *  - On regarde la direction du déplacement (horizontale OU verticale).
 *  - Si, juste devant l’arrivée, il y a un ennemi @p p_adj :
 *      - On cherche une case *derrière l’ennemi* dans la même direction.
 *      - S’il y a un allié à lui (protection), on ne capture pas.
 *      - S’il n’y a personne (ou mur) : pas protégé => on capture.
 *  - La case capturée devient neutre.
 *
 * @param b     Plateau (modifié si capture).
 * @param gs    État (maj captures/points/rois vivants).
 * @param from  Case de départ (sert à connaître la direction du coup).
 * @param to    Case d’arrivée.
 * @param mover Joueur qui vient de jouer.
 * @return 1 si une capture a eu lieu, 0 sinon.
 */
static int apply_seultou_captures(Board* b, GameState* gs, Coord from, Coord to, Player mover){
    int dx = (to.x>from.x) ? 1 : (to.x<from.x ? -1 : 0);
    int dy = (to.y>from.y) ? 1 : (to.y<from.y ? -1 : 0);
    if(dx!=0 && dy!=0) return 0;
    if(dx==0 && dy==0) return 0;

    Coord adj={to.x+dx,to.y+dy};
    if(!coord_in_bounds(adj)) return 0;
    Piece p_adj=board_get(b,adj);
    if(p_adj==EMPTY || !piece_is_enemy(p_adj,mover)) return 0;

    // protection par ALLIÉ du défenseur ?
    Coord behind={to.x+2*dx,to.y+2*dy};
    bool protected=false;
    if(coord_in_bounds(behind)){
        Piece p_back=board_get(b,behind);
        if(p_back!=EMPTY && piece_is_ally(p_back, piece_is_ally(p_adj,PLAYER_BLUE)?PLAYER_BLUE:PLAYER_RED))
            protected=true;
    }

    if(!protected){
        if(p_adj==BLUE_KING) gs->blue_king_alive=false;
        else if(p_adj==RED_KING) gs->red_king_alive=false;
        else if(p_adj==BLUE_SOLDIER) gs->blue_soldiers_captured++;
        else if(p_adj==RED_SOLDIER)  gs->red_soldiers_captured++;
        zobrist_remove_piece(gs, adj, p_adj);  // Enlever la pièce capturée dans le hashage Zobrist
        board_set(b,adj,EMPTY);
        release_cell(gs, adj);   // enlève aussi le point de contrôle
        return 1;
    }
    return 0;
}

/* ====== APPLY MOVE ====== */

/**
 * @brief Applique un coup complet sur le plateau en respectant toutes les règles du jeu.
 *
 * Étapes réalisées :
 *  1) Validation de base du coup avec @ref move_is_legal_basic. Si le coup est illégal, la fonction renvoie false.
 *  2) Déplacement de la pièce avec @ref apply_move_basic.
 *  3) Mise à jour du score et du contrôle de la case d’arrivée via @ref score_enter_cell.
 *  4) Mise à jour du hashage Zobrist :
 *      - Retrait de la pièce de la case de départ.
 *      - Ajout de la pièce sur la case d’arrivée.
 *      - Changement de joueur courant.
 *  5) Application des captures spéciales :
 *      - @ref apply_linca_captures (capture en sandwich).
 *      - @ref apply_seultou_captures (capture directionnelle protégée ou non).
 *  6) Recalcul strict des scores après toutes les modifications via @ref game_recompute_scores.
 *  7) Vérification du statut de la partie avec @ref check_victory et écriture dans @p out_result si demandé.
 *
 * @param b          Plateau (modifié).
 * @param gs         État du jeu (points, captures, rois vivants, contrôle modifiés).
 * @param m          Coup à appliquer.
 * @param out_result Optionnel : reçoit le résultat de la partie après le coup (victoire, défaite, nul, en cours).
 * @return true si le coup a été appliqué avec succès, false si le coup est illégal.
 */
bool apply_move(Board* b, GameState* gs, Move m, GameResult* out_result){
    if(!move_is_legal_basic(b,m)) return false;

    Player mover = b->current_player;
    Piece moving_piece = board_get(b, m.from);

    // 1) jouer le coup
    apply_move_basic(b,m);

    // 2) scorer la CASE D’ARRIVÉE
    score_enter_cell(gs, mover, m.to);

    // 3) Enlever la pièce de la case de départ dans le hashage Zobrist
    zobrist_remove_piece(gs, m.from, moving_piece);

    // 4)Mettre la pièce dans la case d'arrivée dans le hashage Zobrist
    zobrist_add_piece(gs, m.to, moving_piece);
    zobrist_toggle_player(gs, mover, b->current_player);

    // 5) captures spéciales
    apply_linca_captures(b,gs,m.to,mover);
    apply_seultou_captures(b,gs,m.from,m.to,mover);

    /* --->  Synchronisation stricte du score après toutes les modifs  <--- */
    game_recompute_scores(gs, b);

    // 6) statut de partie
    GameResult gr=check_victory(b,gs);
    if(out_result) *out_result=gr;
    return true;
}

/* ====== POST-PLACEMENT : MARQUER LES CONTRÔLES INITIAUX & DÉMARRER À 20 ====== */

/**
 * @brief Marque les cases initialement occupées comme contrôlées et ajuste les points de départ.
 *
 * Règle simple :
 *  - Construit les listes des pièces bleues et rouges.
 *  - Marque ces cases comme “déjà comptées” pour éviter des points immédiats.
 *  - Recalcule les scores globaux.
 *  - Ajuste éventuellement les points à 20 pour commencer la partie.
 *
 * @param gs État du jeu (modifié).
 * @param b  Plateau (positions des pièces utilisées pour le calcul).
 */

static void post_place_seed_control(GameState* gs, const Board* b){
    /* Construit les listes de cases des pièces bleues / rouges */
    Coord blues[BOARD_SIZE*BOARD_SIZE]; int nb=0;
    Coord reds [BOARD_SIZE*BOARD_SIZE]; int nr=0;

    for(int y=0;y<BOARD_SIZE;y++){
        for(int x=0;x<BOARD_SIZE;x++){
            Piece p = b->cells[y][x];
            if(p==BLUE_SOLDIER || p==BLUE_KING) blues[nb++] = (Coord){x,y};
            else if(p==RED_SOLDIER || p==RED_KING) reds[nr++] = (Coord){x,y};
        }
    }

    /* Marque contrôle + “déjà compté” (évite un +1 immédiat au premier départ) */
    mark_control_and_visited(gs, PLAYER_BLUE, blues, nb);
    mark_control_and_visited(gs, PLAYER_RED,  reds, nr);

    /* Recompute global des scores (source de vérité) */
    game_recompute_scores(gs, b);

    /* Si tu veux FORCER 20 au départ quoi qu’il arrive (selon la règle fournie) :
       - Le recompute va donner 19 ou 20 selon la prise en compte des cités, etc.
       - On ajuste à 20 si besoin.
       (La logique de jeu reste cohérente car on recalcule à chaque coup derrière.) */
    if(gs->blue_points < 20) gs->blue_points = 20;
    if(gs->red_points  < 20) gs->red_points  = 20;
}


/**
 * @brief Point d’entrée public pour initialiser les contrôles et points de départ après placement.
 *
 * Utilise @ref post_place_seed_control pour construire l’état initial.
 *
 * @param gs État du jeu (modifié).
 * @param b  Plateau (positions des pièces utilisées pour l’initialisation).
 */
void game_post_setup_seed(GameState* gs, const Board* b){
    post_place_seed_control(gs, b);
}
