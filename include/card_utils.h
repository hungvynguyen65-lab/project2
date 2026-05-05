#ifndef CARD_UTILS_H
#define CARD_UTILS_H

#include "game.h"

int card_rank_index(char rank);
int card_suit_index(char suit);
char card_rank_at(int index);
char card_suit_at(int index);
int card_rank_value(char rank);
bool card_is_valid_suit(char suit);
bool card_parse_line(const char *line, Card *card);

#endif
