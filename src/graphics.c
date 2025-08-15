#include "graphics.h"
#include <stdio.h>
#include <math.h>

// Color definitions
const SDL_Color COLOR_BLACK = {0, 0, 0, 255};
const SDL_Color COLOR_WHITE = {255, 255, 255, 255};
const SDL_Color COLOR_BLUE = {0, 0, 255, 255};
const SDL_Color COLOR_RED = {255, 0, 0, 255};
const SDL_Color COLOR_BACKGROUND = {255, 190, 90, 255};

// Static variables
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static VisualMarker visual_markers[MAX_VISUAL_MARKERS];
static int visual_marker_count = 0;
static TTF_Font* font = NULL;
static double last_ai_time = 0.0;


// Internal function declarations
static void draw_board(void);
static void draw_stone(int row, int col, SDL_Color color);
static void draw_circle_filled(int x, int y, int radius, SDL_Color color);
static void draw_visual_markers(void);
static void draw_simple_number(int x, int y, int num);
static void draw_simple_text(int x, int y, const char* text);
static void draw_rule_buttons(const GomokuGame* game); // Add this line

#define BUTTON_WIDTH  180
#define BUTTON_HEIGHT 40
#define BUTTON_MARGIN 10

SDL_Rect button_rects[4] = {
    {CELL_SIZE * BOARD_SIZE + 20, 150, 180, 40}, // Captures
    {CELL_SIZE * BOARD_SIZE + 20, 200, 180, 40}, // Double Three
    {CELL_SIZE * BOARD_SIZE + 20, 250, 180, 40}, // Center Opening
    {CELL_SIZE * BOARD_SIZE + 20, 300, 180, 40}  // AI Mode
};

bool graphics_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        return false;
    }

    // Load font
    font = TTF_OpenFont("font.ttf", 16); // Exemple : "fonts/FreeSans.ttf"
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        return false;
    }

    
    window = SDL_CreateWindow("Gomoku",
                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    visual_marker_count = 0;
    return true;
}

void graphics_cleanup(void) {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    if (font) {
        TTF_CloseFont(font);
        font = NULL;
    }
    TTF_Quit();

    SDL_Quit();
}

static void draw_board(void) {
    // Set background color (light yellow)
    SDL_SetRenderDrawColor(renderer, COLOR_BACKGROUND.r, COLOR_BACKGROUND.g, COLOR_BACKGROUND.b, COLOR_BACKGROUND.a);
    SDL_RenderClear(renderer);
    
    // Draw coordinates
    for (int i = 0; i < BOARD_SIZE; i++) {
        // Draw horizontal numbers (along the top)
        draw_simple_number(CELL_SIZE + i * CELL_SIZE - 7, CELL_SIZE/3, i);
        
        // Draw vertical numbers (along the left side)
        draw_simple_number(CELL_SIZE/3, CELL_SIZE + i * CELL_SIZE - 7, i);
    }

    // Draw instructions
    draw_simple_text(800, 400, "Click to place a stone");
    draw_simple_text(800, 420, "Press 'R' to restart");
    draw_simple_text(800, 440, "Press 'P' to print board");
    
    // Draw grid lines
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for (int i = 0; i < BOARD_SIZE; i++) {
        // Vertical lines
        SDL_RenderDrawLine(renderer, 
                          CELL_SIZE + i * CELL_SIZE, CELL_SIZE,
                          CELL_SIZE + i * CELL_SIZE, CELL_SIZE * BOARD_SIZE);
        // Horizontal lines
        SDL_RenderDrawLine(renderer,
                          CELL_SIZE, CELL_SIZE + i * CELL_SIZE,
                          CELL_SIZE * BOARD_SIZE, CELL_SIZE + i * CELL_SIZE);
    }
    
    // Draw star points (traditional Go board markers)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    int star_points[][2] = {
        {3, 3}, {3, 9}, {3, 15},
        {9, 3}, {9, 9}, {9, 15},
        {15, 3}, {15, 9}, {15, 15}
    };
    
    for (int i = 0; i < 9; i++) {
        int x = CELL_SIZE + star_points[i][1] * CELL_SIZE;
        int y = CELL_SIZE + star_points[i][0] * CELL_SIZE;
        draw_circle_filled(x, y, 3, COLOR_BLACK);
    }
}

