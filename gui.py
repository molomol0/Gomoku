import tkinter as tk
from init import GomokuGame
from gui_helper import erase_stone

game = GomokuGame()
root = tk.Tk()
main_frame = tk.Frame(root)
main_frame.pack()

canvas = tk.Canvas(main_frame, width=800, height=800, bg='lightyellow')
canvas.pack(side=tk.LEFT)

log_text = tk.Text(main_frame, width=40, height=47, state=tk.DISABLED, bg='black', fg='white')
log_text.pack(side=tk.RIGHT)

def log_message(message):
    log_text.config(state=tk.NORMAL)
    log_text.insert(tk.END, message + '\n')
    log_text.see(tk.END)  # Scroll automatiquement en bas
    log_text.config(state=tk.DISABLED)

for i in range(game.board_size):
    # Vertical lines
	canvas.create_line(game.cell_size, game.cell_size + i * game.cell_size, game.cell_size * game.board_size, game.cell_size + i * game.cell_size)
    # Horizontal lines
	canvas.create_line(game.cell_size + i * game.cell_size, game.cell_size, game.cell_size + i * game.cell_size, game.cell_size * game.board_size)

def draw_stone(row, col, color):
    x = game.cell_size + col * game.cell_size
    y = game.cell_size + row * game.cell_size
    r = game.cell_size // 2 - 2
    game.stone_ids[row][col] = canvas.create_oval(x - r, y - r, x + r, y + r, fill=color)

def end_game(winner):
	if winner == 1:
		winner_text = "Black wins!"
	elif winner == 2:
		winner_text = "White wins!"
	else:
		winner_text = "It's a draw!"
	canvas.create_text(400, 400, text=winner_text, font=("Arial", 24), fill="red")
	canvas.unbind("<Button-1>")

def print_board():
	board_str = ""
	for row in game.board:
		board_str += " ".join(str(cell) for cell in row) + "\n"
	log_message(board_str)

def click(event):
	x = event.x
	y = event.y
	row = round((y - game.cell_size) / game.cell_size)
	col = round((x - game.cell_size) / game.cell_size)
	# log_message(f"Clicked cell: {row}, {col}")
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
				erase_stone(game, canvas, r, c)
				game.taken_stones[game.current_player - 1] += 1
			winner = check_winner()
			if winner != 0:
				end_game(winner)
				return
		# print_board()

def check_winner():
	# if any player has taken 10 stones he/she wins
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

canvas.bind("<Button-1>", click)

root.mainloop()
