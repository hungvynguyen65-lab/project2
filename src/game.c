#include "game.h"

#include "card_list.h"
#include "game_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *skip_spaces(const char *text) {
    while (*text == ' ' || *text == '\t') {
        text++;
    }

    return text;
}

static bool command_starts_with_arg(const char *command, const char *name) {
    size_t length = strlen(name);
    return strncmp(command, name, length) == 0 &&
        (command[length] == '\0' || command[length] == ' ' || command[length] == '\t');
}

static bool reject_play_phase_command(GameState *game) {
    if (game->phase != PHASE_PLAY) {
        return false;
    }

    game_set_message(game, "Command not available in the PLAY phase.");
    return true;
}

void game_execute_command(GameState *game, const char *command) {
    const char *arg;

    game_set_last_command(game, command);

    /* Central command dispatcher used by both the terminal UI and SDL UI. */
    if (strcmp(command, "QQ") == 0) {
        game_request_quit(game);
        return;
    }

    if (command_starts_with_arg(command, "LD")) {
        if (!reject_play_phase_command(game)) {
            arg = skip_spaces(command + 2);
            *arg == '\0' ? game_load_default_deck(game) : game_load_deck_file(game, arg);
        }
        return;
    }

    if (strcmp(command, "SW") == 0) {
        if (!reject_play_phase_command(game)) {
            game_show_deck(game);
        }
        return;
    }

    if (command_starts_with_arg(command, "SD")) {
        if (!reject_play_phase_command(game)) {
            arg = skip_spaces(command + 2);
            game_save_deck_file(game, *arg == '\0' ? "cards.txt" : arg);
        }
        return;
    }

    if (command_starts_with_arg(command, "SI")) {
        if (!reject_play_phase_command(game)) {
            arg = skip_spaces(command + 2);
            if (*arg == '\0') {
                int split = game->deck.count > 1 ? 1 + rand() % (game->deck.count - 1) : 0;
                game_shuffle_interleave(game, split);
            } else {
                char *end = NULL;
                long parsed = strtol(arg, &end, 10);
                if (end == arg || *skip_spaces(end) != '\0' || parsed > 100000L) {
                    game_set_message(game, "Invalid split.");
                } else {
                    game_shuffle_interleave(game, (int)parsed);
                }
            }
        }
        return;
    }

    if (strcmp(command, "SR") == 0) {
        if (!reject_play_phase_command(game)) {
            game_shuffle_random(game);
        }
        return;
    }

    if (strcmp(command, "P") == 0) {
        if (game->phase == PHASE_PLAY) {
            game_set_message(game, "Already in PLAY phase.");
        } else {
            game_start(game);
        }
        return;
    }

    if (strcmp(command, "Q") == 0) {
        game_quit_to_startup(game);
        return;
    }

    if (strstr(command, "->") != NULL) {
        game_move_command(game, command);
        return;
    }

    if (command[0] == '\0') {
        game_set_message(game, "");
        return;
    }

    game_set_message(game, "Command not implemented yet.");
}

void game_init(GameState *game) {
    static bool random_seeded = false;

    memset(game, 0, sizeof(*game));
    game->phase = PHASE_STARTUP;
    game->running = true;
    game->won = false;
    game_set_message(game, "");

    if (!random_seeded) {
        srand((unsigned int)time(NULL));
        random_seeded = true;
    }
}

void game_destroy(GameState *game) {
    int i;

    card_list_clear(&game->deck);

    for (i = 0; i < COLUMN_COUNT; i++) {
        card_list_clear(&game->columns[i]);
    }

    for (i = 0; i < FOUNDATION_COUNT; i++) {
        card_list_clear(&game->foundations[i]);
    }
}

void game_set_last_command(GameState *game, const char *command) {
    snprintf(game->last_command, sizeof(game->last_command), "%s", command);
}

void game_set_message(GameState *game, const char *message) {
    snprintf(game->message, sizeof(game->message), "%s", message);
}

void game_request_quit(GameState *game) {
    game->running = false;
    game_set_message(game, "Bye.");
}

void game_update_win_state(GameState *game) {
    int total = 0;
    int i;

    for (i = 0; i < FOUNDATION_COUNT; i++) {
        total += game->foundations[i].count;
    }

    game->won = (total == 52);
}

bool game_is_won(const GameState *game) {
    return game->won;
}

int game_column_count(const GameState *game, int column) {
    if (column < 0 || column >= COLUMN_COUNT) {
        return 0;
    }

    return game->columns[column].count;
}

const Card *game_column_card_at(const GameState *game, int column, int row) {
    const CardNode *node;

    if (column < 0 || column >= COLUMN_COUNT || row < 0) {
        return NULL;
    }

    node = card_list_node_at(&game->columns[column], row);
    return node != NULL ? &node->card : NULL;
}

const Card *game_foundation_top(const GameState *game, int foundation) {
    const CardNode *top;

    if (foundation < 0 || foundation >= FOUNDATION_COUNT) {
        return NULL;
    }

    top = game->foundations[foundation].tail;
    return top != NULL ? &top->card : NULL;
}
