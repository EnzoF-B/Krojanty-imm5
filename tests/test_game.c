/**
 * @file test_game.c
 * @brief Tests de la logique de partie (scores, victoires, captures, invalides).
 *
 * @author marcelin.trag@uha.fr
 * @date 2025-09-17
 * @version 1.0.0
 *
 */
 
#include "test_framework.h"
#include "game.h"

void test_board_place_initial_and_scores(){
    Board b; board_init(&b, PLAYER_BLUE); GameState gs; game_init_state(&gs, 64, &b, 0);
    board_place_initial(&b, &gs);
    ASSERT_TRUE(gs.blue_king_alive && gs.red_king_alive);
    ASSERT_TRUE(gs.blue_points >= 9); /* au moins les soldats et cases occupées */
    ASSERT_TRUE(gs.red_points  >= 9);
    printf("[OK] game: initial placement & scores\n");
}

void test_board_place_from_ascii_and_victory_city(){
    Board b; board_init(&b, PLAYER_BLUE); GameState gs; game_init_state(&gs, 64, &b, 0);
    const char* rows[9] = {
        ". k . . . . . . .", // B9 roi rouge (pas sur la cité)
        ". . . . . . . . .",
        ". . . . . . . . .",
        ". . . . . . . . .",
        ". . . . . . . . .",
        ". . . . . . . . .",
        ". . . . . . . . .",
        ". . . . . . . . .",
        ". . . . . . . K ."  // H1 roi bleu (I1 vide)
    };
    ASSERT_TRUE(board_place_from_ascii(&b, &gs, rows));
    GameResult gr = check_victory(&b, &gs);
    ASSERT_EQ_INT(GAME_ONGOING, gr);

    // Faire entrer le roi bleu en I1 (H1 -> I1, mouvement horizontal légal)
    Move m = { .from={7,8}, .to={8,8} };
    GameResult out = GAME_ONGOING;
    ASSERT_TRUE(apply_move(&b, &gs, m, &out));
    ASSERT_EQ_INT(GAME_WIN_BLUE, out);
    printf("[OK] game: victory by city entry (legal move)\n");
}

void test_illegal_move_no_state_change(){
    Board b; board_init(&b, PLAYER_BLUE); GameState gs; game_init_state(&gs, 64, &b, 0);
    board_place_initial(&b, &gs);

    Board before_b = b; // copie par valeur
    GameState before_gs = gs;

    Move illegal_same = { .from={0,0}, .to={0,0} }; // identique => illégal
    GameResult out = GAME_ONGOING;
    ASSERT_TRUE(!apply_move(&b, &gs, illegal_same, &out));

    // État identique
    ASSERT_EQ_INT(before_b.current_player, b.current_player);
    ASSERT_EQ_INT(before_b.turn_number, b.turn_number);
    ASSERT_EQ_INT(board_get(&before_b,(Coord){0,0}), board_get(&b,(Coord){0,0}));
    ASSERT_EQ_INT(board_get(&before_b,(Coord){1,0}), board_get(&b,(Coord){1,0}));
    ASSERT_EQ_INT(before_gs.blue_points, gs.blue_points);
    ASSERT_EQ_INT(before_gs.red_points, gs.red_points);
    printf("[OK] game: illegal move rejected, state unchanged\n");
}

void test_capture_linca_sandwich(){
    Board b; board_init(&b, PLAYER_BLUE); GameState gs; game_init_state(&gs, 64, &b, 0);
    board_init(&b, PLAYER_BLUE);
    for(int y=0;y<BOARD_SIZE;y++) for(int x=0;x<BOARD_SIZE;x++) board_set(&b,(Coord){x,y},EMPTY);
    /* Disposition (ligne du haut y=0):
       x: 0    1    2    3
           B -> T -> E -> B
       On déplace B: (0,0) -> (1,0)
       À l'arrivée (1,0), adj (2,0)=E ennemi, back (3,0)=B allié => capture E
    */
    board_set(&b,(Coord){0,0}, BLUE_SOLDIER); // from
    board_set(&b,(Coord){2,0}, RED_SOLDIER);  // enemy adj après move
    board_set(&b,(Coord){3,0}, BLUE_SOLDIER); // back allié
    game_post_setup_seed(&gs, &b);

    Move m = { .from={0,0}, .to={1,0} };
    GameResult out = GAME_ONGOING;
    ASSERT_TRUE(apply_move(&b, &gs, m, &out));
    ASSERT_EQ_INT(EMPTY, board_get(&b,(Coord){2,0}));
    ASSERT_EQ_INT(1, gs.red_soldiers_captured);
    printf("[OK] game: capture Linca (sandwich)\n");
}

void test_capture_seultou_directional(){
    Board b; board_init(&b, PLAYER_BLUE); GameState gs; game_init_state(&gs, 64, &b, 0);
    board_init(&b, PLAYER_BLUE);
    for(int y=0;y<BOARD_SIZE;y++) for(int x=0;x<BOARD_SIZE;x++) board_set(&b,(Coord){x,y},EMPTY);
    /* Disposition pour Seultou non protégé:
       Place B at from (0,0), move to (7,0). Enemy at adj (8,0). Behind (9,0) is OOB => not protected.
    */
    board_set(&b,(Coord){0,0}, BLUE_SOLDIER);
    board_set(&b,(Coord){8,0}, RED_SOLDIER);
    game_post_setup_seed(&gs, &b);

    Move m = { .from={0,0}, .to={7,0} };
    GameResult out = GAME_ONGOING;
    ASSERT_TRUE(apply_move(&b, &gs, m, &out));
    ASSERT_EQ_INT(EMPTY, board_get(&b,(Coord){8,0}));
    ASSERT_EQ_INT(1, gs.red_soldiers_captured);
    printf("[OK] game: capture Seultou (directional)\n");
}

void run_tests_game(){
    test_board_place_initial_and_scores();
    test_board_place_from_ascii_and_victory_city();
    test_illegal_move_no_state_change();
    test_capture_linca_sandwich();
    test_capture_seultou_directional();
}

