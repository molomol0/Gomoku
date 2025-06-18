#include "ai_minmax.h"
#include "ai_evaluation.h"
#include "ai_moves.h"
#include "ai_core.h"
#include <string.h>

// External function declarations (from ai_core.c)
extern TTEntry* ai_get_transposition_table(void);
extern AIStats* ai_get_stats_ref(void);

uint64_t game_hash_optimized(const GomokuGame* game) {
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

static int handle_terminal_state(GomokuGame* game, int depth, uint64_t state_key) {
    TTEntry* transposition_table = ai_get_transposition_table();
    int score = ai_evaluate_position(game);
    
    // Store in transposition table
    int tt_index = state_key % MAX_TT_SIZE;
    transposition_table[tt_index].key = state_key;
    transposition_table[tt_index].depth = depth;
    transposition_table[tt_index].score = score;
    transposition_table[tt_index].move = (Move){-1, -1, 0};
    
    return score;
}

static bool check_transposition_table(uint64_t state_key, int depth, Move* best_move, int* score) {
    TTEntry* transposition_table = ai_get_transposition_table();
    AIStats* stats = ai_get_stats_ref();
    
    int tt_index = state_key % MAX_TT_SIZE;
    
    if (transposition_table[tt_index].key == state_key && 
        transposition_table[tt_index].depth >= depth) {
        stats->cache_hits++;
        if (best_move) {
            *best_move = transposition_table[tt_index].move;
        }
        *score = transposition_table[tt_index].score;
        return true;
    }
    return false;
}

static void store_in_transposition_table(uint64_t state_key, int depth, int score, Move move) {
    TTEntry* transposition_table = ai_get_transposition_table();
    int tt_index = state_key % MAX_TT_SIZE;
    
    transposition_table[tt_index].key = state_key;
    transposition_table[tt_index].depth = depth;
    transposition_table[tt_index].score = score;
    transposition_table[tt_index].move = move;
}

static int minimax_maximizing_player(GomokuGame* game, int depth, int alpha, int beta, 
                                   Move* moves, int move_count, Move* best_move, 
                                   uint64_t state_key) {
    AIStats* stats = ai_get_stats_ref();
    int max_eval = -AI_INFINITY;
    Move local_best_move = moves[0];
    
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
                stats->pruned += move_count - i - 1;
                break;
            }
        }
    }
    
    store_in_transposition_table(state_key, depth, max_eval, local_best_move);
    
    if (best_move) {
        *best_move = local_best_move;
    }
    return max_eval;
}

static int minimax_minimizing_player(GomokuGame* game, int depth, int alpha, int beta, 
                                   Move* moves, int move_count, Move* best_move, 
                                   uint64_t state_key) {
    AIStats* stats = ai_get_stats_ref();
    int min_eval = AI_INFINITY;
    Move local_best_move = moves[0];
    
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
                stats->pruned += move_count - i - 1;
                break;
            }
        }
    }
    
    store_in_transposition_table(state_key, depth, min_eval, local_best_move);
    
    if (best_move) {
        *best_move = local_best_move;
    }
    return min_eval;
}

int minimax_balanced(GomokuGame* game, int depth, int alpha, int beta, bool maximizing, Move* best_move) {
	printf("Minimax search at depth %d\n", depth);
    AIStats* stats = ai_get_stats_ref();
    stats->nodes_searched++;
    
    // Use optimized hash
    uint64_t state_key = game_hash_optimized(game);
    
    // Transposition table lookup
    int cached_score;
    if (check_transposition_table(state_key, depth, best_move, &cached_score)) {
        return cached_score;
    }
    
    // Terminal check
    int winner;
    if (depth == 0 || game_check_winner(game, &winner)) {
        return handle_terminal_state(game, depth, state_key);
    }
    
    // Generate moves - DON'T reduce move count for strength
    Move moves[MAX_MOVES * 4];
    int move_count;
    ai_generate_moves(game, moves, &move_count, MAX_MOVES);
    
    if (move_count == 0) {
        return ai_evaluate_position(game);
    }
    
    if (maximizing) {
        return minimax_maximizing_player(game, depth, alpha, beta, moves, move_count, best_move, state_key);
    } else {
        return minimax_minimizing_player(game, depth, alpha, beta, moves, move_count, best_move, state_key);
    }
}