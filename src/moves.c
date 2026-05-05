#include "game.h"

#include "card_list.h"
#include "card_utils.h"
#include "game_internal.h"

#include <stdio.h>
#include <string.h>

typedef enum {
    MOVE_SOURCE_COLUMN,
    MOVE_SOURCE_FOUNDATION
} MoveSourceType;

typedef enum {
    MOVE_DEST_COLUMN,
    MOVE_DEST_FOUNDATION
} MoveDestType;

typedef struct {
    MoveSourceType type;
    int index;
    bool has_card;
    char rank;
    char suit;
} MoveSource;

typedef struct {
    MoveDestType type;
    int index;
} MoveDest;

static int find_visible_card_index(const CardList *list, char rank, char suit) {
    const CardNode *node = list->head;
    int index = 0;

    while (node != NULL) {
        if (node->card.visible && node->card.rank == rank && node->card.suit == suit) {
            return index;
        }

        node = node->next;
        index++;
    }

    return -1;
}

static bool can_place_on_column(const Card *card, const CardList *destination) {
    const Card *bottom;

    if (destination->count == 0) {
        return card->rank == 'K';
    }

    bottom = &destination->tail->card;
    return card_rank_value(card->rank) + 1 == card_rank_value(bottom->rank) && card->suit != bottom->suit;
}

static bool can_place_on_foundation(const Card *card, const CardList *foundation) {
    const Card *top;

    if (foundation->count == 0) {
        return card->rank == 'A';
    }

    top = &foundation->tail->card;
    return card->suit == top->suit && card_rank_value(card->rank) == card_rank_value(top->rank) + 1;
}

static void reveal_column_tail(CardList *column) {
    if (column->tail != NULL && !column->tail->card.visible) {
        column->tail->card.visible = true;
    }
}

static bool parse_column_ref(const char *text, int *column, bool *has_card, char *rank, char *suit) {
    size_t length = strlen(text);

    if (length != 2 && length != 5) {
        return false;
    }

    if (text[0] != 'C' || text[1] < '1' || text[1] > '7') {
        return false;
    }

    *column = text[1] - '1';
    *has_card = false;

    if (length == 5) {
        if (text[2] != ':' || card_rank_value(text[3]) == 0 || !card_is_valid_suit(text[4])) {
            return false;
        }

        *has_card = true;
        *rank = text[3];
        *suit = text[4];
    }

    return true;
}

static bool parse_foundation_ref(const char *text, int *foundation) {
    if (strlen(text) != 2 || text[0] != 'F' || text[1] < '1' || text[1] > '4') {
        return false;
    }

    *foundation = text[1] - '1';
    return true;
}

static bool parse_move_source(const char *text, MoveSource *source) {
    int index;
    bool has_card;
    char rank = '\0';
    char suit = '\0';

    if (parse_column_ref(text, &index, &has_card, &rank, &suit)) {
        source->type = MOVE_SOURCE_COLUMN;
        source->index = index;
        source->has_card = has_card;
        source->rank = rank;
        source->suit = suit;
        return true;
    }

    if (parse_foundation_ref(text, &index)) {
        source->type = MOVE_SOURCE_FOUNDATION;
        source->index = index;
        source->has_card = false;
        source->rank = '\0';
        source->suit = '\0';
        return true;
    }

    return false;
}

static bool parse_move_dest(const char *text, MoveDest *dest) {
    int index;
    bool has_card;
    char rank = '\0';
    char suit = '\0';

    if (parse_column_ref(text, &index, &has_card, &rank, &suit) && !has_card) {
        dest->type = MOVE_DEST_COLUMN;
        dest->index = index;
        return true;
    }

    if (parse_foundation_ref(text, &index)) {
        dest->type = MOVE_DEST_FOUNDATION;
        dest->index = index;
        return true;
    }

    return false;
}

static bool resolve_move_source(
    const GameState *game,
    MoveSource source,
    const CardList **source_list,
    int *source_row,
    const Card **moving_card
) {
    /* Convert a parsed source like C6:4H or F3 into the actual list and card. */
    if (source.type == MOVE_SOURCE_COLUMN) {
        *source_list = &game->columns[source.index];
        if ((*source_list)->count == 0) {
            return false;
        }

        *source_row = source.has_card
            ? find_visible_card_index(*source_list, source.rank, source.suit)
            : (*source_list)->count - 1;

        if (*source_row < 0) {
            return false;
        }

        *moving_card = &card_list_node_at(*source_list, *source_row)->card;
        return (*moving_card)->visible;
    }

    *source_list = &game->foundations[source.index];
    if ((*source_list)->count == 0) {
        return false;
    }

    *source_row = (*source_list)->count - 1;
    *moving_card = &(*source_list)->tail->card;
    return true;
}

