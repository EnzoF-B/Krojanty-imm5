/**
 * @file test_move.c
 * @brief Tests unitaires des conversions de coups (texte <-> structure).
 *
 * @author marcelin.trag@uha.fr
 * @date 2025-09-17
 * @version 1.0.0
 *
 */
 
#include "test_framework.h"
#include "move.h"

void test_move_str_roundtrip(){
    Move m = { .from = {1,1}, .to = {1,5} };
    char s[6] = {0};
    ASSERT_TRUE(move_to_str(m, s));
    ASSERT_EQ_STR("B8 B4", s);

    Move p;
    ASSERT_TRUE(move_from_str("B8 B4", &p));
    ASSERT_EQ_INT(m.from.x, p.from.x);
    ASSERT_EQ_INT(m.from.y, p.from.y);
    ASSERT_EQ_INT(m.to.x, p.to.x);
    ASSERT_EQ_INT(m.to.y, p.to.y);

    ASSERT_TRUE(!move_from_str("invalid", &p));
    printf("[OK] move: string roundtrip\n");
}

void run_tests_move(){
    test_move_str_roundtrip();
}

