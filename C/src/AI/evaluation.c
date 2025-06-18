#include "ai_evaluation.h"
#include <string.h>

// Static constants
static const Direction directions[4] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

// Pattern recognition structures
typedef struct {
    int pattern[7];  // -1 = opponent, 0 = empty, 1 = our stone, 2 = any/don't care
    int score;
    const char* name;
} Pattern;

// Strategic patterns to recognize
static const Pattern strategic_patterns[] = {
    {{0, 1, 1, 1, 0, 2, 2}, 20000, "Open three"},
    {{2, 1, 1, 0, 1, 0, 2}, 15000, "Split three"},
    {{2, 1, 0, 1, 1, 0, 2}, 12000, "Jump three"},
    {{0, 1, 1, 0, 1, 0, 2}, 18000, "Double split"},
    {{2, 1, 1, 1, 1, 0, 2}, 50000, "Open four"},
    {{2, 0, 1, 1, 1, 0, 2}, 25000, "Open four threat"},
    {{0, 0, 0, 0, 0, 0, 0}, 0, NULL}  // Sentinel
};

int count_consecutive_optimized(const GomokuGame* game, int row, int col, int dx, int dy, int player) {
    if (game->board[row][col] != player) {
        return 0;
    }
    
    int count = 1;
    
    // Positive direction
    int r = row + dx, c = col + dy;
    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && game->board[r][c] == player) {
        count++;
        r += dx;
        c += dy;
    }
    
    // Negative direction
    r = row - dx;
    c = col - dy;
    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && game->board[r][c] == player) {
        count++;
        r -= dx;
        c -= dy;
    }
    
    return count;
}

// NEW: Check if a pattern matches at a position
static bool matches_pattern(const GomokuGame* game, int row, int col, int dx, int dy, 
                          const Pattern* pattern, int player) {
    int opponent = (player == BLACK) ? WHITE : BLACK;
    
    // Try to match pattern starting from different positions
    for (int start = -6; start <= 0; start++) {
        bool match = true;
        
        for (int i = 0; i < 7 && pattern->pattern[i] != 0 || i == 0; i++) {
            int r = row + (start + i) * dx;
            int c = col + (start + i) * dy;
            
            // Check bounds
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) {
                match = false;
                break;
            }
            
            // Check pattern match
            int board_val = game->board[r][c];
            int pattern_val = pattern->pattern[i];
            
            if (pattern_val == 2) continue; // Don't care
            if (pattern_val == 0 && board_val != EMPTY) { match = false; break; }
            if (pattern_val == 1 && board_val != player) { match = false; break; }
            if (pattern_val == -1 && board_val != opponent) { match = false; break; }
        }
        
        if (match) return true;
    }
    
    return false;
}

// NEW: Evaluate patterns for strategic play
static int evaluate_patterns(const GomokuGame* game, int row, int col, int player) {
    int pattern_score = 0;
    
    for (int d = 0; d < 4; d++) {
        int dx = directions[d].dx;
        int dy = directions[d].dy;
        
        // Check each strategic pattern
        for (int p = 0; strategic_patterns[p].name != NULL; p++) {
            if (matches_pattern(game, row, col, dx, dy, &strategic_patterns[p], player)) {
                pattern_score += strategic_patterns[p].score;
            }
        }
    }
    
    return pattern_score;
}

static int calculate_line_score(int total_count, int open_ends) {
    int base_score;
    if (total_count >= 5) base_score = PATTERN_WIN;
    else if (total_count == 4) base_score = PATTERN_FOUR;
    else if (total_count == 3) base_score = PATTERN_THREE;
    else if (total_count == 2) base_score = PATTERN_TWO;
    else base_score = PATTERN_ONE;
    
    // More aggressive multipliers for open positions
    double multiplier = 1.0;
    if (open_ends == 2) multiplier = 4.0;  // Increased from 3.0
    else if (open_ends == 1) multiplier = 2.0;  // Increased from 1.5
    else multiplier = 0.5;  // Increased from 0.3
    
    return (int)(base_score * multiplier);
}

static int count_stones_in_direction(const GomokuGame* game, int row, int col, int dx, int dy, int player) {
    int count = 0;
    int r = row + dx, c = col + dy;
    
    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && game->board[r][c] == player) {
        count++;
        r += dx;
        c += dy;
    }
    
    return count;
}

static bool is_position_open(const GomokuGame* game, int row, int col) {
    return (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && 
            game->board[row][col] == EMPTY);
}

// NEW: Check for potential threats that could be created
static int evaluate_potential_threats(const GomokuGame* game, int row, int col, int player) {
    int potential_score = 0;
    GomokuGame temp_game;
    memcpy(&temp_game, game, sizeof(GomokuGame));
    temp_game.board[row][col] = player;
    
    // Look for positions that would become immediate threats
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (temp_game.board[i][j] == EMPTY) {
                // Check if placing here would create a win
                temp_game.board[i][j] = player;
                
                for (int d = 0; d < 4; d++) {
                    int count = count_consecutive_optimized(&temp_game, i, j, 
                                                          directions[d].dx, 
                                                          directions[d].dy, player);
                    if (count >= 4) {
                        potential_score += 5000; // This move creates a threat
                    }
                }
                
                temp_game.board[i][j] = EMPTY;
            }
        }
    }
    
    return potential_score;
}

