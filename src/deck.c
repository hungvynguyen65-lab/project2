#include "game.h"

#include "card_list.h"
#include "card_utils.h"
#include "game_internal.h"

#include <stdio.h>
#include <stdlib.h>

static const int COLUMN_LENGTHS[COLUMN_COUNT] = {1, 6, 7, 8, 9, 10, 11};

static void clear_tableau(GameState *game) {
    int i;

    for (i = 0; i < COLUMN_COUNT; i++) {
        card_list_clear(&game->columns[i]);
    }

    for (i = 0; i < FOUNDATION_COUNT; i++) {
        card_list_clear(&game->foundations[i]);
    }
}

static void rebuild_startup_columns_from_deck(GameState *game, bool visible) {
    CardNode *deck_node = game->deck.head;
    int col = 0;

    clear_tableau(game);

    /* Display the raw deck order across the seven startup columns. */
    while (deck_node != NULL) {
        Card card = deck_node->card;
        card.visible = visible;
        card_list_append_card(&game->columns[col], card);
        col = (col + 1) % COLUMN_COUNT;
        deck_node = deck_node->next;
    }
}

static void rebuild_play_columns_from_deck(GameState *game) {
    CardNode *deck_node = game->deck.head;
    int row;

    clear_tableau(game);

    /* Deal the stored deck into the seven Yukon columns row by row. */
    for (row = 0; deck_node != NULL && row < TABLEAU_MAX_ROWS; row++) {
        int col;

        for (col = 0; col < COLUMN_COUNT && deck_node != NULL; col++) {
            Card card;

            if (row >= COLUMN_LENGTHS[col]) {
                continue;
            }

            card = deck_node->card;
            card.visible = row >= col;
            card_list_append_card(&game->columns[col], card);
            deck_node = deck_node->next;
        }
    }
}

static void clear_all(GameState *game) {
    card_list_clear(&game->deck);
    clear_tableau(game);
}

static void replace_deck_from_list(GameState *game, CardList *new_deck) {
    clear_all(game);
    game->deck = *new_deck;
    new_deck->head = NULL;
    new_deck->tail = NULL;
    new_deck->count = 0;
    rebuild_startup_columns_from_deck(game, false);
    game->phase = PHASE_STARTUP;
    game->won = false;
}

void game_load_default_deck(GameState *game) {
    int suit_index;
    int rank_index;

    clear_all(game);

    /* Build the ordered deck: all clubs, then diamonds, hearts, and spades. */
    for (suit_index = 0; suit_index < 4; suit_index++) {
        for (rank_index = 0; rank_index < 13; rank_index++) {
            Card card;
            card.rank = card_rank_at(rank_index);
            card.suit = card_suit_at(suit_index);
            card.visible = false;
            if (!card_list_append_card(&game->deck, card)) {
                game_set_message(game, "Out of memory.");
                return;
            }
        }
    }

    rebuild_startup_columns_from_deck(game, false);
    game->phase = PHASE_STARTUP;
    game->won = false;
    game_set_message(game, "OK");
}

void game_load_deck_file(GameState *game, const char *filename) {
    FILE *file;
    CardList loaded = {0};
    bool seen[4][13] = {{false}};
    char line[64];
    int line_number = 0;

    file = fopen(filename, "r");
    if (file == NULL) {
        game_set_message(game, "File does not exist.");
        return;
    }

    /* Validate each input line while tracking duplicates in rank/suit slots. */
    while (fgets(line, sizeof(line), file) != NULL) {
        Card card;
        int r;
        int s;

        line_number++;
        if (!card_parse_line(line, &card)) {
            snprintf(game->message, sizeof(game->message), "Invalid card at line %d.", line_number);
            fclose(file);
            card_list_clear(&loaded);
            return;
        }

        r = card_rank_index(card.rank);
        s = card_suit_index(card.suit);
        if (seen[s][r]) {
            snprintf(game->message, sizeof(game->message), "Duplicate card at line %d.", line_number);
            fclose(file);
            card_list_clear(&loaded);
            return;
        }

        seen[s][r] = true;
        if (!card_list_append_card(&loaded, card)) {
            game_set_message(game, "Out of memory.");
            fclose(file);
            card_list_clear(&loaded);
            return;
        }
    }

    fclose(file);

    if (loaded.count != 52) {
        snprintf(game->message, sizeof(game->message), "Invalid deck: expected 52 cards, got %d.", loaded.count);
        card_list_clear(&loaded);
        return;
    }

    replace_deck_from_list(game, &loaded);
    game_set_message(game, "OK");
}

