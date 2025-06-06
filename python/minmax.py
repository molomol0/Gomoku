# Ultra-Optimized Aggressive Gomoku AI Engine
# Focused on speed and winning, not just defense

import time
from functools import lru_cache

# Global configuration
MAX_TT_SIZE = 500000
INFINITY = 1000000
DIRECTIONS = [(1, 0), (0, 1), (1, 1), (1, -1)]

# Global caches
transposition_table = {}
pattern_cache = {}

class FastPatternDetector:
    """Ultra-fast pattern detection with minimal function calls"""
    
    # Aggressive pattern values - prioritize winning over defense
    PATTERN_SCORES = {
        5: 100000,    # Win
        4: 50000,     # Four in a row
        3: 8000,      # Three in a row  
        2: 800,       # Two in a row
        1: 80         # Single stone
    }
    
    OPEN_MULTIPLIERS = {
        2: 3.0,   # Both ends open
        1: 1.5,   # One end open
        0: 0.3    # No ends open
    }
    
    @staticmethod
    def count_consecutive_fast(board, row, col, dx, dy, player, board_size):
        """Ultra-fast consecutive counting with single loop"""
        if board[row][col] != player:
            return 0, 0
            
        # Count positive direction
        pos_count = 0
        r, c = row + dx, col + dy
        while 0 <= r < board_size and 0 <= c < board_size and board[r][c] == player:
            pos_count += 1
            r += dx
            c += dy
        
        # Count negative direction
        neg_count = 0
        r, c = row - dx, col - dy
        while 0 <= r < board_size and 0 <= c < board_size and board[r][c] == player:
            neg_count += 1
            r -= dx
            c -= dy
            
        return pos_count, neg_count
    
    @staticmethod
    def evaluate_line_fast(board, row, col, dx, dy, player, board_size):
        """Fast line evaluation with minimal calculations"""
        pos_count, neg_count = FastPatternDetector.count_consecutive_fast(
            board, row, col, dx, dy, player, board_size)
        
        total_count = pos_count + neg_count + 1
        
        if total_count < 2:
            return 0
            
        # Quick open end check
        open_ends = 0
        
        # Check positive end
        end_r, end_c = row + dx * (pos_count + 1), col + dy * (pos_count + 1)
        if 0 <= end_r < board_size and 0 <= end_c < board_size and board[end_r][end_c] == 0:
            open_ends += 1
            
        # Check negative end  
        end_r, end_c = row - dx * (neg_count + 1), col - dy * (neg_count + 1)
        if 0 <= end_r < board_size and 0 <= end_c < board_size and board[end_r][end_c] == 0:
            open_ends += 1
        
        # Calculate score
        base_score = FastPatternDetector.PATTERN_SCORES.get(min(total_count, 5), 0)
        multiplier = FastPatternDetector.OPEN_MULTIPLIERS.get(open_ends, 1.0)
        
        return int(base_score * multiplier)

    
    @staticmethod
    def evaluate_directions_fast(board, row, col, player, board_size):
        """Evaluate all directions for a position and return max_score, threat_count"""
        max_score = 0
        threat_count = 0
        for dx, dy in DIRECTIONS:
            score = FastPatternDetector.evaluate_line_fast(board, row, col, dx, dy, player, board_size)
            if score >= 100000:  # Winning move
                return INFINITY, threat_count
            elif score >= 8000:  # Strong threat
                threat_count += 1
            max_score = max(max_score, score)
        return max_score, threat_count

    @staticmethod
    def evaluate_position_fast(board, row, col, player, board_size):
        """Ultra-fast position evaluation"""
        if board[row][col] != 0:
            return 0

        # Use pattern cache
        cache_key = (row, col, player, hash(tuple(tuple(r) for r in board)))
        if cache_key in pattern_cache:
            return pattern_cache[cache_key]

        board[row][col] = player

        # Refactored: call the new function
        max_score, threat_count = FastPatternDetector.evaluate_directions_fast(
            board, row, col, player, board_size
        )

        if max_score >= INFINITY:
            board[row][col] = 0
            pattern_cache[cache_key] = INFINITY
            return INFINITY

        # Massive bonus for multiple threats (winning tactic)
        if threat_count >= 2:
            max_score += 30000
        elif threat_count == 1:
            max_score += 5000

        board[row][col] = 0

        # Clean cache if too large
        if len(pattern_cache) > 10000:
            pattern_cache.clear()

        pattern_cache[cache_key] = max_score
        return max_score

