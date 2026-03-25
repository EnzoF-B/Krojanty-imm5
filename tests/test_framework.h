/**
 * @file test_framework.h
 * @brief Mini-framework d'assertions pour les tests C du projet.
 *
 * @author marcelin.trag@uha.fr
 * @date 2025-09-17
 * @version 1.0.0
 *
 *
 * Fournit des macros d'assertion simples qui incrémentent des compteurs
 * globaux et impriment un message en cas d'échec. Les compteurs sont
 * définis dans `tests/test_main.c`.
 */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>

/** @brief Compteur global du nombre total d'assertions (fourni par test_main.c). */
extern int g_total_asserts;
/** @brief Compteur global du nombre d'assertions en échec (fourni par test_main.c). */
extern int g_failed_asserts;

/**
 * @brief Vérifie qu'une condition est vraie, sinon log un échec.
 * @param cond expression booléenne à vérifier.
 */
#define ASSERT_TRUE(cond) do { \
    g_total_asserts++; \
    if(!(cond)) { \
        g_failed_asserts++; \
        printf("[FAIL] %s:%d: ASSERT_TRUE(%s)\n", __FILE__, __LINE__, #cond); \
    } \
} while(0)

/**
 * @brief Compare deux entiers pour l'égalité.
 * @param exp valeur attendue
 * @param act valeur obtenue
 */
#define ASSERT_EQ_INT(exp, act) do { \
    long _e = (long)(exp); \
    long _a = (long)(act); \
    g_total_asserts++; \
    if(_e != _a) { \
        g_failed_asserts++; \
        printf("[FAIL] %s:%d: expected %ld, got %ld\n", __FILE__, __LINE__, _e, _a); \
    } \
} while(0)

/**
 * @brief Compare deux chaînes pour l'égalité.
 * @param exp chaîne attendue (nul-terminée)
 * @param act chaîne obtenue (nul-terminée)
 */
#define ASSERT_EQ_STR(exp, act) do { \
    const char* _e = (exp); \
    const char* _a = (act); \
    g_total_asserts++; \
    if(strcmp(_e, _a) != 0) { \
        g_failed_asserts++; \
        printf("[FAIL] %s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, _e, _a); \
    } \
} while(0)

#endif /* TEST_FRAMEWORK_H */

