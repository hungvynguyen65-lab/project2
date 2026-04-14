#include "game.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int COLUMN_LENGTHS[COLUMN_COUNT] = {1, 6, 7, 8, 9, 10, 11};
static const char RANKS[] = {'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K'};
static const char SUITS[] = {'C', 'D', 'H', 'S'};

static void clear_list(CardList *list) {
    CardNode *current = list->head;
    while (current != NULL) {
        CardNode *next = current->next;
        free(current);
        current = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

static bool append_card(CardList *list, Card card) {
    CardNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return false;
    }

    node->card = card;
    node->next = NULL;

    if (list->tail != NULL) {
        list->tail->next = node;
    } else {
        list->head = node;
    }

    list->tail = node;
    list->count++;
    return true;
}

static void clear_tableau(GameState *game) {
    int i;

    for (i = 0; i < COLUMN_COUNT; i++) {
        clear_list(&game->columns[i]);
    }

    for (i = 0; i < FOUNDATION_COUNT; i++) {
        clear_list(&game->foundations[i]);
    }
}

static int total_column_cards(void) {
    int total = 0;
    int i;

    for (i = 0; i < COLUMN_COUNT; i++) {
        total += COLUMN_LENGTHS[i];
    }

    return total;
}

static void set_message(GameState *game, const char *message) {
    snprintf(game->message, sizeof(game->message), "%s", message);
}

static void trim_newline(char *text) {
    size_t length = strlen(text);
    while (length > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r')) {
        text[length - 1] = '\0';
        length--;
    }
}

static int row_target_column(int row, int position_in_row) {
    int col;
    int seen = 0;

    for (col = 0; col < COLUMN_COUNT; col++) {
        if (row < COLUMN_LENGTHS[col]) {
            if (seen == position_in_row) {
                return col;
            }
            seen++;
        }
    }

    return -1;
}

static void rebuild_columns_from_deck(GameState *game, bool play_visibility) {
    CardNode *deck_node = game->deck.head;
    int row;
    int total = total_column_cards();
    int dealt = 0;

    clear_tableau(game);

    for (row = 0; deck_node != NULL && dealt < total; row++) {
        int cards_this_row = 0;
        int col;

        for (col = 0; col < COLUMN_COUNT; col++) {
            if (row < COLUMN_LENGTHS[col]) {
                cards_this_row++;
            }
        }

        for (col = 0; col < cards_this_row && deck_node != NULL; col++) {
            int target = row_target_column(row, col);
            Card card = deck_node->card;

            if (play_visibility) {
                card.visible = row >= target;
            } else {
                card.visible = false;
            }

            if (target >= 0) {
                append_card(&game->columns[target], card);
            }

            deck_node = deck_node->next;
            dealt++;
        }
    }
}

static void clear_all(GameState *game) {
    clear_list(&game->deck);
    clear_tableau(game);
}

static void load_default_deck(GameState *game) {
    int suit_index;
    int rank_index;

    clear_all(game);

    for (suit_index = 0; suit_index < 4; suit_index++) {
        for (rank_index = 0; rank_index < 13; rank_index++) {
            Card card;
            card.rank = RANKS[rank_index];
            card.suit = SUITS[suit_index];
            card.visible = false;
            if (!append_card(&game->deck, card)) {
                set_message(game, "Out of memory.");
                return;
            }
        }
    }

    rebuild_columns_from_deck(game, false);
    game->phase = PHASE_STARTUP;
    set_message(game, "OK");
}

static void show_deck(GameState *game) {
    CardNode *deck_node;

    if (game->deck.count == 0) {
        set_message(game, "No deck loaded.");
        return;
    }

    deck_node = game->deck.head;
    while (deck_node != NULL) {
        deck_node->card.visible = true;
        deck_node = deck_node->next;
    }

    rebuild_columns_from_deck(game, false);
    {
        int i;
        for (i = 0; i < COLUMN_COUNT; i++) {
            CardNode *column_node = game->columns[i].head;
            while (column_node != NULL) {
                column_node->card.visible = true;
                column_node = column_node->next;
            }
        }
    }
    set_message(game, "OK");
}

static void start_game(GameState *game) {
    if (game->deck.count == 0) {
        set_message(game, "No deck loaded.");
        return;
    }

    rebuild_columns_from_deck(game, true);
    game->phase = PHASE_PLAY;
    set_message(game, "OK");
}

static void quit_to_startup(GameState *game) {
    if (game->phase != PHASE_PLAY) {
        set_message(game, "Not in PLAY phase.");
        return;
    }

    rebuild_columns_from_deck(game, false);
    game->phase = PHASE_STARTUP;
    set_message(game, "OK");
}

static const CardNode *node_at(const CardList *list, int index) {
    const CardNode *node = list->head;
    int i = 0;

    while (node != NULL && i < index) {
        node = node->next;
        i++;
    }

    return node;
}

static void card_to_text(const Card *card, char *buffer, size_t size) {
    if (card == NULL) {
        snprintf(buffer, size, "");
        return;
    }

    if (!card->visible) {
        snprintf(buffer, size, "[ ]");
        return;
    }

    snprintf(buffer, size, "%c%c", card->rank, card->suit);
}

void game_init(GameState *game) {
    memset(game, 0, sizeof(*game));
    game->phase = PHASE_STARTUP;
    game->running = true;
    set_message(game, "");
}

void game_destroy(GameState *game) {
    clear_all(game);
}

void game_render(const GameState *game) {
    int row;

    printf("C1\tC2\tC3\tC4\tC5\tC6\tC7\n");

    for (row = 0; row < 11; row++) {
        int col;

        for (col = 0; col < COLUMN_COUNT; col++) {
            char card_text[8];
            const CardNode *node = node_at(&game->columns[col], row);
            const Card *card = node != NULL ? &node->card : NULL;
            card_to_text(card, card_text, sizeof(card_text));
            printf("%s", card_text);
            if (col < COLUMN_COUNT - 1) {
                printf("\t");
            }
        }

        if (row % 2 == 0 && row / 2 < FOUNDATION_COUNT) {
            int foundation_index = row / 2;
            char card_text[8];
            const CardNode *top = game->foundations[foundation_index].tail;
            const Card *card = top != NULL ? &top->card : NULL;
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

void game_handle_command(GameState *game, const char *input) {
    char command[INPUT_BUFFER_SIZE];

    snprintf(command, sizeof(command), "%s", input);
    trim_newline(command);
    snprintf(game->last_command, sizeof(game->last_command), "%s", command);

    if (strcmp(command, "QQ") == 0) {
        game->running = false;
        set_message(game, "Bye.");
        return;
    }

    if (strcmp(command, "LD") == 0) {
        if (game->phase == PHASE_PLAY) {
            set_message(game, "Command not available in the PLAY phase.");
        } else {
            load_default_deck(game);
        }
        return;
    }

    if (strcmp(command, "SW") == 0) {
        if (game->phase == PHASE_PLAY) {
            set_message(game, "Command not available in the PLAY phase.");
        } else {
            show_deck(game);
        }
        return;
    }

    if (strcmp(command, "P") == 0) {
        if (game->phase == PHASE_PLAY) {
            set_message(game, "Already in PLAY phase.");
        } else {
            start_game(game);
        }
        return;
    }

    if (strcmp(command, "Q") == 0) {
        quit_to_startup(game);
        return;
    }

    if (command[0] == '\0') {
        set_message(game, "");
        return;
    }

    set_message(game, "Command not implemented yet.");
}
