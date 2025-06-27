#include "game.h"
#include <stdio.h>
#include <string.h>

// Direction vectors for pattern detection
static const Direction directions[4] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

void game_init(GomokuGame* game) {
    memset(game->board, 0, sizeof(game->board));
    game->current_player = BLACK;
    game->taken_stones[0] = 0;
    game->taken_stones[1] = 0;
    game->rule_center_opening = true;
    game->rule_no_double_threes = true;
    game->rule_captures = true;
    game->mode_ai = true;
    
}

void game_copy(GomokuGame* dest, const GomokuGame* src) {
    memcpy(dest, src, sizeof(GomokuGame));
}

bool game_is_valid_position(const GomokuGame* game, int row, int col) {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && 
           game->board[row][col] == EMPTY;
}

bool game_is_double_free_three(GomokuGame* game, int row, int col, int player) {
    // Temporarily place stone
    game->board[row][col] = player;
    
    int open_three_directions = 0;
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d].dx;
        int dy = directions[d].dy;
        
        // Count consecutive stones
        int count = 1;
        
        // Positive direction
        int r = row + dx, c = col + dy;
        while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && 
               game->board[r][c] == player) {
            count++;
            r += dx;
            c += dy;
        }
        
        // Negative direction
        r = row - dx;
        c = col - dy;
        while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && 
               game->board[r][c] == player) {
            count++;
            r -= dx;
            c -= dy;
        }
        
        // Check if this forms an open three
        if (count == 3) {
            // Check if both ends are open
            int end1_r = row + dx * (count - 1);
            int end1_c = col + dy * (count - 1);
            int end2_r = row - dx * (count - 1);
            int end2_c = col - dy * (count - 1);
            
            bool end1_open = (end1_r >= 0 && end1_r < BOARD_SIZE && 
                             end1_c >= 0 && end1_c < BOARD_SIZE && 
                             game->board[end1_r][end1_c] == EMPTY);
            bool end2_open = (end2_r >= 0 && end2_r < BOARD_SIZE && 
                             end2_c >= 0 && end2_c < BOARD_SIZE && 
                             game->board[end2_r][end2_c] == EMPTY);
            
            if (end1_open && end2_open) {
                open_three_directions++;
            }
        }
        
        if (open_three_directions >= 2) {
            game->board[row][col] = EMPTY;
            return true;
        }
    }
    
    game->board[row][col] = EMPTY;
    return false;
}

int game_capture_stones(GomokuGame* game, int row, int col) {
    int captured_count = 0;
    int player = game->board[row][col];
    int opponent = (player == BLACK) ? WHITE : BLACK;
    
    // Check all 8 directions (4 directions + their opposites)
    for (int d = 0; d < 8; d++) {
        int dx = (d < 4) ? directions[d].dx : -directions[d-4].dx;
        int dy = (d < 4) ? directions[d].dy : -directions[d-4].dy;
        
        // Check pattern: player-opponent-opponent-player
        int r0 = row - 3 * dx, c0 = col - 3 * dy;
        int r1 = row - 2 * dx, c1 = col - 2 * dy;
        int r2 = row - 1 * dx, c2 = col - 1 * dy;
        
        if (r0 >= 0 && r0 < BOARD_SIZE && c0 >= 0 && c0 < BOARD_SIZE &&
            r1 >= 0 && r1 < BOARD_SIZE && c1 >= 0 && c1 < BOARD_SIZE &&
            r2 >= 0 && r2 < BOARD_SIZE && c2 >= 0 && c2 < BOARD_SIZE) {
            
            if (game->board[r0][c0] == player &&
                game->board[r1][c1] == opponent &&
                game->board[r2][c2] == opponent) {
                game->board[r1][c1] = EMPTY;
                game->board[r2][c2] = EMPTY;
                captured_count += 2;
            }
        }
        
        // Check forward pattern
        r0 = row + 3 * dx; c0 = col + 3 * dy;
        r1 = row + 2 * dx; c1 = col + 2 * dy;
        r2 = row + 1 * dx; c2 = col + 1 * dy;
        
        if (r0 >= 0 && r0 < BOARD_SIZE && c0 >= 0 && c0 < BOARD_SIZE &&
            r1 >= 0 && r1 < BOARD_SIZE && c1 >= 0 && c1 < BOARD_SIZE &&
            r2 >= 0 && r2 < BOARD_SIZE && c2 >= 0 && c2 < BOARD_SIZE) {
            
            if (game->board[r0][c0] == player &&
                game->board[r1][c1] == opponent &&
                game->board[r2][c2] == opponent) {
                game->board[r1][c1] = EMPTY;
                game->board[r2][c2] = EMPTY;
                captured_count += 2;
            }
        }
    }
    
    return captured_count;
}

