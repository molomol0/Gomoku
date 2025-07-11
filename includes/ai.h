#ifndef AI_H
#define AI_H

#include "types.h"
#include "game.h"

/**
 * @brief Initialize AI engine
 */
void ai_init(void);

/**
 * @brief Cleanup AI resources
 */
void ai_cleanup(void);

/**
 * @brief Get best move for current player
 * @param game Game state
 * @param depth Search depth
 * @param stats Pointer to store AI statistics (can be NULL)
 * @return Best move found
 */
Move ai_get_best_move(const GomokuGame* game, int depth, AIStats* stats);

/**
 * @brief Evaluate position for a specific player
 * @param game Game state
 * @param row Row position
 * @param col Column position
 * @param player Player to evaluate for
 * @return Position score
 */
int ai_evaluate_position_for_player(GomokuGame* game, int row, int col, int player);

/**
 * @brief Generate candidate moves
 * @param game Game state
 * @param moves Array to store generated moves
 * @param move_count Pointer to store number of moves generated
 * @param max_moves Maximum number of moves to generate
 */
void ai_generate_moves(const GomokuGame* game, Move* moves, int* move_count, int max_moves);

/**
 * @brief Evaluate game position
 * @param game Game state
 * @return Position evaluation score
 */
int ai_evaluate_position(const GomokuGame* game);

#endif // AI_H