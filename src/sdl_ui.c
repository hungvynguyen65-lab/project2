#include "sdl_ui.h"

#include <SDL3/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 1360
#define WINDOW_HEIGHT 900
#define CARD_WIDTH 92.0f
#define CARD_HEIGHT 126.0f
#define COLUMN_GAP 26.0f
#define ROW_GAP 34.0f
#define TABLEAU_X 38.0f
#define TABLEAU_Y 206.0f
#define FOUNDATION_X 946.0f
#define FOUNDATION_Y 28.0f
#define FOUNDATION_GAP 96.0f

typedef enum {
    PROMPT_NONE,
    PROMPT_LOAD,
    PROMPT_SAVE,
    PROMPT_SPLIT
} GuiPromptType;

typedef struct {
    SDL_FRect rect;
    const char *label;
    const char *command;
    GuiPromptType prompt_type;
} GuiButton;

typedef struct {
    bool active;
    bool from_foundation;
    int index;
    int row;
    char source_text[12];
} GuiSelection;

typedef struct {
    bool active;
    GuiPromptType type;
    char text[COMMAND_SIZE];
} GuiPrompt;

static const GuiButton BUTTONS[] = {
    {{34.0f, 32.0f, 96.0f, 38.0f}, "LD", "LD", PROMPT_LOAD},
    {{146.0f, 32.0f, 96.0f, 38.0f}, "SW", "SW", PROMPT_NONE},
    {{258.0f, 32.0f, 96.0f, 38.0f}, "SI", "SI", PROMPT_SPLIT},
    {{370.0f, 32.0f, 96.0f, 38.0f}, "SR", "SR", PROMPT_NONE},
    {{34.0f, 82.0f, 96.0f, 38.0f}, "SD", "SD", PROMPT_SAVE},
    {{146.0f, 82.0f, 96.0f, 38.0f}, "P", "P", PROMPT_NONE},
    {{258.0f, 82.0f, 96.0f, 38.0f}, "Q", "Q", PROMPT_NONE},
    {{370.0f, 82.0f, 96.0f, 38.0f}, "QQ", "QQ", PROMPT_NONE}
};

typedef struct {
    GuiPromptType type;
    const char *title;
    const char *primary_hint;
    const char *secondary_hint;
    const char *command;
} PromptInfo;

static const PromptInfo PROMPTS[] = {
    {PROMPT_LOAD, "LOAD DECK", "TYPE A FILE NAME", "OR LEAVE EMPTY FOR DEFAULT", "LD"},
    {PROMPT_SAVE, "SAVE DECK", "TYPE A FILE NAME", "OR LEAVE EMPTY FOR CARDS.TXT", "SD"},
    {PROMPT_SPLIT, "INTERLEAVE SPLIT", "TYPE A SPLIT NUMBER", "OR LEAVE EMPTY FOR RANDOM", "SI"}
};

static const unsigned char FONT[][7] = {
    {0, 0, 0, 0, 0, 0, 0},
    {14, 17, 19, 21, 25, 17, 14}, {4, 12, 4, 4, 4, 4, 14},
    {14, 17, 1, 2, 4, 8, 31}, {30, 1, 1, 14, 1, 1, 30},
    {2, 6, 10, 18, 31, 2, 2}, {31, 16, 30, 1, 1, 17, 14},
    {6, 8, 16, 30, 17, 17, 14}, {31, 1, 2, 4, 8, 8, 8},
    {14, 17, 17, 14, 17, 17, 14}, {14, 17, 17, 15, 1, 2, 12},
    {14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30},
    {14, 17, 16, 16, 16, 17, 14}, {30, 17, 17, 17, 17, 17, 30},
    {31, 16, 16, 30, 16, 16, 31}, {31, 16, 16, 30, 16, 16, 16},
    {14, 17, 16, 23, 17, 17, 15}, {17, 17, 17, 31, 17, 17, 17},
    {14, 4, 4, 4, 4, 4, 14}, {7, 2, 2, 2, 18, 18, 12},
    {17, 18, 20, 24, 20, 18, 17}, {16, 16, 16, 16, 16, 16, 31},
    {17, 27, 21, 21, 17, 17, 17}, {17, 25, 21, 19, 17, 17, 17},
    {14, 17, 17, 17, 17, 17, 14}, {30, 17, 17, 30, 16, 16, 16},
    {14, 17, 17, 17, 21, 18, 13}, {30, 17, 17, 30, 20, 18, 17},
    {15, 16, 16, 14, 1, 1, 30}, {31, 4, 4, 4, 4, 4, 4},
    {17, 17, 17, 17, 17, 17, 14}, {17, 17, 17, 17, 17, 10, 4},
    {17, 17, 17, 21, 21, 21, 10}, {17, 17, 10, 4, 10, 17, 17},
    {17, 17, 10, 4, 4, 4, 4}, {31, 1, 2, 4, 8, 16, 31},
    {14, 8, 8, 8, 8, 8, 14}, {14, 2, 2, 2, 2, 2, 14},
    {0, 4, 4, 0, 4, 4, 0}, {0, 0, 0, 31, 0, 0, 0},
    {0, 0, 0, 0, 0, 12, 12}, {0, 0, 0, 0, 0, 0, 31},
    {1, 2, 4, 8, 16, 0, 0}, {16, 8, 4, 2, 1, 0, 0}
};

