#ifndef TERMINAL_UI_H
#define TERMINAL_UI_H

#include "game.h"

void terminal_run(GameState *game);
void terminal_render(const GameState *game);
void terminal_handle_command(GameState *game, const char *input);

#endif