static void draw_circle_filled(int x, int y, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx*dx + dy*dy <= radius*radius) {
                SDL_RenderDrawPoint(renderer, x + dx, y + dy);
            }
        }
    }
}

static void draw_stone(int row, int col, SDL_Color color) {
    int x = CELL_SIZE + col * CELL_SIZE;
    int y = CELL_SIZE + row * CELL_SIZE;
    int radius = CELL_SIZE / 2 - 2;
    
    // Draw stone shadow (slightly offset)
    if (color.r == 0 && color.g == 0 && color.b == 0) { // Black stone
        SDL_Color shadow = {64, 64, 64, 255};
        draw_circle_filled(x + 2, y + 2, radius, shadow);
    }
    
    // Draw main stone
    draw_circle_filled(x, y, radius, color);
    
    // Add highlight for white stones
    if (color.r == 255 && color.g == 255 && color.b == 255) {
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        for (int dy = -radius + 2; dy <= radius - 2; dy++) {
            for (int dx = -radius + 2; dx <= radius - 2; dx++) {
                if (dx*dx + dy*dy <= (radius-2)*(radius-2)) {
                    SDL_RenderDrawPoint(renderer, x + dx, y + dy);
                }
            }
        }
        // Re-draw border
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        for (int angle = 0; angle < 360; angle++) {
            int px = x + (int)(radius * cos(angle * M_PI / 180));
            int py = y + (int)(radius * sin(angle * M_PI / 180));
            SDL_RenderDrawPoint(renderer, px, py);
        }
    }
}

static void draw_visual_markers(void) {
    for (int i = 0; i < visual_marker_count; i++) {
        VisualMarker* marker = &visual_markers[i];
        int x = CELL_SIZE + marker->col * CELL_SIZE;
        int y = CELL_SIZE + marker->row * CELL_SIZE;
        int radius = CELL_SIZE / 5;
        
        draw_circle_filled(x, y, radius, marker->color);
    }
}

static void draw_simple_number(int x, int y, int num) {
    char str[4];
    snprintf(str, sizeof(str), "%d", num);

    SDL_Color textColor = COLOR_BLACK;

    SDL_Surface* textSurface = TTF_RenderText_Blended(font, str, textColor);
    if (!textSurface) {
        printf("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        printf("Unable to create texture from rendered text! SDL_Error: %s\n", SDL_GetError());
        SDL_FreeSurface(textSurface);
        return;
    }

    SDL_Rect renderQuad = { x, y, textSurface->w, textSurface->h };
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

static void draw_simple_text(int x, int y, const char* text) {
    SDL_Color textColor = COLOR_BLACK;
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, textColor);
    if (!textSurface) {
        printf("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        printf("Unable to create texture from rendered text! SDL_Error: %s\n", SDL_GetError());
        SDL_FreeSurface(textSurface);
        return;
    }
    SDL_Rect renderQuad = { x, y, textSurface->w, textSurface->h };
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

void graphics_draw_game(const GomokuGame* game) {
    draw_board();

    // Draw stones
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game->board[i][j] == BLACK) {
                draw_stone(i, j, COLOR_BLACK);
            } else if (game->board[i][j] == WHITE) {
                draw_stone(i, j, COLOR_WHITE);
            }
        }
    }

    // Draw visual markers
    if (!game->mode_ai)
        draw_visual_markers();
    
    // Draw capture count (simple text representation using rectangles)
    // Black captures
    for (int i = 0; i < game->taken_stones[0] && i < 10; i++) {
        SDL_Rect rect = {CELL_SIZE * BOARD_SIZE + 20 + i * 15, 50, 10, 10};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
    
    // White captures
    for (int i = 0; i < game->taken_stones[1] && i < 10; i++) {
        SDL_Rect rect = {CELL_SIZE * BOARD_SIZE + 20 + i * 15, 80, 10, 10};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &rect);
    }
    
    // Current player indicator
    SDL_Rect indicator = {CELL_SIZE * BOARD_SIZE + 20, 20, 20, 20};
    if (game->current_player == BLACK) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    }
    SDL_RenderFillRect(renderer, &indicator);
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderDrawRect(renderer, &indicator);
    
    // Draw simple number for black captures
    // draw_simple_number(CELL_SIZE * BOARD_SIZE + 20, 50, game->taken_stones[0]);
    
    // Draw simple number for white captures
    // draw_simple_number(CELL_SIZE * BOARD_SIZE + 20, 80, game->taken_stones[1]);
    
    draw_rule_buttons(game);

    // Display AI thinking time
    char ai_time_str[64];
    snprintf(ai_time_str, sizeof(ai_time_str), "AI Time: %.3f s", last_ai_time);
    draw_simple_text(800, 480, ai_time_str);

    SDL_RenderPresent(renderer);
}