bool game_place_stone(GomokuGame* game, int row, int col) {
    if (!game_is_valid_position(game, row, col)) {
        return false;
    }
    
    // Check center opening rule
    if (game->rule_center_opening && game->current_player == BLACK) {
        bool empty_board = true;
        for (int i = 0; i < BOARD_SIZE && empty_board; i++) {
            for (int j = 0; j < BOARD_SIZE && empty_board; j++) {
                if (game->board[i][j] != EMPTY) {
                    empty_board = false;
                }
            }
        }
        if (empty_board && (row != BOARD_SIZE/2 || col != BOARD_SIZE/2)) {
            return false;
        }
    }
    
    // Check double free three rule
    if (game->rule_no_double_threes && 
        game_is_double_free_three(game, row, col, game->current_player)) {
        return false;
    }
    
    game->board[row][col] = game->current_player;
    
    // Handle captures
    if (game->rule_captures) {
        int captured = game_capture_stones(game, row, col);
        game->taken_stones[game->current_player - 1] += captured;
    }
    
    game->current_player = (game->current_player == BLACK) ? WHITE : BLACK;
    return true;
}

bool game_check_winner(const GomokuGame* game, int* winner) {
    *winner = EMPTY;
    
    // Check captures
    if (game->taken_stones[0] >= 10) {
        *winner = BLACK;
        return true;
    }
    if (game->taken_stones[1] >= 10) {
        *winner = WHITE;
        return true;
    }
    
    // Check five in a row
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game->board[i][j] != EMPTY) {
                int player = game->board[i][j];
                
                for (int d = 0; d < 4; d++) {
                    int count = 1;
                    int dx = directions[d].dx;
                    int dy = directions[d].dy;
                    
                    // Count in positive direction
                    int r = i + dx, c = j + dy;
                    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && 
                           game->board[r][c] == player) {
                        count++;
                        r += dx;
                        c += dy;
                    }
                    
                    // Count in negative direction
                    r = i - dx; c = j - dy;
                    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && 
                           game->board[r][c] == player) {
                        count++;
                        r -= dx;
                        c -= dy;
                    }
                    
                    if (count >= 5) {
                        *winner = player;
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

uint64_t game_hash(const GomokuGame* game) {
    uint64_t hash = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game->board[i][j] != EMPTY) {
                hash ^= ((uint64_t)(i * BOARD_SIZE + j + game->board[i][j]) << 32) | 
                       ((uint64_t)game->board[i][j] << 16) | (i << 8) | j;
            }
        }
    }
    hash ^= ((uint64_t)game->taken_stones[0] << 8) | game->taken_stones[1];
    hash ^= game->current_player;
    return hash;
}

void game_print_board(const GomokuGame* game) {
    printf("\n  ");
    for (int j = 0; j < BOARD_SIZE; j++) {
        printf("%2d", j);
    }
    printf("\n");
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%2d ", i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            switch (game->board[i][j]) {
                case EMPTY: printf(" ."); break;
                case BLACK: printf(" B"); break;
                case WHITE: printf(" W"); break;
                default: printf(" ?"); break;
            }
        }
        printf("\n");
    }
    printf("Current player: %s\n", (game->current_player == BLACK) ? "Black" : "White");
    printf("Captured stones - Black: %d, White: %d\n", 
           game->taken_stones[0], game->taken_stones[1]);
}