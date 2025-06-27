#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"
#include "game.h"

extern SDL_Rect button_rects[4]; // Rule buttons

/**
 * @brief Initialize graphics system
 * @return true if successful, false otherwise
 */
bool graphics_init(void);

/**
 * @brief Cleanup graphics resources
 */
void graphics_cleanup(void);

/**
 * @brief Draw the complete game state
 * @param game Game state to draw
 */
void graphics_draw_game(const GomokuGame* game);

/**
 * @brief Handle mouse click events
 * @param x Mouse x coordinate
 * @param y Mouse y coordinate
 * @param row Pointer to store board row
 * @param col Pointer to store board column
 * @return true if click is on valid board position
 */
bool graphics_handle_click(int x, int y, int* row, int* col);

/**
 * @brief Add visual marker for AI visualization
 * @param row Board row
 * @param col Board column
 * @param color Marker color
 */
void graphics_add_visual_marker(int row, int col, SDL_Color color);

/**
 * @brief Clear all visual markers
 */
void graphics_clear_visual_markers(void);

/**
 * @brief Remove visual marker at specific position
 * @param row Board row
 * @param col Board column
 */
void graphics_remove_visual_marker(int row, int col);

/**
 * @brief Show winner message
 * @param winner Winner player (BLACK, WHITE, or EMPTY for draw)
 */
void graphics_show_winner(int winner);

/**
 * @brief Get SDL renderer (for custom drawing)
 * @return SDL renderer pointer
 */
SDL_Renderer* graphics_get_renderer(void);

/**
 * @brief Get SDL window (for event handling)
 * @return SDL window pointer
 */
SDL_Window* graphics_get_window(void);

#endif // GRAPHICS_H