import time
import copy

# Global transposition table with size limit
MAX_TT_SIZE = 1000000
transposition_table = {}

def clear_transposition_table():
    global transposition_table
    if len(transposition_table) > MAX_TT_SIZE:
        transposition_table.clear()

def board_hash(game):
    # More efficient hashing using only relevant board positions
    board_tuple = tuple(tuple(row) for row in game.board)
    return hash((board_tuple, tuple(game.taken_stones), game.current_player))

def game_over(game):
    # Check captures first (faster)
    if any(taken >= 10 for taken in game.taken_stones):
        return True
    
    # Optimized five-in-a-row check
    board = game.board
    size = game.board_size
    directions = [(0, 1), (1, 0), (1, 1), (-1, 1)]
    
    for i in range(size):
        for j in range(size):
            if board[i][j] == 0:
                continue
            color = board[i][j]
            
            for dx, dy in directions:
                # Check if we can fit 5 stones in this direction
                if i + 4 * dx >= size or i + 4 * dx < 0 or j + 4 * dy >= size or j + 4 * dy < 0:
                    continue
                    
                # Quick check for 5 in a row
                count = 0
                for k in range(5):
                    if board[i + k * dx][j + k * dy] == color:
                        count += 1
                    else:
                        break
                if count == 5:
                    return True
    return False

def evaluate_fast(game):
    """Highly optimized evaluation function"""
    if game_over(game):
        player = 3 - game.current_player
        if any(taken >= 10 for taken in game.taken_stones):
            if game.taken_stones[player - 1] >= 10:
                return float('inf')
            else:
                return -float('inf')
        # Five in a row win
        return float('inf') if game.current_player != player else -float('inf')
    
    score = 0
    player = 3 - game.current_player
    opponent = 3 - player
    board = game.board
    size = game.board_size
    
    # Capture advantage
    score += (game.taken_stones[player - 1] - game.taken_stones[opponent - 1]) * 500
    
    # Pattern-based evaluation (much faster than checking all 5-stone windows)
    directions = [(1, 0), (0, 1), (1, 1), (1, -1)]
    
    for i in range(size):
        for j in range(size):
            if board[i][j] == 0:
                continue
                
            stone_player = board[i][j]
            multiplier = 1 if stone_player == player else -1
            
            for dx, dy in directions:
                # Count consecutive stones in this direction
                consecutive = 1
                k = 1
                while (i + k * dx < size and i + k * dx >= 0 and 
                       j + k * dy < size and j + k * dy >= 0 and 
                       board[i + k * dx][j + k * dy] == stone_player):
                    consecutive += 1
                    k += 1
                
                # Check if blocked on both ends
                left_blocked = (i - dx < 0 or i - dx >= size or j - dy < 0 or j - dy >= size or 
                               board[i - dx][j - dy] != 0)
                right_blocked = (i + consecutive * dx >= size or i + consecutive * dx < 0 or 
                                j + consecutive * dy >= size or j + consecutive * dy < 0 or 
                                board[i + consecutive * dx][j + consecutive * dy] != 0)
                
                # Score based on consecutive count and openness
                if consecutive >= 4:
                    score += multiplier * 10000
                elif consecutive == 3:
                    if not left_blocked and not right_blocked:
                        score += multiplier * 1000  # Open three
                    elif not left_blocked or not right_blocked:
                        score += multiplier * 100   # Semi-open three
                elif consecutive == 2:
                    if not left_blocked and not right_blocked:
                        score += multiplier * 100   # Open two
                    elif not left_blocked or not right_blocked:
                        score += multiplier * 10    # Semi-open two
    
    return score

def generate_smart_moves(game, max_moves=12):
    """Generate moves around existing stones with threat analysis"""
    if not any(game.board[i][j] != 0 for i in range(game.board_size) for j in range(game.board_size)):
        # First move - center
        center = game.board_size // 2
        return [(center, center)]
    
    candidates = set()
    threat_moves = set()
    board = game.board
    size = game.board_size
    
    # Generate moves around existing stones
    for i in range(size):
        for j in range(size):
            if board[i][j] != 0:
                # Add empty cells within distance 2
                for di in range(-2, 3):
                    for dj in range(-2, 3):
                        ni, nj = i + di, j + dj
                        if (0 <= ni < size and 0 <= nj < size and 
                            board[ni][nj] == 0):
                            candidates.add((ni, nj))
    
    # Quick threat detection - prioritize blocking/creating threats
    current_player = game.current_player
    opponent = 3 - current_player
    
    for i, j in list(candidates):
        # Check if this move creates or blocks immediate threats
        threat_score = 0
        
        # Simulate placing current player's stone
        board[i][j] = current_player
        if is_winning_move(board, i, j, current_player, size):
            threat_moves.add((i, j, 100000))  # Winning move
        elif creates_threat(board, i, j, current_player, size):
            threat_score += 1000
        board[i][j] = 0
        
        # Simulate placing opponent's stone to check blocking
        board[i][j] = opponent
        if is_winning_move(board, i, j, opponent, size):
            threat_moves.add((i, j, 50000))  # Block winning move
        elif creates_threat(board, i, j, opponent, size):
            threat_score += 500
        board[i][j] = 0
        
        if threat_score > 0:
            threat_moves.add((i, j, threat_score))
    
    # Sort moves by threat level and return top candidates
    if threat_moves:
        sorted_threats = sorted(threat_moves, key=lambda x: x[2], reverse=True)
        return [move[:2] for move in sorted_threats[:max_moves]]
    
    # If no threats, return candidates around stones (limited number)
    return list(candidates)[:max_moves]

