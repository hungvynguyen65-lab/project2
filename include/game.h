#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#define COLUMN_COUNT 7
#define FOUNDATION_COUNT 4
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
    bool running;
} GameState;

void game_init(GameState *game);
void game_destroy(GameState *game);
void game_render(const GameState *game);
void game_handle_command(GameState *game, const char *input);

#endif
