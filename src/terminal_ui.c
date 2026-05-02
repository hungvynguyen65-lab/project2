#include "terminal_ui.h"

#include <stdio.h>
#include <string.h>

static void trim_newline(char *text) {
    size_t length = strlen(text);
    while (length > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r')) {
        text[length - 1] = '\0';
        length--;
    }
}

void terminal_render(const GameState *game) {
    int row;

    /* Print the tableau row by row, with foundations shown on every second row. */
    printf("C1\tC2\tC3\tC4\tC5\tC6\tC7\n");

    for (row = 0; row < TABLEAU_MAX_ROWS; row++) {
        int col;

        for (col = 0; col < COLUMN_COUNT; col++) {
            char card_text[8];
            const Card *card = game_column_card_at(game, col, row);
            card_to_text(card, card_text, sizeof(card_text));
            printf("%s", card_text);
            if (col < COLUMN_COUNT - 1) {
                printf("\t");
            }
        }

        if (row % 2 == 0 && row / 2 < FOUNDATION_COUNT) {
            int foundation_index = row / 2;
            char card_text[8];
            const Card *card = game_foundation_top(game, foundation_index);

            if (card == NULL) {
                snprintf(card_text, sizeof(card_text), "[ ]");
            } else {
                card_to_text(card, card_text, sizeof(card_text));
            }

            printf("\t%s F%d", card_text, foundation_index + 1);
        }

        printf("\n");
    }

    printf("LAST Command: %s\n", game->last_command);
    printf("Message: %s\n", game->message);
    printf("INPUT > ");
}

void terminal_handle_command(GameState *game, const char *input) {
    char command[INPUT_BUFFER_SIZE];

    /* Normalize the input line before passing it to the shared game command handler. */
    snprintf(command, sizeof(command), "%s", input);
    trim_newline(command);
    game_execute_command(game, command);
}

void terminal_run(GameState *game) {
    char input[INPUT_BUFFER_SIZE];

    /* Re-render after each command until the shared game state requests exit. */
    while (game->running) {
        terminal_render(game);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        terminal_handle_command(game, input);
    }
}
