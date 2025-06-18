#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "../SDL2_ttf/SDL_ttf.h"

// Game configuration constants
#define BOARD_SIZE 19
#define CELL_SIZE 40
#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 800

// AI constants
#define AI_INFINITY 1000000000
#define MAX_TT_SIZE 500000
#define MAX_MOVES 15
#define MAX_VISUAL_MARKERS 1000

// Pattern scores (aggressive values)
#define PATTERN_WIN 100000
#define PATTERN_FOUR 50000
#define PATTERN_THREE 8000
#define PATTERN_TWO 800
#define PATTERN_ONE 80

// Players
#define EMPTY 0
#define BLACK 1
#define WHITE 2

#define M_PI 3.14159265358979323846

// Colors
extern const SDL_Color COLOR_BLACK;
extern const SDL_Color COLOR_WHITE;
extern const SDL_Color COLOR_BLUE;
extern const SDL_Color COLOR_RED;
extern const SDL_Color COLOR_BACKGROUND;

/**
 * @brief Direction structure for pattern detection
 */
typedef struct {
    int dx, dy;
} Direction;

/**
 * @brief Move structure
 */
typedef struct {
    int row, col;
    int score;
} Move;

/**
 * @brief Game state structure
 */
typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int current_player;
    int taken_stones[2];
    bool rule_center_opening;
    bool rule_no_double_threes;
    bool rule_captures;
} GomokuGame;

/**
 * @brief Transposition table entry
 */
typedef struct {
    uint64_t key;
    int depth;
    int score;
    Move move;
} TTEntry;

/**
 * @brief Visual marker for AI visualization
 */
typedef struct {
    int row, col;
    SDL_Color color;
} VisualMarker;

/**
 * @brief AI statistics
 */
typedef struct {
    int nodes_searched;
    int cache_hits;
    int pruned;
    double time_taken;
} AIStats;

#endif // TYPES_H