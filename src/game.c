#include "game.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const int COLUMN_LENGTHS[COLUMN_COUNT] = {1, 6, 7, 8, 9, 10, 11};
static const char RANKS[] = {'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K'};
static const char SUITS[] = {'C', 'D', 'H', 'S'};

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

static void init_list(CardList *list) {
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

static void append_node(CardList *list, CardNode *node) {
    node->next = NULL;

    if (list->tail != NULL) {
        list->tail->next = node;
    } else {
        list->head = node;
    }

    list->tail = node;
    list->count++;
}

static CardNode *pop_front(CardList *list) {
    CardNode *node = list->head;

    if (node == NULL) {
        return NULL;
    }

    list->head = node->next;
    if (list->head == NULL) {
        list->tail = NULL;
    }

    node->next = NULL;
    list->count--;
    return node;
}

static void insert_node_at(CardList *list, CardNode *node, int index) {
    CardNode *previous;
    int i;

    if (index <= 0 || list->head == NULL) {
        node->next = list->head;
        list->head = node;
        if (list->tail == NULL) {
            list->tail = node;
        }
        list->count++;
        return;
    }

    if (index >= list->count) {
        append_node(list, node);
        return;
    }

    previous = list->head;
    for (i = 0; previous != NULL && i < index - 1; i++) {
        previous = previous->next;
    }

    node->next = previous->next;
    previous->next = node;
    list->count++;
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

static void rebuild_columns_from_deck(GameState *game, bool play_visibility) {
    CardNode *deck_node = game->deck.head;
    int row;

    clear_tableau(game);

    /* Deal the stored deck into the seven Yukon columns row by row. */
    for (row = 0; deck_node != NULL && row < TABLEAU_MAX_ROWS; row++) {
        int col;

        for (col = 0; col < COLUMN_COUNT && deck_node != NULL; col++) {
            if (row >= COLUMN_LENGTHS[col]) {
                continue;
            }

            Card card = deck_node->card;
            if (play_visibility) {
                card.visible = row >= col;
            } else {
                card.visible = false;
            }

            append_card(&game->columns[col], card);
            deck_node = deck_node->next;
        }
    }
}

static void clear_all(GameState *game) {
    clear_list(&game->deck);
    clear_tableau(game);
}

static int rank_index(char rank) {
    int i;

    for (i = 0; i < 13; i++) {
        if (RANKS[i] == rank) {
            return i;
        }
    }

    return -1;
}

static int suit_index(char suit) {
    int i;

    for (i = 0; i < 4; i++) {
        if (SUITS[i] == suit) {
            return i;
        }
    }

    return -1;
}

static bool parse_card_line(const char *line, Card *card) {
    size_t length = strlen(line);

    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        length--;
    }

    if (length != 2) {
        return false;
    }

    if (rank_index(line[0]) < 0 || suit_index(line[1]) < 0) {
        return false;
    }

    card->rank = line[0];
    card->suit = line[1];
    card->visible = false;
    return true;
}

static void replace_deck_from_list(GameState *game, CardList *new_deck) {
    clear_all(game);
    game->deck = *new_deck;
    new_deck->head = NULL;
    new_deck->tail = NULL;
    new_deck->count = 0;
    rebuild_columns_from_deck(game, false);
    game->phase = PHASE_STARTUP;
    game->won = false;
}

static void update_win_state(GameState *game) {
    int total = 0;
    int i;

    for (i = 0; i < FOUNDATION_COUNT; i++) {
        total += game->foundations[i].count;
    }

    game->won = (total == 52);
}

void game_load_default_deck(GameState *game) {
    int suit_index;
    int rank_index;

    clear_all(game);

    /* Build the ordered deck: all clubs, then diamonds, hearts, and spades. */
    for (suit_index = 0; suit_index < 4; suit_index++) {
        for (rank_index = 0; rank_index < 13; rank_index++) {
            Card card;
            card.rank = RANKS[rank_index];
            card.suit = SUITS[suit_index];
            card.visible = false;
            if (!append_card(&game->deck, card)) {
                game_set_message(game, "Out of memory.");
                return;
            }
        }
    }

    rebuild_columns_from_deck(game, false);
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
        if (!parse_card_line(line, &card)) {
            snprintf(game->message, sizeof(game->message), "Invalid card at line %d.", line_number);
            fclose(file);
            clear_list(&loaded);
            return;
        }

        r = rank_index(card.rank);
        s = suit_index(card.suit);
        if (seen[s][r]) {
            snprintf(game->message, sizeof(game->message), "Duplicate card at line %d.", line_number);
            fclose(file);
            clear_list(&loaded);
            return;
        }

        seen[s][r] = true;
        if (!append_card(&loaded, card)) {
            game_set_message(game, "Out of memory.");
            fclose(file);
            clear_list(&loaded);
            return;
        }
    }

    fclose(file);

    if (loaded.count != 52) {
        snprintf(game->message, sizeof(game->message), "Invalid deck: expected 52 cards, got %d.", loaded.count);
        clear_list(&loaded);
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

    init_list(&first);
    init_list(&second);
    init_list(&shuffled);

    /* Split the deck, then alternately move cards from each pile. */
    for (i = 0; i < split; i++) {
        append_node(&first, pop_front(&game->deck));
    }

    while (game->deck.count > 0) {
        append_node(&second, pop_front(&game->deck));
    }

    while (first.count > 0 && second.count > 0) {
        append_node(&shuffled, pop_front(&first));
        append_node(&shuffled, pop_front(&second));
    }

    while (first.count > 0) {
        append_node(&shuffled, pop_front(&first));
    }

    while (second.count > 0) {
        append_node(&shuffled, pop_front(&second));
    }

    game->deck = shuffled;
    rebuild_columns_from_deck(game, false);
    game->won = false;
    game_set_message(game, "OK");
}

void game_shuffle_random(GameState *game) {
    CardList shuffled;

    if (game->deck.count == 0) {
        game_set_message(game, "No deck loaded.");
        return;
    }

    init_list(&shuffled);

    /* Insert each card into a random position in the new shuffled pile. */
    while (game->deck.count > 0) {
        CardNode *node = pop_front(&game->deck);
        int position = shuffled.count == 0 ? 0 : rand() % (shuffled.count + 1);
        insert_node_at(&shuffled, node, position);
    }

    game->deck = shuffled;
    rebuild_columns_from_deck(game, false);
    game->won = false;
    game_set_message(game, "OK");
}

void game_start(GameState *game) {
    if (game->deck.count == 0) {
        game_set_message(game, "No deck loaded.");
        return;
    }

    /* Re-deal from the saved deck and apply the initial Yukon visibility. */
    rebuild_columns_from_deck(game, true);
    game->phase = PHASE_PLAY;
    update_win_state(game);
    game_set_message(game, "OK");
}

void game_quit_to_startup(GameState *game) {
    if (game->phase != PHASE_PLAY) {
        game_set_message(game, "Not in PLAY phase.");
        return;
    }

    rebuild_columns_from_deck(game, false);
    game->phase = PHASE_STARTUP;
    game->won = false;
    game_set_message(game, "OK");
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

static CardNode *mutable_node_at(CardList *list, int index) {
    CardNode *node = list->head;
    int i = 0;

    while (node != NULL && i < index) {
        node = node->next;
        i++;
    }

    return node;
}

static int rank_value(char rank) {
    int index = rank_index(rank);
    return index >= 0 ? index + 1 : 0;
}

static bool is_valid_suit(char suit) {
    return suit_index(suit) >= 0;
}

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
    return rank_value(card->rank) + 1 == rank_value(bottom->rank) && card->suit != bottom->suit;
}

static bool can_place_on_foundation(const Card *card, const CardList *foundation) {
    const Card *top;

    if (foundation->count == 0) {
        return card->rank == 'A';
    }

    top = &foundation->tail->card;
    return card->suit == top->suit && rank_value(card->rank) == rank_value(top->rank) + 1;
}

static int segment_count(CardNode *segment) {
    int count = 0;

    while (segment != NULL) {
        count++;
        segment = segment->next;
    }

    return count;
}

static CardNode *detach_segment(CardList *list, int start_index) {
    CardNode *segment;
    CardNode *previous = NULL;
    int removed;

    if (start_index < 0 || start_index >= list->count) {
        return NULL;
    }

    if (start_index > 0) {
        previous = mutable_node_at(list, start_index - 1);
        segment = previous->next;
        previous->next = NULL;
        list->tail = previous;
    } else {
        segment = list->head;
        list->head = NULL;
        list->tail = NULL;
    }

    removed = segment_count(segment);
    list->count -= removed;
    if (list->count == 0) {
        list->head = NULL;
        list->tail = NULL;
    }

    return segment;
}

static void append_segment(CardList *list, CardNode *segment) {
    CardNode *tail = segment;
    int added = 1;

    while (tail->next != NULL) {
        tail = tail->next;
        added++;
    }

    if (list->tail != NULL) {
        list->tail->next = segment;
    } else {
        list->head = segment;
    }

    list->tail = tail;
    list->count += added;
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
        if (text[2] != ':' || rank_value(text[3]) == 0 || !is_valid_suit(text[4])) {
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

        *moving_card = &node_at(*source_list, *source_row)->card;
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
    CardList *source_list = NULL;
    CardList *dest_list = NULL;
    CardNode *moving_segment = NULL;
    Card *moving_card = NULL;
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

    if (!resolve_move_source(game, source, (const CardList **)&source_list, &source_row, (const Card **)&moving_card)) {
        game_set_message(game, "Move is not valid.");
        return false;
    }

    if (!validate_move_destination(game, source, source_list, source_row, moving_card, dest)) {
        game_set_message(game, "Move is not valid.");
        return false;
    }

    if (dest.type == MOVE_DEST_COLUMN) {
        dest_list = &game->columns[dest.index];
    } else {
        dest_list = &game->foundations[dest.index];
    }

    if (source.type == MOVE_SOURCE_COLUMN) {
        moving_segment = mutable_node_at(source_list, source_row);
    } else {
        moving_segment = source_list->tail;
    }

    moving_segment = detach_segment(source_list, source_row);
    if (dest.type == MOVE_DEST_FOUNDATION) {
        moving_segment->next = NULL;
    }
    append_segment(dest_list, moving_segment);

    if (source.type == MOVE_SOURCE_COLUMN) {
        reveal_column_tail(source_list);
    }

    update_win_state(game);
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

void card_to_text(const Card *card, char *buffer, size_t size) {
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
    clear_all(game);
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

    node = node_at(&game->columns[column], row);
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