class AggressiveMoveGenerator:
    """Move generation focused on winning quickly"""
    
    @staticmethod
    def get_neighbor_positions(board, board_size, radius=2):
        """Get positions near existing stones"""
        positions = []
        seen = set()
        
        for i in range(board_size):
            for j in range(board_size):
                if board[i][j] != 0:
                    # Add neighbors
                    for di in range(-radius, radius + 1):
                        for dj in range(-radius, radius + 1):
                            if di == 0 and dj == 0:
                                continue
                            ni, nj = i + di, j + dj
                            if (0 <= ni < board_size and 0 <= nj < board_size and 
                                board[ni][nj] == 0 and (ni, nj) not in seen):
                                positions.append((ni, nj))
                                seen.add((ni, nj))
        
        return positions
    
    @staticmethod
    def find_winning_moves(board, board_size, player):
        """Find immediate winning moves"""
        winning_moves = []
        
        for i in range(board_size):
            for j in range(board_size):
                if board[i][j] == 0:
                    score = FastPatternDetector.evaluate_position_fast(board, i, j, player, board_size)
                    if score >= INFINITY:
                        winning_moves.append((i, j, score))
        
        return winning_moves
    
    @staticmethod
    def find_threat_moves(board, board_size, player, min_threat=8000):
        """Find moves that create threats"""
        threat_moves = []
        
        # Only check promising positions
        candidates = AggressiveMoveGenerator.get_neighbor_positions(board, board_size, 2)
        
        for row, col in candidates:
            score = FastPatternDetector.evaluate_position_fast(board, row, col, player, board_size)
            if score >= min_threat:
                threat_moves.append((row, col, score))
        
        return sorted(threat_moves, key=lambda x: x[2], reverse=True)
    
    @staticmethod
    def generate_moves_ultra_fast(game, max_moves=12):
        """Ultra-fast move generation with aggressive prioritization"""
        board = game.board
        board_size = game.board_size
        current_player = game.current_player
        opponent = 3 - current_player
        
        # First move optimization
        if all(board[i][j] == 0 for i in range(board_size) for j in range(board_size)):
            center = board_size // 2
            return [(center, center)]
        
        # 1. Check for immediate wins (TOP PRIORITY)
        winning_moves = AggressiveMoveGenerator.find_winning_moves(board, board_size, current_player)
        if winning_moves:
            return [(move[0], move[1]) for move in winning_moves[:1]]  # Take first win
        
        # 2. Block opponent wins (CRITICAL DEFENSE)
        opponent_wins = AggressiveMoveGenerator.find_winning_moves(board, board_size, opponent)
        if opponent_wins:
            return [(move[0], move[1]) for move in opponent_wins[:3]]  # Block up to 3 wins
        
        # 3. Create strong threats (AGGRESSIVE OFFENSE)
        our_threats = AggressiveMoveGenerator.find_threat_moves(board, board_size, current_player, 8000)
        if our_threats:
            moves = [(move[0], move[1]) for move in our_threats[:max_moves//2]]
            if len(moves) >= max_moves//3:  # If we have good threats, focus on them
                return moves
        
        # 4. Block opponent threats (TACTICAL DEFENSE)
        opponent_threats = AggressiveMoveGenerator.find_threat_moves(board, board_size, opponent, 8000)
        
        # 5. Combine offensive and defensive moves
        all_moves = []
        
        # Add our best threats
        all_moves.extend([(move[0], move[1], move[2] * 1.2) for move in our_threats[:max_moves//2]])
        
        # Add opponent threat blocks
        all_moves.extend([(move[0], move[1], move[2]) for move in opponent_threats[:max_moves//2]])
        
        # If we don't have enough moves, add some tactical moves
        if len(all_moves) < max_moves:
            tactical_moves = AggressiveMoveGenerator.find_threat_moves(board, board_size, current_player, 800)
            all_moves.extend([(move[0], move[1], move[2] * 0.8) for move in tactical_moves[:max_moves]])
        
        # Sort by score and return
        all_moves.sort(key=lambda x: x[2], reverse=True)
        result = [(move[0], move[1]) for move in all_moves[:max_moves]]
        
        # Fallback
        if not result:
            candidates = AggressiveMoveGenerator.get_neighbor_positions(board, board_size, 2)
            result = candidates[:max_moves] if candidates else [(board_size//2, board_size//2)]
        
        return result

class UltraFastEvaluator:
    """Minimal evaluation focused on speed"""
    
    @staticmethod
    def is_terminal_fast(game, last_move=None):
        """Ultra-fast terminal check"""
        # Check captures
        if hasattr(game, 'taken_stones'):
            if max(game.taken_stones) >= 10:
                return True
        
        # Check five in a row from last move only
        if last_move:
            row, col = last_move
            if game.board[row][col] == 0:
                return False
                
            player = game.board[row][col]
            board_size = game.board_size
            
            for dx, dy in DIRECTIONS:
                count = 1
                
                # Positive direction
                r, c = row + dx, col + dy
                while 0 <= r < board_size and 0 <= c < board_size and game.board[r][c] == player:
                    count += 1
                    r += dx
                    c += dy
                    if count >= 5:
                        return True
                
                # Negative direction
                r, c = row - dx, col - dy
                while 0 <= r < board_size and 0 <= c < board_size and game.board[r][c] == player:
                    count += 1
                    r -= dx
                    c -= dy
                    if count >= 5:
                        return True
        
        return False
    
    @staticmethod
    def evaluate_fast(game, last_move=None):
        """Lightning-fast evaluation"""
        if UltraFastEvaluator.is_terminal_fast(game, last_move):
            # Determine winner
            if hasattr(game, 'taken_stones'):
                if game.taken_stones[0] >= 10:
                    return INFINITY if game.current_player == 2 else -INFINITY
                elif game.taken_stones[1] >= 10:
                    return INFINITY if game.current_player == 1 else -INFINITY
            
            # Five in a row win
            if last_move:
                return INFINITY if game.board[last_move[0]][last_move[1]] == (3 - game.current_player) else -INFINITY
        
        # Quick positional evaluation
        score = 0
        board = game.board
        board_size = game.board_size
        
        # Simple material and position evaluation
        for i in range(board_size):
            for j in range(board_size):
                if board[i][j] != 0:
                    player = board[i][j]
                    multiplier = 1 if player == 1 else -1
                    
                    # Quick pattern check
                    position_value = 0
                    for dx, dy in DIRECTIONS:
                        line_score = FastPatternDetector.evaluate_line_fast(board, i, j, dx, dy, player, board_size)
                        position_value = max(position_value, line_score)
                    
                    score += multiplier * position_value
        
        # Capture bonus
        if hasattr(game, 'taken_stones'):
            score += (game.taken_stones[0] - game.taken_stones[1]) * 2000
        
        return score

class UltraFastMinimax:
    """Hyper-optimized minimax with aggressive pruning"""
    
    def __init__(self):
        self.nodes_searched = 0
        self.cache_hits = 0
        self.pruned = 0
    
    def get_best_move(self, game, depth=5, visualize=False, main_game=None):
        """Get best move with time optimization"""
        self.nodes_searched = 0
        self.cache_hits = 0
        self.pruned = 0
        
        start_time = time.time()
        
        # Quick win check
        winning_moves = AggressiveMoveGenerator.find_winning_moves(game.board, game.board_size, game.current_player)
        if winning_moves:
            move = (winning_moves[0][0], winning_moves[0][1])
            print(f"Immediate win found: {move}")
            return INFINITY, move
        
        # Quick defense check
        opponent = 3 - game.current_player
        opponent_wins = AggressiveMoveGenerator.find_winning_moves(game.board, game.board_size, opponent)
        if opponent_wins:
            move = (opponent_wins[0][0], opponent_wins[0][1])
            print(f"Blocking opponent win: {move}")
            return -INFINITY + 1, move
        
        score, move = self.minimax_ultra(game, depth, -INFINITY, INFINITY, True, visualize, main_game)
        end_time = time.time()
        
        # Safety check
        if move is None:
            print("WARNING: No move found, using fallback")
            moves = AggressiveMoveGenerator.generate_moves_ultra_fast(game, 1)
            move = moves[0] if moves else (game.board_size // 2, game.board_size // 2)
        
        print(f"Search: {self.nodes_searched} nodes, {self.cache_hits} hits, {self.pruned} pruned, {end_time - start_time:.2f}s")
        return score, move
    
    def minimax_ultra(self, game, depth, alpha, beta, maximizing_player, visualize=False, main_game=None, last_move=None):
        """Ultra-optimized minimax"""
        self.nodes_searched += 1
        
        # Clean cache periodically
        if len(transposition_table) > MAX_TT_SIZE:
            transposition_table.clear()
        
        # Transposition table lookup
        state_key = self.hash_game_fast(game)
        if state_key in transposition_table:
            stored_depth, stored_score, stored_move = transposition_table[state_key]
            if stored_depth >= depth:
                self.cache_hits += 1
                return stored_score, stored_move
        
        # Terminal check
        if depth == 0 or UltraFastEvaluator.is_terminal_fast(game, last_move):
            score = UltraFastEvaluator.evaluate_fast(game, last_move)
            transposition_table[state_key] = (depth, score, None)
            return score, None
        
        # Aggressive move count reduction
        move_count = max(6, min(15, 20 - depth * 2))
        moves = AggressiveMoveGenerator.generate_moves_ultra_fast(game, move_count)
        
        if not moves:
            score = UltraFastEvaluator.evaluate_fast(game, last_move)
            return score, None
        
        best_move = moves[0]
        
        if maximizing_player:
            max_eval = -INFINITY
            
            for i, move in enumerate(moves):
                row, col = move
                
                # Make move
                new_game = self.clone_game_fast(game)
                captured, error = new_game.place_stone(row, col)
                if error:
                    continue
                
                # Visualization
                if visualize and main_game:
                    main_game.show_visual_move(row, col, color="blue")
                
                # Recursive call
                eval_score, _ = self.minimax_ultra(new_game, depth - 1, alpha, beta, False, 
                                                 visualize, main_game, (row, col))
                
                # Remove visualization
                if visualize and main_game:
                    main_game.remove_visual_move(row, col)
                
                if eval_score > max_eval:
                    max_eval = eval_score
                    best_move = move
                    
                    if visualize and main_game:
                        main_game.show_visual_move(row, col, color="red")
                
                alpha = max(alpha, eval_score)
                if beta <= alpha:
                    self.pruned += len(moves) - i - 1
                    break  # Alpha-beta pruning
            
            result = (max_eval, best_move)
        else:
            min_eval = INFINITY
            
            for i, move in enumerate(moves):
                row, col = move
                
                # Make move
                new_game = self.clone_game_fast(game)
                captured, error = new_game.place_stone(row, col)
                if error:
                    continue
                
                # Visualization
                if visualize and main_game:
                    main_game.show_visual_move(row, col, color="blue")
                
                # Recursive call
                eval_score, _ = self.minimax_ultra(new_game, depth - 1, alpha, beta, True, 
                                                 visualize, main_game, (row, col))
                
                # Remove visualization
                if visualize and main_game:
                    main_game.remove_visual_move(row, col)
                
                if eval_score < min_eval:
                    min_eval = eval_score
                    best_move = move
                    
                    if visualize and main_game:
                        main_game.show_visual_move(row, col, color="red")
                
                beta = min(beta, eval_score)
                if beta <= alpha:
                    self.pruned += len(moves) - i - 1
                    break  # Alpha-beta pruning
            
            result = (min_eval, best_move)
        
        # Store in transposition table
        transposition_table[state_key] = (depth, result[0], result[1])
        
        # Clear visualization
        if visualize and main_game:
            main_game.clear_visual_markers_by_color("red")
        
        return result
    
    def hash_game_fast(self, game):
        """Fast game hashing"""
        # Use a simpler hash for speed
        board_hash = 0
        for i, row in enumerate(game.board):
            for j, cell in enumerate(row):
                if cell != 0:
                    board_hash ^= hash((i, j, cell))
        
        taken_hash = hash(tuple(getattr(game, 'taken_stones', [0, 0])))
        return hash((board_hash, taken_hash, game.current_player))
    
    def clone_game_fast(self, game):
        """Ultra-fast game cloning"""
        new_game = game.__class__()
        new_game.board_size = game.board_size
        new_game.cell_size = getattr(game, 'cell_size', 40)
        new_game.board = [row[:] for row in game.board]
        new_game.taken_stones = getattr(game, 'taken_stones', [0, 0])[:]
        new_game.current_player = game.current_player
        
        # Essential rules only
        new_game.rule_captures = getattr(game, 'rule_captures', True)
        
        return new_game

# Global AI instance
ai = UltraFastMinimax()

# Legacy function wrappers for compatibility
def minmax(game, depth, alpha, beta, maximizing_player, visualize=False, main_game=None, last_move=None, previous_score=None):
    """Legacy wrapper for minimax function"""
    return ai.minimax_ultra(game, depth, alpha, beta, maximizing_player, visualize, main_game, last_move)

def generate_smart_moves(game, max_moves=12):
    """Legacy wrapper for move generation"""
    return AggressiveMoveGenerator.generate_moves_ultra_fast(game, max_moves)

def evaluate_fast(game, last_move=None, previous_score=None):
    """Legacy wrapper for evaluation"""
    return UltraFastEvaluator.evaluate_fast(game, last_move)

def game_over(game, last_move=None):
    """Legacy wrapper for terminal check"""
    return UltraFastEvaluator.is_terminal_fast(game, last_move)

def clear_transposition_table():
    """Clear the transposition table"""
    global transposition_table
    transposition_table.clear()

def clear_move_cache():
    """Clear the pattern cache"""
    global pattern_cache
    pattern_cache.clear()

# Main AI interface
def get_ai_move(game, difficulty=5, visualize=False, main_game=None):
    """Get AI move with specified difficulty (depth)"""
    return ai.get_best_move(game, difficulty, visualize, main_game)