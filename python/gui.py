import tkinter as tk
import time
from init import GomokuGame
from gui_helper import erase_stone
from minmax import minmax, get_ai_move

game = GomokuGame()
game.root = tk.Tk()
main_frame = tk.Frame(game.root)
main_frame.pack()

game.canvas = tk.Canvas(main_frame, width=800, height=850, bg='lightyellow')
game.canvas.pack(side=tk.LEFT)

temp_stone_id = None

log_text = tk.Text(main_frame, width=40, height=50, state=tk.DISABLED, bg='black', fg='white')
log_text.pack(side=tk.RIGHT)

def log_message(message):
    log_text.config(state=tk.NORMAL)
    log_text.insert(tk.END, message + '\n')
    log_text.see(tk.END)
    log_text.config(state=tk.DISABLED)

for i in range(game.board_size):
    # Vertical lines
    game.canvas.create_line(game.cell_size, game.cell_size + i * game.cell_size, game.cell_size * game.board_size, game.cell_size + i * game.cell_size)
    # Horizontal lines
    game.canvas.create_line(game.cell_size + i * game.cell_size, game.cell_size, game.cell_size + i * game.cell_size, game.cell_size * game.board_size)

def draw_stone(row, col, color):
    x = game.cell_size + col * game.cell_size
    y = game.cell_size + row * game.cell_size
    r = game.cell_size // 2 - 2
    game.stone_ids[row][col] = game.canvas.create_oval(x - r, y - r, x + r, y + r, fill=color)

def end_game(winner):
    if winner == 1:
        winner_text = "Black wins!"
    elif winner == 2:
        winner_text = "White wins!"
    else:
        winner_text = "It's a draw!"
    game.canvas.create_text(400, 400, text=winner_text, font=("Arial", 24), fill="red")
    log_message(winner_text)
    game.canvas.unbind("<Button-1>")

def print_board():
    board_str = ""
    for row in game.board:
        board_str += " ".join(str(cell) for cell in row) + "\n"
    log_message(board_str)

def draw_temp_stone(row, col):
    """Draw a small red stone at (row, col) and return its canvas id."""
    x = game.cell_size + col * game.cell_size
    y = game.cell_size + row * game.cell_size
    r = game.cell_size // 4  # smaller radius
    return game.canvas.create_oval(x - r, y - r, x + r, y + r, fill="red", outline="red")

def clone_game_simple(game):
    """Simple game cloning for AI calculation"""
    new_game = GomokuGame()
    new_game.board_size = game.board_size
    new_game.cell_size = game.cell_size
    new_game.board = [row[:] for row in game.board]
    new_game.taken_stones = game.taken_stones[:]
    new_game.current_player = game.current_player
    new_game.rule_center_opening = game.rule_center_opening
    new_game.rule_no_double_threes = game.rule_no_double_threes
    new_game.rule_captures = game.rule_captures
    return new_game

def click(event):
    global temp_stone_id
    x = event.x
    y = event.y
    row = round((y - game.cell_size) / game.cell_size)
    col = round((x - game.cell_size) / game.cell_size)
    
    if 0 <= row < game.board_size and 0 <= col < game.board_size:
        result, error = game.place_stone(row, col)
        if error:
            log_message(error)
            return
        
        captured = result
        if captured is not False:
            color = "black" if game.board[row][col] == 1 else "white"
            draw_stone(row, col, color)
            for r, c in captured:
                erase_stone(game, game.canvas, r, c)
                game.taken_stones[game.current_player - 1] += 1
            
            winner = check_winner()
            if winner != 0:
                end_game(winner)
                return
        
        # Only use AI for the second player
        if game.current_player == 2:
            # log_message(f"AI is thinking... (current player: {game.current_player})")
            game.canvas.unbind("<Button-1>")
            # AI move calculation with optimized algorithm
            start_time = time.time()
            
            # Use higher depth now that it's optimized
            search_depth = 5
            max_eval, best_move = minmax(clone_game_simple(game), search_depth, -float('inf'), float('inf'), True, visualize=True, main_game=game)
            # max_eval, best_move = get_ai_move(clone_game_simple(game), search_depth, visualize=True, main_game=game)
            
            elapsed_time = time.time() - start_time
            log_message(f"AI found move {best_move} (score: {max_eval}) in {elapsed_time:.2f}s at depth {search_depth}")
            
            # Erase previous temp stone if it exists
            if temp_stone_id is not None:
                game.canvas.delete(temp_stone_id)
                temp_stone_id = None

            # Draw new temp stone if best_move is valid
            if best_move is not None:
                temp_stone_id = draw_temp_stone(*best_move)
            game.canvas.bind("<Button-1>", click)

def check_winner():
    # Check if any player has taken 10 stones
    if game.taken_stones[0] >= 10 or game.taken_stones[1] >= 10:
        return 2 if game.taken_stones[0] >= 10 else 1
    
    # Check for five in a row
    for i in range(game.board_size):
        for j in range(game.board_size):
            if game.board[i][j] != 0:
                color = game.board[i][j]
                # Check horizontal
                if j + 4 < game.board_size and all(game.board[i][j + k] == color for k in range(5)):
                    return color
                # Check vertical
                if i + 4 < game.board_size and all(game.board[i + k][j] == color for k in range(5)):
                    return color
                # Check diagonal
                if i + 4 < game.board_size and j + 4 < game.board_size and all(game.board[i + k][j + k] == color for k in range(5)):
                    return color
                # Check reverse diagonal
                if i - 4 >= 0 and j + 4 < game.board_size and all(game.board[i - k][j + k] == color for k in range(5)):
                    return color
    return 0

game.canvas.bind("<Button-1>", click)

log_message("Gomoku AI - Optimized Version")
log_message("Click to place your stone. AI will respond automatically.")
log_message("Current search depth: 8")

game.root.mainloop()