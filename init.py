class GomokuGame:
    def __init__(self):
        self.board_size = 19
        self.cell_size = 40
        self.board = [[0 for _ in range(self.board_size)] for _ in range(self.board_size)]  # 0=empty, 1=black, 2=white
        self.current_player = 1  # 1=black, 2=white

    def place_stone(self, row, col):
        if self.board[row][col] != 0:
            return False  # Already occupied
        self.board[row][col] = self.current_player
        self.current_player = 3 - self.current_player  # Toggle between 1 and 2
        return True
