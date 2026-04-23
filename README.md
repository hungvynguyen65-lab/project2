# 02322 Project 2

Yukon Solitaire implemented in C for 02322 Machine Oriented Programming.

Current status:
- terminal-based project skeleton in C
- VS Code setup for build/run
- foundation for shared game logic

## GUI plan

Step 1 is to keep the game logic separate from the user interface. The project now has:
- `src/game.c` and `include/game.h` for reusable game state and core actions
- `src/terminal_ui.c` and `include/terminal_ui.h` for terminal rendering and command input

The SDL GUI should be added as a separate UI layer that reads the same `GameState` and calls the
same core game functions.

Step 2 adds SDL3 and a first GUI entry point:
- run `run.bat` for the terminal version
- run `gui.bat` for the SDL window

Step 3 renders the shared game state in SDL:
- columns and foundations are drawn from `GameState`
- visible cards show rank and suit, hidden cards show `[ ]`
- the GUI has buttons for `LD`, `SW`, `SI`, `SR`, `SD`, `P`, `Q`, and `QQ`
- GUI buttons now follow the same startup/play command availability rules as the terminal version
- the GUI also has a command input box, so commands like `LD valid_deck.txt`, `SD out.txt`, and `SI 26` can be run directly

Step 4 adds shared move execution and basic GUI interaction:
- terminal move commands like `C1->F1` and `C7->C1` now use the shared core
- click a visible card in the SDL GUI to select it
- click a column or foundation to attempt the move
- selected cards are highlighted in yellow
- legal GUI destinations are highlighted in green using the shared move rules

Backend step 1 adds file loading:
- `LD` loads the default ordered deck
- `LD <filename>` loads and validates a deck file with one card per line
- invalid cards, duplicate cards, missing files, and wrong deck sizes report errors

Backend step 2 adds shuffling:
- `SI <split>` interleaves two piles split at the requested card count
- `SI` chooses a random valid split
- `SR` shuffles by inserting each card at a random position in a new pile

Backend step 3 adds deck saving:
- `SD <filename>` saves the current deck order to a file
- `SD` saves to the default file `cards.txt`

Backend step 4 adds win detection:
- the game now tracks when all 52 cards are in the foundations
- successful final moves set the message to `You won!`
- the SDL GUI also shows `STATUS: WON`
