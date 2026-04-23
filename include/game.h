#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stddef.h>

#define COLUMN_COUNT 7
#define FOUNDATION_COUNT 4
#define TABLEAU_MAX_ROWS 11
#define INPUT_BUFFER_SIZE 128
#define MESSAGE_SIZE 128
#define COMMAND_SIZE 128

typedef enum {
    PHASE_STARTUP,
    PHASE_PLAY
} GamePhase;

typedef struct Card {
    char rank;
    char suit;
    bool visible;
} Card;

typedef struct CardNode {
    Card card;
    struct CardNode *next;
} CardNode;

typedef struct {
    CardNode *head;
    CardNode *tail;
    int count;
} CardList;

typedef struct {
    GamePhase phase;
    CardList deck;
    CardList columns[COLUMN_COUNT];
    CardList foundations[FOUNDATION_COUNT];
    char last_command[COMMAND_SIZE];
    char message[MESSAGE_SIZE];
    bool won;
    bool running;
} GameState;

void game_init(GameState *game);
void game_destroy(GameState *game);
void game_execute_command(GameState *game, const char *command);
void game_set_last_command(GameState *game, const char *command);
void game_set_message(GameState *game, const char *message);
void game_request_quit(GameState *game);
void game_load_default_deck(GameState *game);
void game_load_deck_file(GameState *game, const char *filename);
void game_save_deck_file(GameState *game, const char *filename);
void game_show_deck(GameState *game);
void game_shuffle_interleave(GameState *game, int split);
void game_shuffle_random(GameState *game);
void game_start(GameState *game);
void game_quit_to_startup(GameState *game);
bool game_move_command(GameState *game, const char *command);
bool game_can_move_from_column_to_column(const GameState *game, int source_column, int source_row, int dest_column);
bool game_can_move_from_column_to_foundation(const GameState *game, int source_column, int source_row, int foundation);
bool game_can_move_from_foundation_to_column(const GameState *game, int foundation, int dest_column);
bool game_is_won(const GameState *game);
int game_column_count(const GameState *game, int column);
const Card *game_column_card_at(const GameState *game, int column, int row);
const Card *game_foundation_top(const GameState *game, int foundation);
void card_to_text(const Card *card, char *buffer, size_t size);

#endif
