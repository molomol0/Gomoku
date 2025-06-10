#include "ai.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Static variables
static TTEntry* transposition_table = NULL;
static AIStats last_stats = {0};
static const Direction directions[4] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

// Internal function declarations
static int minimax_balanced(GomokuGame* game, int depth, int alpha, int beta, bool maximizing, Move* best_move);
static inline int count_consecutive_optimized(const GomokuGame* game, int row, int col, int dx, int dy, int player);
static inline int evaluate_line_optimized(const GomokuGame* game, int row, int col, int dx, int dy, int player);
static void find_winning_moves_smart(const GomokuGame* game, Move* moves, int* count, int player);
static void find_neighbor_positions_smart(const GomokuGame* game, Move* moves, int* count, int max_moves);
static uint64_t game_hash_optimized(const GomokuGame* game);

void ai_init(void) {
    transposition_table = calloc(MAX_TT_SIZE, sizeof(TTEntry));
    if (!transposition_table) {
        fprintf(stderr, "Failed to allocate transposition table!\n");
        exit(1);
    }
    memset(&last_stats, 0, sizeof(AIStats));
}

void ai_cleanup(void) {
    if (transposition_table) {
        free(transposition_table);
        transposition_table = NULL;
    }
}

void ai_clear_transposition_table(void) {
    if (transposition_table) {
        memset(transposition_table, 0, MAX_TT_SIZE * sizeof(TTEntry));
    }
}

static uint64_t game_hash_optimized(const GomokuGame* game) {
    uint64_t hash = 0;
    
    // Hash only occupied positions efficiently
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game->board[i][j] != EMPTY) {
                hash ^= ((uint64_t)(i + j * BOARD_SIZE + game->board[i][j]) << ((i + j) & 63));
            }
        }
    }
    
    return hash ^ game->current_player ^ 
           ((uint64_t)game->taken_stones[0] << 8) ^ 
           ((uint64_t)game->taken_stones[1] << 16);
}

static inline int count_consecutive_optimized(const GomokuGame* game, int row, int col, int dx, int dy, int player) {
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

static inline int evaluate_line_optimized(const GomokuGame* game, int row, int col, int dx, int dy, int player) {
    int pos_count, neg_count;
    
    // Count consecutive stones in both directions
    if (game->board[row][col] != player) {
        return 0;
    }
    
    pos_count = 0;
    neg_count = 0;
    
    // Positive direction
    int r = row + dx, c = col + dy;
    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && game->board[r][c] == player) {
        pos_count++;
        r += dx;
        c += dy;
    }
    
    // Negative direction
    r = row - dx;
    c = col - dy;
    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && game->board[r][c] == player) {
        neg_count++;
        r -= dx;
        c -= dy;
    }
    
    int total_count = pos_count + neg_count + 1;
    
    if (total_count < 2) return 0;
    
    int open_ends = 0;
    
    // Check positive end
    int end_r = row + dx * (pos_count + 1);
    int end_c = col + dy * (pos_count + 1);
    if (end_r >= 0 && end_r < BOARD_SIZE && end_c >= 0 && end_c < BOARD_SIZE && 
        game->board[end_r][end_c] == EMPTY) {
        open_ends++;
    }
    
    // Check negative end
    end_r = row - dx * (neg_count + 1);
    end_c = col - dy * (neg_count + 1);
    if (end_r >= 0 && end_r < BOARD_SIZE && end_c >= 0 && end_c < BOARD_SIZE && 
        game->board[end_r][end_c] == EMPTY) {
        open_ends++;
    }
    
    int base_score;
    if (total_count >= 5) base_score = PATTERN_WIN;
    else if (total_count == 4) base_score = PATTERN_FOUR;
    else if (total_count == 3) base_score = PATTERN_THREE;
    else if (total_count == 2) base_score = PATTERN_TWO;
    else base_score = PATTERN_ONE;
    
    double multiplier = 1.0;
    if (open_ends == 2) multiplier = 3.0;
    else if (open_ends == 1) multiplier = 1.5;
    else multiplier = 0.3;
    
    return (int)(base_score * multiplier);
}

