#include "card_list.h"

#include <stdlib.h>

void card_list_init(CardList *list) {
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

void card_list_clear(CardList *list) {
    CardNode *current = list->head;
    while (current != NULL) {
        CardNode *next = current->next;
        free(current);
        current = next;
    }

    card_list_init(list);
}

bool card_list_append_card(CardList *list, Card card) {
    CardNode *node = malloc(sizeof(*node));
    if (node == NULL) {
        return false;
    }

    node->card = card;
    node->next = NULL;
    card_list_append_node(list, node);
    return true;
}

void card_list_append_node(CardList *list, CardNode *node) {
    node->next = NULL;

    if (list->tail != NULL) {
        list->tail->next = node;
    } else {
        list->head = node;
    }

    list->tail = node;
    list->count++;
}

CardNode *card_list_pop_front(CardList *list) {
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

void card_list_insert_node_at(CardList *list, CardNode *node, int index) {
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
        card_list_append_node(list, node);
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

const CardNode *card_list_node_at(const CardList *list, int index) {
    const CardNode *node = list->head;
    int i = 0;

    while (node != NULL && i < index) {
        node = node->next;
        i++;
    }

    return node;
}

CardNode *card_list_mutable_node_at(CardList *list, int index) {
    CardNode *node = list->head;
    int i = 0;

    while (node != NULL && i < index) {
        node = node->next;
        i++;
    }

    return node;
}

static int segment_count(CardNode *segment) {
    int count = 0;

    while (segment != NULL) {
        count++;
        segment = segment->next;
    }

    return count;
}

CardNode *card_list_detach_segment(CardList *list, int start_index) {
    CardNode *segment;
    CardNode *previous = NULL;
    int removed;

    if (start_index < 0 || start_index >= list->count) {
        return NULL;
    }

    if (start_index > 0) {
        previous = card_list_mutable_node_at(list, start_index - 1);
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

void card_list_append_segment(CardList *list, CardNode *segment) {
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
