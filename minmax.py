import time
import copy

# Global transposition table with size limit
MAX_TT_SIZE = 2000000  # Increased size
transposition_table = {}

# Cache for move generation to avoid recomputing
move_cache = {}
MAX_MOVE_CACHE_SIZE = 100000

def clear_transposition_table():
    global transposition_table
    if len(transposition_table) > MAX_TT_SIZE:
        transposition_table.clear()

def clear_move_cache():
    global move_cache
    if len(move_cache) > MAX_MOVE_CACHE_SIZE:
        move_cache.clear()

def board_hash(game):
    # More efficient hashing using only relevant board positions
    board_tuple = tuple(tuple(row) for row in game.board)
    return hash((board_tuple, tuple(game.taken_stones), game.current_player))

def check_line_fast(board, row, col, dx, dy, size, target_count=5):
    """Ultra-fast line checking with early termination"""
    if board[row][col] == 0:
        return 0
    
    player = board[row][col]
    count = 1
    
    # Positive direction - unrolled for speed
    r, c = row + dx, col + dy
    if 0 <= r < size and 0 <= c < size and board[r][c] == player:
        count += 1
        r, c = r + dx, c + dy
        if 0 <= r < size and 0 <= c < size and board[r][c] == player:
            count += 1
            r, c = r + dx, c + dy
            if 0 <= r < size and 0 <= c < size and board[r][c] == player:
                count += 1
                r, c = r + dx, c + dy
                if 0 <= r < size and 0 <= c < size and board[r][c] == player:
                    count += 1
                    # Continue if we need more than 5
                    while count < target_count and 0 <= r + dx < size and 0 <= c + dy < size and board[r + dx][c + dy] == player:
                        count += 1
                        r, c = r + dx, c + dy
    
    # Negative direction - unrolled for speed
    r, c = row - dx, col - dy
    if 0 <= r < size and 0 <= c < size and board[r][c] == player:
        count += 1
        r, c = r - dx, c - dy
        if 0 <= r < size and 0 <= c < size and board[r][c] == player:
            count += 1
            r, c = r - dx, c - dy
            if 0 <= r < size and 0 <= c < size and board[r][c] == player:
                count += 1
                r, c = r - dx, c - dy
                if 0 <= r < size and 0 <= c < size and board[r][c] == player:
                    count += 1
                    # Continue if we need more than 5
                    while count < target_count and 0 <= r - dx < size and 0 <= c - dy < size and board[r - dx][c - dy] == player:
                        count += 1
                        r, c = r - dx, c - dy
    
    return count

def check_five_in_row_from_position(board, row, col, size):
    """Optimized 5-in-a-row check with early termination"""
    if board[row][col] == 0:
        return False
    
    # Check each direction - return immediately on finding 5
    directions = [(0, 1), (1, 0), (1, 1), (-1, 1)]
    for dx, dy in directions:
        if check_line_fast(board, row, col, dx, dy, size, 5) >= 5:
            return True
    
    return False

def game_over_incremental(game, last_move=None):
    """Optimized game_over check using last move position"""
    # Check captures first (faster and doesn't depend on position)
    if game.taken_stones[0] >= 10 or game.taken_stones[1] >= 10:
        return True
    
    # If no last move provided, fall back to full board check
    if last_move is None:
        return game_over_full_board(game)
    
    # Only check 5-in-a-row from the last move position
    row, col = last_move
    return check_five_in_row_from_position(game.board, row, col, game.board_size)

def game_over_full_board(game):
    """Original full board check - fallback method"""
    board = game.board
    size = game.board_size
    directions = [(0, 1), (1, 0), (1, 1), (-1, 1)]
    
    for i in range(size):
        for j in range(size):
            if board[i][j] == 0:
                continue
            
            for dx, dy in directions:
                if check_line_fast(board, i, j, dx, dy, size, 5) >= 5:
                    return True
    return False

def game_over(game, last_move=None):
    """Main game_over function with incremental optimization"""
    return game_over_incremental(game, last_move)