void draw_rule_buttons(const GomokuGame* game) {
    const char* labels[4] = {
        "Captures",
        "Double Three",
        "Free Opening",
        "AI Mode"
    };
    const bool states[4] = {
        game->rule_captures,
        game->rule_no_double_threes,
        game->rule_center_opening,
        game->mode_ai
    };

    for (int i = 0; i < 4; i++) {
        SDL_Rect rect = button_rects[i];
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);

        char text[64];
        snprintf(text, sizeof(text), "%s: %s", labels[i], states[i] ? "ON" : "OFF");
        draw_simple_text(rect.x + 10, rect.y + 10, text);
    }
}

bool graphics_handle_click(int x, int y, int* row, int* col) {
    *row = (y - CELL_SIZE / 2) / CELL_SIZE;
    *col = (x - CELL_SIZE / 2) / CELL_SIZE;
    
    return (*row >= 0 && *row < BOARD_SIZE && *col >= 0 && *col < BOARD_SIZE);
}

void graphics_add_visual_marker(int row, int col, SDL_Color color) {
    if (visual_marker_count < MAX_VISUAL_MARKERS) {
        visual_markers[visual_marker_count].row = row;
        visual_markers[visual_marker_count].col = col;
        visual_markers[visual_marker_count].color = color;
        visual_marker_count++;
    }
}

void graphics_clear_visual_markers(void) {
    visual_marker_count = 0;
}

void graphics_remove_visual_marker(int row, int col) {
    for (int i = 0; i < visual_marker_count; i++) {
        if (visual_markers[i].row == row && visual_markers[i].col == col) {
            // Remove by swapping with last element
            visual_markers[i] = visual_markers[visual_marker_count - 1];
            visual_marker_count--;
            break;
        }
    }
}

void graphics_show_winner(int winner) {
    // Draw winner message using simple shapes
    SDL_Rect message_bg = {WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 50, 300, 100};
    
    // Background
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
    SDL_RenderFillRect(renderer, &message_bg);
    
    // Border
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &message_bg);
    
    // Winner indication using colored rectangles
    if (winner == BLACK) {
        SDL_Rect winner_rect = {WINDOW_WIDTH / 2 - 50, WINDOW_HEIGHT / 2 - 25, 100, 50};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &winner_rect);
    } else if (winner == WHITE) {
        SDL_Rect winner_rect = {WINDOW_WIDTH / 2 - 50, WINDOW_HEIGHT / 2 - 25, 100, 50};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &winner_rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &winner_rect);
    } else {
        // Draw pattern for draw
        for (int i = 0; i < 10; i++) {
            SDL_Rect rect = {WINDOW_WIDTH / 2 - 50 + i * 10, WINDOW_HEIGHT / 2 - 25, 10, 50};
            if (i % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            }
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    
    SDL_RenderPresent(renderer);
}

void graphics_set_ai_time(double seconds) {
    last_ai_time = seconds;
}

SDL_Renderer* graphics_get_renderer(void) {
    return renderer;
}

SDL_Window* graphics_get_window(void) {
    return window;
}