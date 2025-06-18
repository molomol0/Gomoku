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
    
    // Main game loop
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN && !game_over) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    int row, col;
                    if (graphics_handle_click(e.button.x, e.button.y, &row, &col)) {
                        // Only allow human moves if it's not AI's turn
                        if (game.current_player != AI_PLAYER) {
                            handle_player_move(&game, row, col);
                            
                            // Render the game immediately after player's move
                            graphics_draw_game(&game);
                            SDL_RenderPresent(graphics_get_renderer());
                            
                            // Check for winner after player move
                            int winner;
                            if (game_check_winner(&game, &winner)) {
                                printf("\n=== GAME OVER ===\n");
                                if (winner == BLACK) {
                                    printf("Black wins!\n");
                                } else if (winner == WHITE) {
                                    printf("White wins!\n");
                                } else {
                                    printf("Draw!\n");
                                }
                                graphics_show_winner(winner);
                                game_over = true;
                            } else if (game.current_player == AI_PLAYER) {
                                // AI's turn
                                handle_ai_move(&game);
                                
                                // Check for winner after AI move
                                if (game_check_winner(&game, &winner)) {
                                    printf("\n=== GAME OVER ===\n");
                                    if (winner == BLACK) {
                                        printf("Black wins!\n");
                                    } else if (winner == WHITE) {
                                        printf("White wins!\n");
                                    } else {
                                        printf("Draw!\n");
                                    }
                                    graphics_show_winner(winner);
                                    game_over = true;
                                }
                            }
                        }
                    }
                }
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_r:
                        // Restart game
                        if (game_over) {
                            printf("\n=== RESTARTING GAME ===\n");
                            game_init(&game);
                            graphics_clear_visual_markers();
                            game_over = false;
                        }
                        break;
                    case SDLK_p:
                        // Print board
                        game_print_board(&game);
                        break;
                    case SDLK_c:
                        // Clear transposition table
                        ai_clear_transposition_table();
                        printf("AI cache cleared.\n");
                        break;
                }
            }
        }
        
        // Render game
        graphics_draw_game(&game);
        
        // Small delay to prevent excessive CPU usage
        SDL_Delay(16); // ~60 FPS
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