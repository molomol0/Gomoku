# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O3 -march=native -flto -I includes
DEBUG_FLAGS = -g -DDEBUG -O0
LIBS = -lSDL2 -lm

# Directories
SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
ANALYZE_DIR = analyze

# Source files
SRCS = $(addprefix $(SRC_DIR)/, main.c game.c ai.c graphics.c)
OBJECTS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
NAME = gomoku

# Default target
all: $(NAME)

# Create directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(ANALYZE_DIR):
	mkdir -p $(ANALYZE_DIR)

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link executable
$(NAME): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LIBS)
	@echo "Build complete: $(NAME)"

# Clean build files but keep executable
clean:
	rm -rf $(BUILD_DIR) $(ANALYZE_DIR)
	@echo "Clean complete"

# Clean everything including executable
fclean: clean
	rm -f $(NAME)
	@echo "Full clean complete"

# Rebuild everything
re: fclean all

# Static analysis with profiling
analyze:
	mkdir -p $(ANALYZE_DIR)
	$(CC) -pg $(SRCS) -o $(NAME) $(LIBS) -I includes
	./$(NAME)
	mv gmon.out $(ANALYZE_DIR)/
	gprof $(NAME) $(ANALYZE_DIR)/gmon.out > $(ANALYZE_DIR)/profile.txt
	@echo "Analysis saved to $(ANALYZE_DIR)/profile.txt"

# Memory check
memcheck: CFLAGS += $(DEBUG_FLAGS)
memcheck: $(NAME)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(NAME)

# Show help
help:
	@echo "Available targets:"
	@echo "  all      - Build the game (default)"
	@echo "  clean    - Remove build files"
	@echo "  fclean   - Remove build files and executable"
	@echo "  re       - Rebuild everything"
	@echo "  analyze  - Build with profiling and generate analysis"
	@echo "  memcheck - Run with valgrind memory checking"
	@echo "  help     - Show this help message"

# Phony targets
.PHONY: all clean fclean re analyze memcheck help

# Dependencies (auto-generated)
-include $(OBJECTS:.o=.d)

# Generate dependency files
$(OBJ_DIR)/%.d: %.c | $(OBJ_DIR)
	@$(CC) $(CFLAGS) -MM -MT $(OBJ_DIR)/$*.o $< > $@