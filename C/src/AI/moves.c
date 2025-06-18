#include "ai_moves.h"
#include "ai_evaluation.h"
#include <string.h>

static bool is_near_existing_stone(const GomokuGame* game, int row, int col, int radius) {
    for (int di = -radius; di <= radius; di++) {
        for (int dj = -radius; dj <= radius; dj++) {
            int ni = row + di, nj = col + dj;
            if (ni >= 0 && ni < BOARD_SIZE && nj >= 0 && nj < BOARD_SIZE &&
                game->board[ni][nj] != EMPTY) {
                return true;
            }
        }
    }
    return false;
}

void find_winning_moves_smart(const GomokuGame* game, Move* moves, int* count, int player) {
    *count = 0;
    
    // Check all empty positions
    for (int i = 0; i < BOARD_SIZE && *count < 5; i++) {
        for (int j = 0; j < BOARD_SIZE && *count < 5; j++) {
            if (game->board[i][j] == EMPTY) {
                // Quick proximity check - only positions within 2 of existing stones
                if (is_near_existing_stone(game, i, j, 2)) {
                    GomokuGame temp_game;
                    memcpy(&temp_game, game, sizeof(GomokuGame));
                    int score = ai_evaluate_position_for_player(&temp_game, i, j, player);
                    if (score >= AI_INFINITY) {
                        moves[*count].row = i;
                        moves[*count].col = j;
                        moves[*count].score = score;
                        (*count)++;
                    }
                }
            }
        }
    }
}

static void add_neighbors_around_stone(const GomokuGame* game, int stone_row, int stone_col, 
                                     Move* moves, int* count, int max_moves, 
                                     bool visited[BOARD_SIZE][BOARD_SIZE]) {
    // Add neighbors within distance 3 for better strategic coverage
    for (int di = -3; di <= 3; di++) {
        for (int dj = -3; dj <= 3; dj++) {
            if (di == 0 && dj == 0) continue;
            int ni = stone_row + di, nj = stone_col + dj;
            
            if (ni >= 0 && ni < BOARD_SIZE && nj >= 0 && nj < BOARD_SIZE &&
                game->board[ni][nj] == EMPTY && !visited[ni][nj] && *count < max_moves) {
                
                moves[*count].row = ni;
                moves[*count].col = nj;
                moves[*count].score = 0;
                (*count)++;
                visited[ni][nj] = true;
            }
        }
    }
}

void find_neighbor_positions_smart(const GomokuGame* game, Move* moves, int* count, int max_moves) {
    *count = 0;
    bool visited[BOARD_SIZE][BOARD_SIZE] = {false};
    
    // Use radius 3 for better strategic play (was too restrictive at radius 1-2)
    for (int i = 0; i < BOARD_SIZE && *count < max_moves; i++) {
        for (int j = 0; j < BOARD_SIZE && *count < max_moves; j++) {
            if (game->board[i][j] != EMPTY) {
                add_neighbors_around_stone(game, i, j, moves, count, max_moves, visited);
            }
        }
    }
}

static bool is_board_empty(const GomokuGame* game) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game->board[i][j] != EMPTY) {
                return false;
            }
        }
    }
    return true;
}

// NEW: Count available space in a direction
static int count_available_space(const GomokuGame* game, int row, int col, int dx, int dy) {
    int space = 1; // Count the current position
    
    // Count in positive direction
    int r = row + dx, c = col + dy;
    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && 
           (game->board[r][c] == EMPTY || game->board[r][c] == game->current_player)) {
        space++;
        r += dx;
        c += dy;
    }
    
    // Count in negative direction
    r = row - dx;
    c = col - dy;
    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && 
           (game->board[r][c] == EMPTY || game->board[r][c] == game->current_player)) {
        space++;
        r -= dx;
        c -= dy;
    }
    
    return space;
}

