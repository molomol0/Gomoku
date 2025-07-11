#ifndef GAME_H
#define GAME_H

#include "types.h"

/**
 * @brief Initialize game state
 * @param game Pointer to game structure
 */
void game_init(GomokuGame* game);

/**
 * @brief Copy game state
 * @param dest Destination game structure
 * @param src Source game structure
 */
void game_copy(GomokuGame* dest, const GomokuGame* src);

/**
 * @brief Place a stone on the board
 * @param game Game state
 * @param row Row position
 * @param col Column position
 * @return true if move was successful, false otherwise
 */
bool game_place_stone(GomokuGame* game, int row, int col);

/**
 * @brief Check if the game has a winner
 * @param game Game state
 * @param winner Pointer to store winner (if any)
 * @return true if game is over, false otherwise
 */
bool game_check_winner(const GomokuGame* game, int* winner);

/**
 * @brief Check if position is valid for placement
 * @param game Game state
 * @param row Row position
 * @param col Column position
 * @return true if position is valid, false otherwise
 */
bool game_is_valid_position(const GomokuGame* game, int row, int col);

/**
 * @brief Check if move creates double free three
 * @param game Game state
 * @param row Row position
 * @param col Column position
 * @param player Player making the move
 * @return true if creates double free three, false otherwise
 */
bool game_is_double_free_three(GomokuGame* game, int row, int col, int player);

/**
 * @brief Capture stones around a placed stone
 * @param game Game state
 * @param row Row of placed stone
 * @param col Column of placed stone
 * @return Number of stones captured
 */
int game_capture_stones(GomokuGame* game, int row, int col);

/**
 * @brief Get hash value for game state
 * @param game Game state
 * @return Hash value
 */
uint64_t game_hash(const GomokuGame* game);

/**
 * @brief Print board state to console
 * @param game Game state
 */
void game_print_board(const GomokuGame* game);

#endif // GAME_H