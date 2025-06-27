#include "types.h"
#include "game.h"
#include "ai_core.h"
#include "graphics.h"
#include <stdio.h>
#include <stdbool.h>

// Configuration
#define AI_SEARCH_DEPTH 10
#define AI_PLAYER WHITE

// Function declarations
static void handle_player_move(GomokuGame* game, int row, int col);
static void handle_ai_move(GomokuGame* game);
static void print_game_info(const GomokuGame* game);
static int get_rule_button_clicked(int x, int y);

int main(void) {
    printf("=== Gomoku AI ===\n");
    printf("Click to place your stone. AI will respond automatically.\n");
    printf("Search depth: %d\n", AI_SEARCH_DEPTH);
    printf("You are playing as: %s\n", (AI_PLAYER == WHITE) ? "Black" : "White");
    printf("AI is playing as: %s\n", (AI_PLAYER == WHITE) ? "White" : "Black");
    printf("================================================\n\n");
    
    // Initialize systems
    if (!graphics_init()) {
        fprintf(stderr, "Failed to initialize graphics!\n");
        return 1;
    }
    
    ai_init();
    
    // Initialize game
    GomokuGame game;
    game_init(&game);
    
    bool quit = false;
    bool game_over = false;
    SDL_Event e;
    bool marker_active = false;
    int marker_row = -1, marker_col = -1;

    // Main game loop
    while (!quit) {
        // If not in AI mode, show marker before player's move (except first move)
        if (!game.mode_ai && !game_over) {
            // Check if board is empty (first move)
            bool empty_board = true;
            for (int i = 0; i < BOARD_SIZE && empty_board; i++)
                for (int j = 0; j < BOARD_SIZE && empty_board; j++)
                    if (game.board[i][j] != EMPTY) empty_board = false;

            // Only show marker if not already active and not first move
            if (!marker_active && !empty_board) {
                AIStats stats;
                Move ai_move = ai_get_best_move(&game, AI_SEARCH_DEPTH, &stats);
                if (ai_move.row >= 0 && ai_move.col >= 0) {
                    graphics_clear_visual_markers();
                    graphics_add_visual_marker(ai_move.row, ai_move.col, COLOR_RED);
                    marker_active = true;
                    marker_row = ai_move.row;
                    marker_col = ai_move.col;
                }
            }
        }

        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN && !game_over) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    int btn = get_rule_button_clicked(e.button.x, e.button.y);
                    if (btn != -1) {
                        switch (btn) {
                            case 0: game.rule_captures = !game.rule_captures; break;
                            case 1: game.rule_no_double_threes = !game.rule_no_double_threes; break;
                            case 2: game.rule_center_opening = !game.rule_center_opening; break;
                            case 3: game.mode_ai = !game.mode_ai; 
                                    graphics_clear_visual_markers();
                                    marker_active = false;
                                    break;
                        }
                        graphics_draw_game(&game);
                        SDL_RenderPresent(graphics_get_renderer());
                        continue;
                    }
                    int row, col;
                    if (graphics_handle_click(e.button.x, e.button.y, &row, &col)) {
                        if (game.mode_ai) {
                            // Only allow human moves if it's not AI's turn
                            if (game.current_player != AI_PLAYER) {
                                handle_player_move(&game, row, col);
                                graphics_draw_game(&game);
                                SDL_RenderPresent(graphics_get_renderer());
                                int winner;
                                if (game_check_winner(&game, &winner)) {
                                    printf("\n=== GAME OVER ===\n");
                                    if (winner == BLACK) printf("Black wins!\n");
                                    else if (winner == WHITE) printf("White wins!\n");
                                    else printf("Draw!\n");
                                    graphics_show_winner(winner);
                                    game_over = true;
                                } else if (game.current_player == AI_PLAYER) {
                                    handle_ai_move(&game);
                                    if (game_check_winner(&game, &winner)) {
                                        printf("\n=== GAME OVER ===\n");
                                        if (winner == BLACK) printf("Black wins!\n");
                                        else if (winner == WHITE) printf("White wins!\n");
                                        else printf("Draw!\n");
                                        graphics_show_winner(winner);
                                        game_over = true;
                                    }
                                }
                            }
                        } else {
                            // AI suggests for both players, but only marks, doesn't place
                            // Don't run AI for first move (must be center)
                            bool empty_board = true;
                            for (int i = 0; i < BOARD_SIZE && empty_board; i++)
                                for (int j = 0; j < BOARD_SIZE && empty_board; j++)
                                    if (game.board[i][j] != EMPTY) empty_board = false;

                            if (marker_active) {
                                graphics_clear_visual_markers();
                                marker_active = false;
                            }

                            if (!empty_board) {
                                // Remove marker after player places a stone
                                if (game_is_valid_position(&game, row, col)) {
                                    game_place_stone(&game, row, col);
                                    print_game_info(&game);

                                    // Immediately update the display so the stone appears
                                    graphics_draw_game(&game);
                                    SDL_RenderPresent(graphics_get_renderer());

                                    // After placing, check for game over
                                    int winner;
                                    if (game_check_winner(&game, &winner)) {
                                        printf("\n=== GAME OVER ===\n");
                                        if (winner == BLACK) printf("Black wins!\n");
                                        else if (winner == WHITE) printf("White wins!\n");
                                        else printf("Draw!\n");
                                        graphics_show_winner(winner);
                                        game_over = true;
                                    } else {
                                        // Show marker for next player (if not game over and not first move)
                                        bool now_empty = true;
                                        for (int i = 0; i < BOARD_SIZE && now_empty; i++)
                                            for (int j = 0; j < BOARD_SIZE && now_empty; j++)
                                                if (game.board[i][j] != EMPTY) now_empty = false;
                                        if (!now_empty) {
                                            AIStats stats;
                                            Move ai_move = ai_get_best_move(&game, AI_SEARCH_DEPTH, &stats);
                                            if (ai_move.row >= 0 && ai_move.col >= 0) {
                                                graphics_add_visual_marker(ai_move.row, ai_move.col, COLOR_RED);
                                                marker_active = true;
                                                marker_row = ai_move.row;
                                                marker_col = ai_move.col;
                                            }
                                        }
                                    }
                                }
                            } else {
                                // Only allow center for first move
                                if (row == BOARD_SIZE/2 && col == BOARD_SIZE/2) {
                                    game_place_stone(&game, row, col);
                                    print_game_info(&game);
                                } else {
                                    printf("First move must be center!\n");
                                }
                            }

                            // After move, if not game over, show next best move as marker
                            int winner;
                            if (game_check_winner(&game, &winner)) {
                                printf("\n=== GAME OVER ===\n");
                                if (winner == BLACK) printf("Black wins!\n");
                                else if (winner == WHITE) printf("White wins!\n");
                                else printf("Draw!\n");
                                graphics_show_winner(winner);
                                game_over = true;
                            } else {
                                // Only show marker if not game over and not first move
                                bool now_empty = true;
                                for (int i = 0; i < BOARD_SIZE && now_empty; i++)
                                    for (int j = 0; j < BOARD_SIZE && now_empty; j++)
                                        if (game.board[i][j] != EMPTY) now_empty = false;
                                if (!now_empty) {
                                    AIStats stats;
                                    Move ai_move = ai_get_best_move(&game, AI_SEARCH_DEPTH, &stats);
                                    if (ai_move.row >= 0 && ai_move.col >= 0) {
                                        graphics_add_visual_marker(ai_move.row, ai_move.col, COLOR_RED);
                                        marker_active = true;
                                        marker_row = ai_move.row;
                                        marker_col = ai_move.col;
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_r:
                        if (game_over) {
                            printf("\n=== RESTARTING GAME ===\n");
                            game_init(&game);
                            graphics_clear_visual_markers();
                            marker_active = false;
                            game_over = false;
                        }
                        break;
                    case SDLK_p:
                        game_print_board(&game);
                        break;
                }
            }
        }

        graphics_draw_game(&game);
        SDL_Delay(16);
    }
    
    // Cleanup
    ai_cleanup();
    graphics_cleanup();
    
    printf("Thanks for playing!\n");
    return 0;
}

static void handle_player_move(GomokuGame* game, int row, int col) {
    if (game_place_stone(game, row, col)) {
        printf("Player placed stone at (%d, %d)\n", row, col);
        print_game_info(game);
    } else {
        printf("Invalid move at (%d, %d)\n", row, col);
    }
}

static void handle_ai_move(GomokuGame* game) {
    printf("AI is thinking...\n");
    
    AIStats stats;
    Move ai_move = ai_get_best_move(game, AI_SEARCH_DEPTH, &stats);
    
    if (ai_move.row >= 0 && ai_move.col >= 0) {
        if (game_place_stone(game, ai_move.row, ai_move.col)) {
            printf("AI placed stone at (%d, %d)\n", ai_move.row, ai_move.col);
            print_game_info(game);
            
            // Add visual marker for AI move
            graphics_add_visual_marker(ai_move.row, ai_move.col, COLOR_RED);
        } else {
            printf("ERROR: AI suggested invalid move at (%d, %d)\n", ai_move.row, ai_move.col);
        }
    } else {
        printf("ERROR: AI failed to find a valid move\n");
    }
}

static void print_game_info(const GomokuGame* game) {
    printf("Current player: %s\n", (game->current_player == BLACK) ? "Black" : "White");
    printf("Captured stones - Black: %d, White: %d\n", 
           game->taken_stones[0], game->taken_stones[1]);
    printf("---\n");
}

static int get_rule_button_clicked(int x, int y) {
    for (int i = 0; i < 4; i++) { // 4 buttons now
        SDL_Rect rect = button_rects[i];
        if (x >= rect.x && x < rect.x + rect.w &&
            y >= rect.y && y < rect.y + rect.h) {
            return i;
        }
    }
    return -1;
}