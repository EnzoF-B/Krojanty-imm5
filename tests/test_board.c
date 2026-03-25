/**
 * @file test_board.c
 * @brief Tests unitaires du plateau (init, get/set, chemins).
 *
 * @author marcelin.trag@uha.fr
 * @date 2025-09-17
 * @version 1.0.0
 *
 */
#include "test_framework.h"
#include "board.h"

void test_board_init_and_get_set(){
    Board b; board_init(&b, PLAYER_BLUE);
    for(int y=0;y<BOARD_SIZE;y++) for(int x=0;x<BOARD_SIZE;x++)
        ASSERT_EQ_INT(EMPTY, b.cells[y][x]);
    ASSERT_EQ_INT(PLAYER_BLUE, b.current_player);
    ASSERT_EQ_INT(0, b.turn_number);

    board_set(&b, (Coord){4,4}, BLUE_KING);
    ASSERT_EQ_INT(BLUE_KING, board_get(&b,(Coord){4,4}));
    printf("[OK] board: init/get/set\n");
}

void test_board_path_clear(){
    Board b; board_init(&b, PLAYER_BLUE);
    for(int y=0;y<BOARD_SIZE;y++) for(int x=0;x<BOARD_SIZE;x++) board_set(&b,(Coord){x,y},EMPTY);
    board_set(&b, (Coord){0,0}, BLUE_KING);
    board_set(&b, (Coord){0,5}, RED_SOLDIER);
    ASSERT_TRUE(board_path_clear(&b,(Coord){0,0},(Coord){0,4}));
    ASSERT_TRUE(!board_path_clear(&b,(Coord){0,0},(Coord){0,6}));
    ASSERT_TRUE(!board_path_clear(&b,(Coord){0,0},(Coord){1,1}));
    printf("[OK] board: path_clear\n");
}

void run_tests_board(){
    test_board_init_and_get_set();
    test_board_path_clear();
}