int ai_evaluate_position_for_player(GomokuGame* game, int row, int col, int player) {
    if (game->board[row][col] != EMPTY) return 0;
    
    game->board[row][col] = player;
    
    int max_score = 0;
    int threat_count = 0;
    
    for (int d = 0; d < 4; d++) {
        int score = evaluate_line_optimized(game, row, col, directions[d].dx, directions[d].dy, player);
        if (score >= PATTERN_WIN) {
            game->board[row][col] = EMPTY;
            return AI_INFINITY;
        }
        if (score >= PATTERN_THREE) {
            threat_count++;
        }
        max_score = (score > max_score) ? score : max_score;
    }
    
    // ORIGINAL threat bonus logic
    if (threat_count >= 2) max_score += 30000;
    else if (threat_count == 1) max_score += 5000;
    
    game->board[row][col] = EMPTY;
    return max_score;
}

static void find_winning_moves_smart(const GomokuGame* game, Move* moves, int* count, int player) {
    *count = 0;
    
    // Check all empty positions
    for (int i = 0; i < BOARD_SIZE && *count < 5; i++) {
        for (int j = 0; j < BOARD_SIZE && *count < 5; j++) {
            if (game->board[i][j] == EMPTY) {
                // Quick proximity check - only positions within 2 of existing stones
                bool near_stone = false;
                for (int di = -2; di <= 2 && !near_stone; di++) {
                    for (int dj = -2; dj <= 2 && !near_stone; dj++) {
                        int ni = i + di, nj = j + dj;
                        if (ni >= 0 && ni < BOARD_SIZE && nj >= 0 && nj < BOARD_SIZE &&
                            game->board[ni][nj] != EMPTY) {
                            near_stone = true;
                        }
                    }
                }
                
                if (near_stone) {
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

static void find_neighbor_positions_smart(const GomokuGame* game, Move* moves, int* count, int max_moves) {
    *count = 0;
    bool visited[BOARD_SIZE][BOARD_SIZE] = {false};
    
    // Use radius 3 for better strategic play (was too restrictive at radius 1-2)
    for (int i = 0; i < BOARD_SIZE && *count < max_moves; i++) {
        for (int j = 0; j < BOARD_SIZE && *count < max_moves; j++) {
            if (game->board[i][j] != EMPTY) {
                // Add neighbors within distance 3 for better strategic coverage
                for (int di = -3; di <= 3; di++) {
                    for (int dj = -3; dj <= 3; dj++) {
                        if (di == 0 && dj == 0) continue;
                        int ni = i + di, nj = j + dj;
                        if (ni >= 0 && ni < BOARD_SIZE && nj >= 0 && nj < BOARD_SIZE &&
                            game->board[ni][nj] == EMPTY && !visited[ni][nj]) {
                            
                            moves[*count].row = ni;
                            moves[*count].col = nj;
                            moves[*count].score = 0;
                            (*count)++;
                            visited[ni][nj] = true;
                        }
                    }
                }
            }
        }
    }
}

void ai_generate_moves(const GomokuGame* game, Move* moves, int* move_count, int max_moves) {
    *move_count = 0;
    
    // Check for empty board
    bool empty_board = true;
    for (int i = 0; i < BOARD_SIZE && empty_board; i++) {
        for (int j = 0; j < BOARD_SIZE && empty_board; j++) {
            if (game->board[i][j] != EMPTY) {
                empty_board = false;
            }
        }
    }
    
    if (empty_board) {
        moves[0].row = BOARD_SIZE / 2;
        moves[0].col = BOARD_SIZE / 2;
        moves[0].score = 0;
        *move_count = 1;
        return;
    }
    
    // 1. Find immediate wins for current player
    find_winning_moves_smart(game, moves, move_count, game->current_player);
    if (*move_count > 0) return;
    
    // 2. Find opponent winning moves to block
    int opponent = (game->current_player == BLACK) ? WHITE : BLACK;
    find_winning_moves_smart(game, moves, move_count, opponent);
    if (*move_count > 0) return;
    
    // 3. Generate neighbor positions and evaluate them
    find_neighbor_positions_smart(game, moves, move_count, max_moves);
    
    // Evaluate all positions
    for (int i = 0; i < *move_count; i++) {
        GomokuGame temp_game;
        memcpy(&temp_game, game, sizeof(GomokuGame));
        
        int our_score = ai_evaluate_position_for_player(&temp_game, moves[i].row, moves[i].col, game->current_player);
        int opp_score = ai_evaluate_position_for_player(&temp_game, moves[i].row, moves[i].col, opponent);
        
        moves[i].score = (int)(our_score + opp_score);
    }
    
    // Sort moves by score (descending)
    for (int i = 0; i < *move_count - 1; i++) {
        for (int j = i + 1; j < *move_count; j++) {
            if (moves[j].score > moves[i].score) {
                Move temp = moves[i];
                moves[i] = moves[j];
                moves[j] = temp;
            }
        }
    }
    // print the 5 best moves
    // printf("Best moves found:\n");
    // for (int i = 0; i < *move_count && i < 5; i++) {
    //     printf("Move (%d, %d) with score %d\n", moves[i].row, moves[i].col, moves[i].score);
    // }
    
    // Limit to best moves
    if (*move_count > max_moves) {
        *move_count = max_moves;
    }
}

int ai_evaluate_position(const GomokuGame* game) {
    int score = 0;
    
    // Check for terminal states
    int winner;
    if (game_check_winner(game, &winner)) {
        if (winner == BLACK) return (game->current_player == BLACK) ? -AI_INFINITY : AI_INFINITY;
        if (winner == WHITE) return (game->current_player == WHITE) ? -AI_INFINITY : AI_INFINITY;
    }
    
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
                
                score += multiplier * position_value;
            }
        }
    }
    
    // Add capture bonus
    score += (game->taken_stones[0] - game->taken_stones[1]) * 2000;
    
    return score;
}

static int minimax_balanced(GomokuGame* game, int depth, int alpha, int beta, bool maximizing, Move* best_move) {
    last_stats.nodes_searched++;
    
    // Use optimized hash
    uint64_t state_key = game_hash_optimized(game);
    int tt_index = state_key % MAX_TT_SIZE;
    
    // Transposition table lookup
    if (transposition_table[tt_index].key == state_key && 
        transposition_table[tt_index].depth >= depth) {
        last_stats.cache_hits++;
        if (best_move) {
            *best_move = transposition_table[tt_index].move;
        }
        return transposition_table[tt_index].score;
    }
    
    // Terminal check
    int winner;
    if (depth == 0 || game_check_winner(game, &winner)) {
        int score = ai_evaluate_position(game);
        
        // Store in transposition table
        transposition_table[tt_index].key = state_key;
        transposition_table[tt_index].depth = depth;
        transposition_table[tt_index].score = score;
        transposition_table[tt_index].move = (Move){-1, -1, 0};
        
        return score;
    }
    
    // Generate moves - DON'T reduce move count for strength
    Move moves[MAX_MOVES * 4];
    int move_count;
    ai_generate_moves(game, moves, &move_count, MAX_MOVES);
    
    if (move_count == 0) {
        return ai_evaluate_position(game);
    }
    
    Move local_best_move = moves[0];
    
    if (maximizing) {
        int max_eval = -AI_INFINITY;
        
        for (int i = 0; i < move_count; i++) {
            GomokuGame new_game;
            memcpy(&new_game, game, sizeof(GomokuGame));
            
            if (game_place_stone(&new_game, moves[i].row, moves[i].col)) {
                Move dummy_move;
                int eval = minimax_balanced(&new_game, depth - 1, alpha, beta, false, &dummy_move);
                
                if (eval > max_eval) {
                    max_eval = eval;
                    local_best_move = moves[i];
                }
                
                alpha = (eval > alpha) ? eval : alpha;
                if (beta <= alpha) {
                    last_stats.pruned += move_count - i - 1;
                    break;
                }
            }
        }
        
        // Store in transposition table
        transposition_table[tt_index].key = state_key;
        transposition_table[tt_index].depth = depth;
        transposition_table[tt_index].score = max_eval;
        transposition_table[tt_index].move = local_best_move;
        
        if (best_move) {
            *best_move = local_best_move;
        }
        return max_eval;
    } else {
        int min_eval = AI_INFINITY;
        
        for (int i = 0; i < move_count; i++) {
            GomokuGame new_game;
            memcpy(&new_game, game, sizeof(GomokuGame));
            
            if (game_place_stone(&new_game, moves[i].row, moves[i].col)) {
                Move dummy_move;
                int eval = minimax_balanced(&new_game, depth - 1, alpha, beta, true, &dummy_move);
                
                if (eval < min_eval) {
                    min_eval = eval;
                    local_best_move = moves[i];
                }
                
                beta = (eval < beta) ? eval : beta;
                if (beta <= alpha) {
                    last_stats.pruned += move_count - i - 1;
                    break;
                }
            }
        }
        
        // Store in transposition table
        transposition_table[tt_index].key = state_key;
        transposition_table[tt_index].depth = depth;
        transposition_table[tt_index].score = min_eval;
        transposition_table[tt_index].move = local_best_move;
        
        if (best_move) {
            *best_move = local_best_move;
        }
        return min_eval;
    }
}

Move ai_get_best_move(const GomokuGame* game, int depth, AIStats* stats) {
    printf("========================================================\n");
    printf("========================================================\n");
    printf("==                                                    ==\n");
    printf("==                  AI getting Move                   ==\n");
    printf("==                                                    ==\n");
    printf("========================================================\n");
    printf("========================================================\n\n");
    // Reset statistics
    last_stats.nodes_searched = 0;
    last_stats.cache_hits = 0;
    last_stats.pruned = 0;
    
    clock_t start = clock();
    
    // Quick win check
    Move moves[MAX_MOVES * 4];
    int move_count;
    ai_generate_moves(game, moves, &move_count, MAX_MOVES);
    
    for (int i = 0; i < move_count; i++) {
        GomokuGame temp_game;
        memcpy(&temp_game, game, sizeof(GomokuGame));
        int score = ai_evaluate_position_for_player(&temp_game, moves[i].row, moves[i].col, game->current_player);
        if (score >= AI_INFINITY) {
            printf("Immediate win found: (%d, %d)\n", moves[i].row, moves[i].col);
            last_stats.time_taken = ((double)(clock() - start)) / CLOCKS_PER_SEC;
            if (stats) *stats = last_stats;
            return moves[i];
        }
    }
    
    // Quick defense check
    int opponent = (game->current_player == BLACK) ? WHITE : BLACK;
    for (int i = 0; i < move_count; i++) {
        GomokuGame temp_game;
        memcpy(&temp_game, game, sizeof(GomokuGame));
        int score = ai_evaluate_position_for_player(&temp_game, moves[i].row, moves[i].col, opponent);
        if (score >= AI_INFINITY) {
            printf("Blocking opponent win: (%d, %d)\n", moves[i].row, moves[i].col);
            last_stats.time_taken = ((double)(clock() - start)) / CLOCKS_PER_SEC;
            if (stats) *stats = last_stats;
            return moves[i];
        }
    }
    
    GomokuGame temp_game;
    memcpy(&temp_game, game, sizeof(GomokuGame));
    
    Move best_move;
    int score = minimax_balanced(&temp_game, depth, -AI_INFINITY, AI_INFINITY, true, &best_move);
    
    clock_t end = clock();
    last_stats.time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    // Safety check
    if (best_move.row < 0 || best_move.col < 0) {
        printf("WARNING: No move found, using fallback\n");
        if (move_count > 0) {
            best_move = moves[0];
        } else {
            best_move.row = BOARD_SIZE / 2;
            best_move.col = BOARD_SIZE / 2;
            best_move.score = 0;
        }
    }
    
    printf("AI found move (%d, %d) with score %d in %.2f seconds\n", 
           best_move.row, best_move.col, score, last_stats.time_taken);
    printf("Nodes searched: %d, Cache hits: %d, Pruned: %d\n", 
           last_stats.nodes_searched, last_stats.cache_hits, last_stats.pruned);
    
    if (stats) *stats = last_stats;
    return best_move;
}

AIStats ai_get_last_stats(void) {
    return last_stats;
}