def evaluate_position_fast(board, row, col, size, player, opponent):
    """Ultra-fast position evaluation with pattern scoring"""
    if board[row][col] == 0:
        return 0
    
    stone_player = board[row][col]
    multiplier = 1 if stone_player == player else -1
    score = 0
    
    directions = [(1, 0), (0, 1), (1, 1), (1, -1)]
    
    for dx, dy in directions:
        count = check_line_fast(board, row, col, dx, dy, size, 6)
        
        if count >= 5:
            return multiplier * 100000  # Immediate win/loss
        elif count == 4:
            score += multiplier * 10000
        elif count == 3:
            score += multiplier * 1000
        elif count == 2:
            score += multiplier * 100
    
    return score

def evaluate_fast_full(game):
    """Optimized full board evaluation"""
    player = 3 - game.current_player
    opponent = 3 - player
    board = game.board
    size = game.board_size
    
    # Quick capture check
    capture_diff = (game.taken_stones[player - 1] - game.taken_stones[opponent - 1]) * 1000
    if abs(capture_diff) > 5000:  # Near capture win
        return capture_diff
    
    score = capture_diff
    
    # Pattern evaluation - optimized loop
    for i in range(size):
        for j in range(size):
            if board[i][j] != 0:
                score += evaluate_position_fast(board, i, j, size, player, opponent)
    
    return score

def evaluate_fast(game, last_move=None, previous_score=None):
    """Main evaluation function"""
    # Check for terminal states first
    if game_over_incremental(game, last_move):
        player = 3 - game.current_player
        if game.taken_stones[0] >= 10 or game.taken_stones[1] >= 10:
            if game.taken_stones[player - 1] >= 10:
                return 1000000
            else:
                return -1000000
        return 1000000 if game.current_player != player else -1000000
    
    return evaluate_fast_full(game)

def get_adjacent_positions(board, size, radius=2):
    """Get positions adjacent to existing stones - cached version"""
    positions = set()
    
    for i in range(size):
        for j in range(size):
            if board[i][j] != 0:
                # Add adjacent positions in radius
                for di in range(-radius, radius + 1):
                    for dj in range(-radius, radius + 1):
                        ni, nj = i + di, j + dj
                        if (0 <= ni < size and 0 <= nj < size and 
                            board[ni][nj] == 0):
                            positions.add((ni, nj))
    
    return list(positions)

def is_critical_move(board, row, col, player, size):
    """Check if move is critical (wins or blocks win) - optimized"""
    # Temporarily place stone
    board[row][col] = player
    
    # Check for immediate win
    if check_five_in_row_from_position(board, row, col, size):
        board[row][col] = 0
        return 3  # Immediate win
    
    # Check for 4-in-a-row threat
    threat_level = 0
    directions = [(0, 1), (1, 0), (1, 1), (-1, 1)]
    for dx, dy in directions:
        count = check_line_fast(board, row, col, dx, dy, size, 4)
        if count >= 4:
            threat_level = 2  # Creates threat
            break
        elif count == 3:
            threat_level = max(threat_level, 1)  # Good move
    
    board[row][col] = 0
    return threat_level

def generate_smart_moves(game, max_moves=15):
    """Highly optimized move generation with aggressive pruning"""
    board = game.board
    size = game.board_size
    
    # First move - center
    if not any(board[i][j] != 0 for i in range(size) for j in range(size)):
        center = size // 2
        return [(center, center)]
    
    # Use cache for repeated positions
    board_key = hash(tuple(tuple(row) for row in board))
    if board_key in move_cache:
        cached_moves, cached_player = move_cache[board_key]
        if cached_player == game.current_player:
            return cached_moves[:max_moves]
    
    clear_move_cache()  # Prevent memory overflow
    
    current_player = game.current_player
    opponent = 3 - current_player
    
    # Get candidate positions
    candidates = get_adjacent_positions(board, size, radius=2)
    
    if not candidates:
        return []
    
    # Score moves by criticality
    scored_moves = []
    
    for row, col in candidates:
        # Check current player moves
        current_score = is_critical_move(board, row, col, current_player, size)
        
        # Check opponent moves (defensive)
        opponent_score = is_critical_move(board, row, col, opponent, size)
        
        # Combined score with priority for winning moves
        total_score = current_score * 1000 + opponent_score * 100
        
        if total_score > 0:
            scored_moves.append((row, col, total_score))
    
    # Sort by score and return best moves
    scored_moves.sort(key=lambda x: x[2], reverse=True)
    best_moves = [move[:2] for move in scored_moves[:max_moves]]
    
    # Cache the result
    move_cache[board_key] = (best_moves, current_player)
    
    return best_moves if best_moves else candidates[:max_moves]

