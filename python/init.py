import copy
import time

class GomokuGame:
    def __init__(self):
        self.board_size = 19
        self.cell_size = 40
        self.board = [[0 for _ in range(self.board_size)] for _ in range(self.board_size)]  # 0=empty, 1=black, 2=white
        self.current_player = 1  # 1=black, 2=white
        self.taken_stones = [0, 0] # first stone taken by black, second is by white
        self.stone_ids = [[None for _ in range(self.board_size)] for _ in range(self.board_size)]
        self.visual_markers = {}
        self.canvas = None  # Placeholder for the Tkinter canvas, to be set later
        self.root = None  # Placeholder for the Tkinter root, to be set later
        # Rules
        self.rule_center_opening = True
        self.rule_no_double_threes = True
        self.rule_captures = True
    
    def __deepcopy__(self, memo):
        # Only copy the game state, not GUI objects
        new_game = GomokuGame()
        new_game.board_size = self.board_size
        new_game.cell_size = self.cell_size
        new_game.board = copy.deepcopy(self.board, memo)
        new_game.current_player = self.current_player
        new_game.taken_stones = copy.deepcopy(self.taken_stones, memo)
        # Do NOT copy stone_ids or any Tkinter objects
        new_game.rule_center_opening = self.rule_center_opening
        new_game.rule_no_double_threes = self.rule_no_double_threes
        new_game.rule_captures = self.rule_captures
        return new_game
    
    def show_visual_move(self, i, j, color="blue"):
        if self.canvas is None:
            return
        x = self.cell_size + j * self.cell_size
        y = self.cell_size + i * self.cell_size
        radius = self.cell_size // 5
        marker = self.canvas.create_oval(
            x - radius, y - radius, x + radius, y + radius,
            fill=color, outline=""
        )
        if (i, j, color) not in self.visual_markers:
            self.visual_markers[(i, j, color)] = []
        self.visual_markers[(i, j, color)].append(marker)
        self.root.update()  # Update the GUI to show the marker immediately
        # time.sleep(0.05)
    
    def clear_visual_markers_by_color(self, color):
        if self.canvas is None:
            return
        keys_to_remove = []
        for (i, j, marker_color), markers in self.visual_markers.items():
            if marker_color == color:
                for marker in markers:
                    self.canvas.delete(marker)
                keys_to_remove.append((i, j, marker_color))
        for key in keys_to_remove:
            del self.visual_markers[key]
        self.root.update()


    def remove_visual_move(self, i, j):
        if self.canvas is None:
            return
        for color in ("blue", "red"):
            markers = self.visual_markers.pop((i, j, color), None)
            if markers:
                for marker in markers:
                    self.canvas.delete(marker)
        self.root.update()  # Update the GUI to remove the marker immediately


    def capture_stone(self, row, col):
        directions = [
            (0, 1), (1, 0), (1, 1), (-1, 1),
            (0, -1), (-1, 0), (-1, -1), (1, -1),
        ]
        captured = []
        player = self.board[row][col]
        opponent = 2 if player == 1 else 1

        for dy, dx in directions:
            # Look backward from the placed stone
            r0 = row - 3 * dy
            c0 = col - 3 * dx
            r1 = row - 2 * dy
            c1 = col - 2 * dx
            r2 = row - 1 * dy
            c2 = col - 1 * dx

            if all(0 <= r < self.board_size and 0 <= c < self.board_size for r, c in [(r0, c0), (r1, c1), (r2, c2)]):
                if (
                    self.board[r0][c0] == player and
                    self.board[r1][c1] == opponent and
                    self.board[r2][c2] == opponent
                ):
                    self.board[r1][c1] = 0
                    self.board[r2][c2] = 0
                    captured.extend([(r1, c1), (r2, c2)])

            # Look forward from the placed stone
            r0 = row + 3 * dy
            c0 = col + 3 * dx
            r1 = row + 2 * dy
            c1 = col + 2 * dx
            r2 = row + 1 * dy
            c2 = col + 1 * dx

            if all(0 <= r < self.board_size and 0 <= c < self.board_size for r, c in [(r0, c0), (r1, c1), (r2, c2)]):
                if (
                    self.board[r0][c0] == player and
                    self.board[r1][c1] == opponent and
                    self.board[r2][c2] == opponent
                ):
                    self.board[r1][c1] = 0
                    self.board[r2][c2] = 0
                    captured.extend([(r1, c1), (r2, c2)])

        return captured

    def count_open_threes_in_line(self, row, col, dr, dc, player):
        line = []
        for i in range(-5, 6):
            nr = row + i * dr
            nc = col + i * dc
            if 0 <= nr < self.board_size and 0 <= nc < self.board_size:
                line.append(self.board[nr][nc])
            else:
                line.append(-1)

        total = 0
        for i in range(len(line) - 5):
            window = line[i:i+6]

            # All free three patterns
            patterns = [
                [0, player, player, player, 0, 0],
                [0, 0, player, player, player, 0],
                [0, player, player, 0, player, 0],
                [0, player, 0, player, player, 0],
                [0, player, player, player, 0, -1],
                [-1, 0, player, player, player, 0]
            ]
            for pattern in patterns:
                match = True
                for a, b in zip(window, pattern):
                    if b == -1:
                        continue
                    if a != b:
                        match = False
                        break
                if match:
                    total += 1
                    break

        return total

    def is_double_free_three(self, row, col, player):
        directions = [(0, 1), (1, 0), (1, 1), (1, -1)]
        open_three_directions = 0

        self.board[row][col] = player  # Simule le coup

        for dr, dc in directions:
            count = self.count_open_threes_in_line(row, col, dr, dc, player)
            if count >= 1:
                open_three_directions += 1
            if open_three_directions >= 2:
                self.board[row][col] = 0
                return True

        self.board[row][col] = 0
        return False



    def place_stone(self, row, col):
        if self.board[row][col] != 0:
            return False, "Cell already occupied"  # Already occupied

        if self.rule_center_opening and self.current_player == 1 and all(cell == 0 for row_ in self.board for cell in row_):
            if (row, col) != (self.board_size // 2, self.board_size // 2):
                return False, "Black must place the first stone in the center."

        if self.rule_no_double_threes and self.is_double_free_three(row, col, self.current_player):
            return False, "Placing this stone creates a double free three."

        self.board[row][col] = self.current_player

        captured = []
        if self.rule_captures:
            captured = self.capture_stone(row, col)

        self.current_player = 3 - self.current_player
        self.stone_ids[row][col] = (row, col)
        return captured, None
