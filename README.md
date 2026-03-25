# Krojanty — Jeu de plateau (C, GTK4, Réseau, IA)

Krojanty est un jeu de plateau 9×9 écrit en C, avec une interface GTK4, un mode console, du jeu en réseau (client/serveur) et une IA (Minimax/Alpha-Beta, mémoïsation/Zobrist). Le dépôt inclut une suite de tests NOGUI et une documentation Doxygen.

## Sommaire
- Aperçu du jeu
- Prérequis
- Compilation
- Exécution (modes et exemples)
- Tests (NOGUI)
- Documentation (Doxygen)
- Structure du dépôt
- Dépannage

## Aperçu
- Plateau 9×9, deux camps: BLEU et ROUGE.
- Déplacements orthogonaux avec validation de chemin libre.
- Deux règles de capture:
  - Linca (sandwich autour de la case d’arrivée).
  - Seultou (directionnelle, non protégée).
- Scores: soldats vivants + cases contrôlées; contrôle initial et recompute après chaque coup.
- Victoires: roi capturé, roi dans la cité adverse, 8 captures de soldats adverses, ou aux points à la limite de tours.
- Modes: local, réseau (serveur/client), IA optionnelle.

## Prérequis
Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential pkg-config libgtk-4-dev
# Optionnel pour la documentation:
sudo apt-get install -y doxygen graphviz
```

Fedora:
```bash
sudo dnf install -y gcc make pkgconfig gtk4-devel doxygen graphviz
```

## Compilation
Le projet utilise `make`.

- Compiler le binaire unique `./game` (GTK4):
```bash
make
# ou
make game
```

- Nettoyer:
```bash
make clean
```

## Exécution
Le binaire principal est `./game` (lien symbolique vers `build/game`). Il gère les modes suivants (ordre strict):

```text
./game -l                         # local sans IA
./game -s PPP                     # serveur sans IA (PPP = port)
./game -c AAAA:PPP                # client sans IA (AAAA = host)
./game -s -ia PPP                 # serveur avec IA
./game -c -ia AAAA:PPP            # client avec IA
```

Cibles pratiques:
```bash
make run                 # lance l’UI GTK4 (sélecteur)
make run_local           # équivalent ./game -l
make run_server PORT=8080
make run_client HOST=127.0.0.1 PORT=8080
```

## Tests (NOGUI)
La suite de tests couvre coordonnées, coups, plateau, règles et logique de partie (scores, victoires, captures, illégalité). Chaque test affiche exactement une ligne `[OK] ...`.

- Exécuter:
```bash
make tests
```

- Exemple de sortie:
```text
Running tests...
[OK] coord: conversions
[OK] coord: string roundtrip
[OK] move: string roundtrip
[OK] board: init/get/set
[OK] board: path_clear
[OK] rules: ally/enemy
[OK] rules: basic legality & apply
[OK] game: initial placement & scores
[OK] game: victory by city entry (legal move)
[OK] game: illegal move rejected, state unchanged
[OK] game: capture Linca (sandwich)
[OK] game: capture Seultou (directional)

All tests passed (... assertions).
```

- Réseau (script d’aide):
```bash
bash tests/test_network.sh
```

## Documentation (Doxygen)
Le dépôt inclut `Doxyfile`. La documentation HTML est générée dans `docs/html/`.

```bash
doxygen Doxyfile
# Ouvrir docs/html/index.html dans votre navigateur
```

## Structure du dépôt
```
include/   # .h (plateau, règles, jeu, IA, réseau, etc.)
src/       # .c (implémentations CLI/GUI/IA/Réseau)
tests/     # tests NOGUI + mini-framework d’assertions
assets/    # ressources UI (images, CSS)
build/     # objets et binaires générés
Makefile   # cibles build/run/tests/docs
Doxyfile   # configuration de la documentation
```

## Dépannage
- « gtk4 non trouvé »: installez `libgtk-4-dev` et vérifiez `pkg-config --cflags gtk4`.
- UI GTK4: nécessite un environnement graphique; les tests NOGUI n’en ont pas besoin.
- Réseau: assurez-vous que le port choisi est libre et ouvert (pare-feu).
- Si la compilation des tests échoue à cause de modules IA/mémoïsation, reconstruisez avec:
```bash
make clean && make tests
```

## Licence
Projet fourni à des fins pédagogiques. Ajoutez votre licence si nécessaire.
