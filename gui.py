import tkinter as tk
from init import GomokuGame

game = GomokuGame()
root = tk.Tk()
root.title("Gomoku")
canvas = tk.Canvas(root, width=800, height=800, bg='lightyellow')
canvas.pack()

for i in range(game.board_size):
	print(f"Drawing line {i}")
    # Vertical lines
	canvas.create_line(game.cell_size, game.cell_size + i * game.cell_size, game.cell_size * game.board_size, game.cell_size + i * game.cell_size)
    # Horizontal lines
	canvas.create_line(game.cell_size + i * game.cell_size, game.cell_size, game.cell_size + i * game.cell_size, game.cell_size * game.board_size)

def draw_stone(row, col, color):
    x = game.cell_size + col * game.cell_size
    y = game.cell_size + row * game.cell_size
    r = game.cell_size // 2 - 2
    canvas.create_oval(x - r, y - r, x + r, y + r, fill=color)

def end_game(winner):
	if winner == 1:
		winner_text = "Black wins!"
	elif winner == 2:
		winner_text = "White wins!"
	else:
		winner_text = "It's a draw!"
	canvas.create_text(400, 400, text=winner_text, font=("Arial", 24), fill="red")
	canvas.unbind("<Button-1>")

def click(event):
	x = event.x
	y = event.y
	row = round((y - game.cell_size) / game.cell_size)
	col = round((x - game.cell_size) / game.cell_size)
	print(f"Clicked cell: {row}, {col}")
	if 0 <= row < game.board_size and 0 <= col < game.board_size:
		if game.place_stone(row, col):
			color = "black" if game.board[row][col] == 1 else "white"
			draw_stone(row, col, color)
			if (check_winner() != 0):
				end_game(check_winner())

def check_winner():
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
