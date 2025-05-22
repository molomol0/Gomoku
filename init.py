class GomokuGame:
    def __init__(self):
        self.board_size = 19
        self.cell_size = 40
        self.board = [[0 for _ in range(self.board_size)] for _ in range(self.board_size)]  # 0=empty, 1=black, 2=white
        self.current_player = 1  # 1=black, 2=white
        self.taken_stones = [0, 0] # first stone taken by black, second is by white
        self.stone_ids = [[None for _ in range(self.board_size)] for _ in range(self.board_size)]
        # Rules
        self.rule_center_opening = True
        self.rule_no_double_threes = True
        self.rule_captures = True
    

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
        total_open_threes = 0

        # Simule le coup
        self.board[row][col] = player

        for dr, dc in directions:
            total_open_threes += self.count_open_threes_in_line(row, col, dr, dc, player)

        self.board[row][col] = 0  # Annule le coup

        return total_open_threes >= 2


    def place_stone(self, row, col):
        if self.board[row][col] != 0:
            return False  # Already occupied

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