def clone_game_fast(game):
    """Optimized game cloning"""
    new_game = game.__class__()
    new_game.board_size = game.board_size
    new_game.cell_size = game.cell_size
    new_game.board = [row[:] for row in game.board]  # List comprehension is faster
    new_game.taken_stones = game.taken_stones[:]
    new_game.current_player = game.current_player
    new_game.rule_center_opening = game.rule_center_opening
    new_game.rule_no_double_threes = game.rule_no_double_threes
    new_game.rule_captures = game.rule_captures
    return new_game

def minmax_optimized(game, depth, alpha, beta, maximizing_player, visualize=False, main_game=None, last_move=None, previous_score=None):
    """Ultra-optimized minimax with aggressive pruning"""
    # Manage transposition table size
    if len(transposition_table) > MAX_TT_SIZE:
        clear_transposition_table()
    
    # Transposition table lookup
    state_key = board_hash(game)
    if state_key in transposition_table:
        stored_depth, stored_score, stored_move = transposition_table[state_key]
        if stored_depth >= depth:
            return stored_score, stored_move
    
    # Terminal node check
    if depth == 0 or game_over(game, last_move):
        score = evaluate_fast(game, last_move, previous_score)
        transposition_table[state_key] = (depth, score, None)
        return score, None
    
    # Generate moves with adaptive count based on depth
    move_count = max(6, 12 - depth)  # Fewer moves at deeper levels
    moves = generate_smart_moves(game, max_moves=move_count)
    
    if not moves:
        score = evaluate_fast(game, last_move, previous_score)
        transposition_table[state_key] = (depth, score, None)
        return score, None
    
    best_move = moves[0]
    
    if maximizing_player:
        max_eval = -float('inf')
        
        for move in moves:
            i, j = move
            
            new_game = clone_game_fast(game)
            captured, error = new_game.place_stone(i, j)
            if error:
                continue
            
            if visualize and main_game is not None:
                main_game.show_visual_move(i, j, color="blue")
            
            eval_score, _ = minmax_optimized(new_game, depth - 1, alpha, beta, False, 
                                           visualize, main_game, (i, j), None)
            
            if visualize and main_game is not None:
                main_game.remove_visual_move(i, j)
            
            if eval_score > max_eval:
                max_eval = eval_score
                best_move = move
                if visualize and main_game is not None:
                    main_game.show_visual_move(i, j, color="red")
            
            alpha = max(alpha, eval_score)
            if beta <= alpha:  # Alpha-beta pruning
                break
        
        result = (max_eval, best_move)
    else:
        min_eval = float('inf')
        
        for move in moves:
            i, j = move
            
            new_game = clone_game_fast(game)
            captured, error = new_game.place_stone(i, j)
            if error:
                continue
            
            if visualize and main_game is not None:
                main_game.show_visual_move(i, j, color="blue")
            
            eval_score, _ = minmax_optimized(new_game, depth - 1, alpha, beta, True, 
                                           visualize, main_game, (i, j), None)
            
            if visualize and main_game is not None:
                main_game.remove_visual_move(i, j)
            
            if eval_score < min_eval:
                min_eval = eval_score
                best_move = move
                if visualize and main_game is not None:
                    main_game.show_visual_move(i, j, color="red")
            
            beta = min(beta, eval_score)
            if beta <= alpha:  # Alpha-beta pruning
                break
        
        result = (min_eval, best_move)
    
    # Store in transposition table
    transposition_table[state_key] = (depth, result[0], result[1])
    
    if visualize and main_game is not None:
        main_game.clear_visual_markers_by_color("red")
    
    return result

def minmax(game, depth, alpha, beta, maximizing_player, visualize=False, main_game=None, last_move=None, previous_score=None):
    """Main minimax entry point"""
    return minmax_optimized(game, depth, alpha, beta, maximizing_player, visualize, main_game, last_move, previous_score)