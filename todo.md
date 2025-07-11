# Gomoku AI Improvement Recommendations

## 1. Balance Offensive and Defensive Play

### Current Issue
The AI prioritizes blocking opponent threats over building its own. In `ai_core.c`, it immediately returns after finding blocking moves without considering offensive opportunities.

### Solution
Modify the move selection logic to evaluate both offensive and defensive moves:

```c
// In ai_get_best_move() - instead of returning immediately after blocking
Move offensive_moves[MAX_MOVES], defensive_moves[MAX_MOVES];
int offensive_count, defensive_count;

// Find offensive opportunities
find_winning_moves_smart(game, offensive_moves, &offensive_count, game->current_player);

// Find defensive necessities  
find_winning_moves_smart(game, defensive_moves, &defensive_count, opponent);

// Evaluate combined threats
if (offensive_count > 0 && defensive_count > 0) {
    // Check if we can win before opponent
    // Consider creating a fork (multiple threats)
}
```

## 2. Improve Pattern Recognition and Evaluation

### Current Issue
The evaluation function doesn't adequately reward building threatening patterns before they become immediate wins.

### Solution
Add pattern-based evaluation for common Gomoku formations:

```c
// Add to evaluation.c
typedef struct {
    int pattern[5];  // -1 = opponent, 0 = empty, 1 = our stone
    int score;
    const char* name;
} Pattern;

static const Pattern strategic_patterns[] = {
    {{1, 1, 0, 1, 0}, 15000, "Split three"},
    {{0, 1, 1, 1, 0}, 20000, "Open three"},
    {{1, 0, 1, 1, 0}, 12000, "Jump three"},
    {{0, 1, 1, 0, 1, 0}, 25000, "Open four threat"},
    // Add more patterns
};

// Check for these patterns in evaluate_threats_for_position()
```

## 3. Enhance Connectivity Scoring

### Current Issue
The connectivity bonus (1000 points per connection) is too small compared to threat scores.

### Solution
```c
// In calculate_connectivity_bonus()
static int calculate_connectivity_bonus(const GomokuGame* game, int row, int col, int player) {
    int connectivity_bonus = 0;
    int pattern_potential = 0;
    
    for (int d = 0; d < 4; d++) {
        // Count total space available in this direction
        int space = count_available_space(game, row, col, dx, dy);
        
        // Count existing friendly stones
        int friendly = count_friendly_stones(game, row, col, dx, dy, player);
        
        // Higher bonus for positions that can form longer lines
        if (space >= 5 && friendly >= 2) {
            pattern_potential += friendly * 5000; // Much higher bonus
        }
        
        // Extra bonus for central positions that connect multiple groups
        if (is_connecting_groups(game, row, col, player)) {
            connectivity_bonus += 10000;
        }
    }
    
    return connectivity_bonus + pattern_potential;
}
```

## 4. Implement Strategic Opening Play

### Current Issue
The AI places stones randomly without building a coherent position.

### Solution
```c
// Add opening book or strategic principles
static Move get_opening_move(const GomokuGame* game, int move_number) {
    // First move: center or near center
    if (move_number == 1) {
        return (Move){BOARD_SIZE/2, BOARD_SIZE/2, 0};
    }
    
    // Second move: create potential for multiple directions
    if (move_number == 2) {
        // Place 2-3 squares away from first stone
        // Diagonal or knight's move for flexibility
    }
    
    // Focus on building a strong center presence
    // Avoid edges in opening
}
```

## 5. Add Fork Detection and Creation

### Current Issue
The AI doesn't recognize or create double threats (forks).

### Solution
```c
// In ai_moves.c
static int count_threats_created(const GomokuGame* game, int row, int col, int player) {
    GomokuGame temp_game;
    memcpy(&temp_game, game, sizeof(GomokuGame));
    temp_game.board[row][col] = player;
    
    int threat_count = 0;
    // Check all empty positions for threats created
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (temp_game.board[i][j] == EMPTY) {
                int score = ai_evaluate_position_for_player(&temp_game, i, j, player);
                if (score >= PATTERN_FOUR * 2) { // Significant threat
                    threat_count++;
                }
            }
        }
    }
    return threat_count;
}

// Heavily reward moves that create multiple threats
if (threat_count >= 2) {
    moves[i].score += 50000; // Fork bonus
}
```

## 6. Improve Move Ordering and Search

### Current Issue
The minimax search might miss good moves due to poor move ordering.

### Solution
```c
// In evaluate_generated_moves()
static void evaluate_generated_moves(const GomokuGame* game, Move* moves, int move_count) {
    for (int i = 0; i < move_count; i++) {
        // Current evaluation...
        
        // Add bonus for moves that:
        // 1. Create multiple threats
        int threats = count_threats_created(game, moves[i].row, moves[i].col, game->current_player);
        moves[i].score += threats * 20000;
        
        // 2. Extend existing patterns
        int extension_value = evaluate_pattern_extension(game, moves[i].row, moves[i].col);
        moves[i].score += extension_value;
        
        // 3. Block while building
        if (blocks_opponent_threat(game, moves[i].row, moves[i].col) && 
            creates_own_threat(game, moves[i].row, moves[i].col)) {
            moves[i].score += 15000; // Dual-purpose bonus
        }
    }
}
```

## 7. Adjust Evaluation Weights

### Current Issue
The current weights favor defense too heavily (our_score * 1.2 vs opp_score * 0.8).

### Solution
```c
// In evaluate_generated_moves()
// For a more aggressive AI:
moves[i].score = (int)(our_score * 1.5 + opp_score * 0.6) + connectivity_bonus;

// Or dynamically adjust based on game state:
float our_weight = 1.2;
float opp_weight = 0.8;

if (game_state_favors_attack(game)) {
    our_weight = 1.6;
    opp_weight = 0.5;
}
```

## Implementation Priority

1. **First**: Fix the immediate return after blocking (allows considering offensive moves)
2. **Second**: Increase connectivity bonus and pattern recognition
3. **Third**: Implement fork detection and creation
4. **Fourth**: Add opening book or principles
5. **Fifth**: Fine-tune evaluation weights

These changes should make the AI play more aggressively, build coherent threats, and create winning positions rather than just defending.