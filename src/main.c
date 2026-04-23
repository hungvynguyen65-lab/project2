#include "game.h"
#include "sdl_ui.h"
#include "terminal_ui.h"

#include <string.h>

int main(int argc, char *argv[]) {
    GameState game;
    int exit_code = 0;

    game_init(&game);

    if (argc > 1 && strcmp(argv[1], "--gui") == 0) {
        bool smoke_test = argc > 2 && strcmp(argv[2], "--smoke-test") == 0;
        exit_code = sdl_run(&game, smoke_test);
    } else {
        terminal_run(&game);
    }

    game_destroy(&game);

    return exit_code;
}
