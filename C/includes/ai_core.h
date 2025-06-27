#ifndef AI_CORE_H
#define AI_CORE_H

#include "ai.h"

// Core AI functions
void ai_init(void);
void ai_cleanup(void);
Move ai_get_best_move(const GomokuGame* game, int depth, AIStats* stats);
AIStats ai_get_last_stats(void);

#endif // AI_CORE_H