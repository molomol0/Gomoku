import copy

transposition_table = {}

def board_hash(game):
    board = tuple(tuple(row) for row in game.board)
    return (board, tuple(game.taken_stones), game.current_player)

def game_over(game):
    if any(taken >= 10 for taken in game.taken_stones):
        return True

    directions = [(0, 1), (1, 0), (1, 1), (-1, 1)]

    for i in range(game.board_size):
        for j in range(game.board_size):
            color = game.board[i][j]
            if color == 0:
                continue
            for dx, dy in directions:
                if all(
                    0 <= i + dx * k < game.board_size and
                    0 <= j + dy * k < game.board_size and
                    game.board[i + dx * k][j + dy * k] == color
                    for k in range(5)
                ):
                    return True
    return False

def evaluate(game):
    score = 0
    player = 3 - game.current_player
    opponent = 3 - player
    board = game.board
    size = game.board_size

    directions = [(1, 0), (0, 1), (1, 1), (1, -1)]

    for i in range(size):
        for j in range(size):
            for dx, dy in directions:
                stones = []
                valid = True
                for k in range(5):
                    ni = i + dx * k
                    nj = j + dy * k
                    if not (0 <= ni < size and 0 <= nj < size):
                        valid = False
                        break
                    stones.append(board[ni][nj])
                if not valid:
                    continue


                if stones.count(player) == 5:
                    return float('inf')
                elif stones.count(opponent) == 5:
                    return -float('inf')
                elif stones.count(player) == 4 and stones.count(0) == 1:
                    score += 1000
                elif stones.count(player) == 3 and stones.count(0) == 2:
                    score += 100
                elif stones.count(player) == 2 and stones.count(0) == 3:
                    score += 10
                elif stones.count(opponent) == 4 and stones.count(0) == 1:
                    score -= 1000
                elif stones.count(opponent) == 3 and stones.count(0) == 2:
                    score -= 100
                elif stones.count(opponent) == 2 and stones.count(0) == 3:
                    score -= 10

    score += (game.taken_stones[player - 1] - game.taken_stones[opponent - 1]) * 500
    return score

def generate_candidates(game):
    candidates = set()
    directions = [(-1, -1), (-1, 0), (-1, 1),
                  (0, -1),          (0, 1),
                  (1, -1),  (1, 0),  (1, 1)]

    for x in range(game.board_size):
        for y in range(game.board_size):
            if game.board[x][y] != 0:
                for dx, dy in directions:
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < game.board_size and 0 <= ny < game.board_size:
                        if game.board[nx][ny] == 0:
                            candidates.add((nx, ny))
    return candidates

def clone_game(game):
    new_game = game.__class__()
    new_game.board = [row[:] for row in game.board]
    new_game.taken_stones = list(game.taken_stones)
    new_game.current_player = game.current_player
    new_game.board_size = game.board_size  # <-- OBLIGATOIRE
    new_game.cell_size = game.cell_size  # <-- OBLIGATOIRE
    return new_game


def generate_all_boards(game):
    possible_games = []
    size = game.board_size
    board = game.board

    valid_moves = set()
    # Check if the board is empty
    if all(board[i][j] == 0 for i in range(size) for j in range(size)):
        # Only allow the center move
        center = size // 2
        valid_moves.add((center, center))
    else:
        for i in range(size):
            for j in range(size):
                if board[i][j] != 0:
                    for dx in range(-2, 3):
                        for dy in range(-2, 3):
                            ni, nj = i + dx, j + dy
                            if 0 <= ni < size and 0 <= nj < size and board[ni][nj] == 0:
                                valid_moves.add((ni, nj))

    scored_games = []
    for i, j in valid_moves:
        new_game = clone_game(game)
        captured, error = new_game.place_stone(i, j)
        if error:
            continue
        score = evaluate(new_game)
        scored_games.append((score, new_game, (i, j)))

    # Trie par score décroissant
    scored_games.sort(key=lambda x: x[0], reverse=True)

    # Ne garde que les 10 meilleurs coups
    scored_games = scored_games[:10]

    # Retourne la liste dans le format attendu
    possible_games = [(g, m) for _, g, m in scored_games]
    return possible_games


def minmax(game, depth, alpha, beta, maximizing_player):
    state_key = (depth, maximizing_player, board_hash(game))
    if state_key in transposition_table:
        return transposition_table[state_key]

    if depth == 0 or game_over(game):
        eval_result = evaluate(game), None
        transposition_table[state_key] = eval_result
        return eval_result

    best_move = None
    all_boards = generate_all_boards(game)
    if not all_boards:
        # No moves available, return evaluation
        eval_result = evaluate(game), None
        transposition_table[state_key] = eval_result
        return eval_result

    if maximizing_player:
        max_eval = -float('inf')
        for new_game, move in all_boards:
            eval_score, _ = minmax(new_game, depth - 1, alpha, beta, False)
            if eval_score > max_eval:
                max_eval = eval_score
                best_move = move
            alpha = max(alpha, eval_score)
            if beta <= alpha:
                break
        result = (max_eval, best_move)
    else:
        min_eval = float('inf')
        for new_game, move in all_boards:
            eval_score, _ = minmax(new_game, depth - 1, alpha, beta, True)
            if eval_score < min_eval:
                min_eval = eval_score
                best_move = move
            beta = min(beta, eval_score)
            if beta <= alpha:
                break
        result = (min_eval, best_move)

    transposition_table[state_key] = result
    return result