#ifndef CARD_LIST_H
#define CARD_LIST_H

#include "game.h"

void card_list_init(CardList *list);
void card_list_clear(CardList *list);
bool card_list_append_card(CardList *list, Card card);
void card_list_append_node(CardList *list, CardNode *node);
CardNode *card_list_pop_front(CardList *list);
void card_list_insert_node_at(CardList *list, CardNode *node, int index);
const CardNode *card_list_node_at(const CardList *list, int index);
CardNode *card_list_mutable_node_at(CardList *list, int index);
CardNode *card_list_detach_segment(CardList *list, int start_index);
void card_list_append_segment(CardList *list, CardNode *segment);

#endif