int evaluate_line_optimized(const GomokuGame* game, int row, int col, int dx, int dy, int player) {
    if (game->board[row][col] != player) {
        return 0;
    }
    
    // Count consecutive stones in both directions
    int pos_count = count_stones_in_direction(game, row, col, dx, dy, player);
    int neg_count = count_stones_in_direction(game, row, col, -dx, -dy, player);
    
    int total_count = pos_count + neg_count + 1;
    
    if (total_count < 2) return 0;
    
    // Check open ends
    int open_ends = 0;
    
    // Check positive end
    int end_r = row + dx * (pos_count + 1);
    int end_c = col + dy * (pos_count + 1);
    if (is_position_open(game, end_r, end_c)) {
        open_ends++;
    }
    
    // Check negative end
    end_r = row - dx * (neg_count + 1);
    end_c = col - dy * (neg_count + 1);
    if (is_position_open(game, end_r, end_c)) {
        open_ends++;
    }
    
    return calculate_line_score(total_count, open_ends);
}

static int evaluate_threats_for_position(GomokuGame* game, int row, int col, int player) {
    int max_score = 0;
    int threat_count = 0;
    int strong_threat_count = 0;
    
    for (int d = 0; d < 4; d++) {
        int score = evaluate_line_optimized(game, row, col, directions[d].dx, directions[d].dy, player);
        if (score >= PATTERN_WIN) {
            return AI_INFINITY;
        }
        if (score >= PATTERN_FOUR) {
            strong_threat_count++;
        }
        if (score >= PATTERN_THREE) {
            threat_count++;
        }
        max_score = (score > max_score) ? score : max_score;
    }
    
    // Enhanced threat bonus logic
    if (strong_threat_count >= 2) max_score += 50000; // Unstoppable
    else if (strong_threat_count == 1) max_score += 20000;
    else if (threat_count >= 2) max_score += 30000;
    else if (threat_count == 1) max_score += 5000;
    
    return max_score;
}

int ai_evaluate_position_for_player(GomokuGame* game, int row, int col, int player) {
    if (game->board[row][col] != EMPTY) return 0;
    
    game->board[row][col] = player;
    
    // Base threat evaluation
    int score = evaluate_threats_for_position(game, row, col, player);
    
    // Add pattern recognition bonus
    int pattern_bonus = evaluate_patterns(game, row, col, player);
    score += pattern_bonus;
    
    // Add potential threat creation bonus
    int potential_bonus = evaluate_potential_threats(game, row, col, player);
    score += potential_bonus;
    
    game->board[row][col] = EMPTY;
    
    return score;
}

// NEW: Evaluate the overall board position more aggressively
static int evaluate_board_control(const GomokuGame* game, int player) {
    int control_score = 0;
    
    // Evaluate center control
    int center = BOARD_SIZE / 2;
    for (int i = center - 2; i <= center + 2; i++) {
        for (int j = center - 2; j <= center + 2; j++) {
            if (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE) {
                if (game->board[i][j] == player) {
                    control_score += 100 * (3 - abs(i - center)) * (3 - abs(j - center));
                }
            }
        }
    }
    
    return control_score;
}

static int evaluate_player_position(const GomokuGame* game) {
    int score = 0;
    
    // Evaluate all positions
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game->board[i][j] != EMPTY) {
                int player = game->board[i][j];
                int multiplier = (player == BLACK) ? 1 : -1;
                
                int position_value = 0;
                for (int d = 0; d < 4; d++) {
                    int line_score = evaluate_line_optimized(game, i, j, directions[d].dx, directions[d].dy, player);
                    position_value = (line_score > position_value) ? line_score : position_value;
                }
                
                // Add pattern evaluation
                position_value += evaluate_patterns(game, i, j, player);
                
                score += multiplier * position_value;
            }
        }
    }
    
    // Add board control evaluation
    score += evaluate_board_control(game, BLACK) - evaluate_board_control(game, WHITE);
    
    return score;
}

int ai_evaluate_position(const GomokuGame* game) {
    // Check for terminal states
    int winner;
    if (game_check_winner(game, &winner)) {
        if (winner == BLACK) return (game->current_player == BLACK) ? -AI_INFINITY : AI_INFINITY;
        if (winner == WHITE) return (game->current_player == WHITE) ? -AI_INFINITY : AI_INFINITY;
    }
    
    int score = evaluate_player_position(game);
    
    // Add capture bonus with higher weight
    score += (game->taken_stones[0] - game->taken_stones[1]) * 3000; // Increased from 2000
    
    return score;
}