static bool validate_move_destination(
    const GameState *game,
    MoveSource source,
    const CardList *source_list,
    int source_row,
    const Card *moving_card,
    MoveDest dest
) {
    const CardList *dest_list;

    /* Columns accept movable piles; foundations only accept bottom column cards. */
    if (dest.type == MOVE_DEST_COLUMN) {
        dest_list = &game->columns[dest.index];
        return can_place_on_column(moving_card, dest_list);
    }

    dest_list = &game->foundations[dest.index];
    if (source.type != MOVE_SOURCE_COLUMN || source_row != source_list->count - 1) {
        return false;
    }

    return can_place_on_foundation(moving_card, dest_list);
}

static bool can_move_from_source(const GameState *game, MoveSource source, MoveDest dest) {
    const CardList *source_list;
    const Card *moving_card;
    int source_row;

    if (game->phase != PHASE_PLAY) {
        return false;
    }

    if (!resolve_move_source(game, source, &source_list, &source_row, &moving_card)) {
        return false;
    }

    return validate_move_destination(game, source, source_list, source_row, moving_card, dest);
}

bool game_move_command(GameState *game, const char *command) {
    char buffer[COMMAND_SIZE];
    char *arrow;
    MoveSource source;
    MoveDest dest;
    const CardList *resolved_source_list = NULL;
    const Card *resolved_moving_card = NULL;
    CardList *source_list = NULL;
    CardList *dest_list = NULL;
    CardNode *moving_segment = NULL;
    int source_row = -1;

    if (game->phase != PHASE_PLAY) {
        game_set_message(game, "Moves are only available in the PLAY phase.");
        return false;
    }

    /* Parse, validate, detach the source segment, and append it to the destination. */
    snprintf(buffer, sizeof(buffer), "%s", command);
    arrow = strstr(buffer, "->");
    if (arrow == NULL) {
        game_set_message(game, "Invalid move command.");
        return false;
    }

    *arrow = '\0';
    arrow += 2;

    if (!parse_move_source(buffer, &source) || !parse_move_dest(arrow, &dest)) {
        game_set_message(game, "Invalid move command.");
        return false;
    }

    if (!resolve_move_source(game, source, &resolved_source_list, &source_row, &resolved_moving_card)) {
        game_set_message(game, "Move is not valid.");
        return false;
    }

    if (!validate_move_destination(game, source, resolved_source_list, source_row, resolved_moving_card, dest)) {
        game_set_message(game, "Move is not valid.");
        return false;
    }

    source_list = (CardList *)resolved_source_list;
    if (dest.type == MOVE_DEST_COLUMN) {
        dest_list = &game->columns[dest.index];
    } else {
        dest_list = &game->foundations[dest.index];
    }

    moving_segment = card_list_detach_segment(source_list, source_row);
    if (dest.type == MOVE_DEST_FOUNDATION) {
        moving_segment->next = NULL;
    }
    card_list_append_segment(dest_list, moving_segment);

    if (source.type == MOVE_SOURCE_COLUMN) {
        reveal_column_tail(source_list);
    }

    game_update_win_state(game);
    if (game->won) {
        game_set_message(game, "You won!");
    } else {
        game_set_message(game, "OK");
    }
    return true;
}

static MoveSource column_source_for_card(const Card *card, int column) {
    MoveSource source;
    source.type = MOVE_SOURCE_COLUMN;
    source.index = column;
    source.has_card = true;
    source.rank = card->rank;
    source.suit = card->suit;
    return source;
}

static MoveSource foundation_source(int foundation) {
    MoveSource source;
    source.type = MOVE_SOURCE_FOUNDATION;
    source.index = foundation;
    source.has_card = false;
    source.rank = '\0';
    source.suit = '\0';
    return source;
}

static MoveDest column_dest(int column) {
    MoveDest dest;
    dest.type = MOVE_DEST_COLUMN;
    dest.index = column;
    return dest;
}

static MoveDest foundation_dest(int foundation) {
    MoveDest dest;
    dest.type = MOVE_DEST_FOUNDATION;
    dest.index = foundation;
    return dest;
}

bool game_can_move_from_column_to_column(const GameState *game, int source_column, int source_row, int dest_column) {
    const Card *card = game_column_card_at(game, source_column, source_row);
    return card != NULL && can_move_from_source(game, column_source_for_card(card, source_column), column_dest(dest_column));
}

bool game_can_move_from_column_to_foundation(const GameState *game, int source_column, int source_row, int foundation) {
    const Card *card = game_column_card_at(game, source_column, source_row);
    return card != NULL && can_move_from_source(game, column_source_for_card(card, source_column), foundation_dest(foundation));
}

bool game_can_move_from_foundation_to_column(const GameState *game, int foundation, int dest_column) {
    return can_move_from_source(game, foundation_source(foundation), column_dest(dest_column));
}
