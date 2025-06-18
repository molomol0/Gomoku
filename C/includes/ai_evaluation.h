#ifndef AI_EVALUATION_H
#define AI_EVALUATION_H

#include "ai.h"

// Evaluation functions
int ai_evaluate_position(const GomokuGame* game);
int ai_evaluate_position_for_player(GomokuGame* game, int row, int col, int player);

// Internal evaluation functions
int count_consecutive_optimized(const GomokuGame* game, int row, int col, int dx, int dy, int player);
int evaluate_line_optimized(const GomokuGame* game, int row, int col, int dx, int dy, int player);

#endif // AI_EVALUATION_H