def is_winning_move(board, row, col, player, size):
    """Check if placing a stone creates 5 in a row"""
    directions = [(0, 1), (1, 0), (1, 1), (-1, 1)]
    
    for dx, dy in directions:
        count = 1  # Count the placed stone
        
        # Count in positive direction
        k = 1
        while (0 <= row + k * dx < size and 0 <= col + k * dy < size and 
               board[row + k * dx][col + k * dy] == player):
            count += 1
            k += 1
        
        # Count in negative direction
        k = 1
        while (0 <= row - k * dx < size and 0 <= col - k * dy < size and 
               board[row - k * dx][col - k * dy] == player):
            count += 1
            k += 1
        
        if count >= 5:
            return True
    
    return False

def creates_threat(board, row, col, player, size):
    """Check if placing a stone creates a 4-in-a-row threat"""
    directions = [(0, 1), (1, 0), (1, 1), (-1, 1)]
    
    for dx, dy in directions:
        count = 1
        
        # Count in positive direction
        k = 1
        while (0 <= row + k * dx < size and 0 <= col + k * dy < size and 
               board[row + k * dx][col + k * dy] == player):
            count += 1
            k += 1
        
        # Count in negative direction
        k = 1
        while (0 <= row - k * dx < size and 0 <= col - k * dy < size and 
               board[row - k * dx][col - k * dy] == player):
            count += 1
            k += 1
        
        if count >= 4:
            return True
    
    return False

def clone_game_fast(game):
    """Faster game cloning"""
    new_game = game.__class__()
    new_game.board_size = game.board_size
    new_game.cell_size = game.cell_size
    new_game.board = [row[:] for row in game.board]  # Faster than deepcopy
    new_game.taken_stones = game.taken_stones[:]  # Faster than deepcopy
    new_game.current_player = game.current_player
    new_game.rule_center_opening = game.rule_center_opening
    new_game.rule_no_double_threes = game.rule_no_double_threes
    new_game.rule_captures = game.rule_captures
    return new_game

def minmax_optimized(game, depth, alpha, beta, maximizing_player, visualize=False, main_game=None):
    # Clear transposition table if it gets too large
    if len(transposition_table) > MAX_TT_SIZE:
        clear_transposition_table()
    
    # Check transposition table
    state_key = board_hash(game)
    if state_key in transposition_table:
        stored_depth, stored_score, stored_move = transposition_table[state_key]
        if stored_depth >= depth:
            return stored_score, stored_move
    
    # Terminal node check
    if depth == 0 or game_over(game):
        score = evaluate_fast(game)
        transposition_table[state_key] = (depth, score, None)
        return score, None
    
    # Generate smart moves (limited set)
    moves = generate_smart_moves(game, max_moves=8 if depth > 3 else 12)
    
    if not moves:
        score = evaluate_fast(game)
        transposition_table[state_key] = (depth, score, None)
        return score, None
    
    best_move = moves[0]
    
    if maximizing_player:
        max_eval = -float('inf')
        
        for move in moves:
            i, j = move
            
            # Make move
            new_game = clone_game_fast(game)
            captured, error = new_game.place_stone(i, j)
            if error:
                continue
            
            if visualize and main_game is not None:
                main_game.show_visual_move(i, j, color="blue")
            
            eval_score, _ = minmax_optimized(new_game, depth - 1, alpha, beta, False, visualize, main_game)
            
            if visualize and main_game is not None:
                main_game.remove_visual_move(i, j)
            
            if eval_score > max_eval:
                max_eval = eval_score
                best_move = move
                if visualize and main_game is not None:
                    main_game.show_visual_move(i, j, color="red")
            
            alpha = max(alpha, eval_score)
            if beta <= alpha:
                break  # Alpha-beta pruning
        
        result = (max_eval, best_move)
    else:
        min_eval = float('inf')
        
        for move in moves:
            i, j = move
            
            # Make move
            new_game = clone_game_fast(game)
            captured, error = new_game.place_stone(i, j)
            if error:
                continue
            
            if visualize and main_game is not None:
                main_game.show_visual_move(i, j, color="blue")
            
            eval_score, _ = minmax_optimized(new_game, depth - 1, alpha, beta, True, visualize, main_game)
            
            if visualize and main_game is not None:
                main_game.remove_visual_move(i, j)
            
            if eval_score < min_eval:
                min_eval = eval_score
                best_move = move
                if visualize and main_game is not None:
                    main_game.show_visual_move(i, j, color="red")
            
            beta = min(beta, eval_score)
            if beta <= alpha:
                break  # Alpha-beta pruning
        
        result = (min_eval, best_move)
    
    # Store in transposition table
    transposition_table[state_key] = (depth, result[0], result[1])
    
    if visualize and main_game is not None:
        main_game.clear_visual_markers_by_color("red")
    
    return result

# Wrapper function to maintain compatibility
def minmax(game, depth, alpha, beta, maximizing_player, visualize=False, main_game=None):
    return minmax_optimized(game, depth, alpha, beta, maximizing_player, visualize, main_game)