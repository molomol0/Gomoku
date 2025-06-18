#include "ai_core.h"
#include "ai_evaluation.h"
#include "ai_moves.h"
#include "ai_minmax.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Static variables
static TTEntry* transposition_table = NULL;
static AIStats last_stats = {0};

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

// Getter for transposition table (for minimax module)
TTEntry* ai_get_transposition_table(void) {
    return transposition_table;
}

// Getter and setter for stats (for minimax module)
AIStats* ai_get_stats_ref(void) {
    return &last_stats;
}

static void ai_print_search_header(void) {
    printf("========================================================\n");
    printf("========================================================\n");
    printf("==                                                    ==\n");
    printf("==                  AI getting Move                   ==\n");
    printf("==                                                    ==\n");
    printf("========================================================\n");
    printf("========================================================\n\n");
}

static void ai_reset_statistics(void) {
    last_stats.nodes_searched = 0;
    last_stats.cache_hits = 0;
    last_stats.pruned = 0;
}

static bool ai_check_immediate_win(const GomokuGame* game, Move* best_move, clock_t start) {
    // Generate moves specifically for win checking
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
            *best_move = moves[i];
            return true;
        }
    }
    return false;
}

static bool ai_must_block_immediate_loss(const GomokuGame* game, Move* best_move, clock_t start) {
    int opponent = (game->current_player == BLACK) ? WHITE : BLACK;
    
    // Generate moves specifically for defense checking  
    Move moves[MAX_MOVES * 4];
    int move_count;
    ai_generate_moves(game, moves, &move_count, MAX_MOVES);
    
    // Count how many winning moves opponent has
    int opponent_wins = 0;
    Move blocking_move = {-1, -1, 0};
    
    for (int i = 0; i < move_count; i++) {
        GomokuGame temp_game;
        memcpy(&temp_game, game, sizeof(GomokuGame));
        int score = ai_evaluate_position_for_player(&temp_game, moves[i].row, moves[i].col, opponent);
        if (score >= AI_INFINITY) {
            opponent_wins++;
            blocking_move = moves[i];
        }
    }
    
    // If opponent has multiple winning moves, we're lost anyway
    // If opponent has exactly one winning move, we MUST block it
    if (opponent_wins == 1) {
        printf("Must block opponent win: (%d, %d)\n", blocking_move.row, blocking_move.col);
        last_stats.time_taken = ((double)(clock() - start)) / CLOCKS_PER_SEC;
        *best_move = blocking_move;
        return true;
    }
    
    return false;
}

// Fixed fork detection - only count actual strong threats
static int count_strong_threats_created(const GomokuGame* game, int row, int col, int player) {
    GomokuGame temp_game;
    memcpy(&temp_game, game, sizeof(GomokuGame));
    temp_game.board[row][col] = player;
    
    int threat_count = 0;
    
    // Only check nearby positions for efficiency
    for (int di = -4; di <= 4; di++) {
        for (int dj = -4; dj <= 4; dj++) {
            int r = row + di;
            int c = col + dj;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && 
                temp_game.board[r][c] == EMPTY) {
                
                // Place a stone and check if it would win
                temp_game.board[r][c] = player;
                
                // Check all directions from this position
                const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
                for (int d = 0; d < 4; d++) {
                    int count = count_consecutive_optimized(&temp_game, r, c, 
                                                          dirs[d][0], dirs[d][1], player);
                    if (count >= 4) {
                        threat_count++;
                        break; // Only count each position once
                    }
                }
                
                temp_game.board[r][c] = EMPTY;
            }
        }
    }
    
    return threat_count;
}

// Check for moves that create multiple threats
static bool ai_check_offensive_opportunity(const GomokuGame* game, Move* best_move, clock_t start) {
    Move moves[MAX_MOVES * 4];
    int move_count;
    ai_generate_moves(game, moves, &move_count, MAX_MOVES * 2); // Get more moves for offense
    
    int best_threats = 0;
    Move offensive_move = {-1, -1, 0};
    
    // Evaluate each move for threat creation
    for (int i = 0; i < move_count && i < 30; i++) { // Limit to top 30 moves
        int threats = count_strong_threats_created(game, moves[i].row, moves[i].col, game->current_player);
        
        if (threats > best_threats) {
            best_threats = threats;
            offensive_move = moves[i];
        }
    }
    
    // Only report as fork if we create 2+ real threats
    if (best_threats >= 2) {
        printf("Strong offensive move found: (%d, %d) creating %d threats\n", 
               offensive_move.row, offensive_move.col, best_threats);
        *best_move = offensive_move;
        last_stats.time_taken = ((double)(clock() - start)) / CLOCKS_PER_SEC;
        return true;
    }
    
    return false;
}

static Move ai_apply_fallback_move(Move* moves, int move_count) {
    Move fallback_move;
    
    if (move_count > 0) {
        fallback_move = moves[0];
    } else {
        fallback_move.row = BOARD_SIZE / 2;
        fallback_move.col = BOARD_SIZE / 2;
        fallback_move.score = 0;
    }
    
    printf("WARNING: No move found, using fallback\n");
    return fallback_move;
}

static void ai_print_search_results(Move best_move, int score) {
    printf("AI found move (%d, %d) with score %d in %.2f seconds\n", 
           best_move.row, best_move.col, score, last_stats.time_taken);
    printf("Nodes searched: %d, Cache hits: %d, Pruned: %d\n", 
           last_stats.nodes_searched, last_stats.cache_hits, last_stats.pruned);
}

static bool is_valid_move(Move move) {
    return move.row >= 0 && move.col >= 0;
}

Move ai_get_best_move(const GomokuGame* game, int depth, AIStats* stats) {
    // ai_print_search_header();
    ai_reset_statistics();
    
    clock_t start = clock();
    
    // 1. Check for immediate win
    Move best_move;
    if (ai_check_immediate_win(game, &best_move, start)) {
        if (stats) *stats = last_stats;
        return best_move;
    }
    
    // 2. Check if we MUST block (only if opponent has exactly one winning move)
    if (ai_must_block_immediate_loss(game, &best_move, start)) {
        if (stats) *stats = last_stats;
        return best_move;
    }
    
    // 3. Check for strong offensive moves (forks, strong threats)
    if (ai_check_offensive_opportunity(game, &best_move, start)) {
        if (stats) *stats = last_stats;
        return best_move;
    }
    
    // 4. Run full minimax search for best strategic move
    GomokuGame temp_game;
    memcpy(&temp_game, game, sizeof(GomokuGame));
    
    int score = minimax_balanced(&temp_game, depth, -AI_INFINITY, AI_INFINITY, true, &best_move);
    
    clock_t end = clock();
    last_stats.time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    // Apply fallback if needed
    if (!is_valid_move(best_move)) {
        Move moves[MAX_MOVES * 4];
        int move_count;
        ai_generate_moves(game, moves, &move_count, MAX_MOVES);
        best_move = ai_apply_fallback_move(moves, move_count);
    }
    
    ai_print_search_results(best_move, score);
    
    if (stats) *stats = last_stats;
    return best_move;
}

AIStats ai_get_last_stats(void) {
    return last_stats;
}