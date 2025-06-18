#ifndef AI_MINMAX_H
#define AI_MINMAX_H

#include "ai.h"

// Minimax algorithm functions
int minimax_balanced(GomokuGame* game, int depth, int alpha, int beta, bool maximizing, Move* best_move);

// Hash and transposition table functions
uint64_t game_hash_optimized(const GomokuGame* game);

#endif // AI_MINMAX_H