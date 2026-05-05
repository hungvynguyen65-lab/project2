#include "card_utils.h"

#include <stdio.h>
#include <string.h>

static const char RANKS[] = {'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K'};
static const char SUITS[] = {'C', 'D', 'H', 'S'};

int card_rank_index(char rank) {
    int i;

    for (i = 0; i < 13; i++) {
        if (RANKS[i] == rank) {
            return i;
        }
    }

    return -1;
}

int card_suit_index(char suit) {
    int i;

    for (i = 0; i < 4; i++) {
        if (SUITS[i] == suit) {
            return i;
        }
    }

    return -1;
}

char card_rank_at(int index) {
    if (index < 0 || index >= 13) {
        return '\0';
    }

    return RANKS[index];
}

char card_suit_at(int index) {
    if (index < 0 || index >= 4) {
        return '\0';
    }

    return SUITS[index];
}

int card_rank_value(char rank) {
    int index = card_rank_index(rank);
    return index >= 0 ? index + 1 : 0;
}

bool card_is_valid_suit(char suit) {
    return card_suit_index(suit) >= 0;
}

bool card_parse_line(const char *line, Card *card) {
    size_t length = strlen(line);

    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
        length--;
    }

    if (length != 2) {
        return false;
    }

    if (card_rank_index(line[0]) < 0 || card_suit_index(line[1]) < 0) {
        return false;
    }

    card->rank = line[0];
    card->suit = line[1];
    card->visible = false;
    return true;
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