void game_save_deck_file(GameState *game, const char *filename) {
    FILE *file;
    const CardNode *node;

    if (game->deck.count == 0) {
        game_set_message(game, "No deck loaded.");
        return;
    }

    file = fopen(filename, "w");
    if (file == NULL) {
        game_set_message(game, "Could not save file.");
        return;
    }

    node = game->deck.head;
    while (node != NULL) {
        fprintf(file, "%c%c\n", node->card.rank, node->card.suit);
        node = node->next;
    }

    fclose(file);
    game_set_message(game, "OK");
}

void game_show_deck(GameState *game) {
    CardNode *deck_node;

    if (game->deck.count == 0) {
        game_set_message(game, "No deck loaded.");
        return;
    }

    deck_node = game->deck.head;
    while (deck_node != NULL) {
        deck_node->card.visible = true;
        deck_node = deck_node->next;
    }

    rebuild_startup_columns_from_deck(game, true);
    game_set_message(game, "OK");
}

void game_shuffle_interleave(GameState *game, int split) {
    CardList first;
    CardList second;
    CardList shuffled;
    int i;

    if (game->deck.count == 0) {
        game_set_message(game, "No deck loaded.");
        return;
    }

    if (split <= 0 || split >= game->deck.count) {
        game_set_message(game, "Invalid split.");
        return;
    }

    card_list_init(&first);
    card_list_init(&second);
    card_list_init(&shuffled);

    /* Split the deck, then alternately move cards from each pile. */
    for (i = 0; i < split; i++) {
        card_list_append_node(&first, card_list_pop_front(&game->deck));
    }

    while (game->deck.count > 0) {
        card_list_append_node(&second, card_list_pop_front(&game->deck));
    }

    while (first.count > 0 && second.count > 0) {
        card_list_append_node(&shuffled, card_list_pop_front(&first));
        card_list_append_node(&shuffled, card_list_pop_front(&second));
    }

    while (first.count > 0) {
        card_list_append_node(&shuffled, card_list_pop_front(&first));
    }

    while (second.count > 0) {
        card_list_append_node(&shuffled, card_list_pop_front(&second));
    }

    game->deck = shuffled;
    rebuild_startup_columns_from_deck(game, false);
    game->won = false;
    game_set_message(game, "OK");
}

void game_shuffle_random(GameState *game) {
    CardList shuffled;

    if (game->deck.count == 0) {
        game_set_message(game, "No deck loaded.");
        return;
    }

    card_list_init(&shuffled);

    /* Insert each card into a random position in the new shuffled pile. */
    while (game->deck.count > 0) {
        CardNode *node = card_list_pop_front(&game->deck);
        int position = shuffled.count == 0 ? 0 : rand() % (shuffled.count + 1);
        card_list_insert_node_at(&shuffled, node, position);
    }

    game->deck = shuffled;
    rebuild_startup_columns_from_deck(game, false);
    game->won = false;
    game_set_message(game, "OK");
}

void game_start(GameState *game) {
    if (game->deck.count == 0) {
        game_set_message(game, "No deck loaded.");
        return;
    }

    /* Re-deal from the saved deck and apply the initial Yukon visibility. */
    rebuild_play_columns_from_deck(game);
    game->phase = PHASE_PLAY;
    game_update_win_state(game);
    game_set_message(game, "OK");
}

void game_quit_to_startup(GameState *game) {
    if (game->phase != PHASE_PLAY) {
        game_set_message(game, "Not in PLAY phase.");
        return;
    }

    rebuild_startup_columns_from_deck(game, false);
    game->phase = PHASE_STARTUP;
    game->won = false;
    game_set_message(game, "OK");
}
