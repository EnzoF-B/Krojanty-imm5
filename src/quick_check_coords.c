/**
 * @file quick_check_coords.c
 * @brief Petit programme de test pour vérifier les conversions de coordonnées et de coups.
 *
 * Ce fichier contient un `main` minimaliste qui teste :
 *   - la conversion de coordonnées (ex: "A9" ↔ (0,0)),
 *   - la conversion inverse (coord ↔ string),
 *   - la construction et l'affichage d’un coup (ex: "A9 A5").
 *
 * Il utilise `assert` pour vérifier automatiquement que les fonctions
 * `coord_from_str`, `coord_to_str`, `move_from_str` et `move_to_str`
 * se comportent comme prévu.
 *
 * Usage :
 *   ```
 *   gcc quick_check_coords.c move.c coord.c -o quick_check
 *   ./quick_check
 *   ```
 *   Le programme doit afficher `OK: A9 A5` si tous les tests passent.
 */

#include <stdio.h>
#include <assert.h>
#include "move.h"

/**
 * @brief Fonction principale de test.
 *
 * Étapes testées :
 *   - Conversion "A9" → Coord (0,0) puis retour en chaîne "A9".
 *   - Conversion "I1" → Coord (8,8) puis retour en chaîne "I1".
 *   - Conversion d’un coup "A9 A5" vers structure Move puis retour vers chaîne.
 *
 * Le programme s’arrête si une assertion échoue.
 *
 * @return 0 si tous les tests réussissent.
 */
int main(void) {
    Coord c;
    char buf2[3];
    Move m;
    char buf6[6];

    // Test 1 : A9 -> (0,0) -> "A9"
    assert(coord_from_str("A9", &c));
    assert(c.x == 0 && c.y == 0);
    assert(coord_to_str(c, buf2));
    assert(buf2[0]=='A' && buf2[1]=='9');

    // Test 2 : I1 -> (8,8) -> "I1"
    assert(coord_from_str("I1", &c));
    assert(c.x == 8 && c.y == 8);
    assert(coord_to_str(c, buf2));
    assert(buf2[0]=='I' && buf2[1]=='1');

    // Test 3 : Coup "A9 A5"
    assert(move_from_str("A9 A5", &m));
    assert(move_to_str(m, buf6));
    printf("OK: %s\n", buf6);

    return 0;
}