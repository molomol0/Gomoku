def erase_stone(game, canvas, row, col):
	# x = game.cell_size + col * game.cell_size
	# y = game.cell_size + row * game.cell_size
	# r = game.cell_size // 3 - 2
	# canvas.create_oval(x - r, y - r, x + r, y + r, fill="red", outline="red")
	if game.stone_ids[row][col] is not None:
		canvas.delete(game.stone_ids[row][col])
		game.stone_ids[row][col] = None