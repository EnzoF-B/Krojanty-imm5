#include "memoization.h"
#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "game.h"
#include "coord.h"
#include <time.h>
#include "ai.h"

/* forward decl si l'entete n'est pas encore expose */
void tt_init();

//Déclaration de la table de stockage
//TTEntry transpositionTable[TT_SIZE];

TTEntry transpositionTable[TT_SIZE];

//Déclaration des variables
uint64_t zobrist_table[BOARD_SIZE][BOARD_SIZE][5];
uint64_t zobrist_player[2];




// Rajouter une position dans la table
//void tt_store(uint64_t key, int score, int depth) {
//    size_t idx = key % TT_SIZE;
//    transpositionTable[idx].key = key;
//    transpositionTable[idx].score = score;
//    transpositionTable[idx].depth = depth;
//}

void tt_store(uint64_t key, int score, int depth, TTRole flag){
    uint32_t idx = tt_index(key);
    TTEntry *e = &transpositionTable[idx];
    /* simple replacement policy : replace if deeper or empty */
    if(e->key == 0 || depth >= e->depth){
        e->key = key;
        e->score = score;
        e->depth = depth;
        e->flag = flag;
    }
}

uint32_t tt_index(uint64_t key) {
    return (uint32_t)(key % TT_SIZE);
}

// Générer un aléatoire de 64 bits max
uint64_t rand64() {
    uint64_t r1 = (uint64_t)(rand() & 0xFFFF);
    uint64_t r2 = (uint64_t)(rand() & 0xFFFF);
    uint64_t r3 = (uint64_t)(rand() & 0xFFFF);
    uint64_t r4 = (uint64_t)(rand() & 0xFFFF);
    return (r1 << 48) ^ (r2 << 32) ^ (r3 << 16) ^ r4;
}

//Vérifier si on a déjà cherché une position
//int tt_lookup(uint64_t key, int depth, int* outScore) {
//    size_t idx = key % TT_SIZE;
//    if(transpositionTable[idx].key == key && transpositionTable[idx].depth >= depth){
//        *outScore = transpositionTable[idx].score;
//        return 1; // trouvé
//    }
//    return 0; // non trouvé
//}

int tt_lookup(uint64_t key, int depth, int *outScore, int *alpha, int *beta){
    uint32_t idx = tt_index(key);
    TTEntry *e = &transpositionTable[idx];
    if(e->key == key && e->depth >= depth){
        if(e->flag == TT_EXACT){
            *outScore = e->score;
            return 1;
        }
        if(e->flag == TT_LOWERBOUND){
            if(e->score > *alpha) *alpha = e->score;
        } else if(e->flag == TT_UPPERBOUND){
            if(e->score < *beta) *beta = e->score;
        }
        if(*alpha >= *beta){
            *outScore = e->score;
            return 1;
        }
    }
    return 0;
}

// Attribue un numéro unique à chaque position
uint64_t zobrist_hash(const Board* b) {
    uint64_t h = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            Piece p = b->cells[y][x];
            if (p != EMPTY) { 
                h ^= zobrist_table[y][x][p];
            }
        }
    }
    h ^= zobrist_player[b->current_player];
    return h;
}

//Diverses fonctions pour update le code associé à chaque position
void zobrist_remove_piece(GameState* gs, Coord c, Piece p) {
    if (!gs->UseAI || p == EMPTY) return;
    gs->zobrist_key ^= zobrist_table[c.y][c.x][p];
}

void zobrist_add_piece(GameState* gs, Coord c, Piece p) {
    if (!gs->UseAI || p == EMPTY) return;
    gs->zobrist_key ^= zobrist_table[c.y][c.x][p];
}

void zobrist_toggle_player(GameState* gs, Player old_player, Player new_player) {
    if (!gs->UseAI) return;
    gs->zobrist_key ^= zobrist_player[old_player];
    gs->zobrist_key ^= zobrist_player[new_player];
}

//Initialisation de la memoization
void zobrist_init() {
     srand((unsigned)time(NULL));
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            for (int p = 0; p < 5; p++) {  // 5 types de pièces
                zobrist_table[y][x][p] = rand64();
            }
        }
    }
    // Nombre aléatoire associé à chaque joueur
    zobrist_player[0] = rand64(); // PLAYER_BLUE
    zobrist_player[1] = rand64();
}