// NEW: Check if move connects separate groups
static bool is_connecting_groups(const GomokuGame* game, int row, int col, int player) {
    int groups_connected = 0;
    const int directions[8][2] = {{1,0}, {0,1}, {1,1}, {1,-1}, {-1,0}, {0,-1}, {-1,-1}, {-1,1}};
    
    for (int d = 0; d < 8; d++) {
        int r = row + directions[d][0];
        int c = col + directions[d][1];
        if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && game->board[r][c] == player) {
            groups_connected++;
        }
    }
    
    return groups_connected >= 2;
}

// MODIFIED: Much stronger connectivity bonus
static int calculate_connectivity_bonus(const GomokuGame* game, int row, int col, int player) {
    int connectivity_bonus = 0;
    int pattern_potential = 0;
    
    // Check all 4 directions for existing stones of the same player
    const int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        // Count available space in this direction
        int space = count_available_space(game, row, col, dx, dy);
        
        // Check both directions along this line
        int consecutive = 0;
        
        // Positive direction
        for (int dist = 1; dist <= 4; dist++) {
            int r = row + dx * dist;
            int c = col + dy * dist;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && game->board[r][c] == player) {
                consecutive++;
            } else {
                break;
            }
        }
        
        // Negative direction  
        for (int dist = 1; dist <= 4; dist++) {
            int r = row - dx * dist;
            int c = col - dy * dist;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && game->board[r][c] == player) {
                consecutive++;
            } else {
                break;
            }
        }
        
        // Much stronger bonus for positions that can form winning patterns
        if (space >= 5 && consecutive >= 2) {
            pattern_potential += consecutive * 5000; // Increased from 1000
        } else if (space >= 5 && consecutive >= 1) {
            pattern_potential += consecutive * 3000;
        }
        
        // Award significant bonus for connecting to existing stones
        if (consecutive > 0) {
            connectivity_bonus += consecutive * 2000; // Increased from 1000
        }
    }
    
    // Extra bonus for connecting multiple groups
    if (is_connecting_groups(game, row, col, player)) {
        connectivity_bonus += 10000;
    }
    
    // Bonus for central positions in early game
    int center = BOARD_SIZE / 2;
    int distance_from_center = abs(row - center) + abs(col - center);
    if (distance_from_center <= 4) {
        connectivity_bonus += (5 - distance_from_center) * 500;
    }
    
    return connectivity_bonus + pattern_potential;
}

// NEW: Check if move creates a threat
static bool creates_own_threat(const GomokuGame* game, int row, int col, int player) {
    GomokuGame temp_game;
    memcpy(&temp_game, game, sizeof(GomokuGame));
    temp_game.board[row][col] = player;
    
    // Check if this move creates any strong patterns
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (temp_game.board[i][j] == EMPTY) {
                int score = ai_evaluate_position_for_player(&temp_game, i, j, player);
                if (score >= PATTERN_FOUR) {
                    return true;
                }
            }
        }
    }
    return false;
}

// NEW: Check if move blocks opponent threat
static bool blocks_opponent_threat(const GomokuGame* game, int row, int col) {
    int opponent = (game->current_player == BLACK) ? WHITE : BLACK;
    int score = ai_evaluate_position_for_player(game, row, col, opponent);
    return score >= PATTERN_THREE * 2;
}

