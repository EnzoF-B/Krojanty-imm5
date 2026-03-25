# ==== Config ====
CC      := gcc
CSTD    := -std=c17
WARN    := -Wall -Wextra -Wpedantic
OPT     := -O2
INCS    := -Iinclude
DEPS    := -MMD -MP
CFLAGS  := $(CSTD) $(WARN) $(OPT) $(INCS) $(DEPS)
LDFLAGS :=

# GTK4
GTK_CFLAGS := $(shell pkg-config --cflags gtk4)
GTK_LIBS   := $(shell pkg-config --libs gtk4)

# ==== Dossiers ====
SRC_DIR := src
BLD_DIR := build
OBJ_DIR := $(BLD_DIR)/obj
TST_DIR := tests

# ==== Binaires ====
BIN_CLI  := $(BLD_DIR)/game_cli
BIN_GUI  := $(BLD_DIR)/game_gui
BIN_GAME := $(BLD_DIR)/game   # binaire unique attendu: ./game
RUN_LINK := game              # lien symbolique à la racine

# ==== Sources (CLI) ====
SRC := \
  $(SRC_DIR)/main.c \
  $(SRC_DIR)/coord.c \
  $(SRC_DIR)/move.c \
  $(SRC_DIR)/board.c \
  $(SRC_DIR)/rules.c \
  $(SRC_DIR)/game.c \
  $(SRC_DIR)/gen.c \
  $(SRC_DIR)/net.c

OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

# ==== Sources (GUI + binaire unique ./game) ====
SRC_GUI := \
  $(SRC_DIR)/main_gui.c \
  $(SRC_DIR)/coord.c \
  $(SRC_DIR)/move.c \
  $(SRC_DIR)/board.c \
  $(SRC_DIR)/rules.c \
  $(SRC_DIR)/game.c \
  $(SRC_DIR)/gen.c \
  $(SRC_DIR)/ui_gtk.c \
  $(SRC_DIR)/net.c \
  $(SRC_DIR)/ai.c \
  $(SRC_DIR)/memoization.c

OBJ_GUI := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_GUI))
DEP_GUI := $(OBJ_GUI:.o=.d)

# ==== Règles par défaut ====
.PHONY: all game gui cli run run_local run_server run_client clean distclean

all: game
# ==== Tests (NOGUI) ====
TEST_BIN := $(BLD_DIR)/tests_runner
TEST_SRC := \
  $(TST_DIR)/test_main.c \
  $(TST_DIR)/test_coord.c \
  $(TST_DIR)/test_move.c \
  $(TST_DIR)/test_board.c \
  $(TST_DIR)/test_rules.c \
  $(TST_DIR)/test_game.c
TEST_OBJ := $(patsubst $(TST_DIR)/%.c,$(OBJ_DIR)/%.o,$(TEST_SRC))

# Objets coeur (sans main.c) pour lier les tests en NOGUI
# Ajout: inclure memoization.o si nécessaire pour les fonctions Zobrist
CORE_OBJ := $(filter-out $(OBJ_DIR)/main.o,$(OBJ)) $(OBJ_DIR)/memoization.o

.PHONY: tests
tests: $(TEST_BIN)
	$(TEST_BIN)

$(TEST_BIN): $(CORE_OBJ) $(TEST_OBJ) | $(BLD_DIR)
	$(CC) $(CORE_OBJ) $(TEST_OBJ) -o $@ $(LDFLAGS) -lpthread

$(OBJ_DIR)/%.o: $(TST_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


# Binaire unique: ./game (construit depuis main_gui.c qui gère la CLI -l/-s/-c/-ia)
game: $(BIN_GAME)
	@ln -sf $(BIN_GAME) $(RUN_LINK)
	@echo "Binaire prêt: ./game"

# Lancer l'UI (sans arguments → boîte de sélection)
run: game
	./$(RUN_LINK)

# Exemples d’exécution imposés par la consigne
run_local: game
	./$(RUN_LINK) -l

# Utiliser 'make run_server PORT=5555'
PORT ?= 8080
run_server: game
	./$(RUN_LINK) -s $(PORT)

# Utiliser 'make run_client HOST=127.0.0.1 PORT=8080'
HOST ?= 127.0.0.1
run_client: game
	./$(RUN_LINK) -c $(HOST):$(PORT)

# Cibles historiques conservées (optionnelles)
gui: $(BIN_GUI)
	./$(BIN_GUI)

cli: $(BIN_CLI)
	./$(BIN_CLI)

# ==== Liens ====
$(BIN_CLI): $(OBJ) | $(BLD_DIR)
	$(CC) $(OBJ) -o $@ $(LDFLAGS) -lpthread

$(BIN_GUI): $(OBJ_GUI) | $(BLD_DIR)
	$(CC) $(OBJ_GUI) -o $@ $(LDFLAGS) $(GTK_LIBS) -lpthread -lm

$(BIN_GAME): $(OBJ_GUI) | $(BLD_DIR)
	$(CC) $(OBJ_GUI) -o $@ $(LDFLAGS) $(GTK_LIBS) -lpthread -lm

# ==== Compilation générique ====
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ==== Overrides CFLAGS pour fichiers GTK ====
$(OBJ_DIR)/ui_gtk.o: $(SRC_DIR)/ui_gtk.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(OBJ_DIR)/main_gui.o: $(SRC_DIR)/main_gui.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

# ==== Dossiers build ====
$(BLD_DIR):
	mkdir -p $(BLD_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ==== Nettoyage ====
clean:
	rm -rf $(BLD_DIR) $(RUN_LINK)

distclean: clean

# ==== Dépendances ====
-include $(DEP)
-include $(DEP_GUI)

.PHONY: docs doc-clean

docs:
	doxygen Doxyfile

doc-clean:
	rm -rf docs
