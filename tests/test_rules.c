/**
 * @file test_rules.c
 * @brief Tests unitaires des règles de base (allié/ennemi, légalité, déplacement basique).
 *
 * @author marcelin.trag@uha.fr
 * @date 2025-09-17
 * @version 1.0.0
 *
 */

#include "test_framework.h"
#include "rules.h"

void test_piece_ally_enemy(){
    ASSERT_TRUE(piece_is_ally(BLUE_SOLDIER, PLAYER_BLUE));
    ASSERT_TRUE(!piece_is_ally(RED_KING, PLAYER_BLUE));
    ASSERT_TRUE(piece_is_enemy(RED_SOLDIER, PLAYER_BLUE));
    ASSERT_TRUE(!piece_is_enemy(BLUE_KING, PLAYER_BLUE));
    printf("[OK] rules: ally/enemy\n");
}

void test_move_is_legal_basic_and_apply(){
    Board b; board_init(&b, PLAYER_BLUE);
    for(int y=0;y<BOARD_SIZE;y++) for(int x=0;x<BOARD_SIZE;x++) board_set(&b,(Coord){x,y},EMPTY);
    board_set(&b,(Coord){0,0},BLUE_SOLDIER);
    board_set(&b,(Coord){0,5},RED_SOLDIER);

    Move bad_diag = { .from={0,0}, .to={1,1} };
    ASSERT_TRUE(!move_is_legal_basic(&b, bad_diag));

    Move blocked = { .from={0,0}, .to={0,6} };
    ASSERT_TRUE(!move_is_legal_basic(&b, blocked));

    Move ok = { .from={0,0}, .to={0,4} };
    ASSERT_TRUE(move_is_legal_basic(&b, ok));

    apply_move_basic(&b, ok);
    ASSERT_EQ_INT(EMPTY, board_get(&b,(Coord){0,0}));
    ASSERT_EQ_INT(BLUE_SOLDIER, board_get(&b,(Coord){0,4}));
    ASSERT_EQ_INT(PLAYER_RED, b.current_player);
    printf("[OK] rules: basic legality & apply\n");
}

void run_tests_rules(){
    test_piece_ally_enemy();
    test_move_is_legal_basic_and_apply();
}

