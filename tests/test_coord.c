/**
 * @file test_coord.c
 * @brief Tests unitaires des conversions et parsing de coordonnées.
 *
 * @author marcelin.trag@uha.fr
 * @date 2025-09-17
 * @version 1.0.0
 *
 */
#include "test_framework.h"
#include "coord.h"

void test_col_row_conversions(){
    ASSERT_EQ_INT(0, col_char_to_x('A'));
    ASSERT_EQ_INT(8, col_char_to_x('I'));
    ASSERT_EQ_INT(0, col_char_to_x('a'));
    ASSERT_EQ_INT(-1, col_char_to_x('J'));

    ASSERT_EQ_INT(0, row_char_to_y('9'));
    ASSERT_EQ_INT(8, row_char_to_y('1'));
    ASSERT_EQ_INT(-1, row_char_to_y('0'));

    ASSERT_EQ_INT('A', x_to_col_char(0));
    ASSERT_EQ_INT('I', x_to_col_char(8));
    ASSERT_EQ_INT('?', x_to_col_char(9));

    ASSERT_EQ_INT('9', y_to_row_char(0));
    ASSERT_EQ_INT('1', y_to_row_char(8));
    ASSERT_EQ_INT('?', y_to_row_char(9));
    printf("[OK] coord: conversions\n");
}

void test_coord_str(){
    Coord c;
    ASSERT_TRUE(coord_from_str("A9", &c));
    ASSERT_EQ_INT(0, c.x); ASSERT_EQ_INT(0, c.y);
    char out[3];
    ASSERT_TRUE(coord_to_str(c, out));
    ASSERT_EQ_STR("A9", out);

    ASSERT_TRUE(!coord_from_str("Z9", &c));
    ASSERT_TRUE(!coord_from_str("A0", &c));
    printf("[OK] coord: string roundtrip\n");
}

void run_tests_coord(){
    test_col_row_conversions();
    test_coord_str();
}

