#include "game.h"

#include <stdio.h>

int main(void) {
    GameState game;
    char input[INPUT_BUFFER_SIZE];

    game_init(&game);

    while (game.running) {
        game_render(&game);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        game_handle_command(&game, input);
    }

    game_destroy(&game);
    return 0;
}