static const unsigned char *glyph_for(char c) {
    /* Map ASCII characters to the small built-in bitmap font. */
    if (c >= 'a' && c <= 'z') {
        c = (char)(c - 'a' + 'A');
    }

    if (c >= '0' && c <= '9') {
        return FONT[1 + c - '0'];
    }

    if (c >= 'A' && c <= 'Z') {
        return FONT[11 + c - 'A'];
    }

    switch (c) {
        case '[': return FONT[37];
        case ']': return FONT[38];
        case ':': return FONT[39];
        case '-': return FONT[40];
        case '.': return FONT[41];
        case '_': return FONT[42];
        case '/': return FONT[43];
        case '\\': return FONT[44];
        default: return FONT[0];
    }
}

static void set_color(SDL_Renderer *renderer, Uint8 r, Uint8 g, Uint8 b) {
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
}

static void draw_text(SDL_Renderer *renderer, const char *text, float x, float y, float scale) {
    size_t index;

    /* Draw text one filled rectangle at a time from 5x7 glyph bitmaps. */
    for (index = 0; text[index] != '\0'; index++) {
        const unsigned char *glyph = glyph_for(text[index]);
        int row;

        for (row = 0; row < 7; row++) {
            int col;
            for (col = 0; col < 5; col++) {
                if ((glyph[row] & (1 << (4 - col))) != 0) {
                    SDL_FRect pixel = {
                        x + (float)(index * 6 + col) * scale,
                        y + (float)row * scale,
                        scale,
                        scale
                    };
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }
    }
}

static void draw_label(SDL_Renderer *renderer, const char *text, float x, float y) {
    set_color(renderer, 236, 230, 215);
    draw_text(renderer, text, x, y, 3.0f);
}

static bool is_red_suit(char suit) {
    return suit == 'D' || suit == 'H';
}

static void draw_pattern(SDL_Renderer *renderer, const char *const *pattern, int rows, float x, float y, float scale) {
    int row;

    for (row = 0; row < rows; row++) {
        int col;
        const char *line = pattern[row];
        for (col = 0; line[col] != '\0'; col++) {
            if (line[col] == '1') {
                SDL_FRect pixel = {
                    x + (float)col * scale,
                    y + (float)row * scale,
                    scale,
                    scale
                };
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
}

static void draw_suit_icon(SDL_Renderer *renderer, char suit, float x, float y, float scale) {
    static const struct {
        char suit;
        const char *pattern[7];
    } patterns[] = {
        {'H', {"0110110", "1111111", "1111111", "1111111", "0111110", "0011100", "0001000"}},
        {'D', {"0001000", "0011100", "0111110", "1111111", "0111110", "0011100", "0001000"}},
        {'C', {"0001000", "0011100", "0011100", "1111111", "1111111", "0011100", "0011100"}},
        {'S', {"0001000", "0011100", "0111110", "1111111", "0011100", "0111110", "0011100"}}
    };
    const char *const *pattern = patterns[3].pattern;
    size_t i;

    /* Pick the simple bitmap pattern that matches the card suit. */
    for (i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        if (patterns[i].suit == suit) {
            pattern = patterns[i].pattern;
            break;
        }
    }

    draw_pattern(renderer, pattern, 7, x, y, scale);
}

static void draw_card_back(SDL_Renderer *renderer, float x, float y) {
    SDL_FRect outer = {x, y, CARD_WIDTH, CARD_HEIGHT};
    SDL_FRect inner = {x + 6.0f, y + 6.0f, CARD_WIDTH - 12.0f, CARD_HEIGHT - 12.0f};
    float stripe_y;

    set_color(renderer, 52, 88, 164);
    SDL_RenderFillRect(renderer, &outer);
    set_color(renderer, 236, 230, 215);
    SDL_RenderRect(renderer, &outer);

    set_color(renderer, 27, 52, 104);
    SDL_RenderFillRect(renderer, &inner);
    set_color(renderer, 198, 214, 247);
    SDL_RenderRect(renderer, &inner);

    for (stripe_y = y + 14.0f; stripe_y < y + CARD_HEIGHT - 14.0f; stripe_y += 12.0f) {
        SDL_FRect stripe = {x + 12.0f, stripe_y, CARD_WIDTH - 24.0f, 4.0f};
        SDL_RenderFillRect(renderer, &stripe);
    }

    set_color(renderer, 236, 230, 215);
    draw_text(renderer, "YS", x + 27.0f, y + 44.0f, 4.0f);
}

static void draw_card(SDL_Renderer *renderer, const Card *card, float x, float y) {
    char rank_text[2] = {0};
    SDL_FRect rect = {x, y, CARD_WIDTH, CARD_HEIGHT};
    SDL_FRect shadow = {x + 2.0f, y + 3.0f, CARD_WIDTH, CARD_HEIGHT};

    /* Empty slots, hidden cards, and visible cards each have a distinct drawing path. */
    if (card == NULL) {
        set_color(renderer, 18, 74, 48);
        SDL_RenderFillRect(renderer, &rect);
        set_color(renderer, 112, 166, 132);
        SDL_RenderRect(renderer, &rect);
        return;
    }

    if (!card->visible) {
        draw_card_back(renderer, x, y);
        return;
    }

    rank_text[0] = card->rank;

    set_color(renderer, 203, 189, 162);
    SDL_RenderFillRect(renderer, &shadow);
    set_color(renderer, 244, 239, 226);
    SDL_RenderFillRect(renderer, &rect);
    set_color(renderer, 42, 45, 43);
    SDL_RenderRect(renderer, &rect);
    set_color(renderer, 227, 219, 198);
    SDL_RenderRect(renderer, &(SDL_FRect){x + 2.0f, y + 2.0f, CARD_WIDTH - 4.0f, CARD_HEIGHT - 4.0f});

    if (is_red_suit(card->suit)) {
        set_color(renderer, 174, 43, 43);
    } else {
        set_color(renderer, 34, 38, 36);
    }

    draw_text(renderer, rank_text, x + 10.0f, y + 8.0f, 3.0f);
    draw_suit_icon(renderer, card->suit, x + 13.0f, y + 28.0f, 1.6f);
    draw_suit_icon(renderer, card->suit, x + 30.0f, y + 46.0f, 4.4f);
    draw_suit_icon(renderer, card->suit, x + CARD_WIDTH - 24.0f, y + CARD_HEIGHT - 26.0f, 1.4f);
    draw_text(renderer, rank_text, x + CARD_WIDTH - 24.0f, y + CARD_HEIGHT - 18.0f, 2.6f);
}

static void draw_selection(SDL_Renderer *renderer, const SDL_FRect *rect) {
    SDL_FRect outer = {rect->x - 4.0f, rect->y - 4.0f, rect->w + 8.0f, rect->h + 8.0f};
    SDL_FRect inner = {rect->x - 2.0f, rect->y - 2.0f, rect->w + 4.0f, rect->h + 4.0f};

    set_color(renderer, 255, 212, 74);
    SDL_RenderRect(renderer, &outer);
    SDL_RenderRect(renderer, &inner);
}

static void draw_valid_target(SDL_Renderer *renderer, const SDL_FRect *rect) {
    SDL_FRect outer = {rect->x - 3.0f, rect->y - 3.0f, rect->w + 6.0f, rect->h + 6.0f};

    set_color(renderer, 116, 242, 148);
    SDL_RenderRect(renderer, &outer);
}

static void draw_button(SDL_Renderer *renderer, const GuiButton *button) {
    set_color(renderer, 229, 171, 76);
    SDL_RenderFillRect(renderer, &button->rect);
    set_color(renderer, 54, 42, 25);
    SDL_RenderRect(renderer, &button->rect);
    draw_text(renderer, button->label, button->rect.x + 16.0f, button->rect.y + 9.0f, 3.0f);
}

static bool point_in_rect(float x, float y, const SDL_FRect *rect) {
    return x >= rect->x && x <= rect->x + rect->w && y >= rect->y && y <= rect->y + rect->h;
}

static void clear_selection(GuiSelection *selection) {
    selection->active = false;
    selection->from_foundation = false;
    selection->index = -1;
    selection->row = -1;
    selection->source_text[0] = '\0';
}

static void clear_prompt(GuiPrompt *prompt) {
    prompt->active = false;
    prompt->type = PROMPT_NONE;
    prompt->text[0] = '\0';
}

static void open_prompt(GuiPrompt *prompt, GuiPromptType type) {
    prompt->active = true;
    prompt->type = type;
    prompt->text[0] = '\0';
}

static SDL_FRect prompt_panel_rect(void) {
    SDL_FRect rect = {330.0f, 276.0f, 700.0f, 286.0f};
    return rect;
}

static SDL_FRect prompt_input_rect(void) {
    SDL_FRect rect = {366.0f, 430.0f, 628.0f, 44.0f};
    return rect;
}

static SDL_FRect prompt_ok_rect(void) {
    SDL_FRect rect = {530.0f, 494.0f, 180.0f, 42.0f};
    return rect;
}

static SDL_FRect prompt_cancel_rect(void) {
    SDL_FRect rect = {730.0f, 494.0f, 180.0f, 42.0f};
    return rect;
}

static const PromptInfo *prompt_info(GuiPromptType type) {
    size_t i;

    for (i = 0; i < sizeof(PROMPTS) / sizeof(PROMPTS[0]); i++) {
        if (PROMPTS[i].type == type) {
            return &PROMPTS[i];
        }
    }

    return NULL;
}

static void submit_prompt(GameState *game, GuiSelection *selection, GuiPrompt *prompt) {
    const PromptInfo *info = prompt_info(prompt->type);
    char command[COMMAND_SIZE];

    /* Turn prompt input into the same command strings used by the terminal UI. */
    clear_selection(selection);

    if (info != NULL) {
        if (prompt->text[0] == '\0') {
            game_execute_command(game, info->command);
        } else {
            snprintf(command, sizeof(command), "%s %s", info->command, prompt->text);
            game_execute_command(game, command);
        }
    }

    clear_prompt(prompt);
}

static void append_prompt_text(GuiPrompt *prompt, const char *text) {
    size_t current;
    size_t remaining;

    if (!prompt->active) {
        return;
    }

    current = strlen(prompt->text);
    remaining = sizeof(prompt->text) - current - 1;
    if (remaining > 0) {
        strncat(prompt->text, text, remaining);
    }
}

static void handle_prompt_key(GameState *game, GuiSelection *selection, GuiPrompt *prompt, SDL_Keycode key) {
    size_t length;

    if (!prompt->active) {
        return;
    }

    if (key == SDLK_ESCAPE) {
        clear_prompt(prompt);
        return;
    }

    if (key == SDLK_BACKSPACE) {
        length = strlen(prompt->text);
        if (length > 0) {
            prompt->text[length - 1] = '\0';
        }
        return;
    }

    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        submit_prompt(game, selection, prompt);
    }
}

static bool handle_button_click(GameState *game, GuiSelection *selection, GuiPrompt *prompt, float x, float y) {
    size_t i;

    /* Buttons either open a prompt for arguments or execute a command directly. */
    if (prompt->active) {
        SDL_FRect ok_rect = prompt_ok_rect();
        SDL_FRect cancel_rect = prompt_cancel_rect();

        if (point_in_rect(x, y, &ok_rect)) {
            submit_prompt(game, selection, prompt);
        } else if (point_in_rect(x, y, &cancel_rect)) {
            clear_prompt(prompt);
        }
        return true;
    }

    for (i = 0; i < sizeof(BUTTONS) / sizeof(BUTTONS[0]); i++) {
        if (point_in_rect(x, y, &BUTTONS[i].rect)) {
            clear_selection(selection);
            if (BUTTONS[i].prompt_type != PROMPT_NONE) {
                open_prompt(prompt, BUTTONS[i].prompt_type);
            } else {
                game_execute_command(game, BUTTONS[i].command);
            }
            return true;
        }
    }

    return true;
}

static SDL_FRect column_card_rect(int column, int row) {
    SDL_FRect rect = {
        TABLEAU_X + (float)column * (CARD_WIDTH + COLUMN_GAP),
        TABLEAU_Y + (float)row * ROW_GAP,
        CARD_WIDTH,
        CARD_HEIGHT
    };
    return rect;
}

static SDL_FRect foundation_rect(int foundation) {
    SDL_FRect rect = {
        FOUNDATION_X + (float)foundation * FOUNDATION_GAP,
        FOUNDATION_Y,
        CARD_WIDTH,
        CARD_HEIGHT
    };
    return rect;
}

static bool hit_column(const GameState *game, float x, float y, int *column) {
    int col;

    for (col = 0; col < COLUMN_COUNT; col++) {
        int row_count = game_column_count(game, col);
        SDL_FRect area = {
            TABLEAU_X + (float)col * (CARD_WIDTH + COLUMN_GAP),
            TABLEAU_Y - 34.0f,
            CARD_WIDTH,
            CARD_HEIGHT + (float)((row_count > 0 ? row_count : 1) - 1) * ROW_GAP + 34.0f
        };

        if (point_in_rect(x, y, &area)) {
            *column = col;
            return true;
        }
    }

    return false;
}

static bool hit_foundation(float x, float y, int *foundation) {
    int i;

    for (i = 0; i < FOUNDATION_COUNT; i++) {
        SDL_FRect rect = foundation_rect(i);
        if (point_in_rect(x, y, &rect)) {
            *foundation = i;
            return true;
        }
    }

    return false;
}

static bool hit_visible_tableau_card(const GameState *game, float x, float y, int *column, int *row) {
    int col;

    for (col = 0; col < COLUMN_COUNT; col++) {
        int r;
        int row_count = game_column_count(game, col);

        for (r = row_count - 1; r >= 0; r--) {
            const Card *card = game_column_card_at(game, col, r);
            SDL_FRect rect = column_card_rect(col, r);

            if (card != NULL && card->visible && point_in_rect(x, y, &rect)) {
                *column = col;
                *row = r;
                return true;
            }
        }
    }

    return false;
}

static void select_tableau_card(GameState *game, GuiSelection *selection, int column, int row) {
    const Card *card = game_column_card_at(game, column, row);

    if (card == NULL || !card->visible) {
        return;
    }

    selection->active = true;
    selection->from_foundation = false;
    selection->index = column;
    selection->row = row;
    snprintf(selection->source_text, sizeof(selection->source_text), "C%d:%c%c", column + 1, card->rank, card->suit);
    game_set_message(game, "Card selected.");
}

static void select_foundation_card(GameState *game, GuiSelection *selection, int foundation) {
    const Card *card = game_foundation_top(game, foundation);

    if (card == NULL) {
        return;
    }

    selection->active = true;
    selection->from_foundation = true;
    selection->index = foundation;
    selection->row = -1;
    snprintf(selection->source_text, sizeof(selection->source_text), "F%d", foundation + 1);
    game_set_message(game, "Card selected.");
}

static void try_gui_move(GameState *game, GuiSelection *selection, const char *dest_text) {
    char command[COMMAND_SIZE];

    snprintf(command, sizeof(command), "%s->%s", selection->source_text, dest_text);
    game_set_last_command(game, command);
    game_move_command(game, command);
    clear_selection(selection);
}

static void handle_game_click(GameState *game, GuiSelection *selection, float x, float y) {
    int column;
    int row;
    int foundation;
    char dest_text[8];

    /* First click selects a source; second click attempts a legal move target. */
    if (selection->active) {
        if (hit_foundation(x, y, &foundation)) {
            snprintf(dest_text, sizeof(dest_text), "F%d", foundation + 1);
            try_gui_move(game, selection, dest_text);
            return;
        }

        if (hit_column(game, x, y, &column)) {
            snprintf(dest_text, sizeof(dest_text), "C%d", column + 1);
            try_gui_move(game, selection, dest_text);
            return;
        }

        clear_selection(selection);
        game_set_message(game, "Selection cleared.");
        return;
    }

    if (hit_visible_tableau_card(game, x, y, &column, &row)) {
        select_tableau_card(game, selection, column, row);
        return;
    }

    if (hit_foundation(x, y, &foundation)) {
        select_foundation_card(game, selection, foundation);
    }
}

static void draw_prompt(SDL_Renderer *renderer, const GuiPrompt *prompt) {
    const PromptInfo *info = prompt_info(prompt->type);
    SDL_FRect overlay = {0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT};
    SDL_FRect panel = prompt_panel_rect();
    SDL_FRect input = prompt_input_rect();
    SDL_FRect ok_rect = prompt_ok_rect();
    SDL_FRect cancel_rect = prompt_cancel_rect();

    set_color(renderer, 7, 32, 21);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 7, 32, 21, 170);
    SDL_RenderFillRect(renderer, &overlay);

    set_color(renderer, 32, 88, 60);
    SDL_RenderFillRect(renderer, &panel);
    set_color(renderer, 195, 221, 171);
    SDL_RenderRect(renderer, &panel);

    set_color(renderer, 245, 239, 225);
    SDL_RenderFillRect(renderer, &input);
    set_color(renderer, 54, 42, 25);
    SDL_RenderRect(renderer, &input);

    set_color(renderer, 229, 171, 76);
    SDL_RenderFillRect(renderer, &ok_rect);
    SDL_RenderFillRect(renderer, &cancel_rect);
    set_color(renderer, 54, 42, 25);
    SDL_RenderRect(renderer, &ok_rect);
    SDL_RenderRect(renderer, &cancel_rect);

    draw_label(renderer, info != NULL ? info->title : "", panel.x + 30.0f, panel.y + 24.0f);
    draw_label(renderer, info != NULL ? info->primary_hint : "", panel.x + 30.0f, panel.y + 68.0f);
    draw_label(renderer, info != NULL ? info->secondary_hint : "", panel.x + 30.0f, panel.y + 108.0f);
    draw_text(
        renderer,
        prompt->text[0] != '\0' ? prompt->text : "TYPE HERE",
        input.x + 14.0f,
        input.y + 10.0f,
        3.0f
    );
    draw_text(renderer, "OK", ok_rect.x + 54.0f, ok_rect.y + 9.0f, 3.0f);
    draw_text(renderer, "CANCEL", cancel_rect.x + 20.0f, cancel_rect.y + 9.0f, 3.0f);
}

static void render_game(SDL_Renderer *renderer, const GameState *game, const GuiSelection *selection, const GuiPrompt *prompt) {
    int col;
    int row;
    char status[96];
    SDL_FRect controls_panel = {20.0f, 18.0f, 886.0f, 122.0f};
    SDL_FRect foundations_panel = {926.0f, 18.0f, 408.0f, 184.0f};
    SDL_FRect tableau_panel = {20.0f, 158.0f, 886.0f, 718.0f};
    SDL_FRect status_panel = {926.0f, 676.0f, 408.0f, 200.0f};

    /* Render the complete current state: controls, foundations, tableau, and status. */
    set_color(renderer, 13, 66, 42);
    SDL_RenderClear(renderer);

    set_color(renderer, 26, 87, 57);
    SDL_RenderFillRect(renderer, &controls_panel);
    SDL_RenderFillRect(renderer, &foundations_panel);
    SDL_RenderFillRect(renderer, &tableau_panel);
    SDL_RenderFillRect(renderer, &status_panel);

    set_color(renderer, 103, 158, 125);
    SDL_RenderRect(renderer, &controls_panel);
    SDL_RenderRect(renderer, &foundations_panel);
    SDL_RenderRect(renderer, &tableau_panel);
    SDL_RenderRect(renderer, &status_panel);

    for (col = 0; col < (int)(sizeof(BUTTONS) / sizeof(BUTTONS[0])); col++) {
        draw_button(renderer, &BUTTONS[col]);
    }
    draw_label(renderer, "FOUNDATIONS", FOUNDATION_X, FOUNDATION_Y - 20.0f);
    for (col = 0; col < FOUNDATION_COUNT; col++) {
        char label[4];
        float x = FOUNDATION_X + (float)col * FOUNDATION_GAP;
        float y = FOUNDATION_Y;
        const Card *card = game_foundation_top(game, col);
        SDL_FRect rect = foundation_rect(col);

        snprintf(label, sizeof(label), "F%d", col + 1);
        draw_label(renderer, label, x + 24.0f, y + CARD_HEIGHT + 12.0f);
        draw_card(renderer, card, x, y);
        if (selection->active && !selection->from_foundation &&
            game_can_move_from_column_to_foundation(game, selection->index, selection->row, col)) {
            draw_valid_target(renderer, &rect);
        }
        if (selection->active && selection->from_foundation && selection->index == col) {
            draw_selection(renderer, &rect);
        }
    }

    for (col = 0; col < COLUMN_COUNT; col++) {
        char label[4];
        float x = TABLEAU_X + (float)col * (CARD_WIDTH + COLUMN_GAP);
        int row_count = game_column_count(game, col);

        snprintf(label, sizeof(label), "C%d", col + 1);
        draw_label(renderer, label, x + 26.0f, TABLEAU_Y - 28.0f);

        for (row = 0; row < row_count; row++) {
            const Card *card = game_column_card_at(game, col, row);
            if (card != NULL) {
                SDL_FRect rect = column_card_rect(col, row);
                draw_card(renderer, card, rect.x, rect.y);
                if (selection->active && !selection->from_foundation && selection->index == col && selection->row == row) {
                    draw_selection(renderer, &rect);
                }
            }
        }

        if (selection->active) {
            SDL_FRect target_rect = column_card_rect(col, row_count > 0 ? row_count - 1 : 0);

            if ((!selection->from_foundation &&
                 game_can_move_from_column_to_column(game, selection->index, selection->row, col)) ||
                (selection->from_foundation &&
                 game_can_move_from_foundation_to_column(game, selection->index, col))) {
                draw_valid_target(renderer, &target_rect);
            }
        }
    }

    draw_label(renderer, "STATUS", 946.0f, 694.0f);
    snprintf(status, sizeof(status), "LAST: %s", game->last_command);
    draw_label(renderer, status, 946.0f, 730.0f);
    snprintf(status, sizeof(status), "MESSAGE: %s", game->message);
    draw_label(renderer, status, 946.0f, 766.0f);
    snprintf(status, sizeof(status), "PHASE: %s", game->phase == PHASE_PLAY ? "PLAY" : "STARTUP");
    draw_label(renderer, status, 946.0f, 802.0f);
    if (game_is_won(game)) {
        draw_label(renderer, "STATUS: WON", 946.0f, 838.0f);
    }

    if (prompt->active) {
        draw_prompt(renderer, prompt);
    }

    SDL_RenderPresent(renderer);
}

int sdl_run(GameState *game, bool smoke_test) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    GuiSelection selection;
    GuiPrompt prompt;
    bool running = true;
    Uint64 smoke_start = 0;

    clear_selection(&selection);
    clear_prompt(&prompt);

    /* Main SDL setup and event loop for the graphical version. */
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("02322 Project 2 - Yukon Solitaire", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_StartTextInput(window);

    if (smoke_test) {
        smoke_start = SDL_GetTicks();
        game_load_default_deck(game);
        game_start(game);
    }

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                running = handle_button_click(game, &selection, &prompt, event.button.x, event.button.y);
                if (running && game->running && !prompt.active) {
                    handle_game_click(game, &selection, event.button.x, event.button.y);
                }
            } else if (event.type == SDL_EVENT_TEXT_INPUT) {
                append_prompt_text(&prompt, event.text.text);
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                handle_prompt_key(game, &selection, &prompt, event.key.key);
            }
        }

        if (!game->running) {
            running = false;
        }

        render_game(renderer, game, &selection, &prompt);
        if (smoke_test && SDL_GetTicks() - smoke_start > 500) {
            running = false;
        }
        SDL_Delay(16);
    }

    SDL_StopTextInput(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
