/**
 * @file test_main.c
 * @brief Point d'entrée du runner de tests. Aggrège et exécute les suites.
 *
 * @author marcelin.trag@uha.fr
 * @date 2025-09-17
 * @version 1.0.0
 *
 */
 
#include <stdio.h>

/** @brief Compteur global du nombre total d'assertions. */
int g_total_asserts = 0;
/** @brief Compteur global du nombre d'assertions en échec. */
int g_failed_asserts = 0;

/** @brief Lance les tests liés aux coordonnées. */
void run_tests_coord();
/** @brief Lance les tests liés aux conversions de coups. */
void run_tests_move();
/** @brief Lance les tests liés au plateau. */
void run_tests_board();
/** @brief Lance les tests liés aux règles de base. */
void run_tests_rules();
/** @brief Lance les tests liés à la logique de partie. */
void run_tests_game();

/**
 * @brief Exécute toutes les suites de tests et imprime un résumé.
 * @return 0 si tout passe, 1 sinon.
 */
int main(void){
    printf("Running tests...\n");
    run_tests_coord();
    run_tests_move();
    run_tests_board();
    run_tests_rules();
    run_tests_game();

    if(g_failed_asserts==0) printf("\nAll tests passed (%d assertions).\n", g_total_asserts);
    else printf("\nTests failed: %d/%d assertions failed.\n", g_failed_asserts, g_total_asserts);

    return g_failed_asserts==0 ? 0 : 1;
}