// MODIFIED: More aggressive evaluation
static void evaluate_generated_moves(const GomokuGame* game, Move* moves, int move_count) {
    int opponent = (game->current_player == BLACK) ? WHITE : BLACK;
    
    // Evaluate all positions
    for (int i = 0; i < move_count; i++) {
        GomokuGame temp_game;
        memcpy(&temp_game, game, sizeof(GomokuGame));
        
        int our_score = ai_evaluate_position_for_player(&temp_game, moves[i].row, moves[i].col, game->current_player);
        int opp_score = ai_evaluate_position_for_player(&temp_game, moves[i].row, moves[i].col, opponent);
        
        // Add connectivity bonus to encourage building connected patterns
        int connectivity_bonus = calculate_connectivity_bonus(game, moves[i].row, moves[i].col, game->current_player);
        
        // More aggressive weighting - prioritize offense
        moves[i].score = (int)(our_score * 1.5 + opp_score * 0.6) + connectivity_bonus;
        
        // Bonus for dual-purpose moves
        if (blocks_opponent_threat(game, moves[i].row, moves[i].col) && 
            creates_own_threat(game, moves[i].row, moves[i].col, game->current_player)) {
            moves[i].score += 15000;
        }
        
        // Count direct threats created (more efficient)
        temp_game.board[moves[i].row][moves[i].col] = game->current_player;
        int threat_count = 0;
        
        // Direction vectors for checking threats
        const int check_dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
        
        // Only check positions near the placed stone
        for (int di = -4; di <= 4; di++) {
            for (int dj = -4; dj <= 4; dj++) {
                int r = moves[i].row + di;
                int c = moves[i].col + dj;
                if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && 
                    temp_game.board[r][c] == EMPTY) {
                    
                    // Quick check if this position could be a threat
                    temp_game.board[r][c] = game->current_player;
                    
                    // Check all directions from this position
                    for (int d = 0; d < 4; d++) {
                        int count = count_consecutive_optimized(&temp_game, r, c, 
                                                              check_dirs[d][0], 
                                                              check_dirs[d][1], 
                                                              game->current_player);
                        if (count >= 4) {
                            threat_count++;
                            break;
                        }
                    }
                    
                    temp_game.board[r][c] = EMPTY;
                }
            }
        }
        
        // Bonus for creating multiple threats (fork)
        if (threat_count >= 2) {
            moves[i].score += 30000 + threat_count * 5000;
        } else if (threat_count == 1) {
            moves[i].score += 8000;
        }
    }
}

static void sort_moves_by_score(Move* moves, int move_count) {
    // Sort moves by score (descending)
    for (int i = 0; i < move_count - 1; i++) {
        for (int j = i + 1; j < move_count; j++) {
            if (moves[j].score > moves[i].score) {
                Move temp = moves[i];
                moves[i] = moves[j];
                moves[j] = temp;
            }
        }
    }
}

static Move create_center_move(void) {
    Move center_move;
    center_move.row = BOARD_SIZE / 2;
    center_move.col = BOARD_SIZE / 2;
    center_move.score = 0;
    return center_move;
}

static bool try_find_winning_moves(const GomokuGame* game, Move* moves, int* move_count, int player) {
    find_winning_moves_smart(game, moves, move_count, player);
    return (*move_count > 0);
}

static bool try_find_blocking_moves(const GomokuGame* game, Move* moves, int* move_count) {
    int opponent = (game->current_player == BLACK) ? WHITE : BLACK;
    find_winning_moves_smart(game, moves, move_count, opponent);
    return (*move_count > 0);
}

static void generate_and_evaluate_moves(const GomokuGame* game, Move* moves, int* move_count, int max_moves) {
    find_neighbor_positions_smart(game, moves, move_count, max_moves);
    evaluate_generated_moves(game, moves, *move_count);
    sort_moves_by_score(moves, *move_count);
    
    // Limit to best moves
    if (*move_count > max_moves) {
        *move_count = max_moves;
    }
}

void ai_generate_moves(const GomokuGame* game, Move* moves, int* move_count, int max_moves) {
    *move_count = 0;
    printf("Generating moves for player %d...\n", game->current_player);
    // Handle empty board
    if (is_board_empty(game)) {
        moves[0] = create_center_move();
        *move_count = 1;
        return;
    }
    
    // 1. Try to find immediate wins
    if (try_find_winning_moves(game, moves, move_count, game->current_player)) {
        return;
    }
    
    // 2. Try to find blocking moves
    if (try_find_blocking_moves(game, moves, move_count)) {
        // Don't return immediately - evaluate all moves including blocks
    }
    
    // 3. Generate and evaluate strategic moves
    generate_and_evaluate_moves(game, moves, move_count, max_moves);
}