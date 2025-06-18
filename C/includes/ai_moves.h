#ifndef AI_MOVES_H
#define AI_MOVES_H

#include "ai.h"

// Move generation functions
void ai_generate_moves(const GomokuGame* game, Move* moves, int* move_count, int max_moves);

// Internal move generation functions
void find_winning_moves_smart(const GomokuGame* game, Move* moves, int* count, int player);
void find_neighbor_positions_smart(const GomokuGame* game, Move* moves, int* count, int max_moves);

#endif // AI_MOVES_H