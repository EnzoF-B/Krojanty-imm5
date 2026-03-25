#include <stdint.h>
#include <stdbool.h>
#include "const.h"
#include "game.h"

#ifndef ZOBRIST_H

#define ZOBRIST_H

#define TT_SIZE 100003 // un nombre premier pour réduire les collisions

// Type de score
typedef enum { TT_EXACT=0, TT_LOWERBOUND=1, TT_UPPERBOUND=2 } TTRole;

// Tableau qui à chaque position donne une clé unique
typedef struct {
    uint64_t key;  // clé Zobrist de la position
    int score;     // score évalué
    int depth;  
    TTRole flag;
} TTEntry;

extern TTEntry transpositionTable[TT_SIZE];

extern uint64_t zobrist_table[BOARD_SIZE][BOARD_SIZE][5];
extern uint64_t zobrist_player[2];

/* === Fonctions === */
// Initialisation
void zobrist_init();

// Calcul complet d’un hash
uint64_t zobrist_hash(const Board* b);

// Mise à jour incrémentale
void zobrist_add_piece(GameState* gs, Coord c, Piece p);
void zobrist_remove_piece(GameState* gs, Coord c, Piece p);
void zobrist_toggle_player(GameState* gs, Player old_player, Player new_player);

// Table de transposition
void tt_store(uint64_t key, int score, int depth, TTRole flag);
int tt_lookup(uint64_t key, int depth, int* outScore, int *alpha, int *beta);

uint32_t tt_index(uint64_t key);

#endif