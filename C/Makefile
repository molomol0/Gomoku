# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O3 -march=native -flto -I includes -I$(HOME)/local/include/SDL2 -I/usr/include/SDL2
LIBS = -L$(HOME)/local/lib -lSDL2 -lSDL2_ttf -lm
DEBUG_FLAGS = -g -DDEBUG -O0

# Directories
SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
ANALYZE_DIR = analyze

# Source files
SRCS = $(addprefix $(SRC_DIR)/, main.c game.c graphics.c ai.c)
OBJECTS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
NAME = gomoku

# Image Docker
DOCKER_IMAGE = gomoku-sdl

# Default target: build only
all: $(NAME)

# Create directories
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/AI

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

# Link executable
$(NAME): $(OBJECTS)
	@$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LIBS)
	@echo "Build complete: $(NAME)"

# Run the game
run: $(NAME)
	@echo "Launching game..."
	@./$(NAME)

# Clean build files but keep executable
clean:
	@rm -rf $(BUILD_DIR) $(ANALYZE_DIR)
	@echo "Clean complete"

# Clean everything including executable
fclean: clean
	@rm -f $(NAME)
	@echo "Full clean complete"

# Rebuild everything
re: fclean all

# Static analysis with profiling
analyze:
	@mkdir -p $(ANALYZE_DIR)
	@$(CC) $(CFLAGS) -pg $(SRCS) -o $(NAME) $(LIBS) -I includes
	@./$(NAME)
	@mv gmon.out $(ANALYZE_DIR)/
	@gprof $(NAME) $(ANALYZE_DIR)/gmon.out > $(ANALYZE_DIR)/profile.txt
	@echo "Analysis saved to $(ANALYZE_DIR)/profile.txt"

# Construction de l'image
docker-build:
	docker build -t $(DOCKER_IMAGE) .

# Lancer le jeu dans un conteneur Docker avec X11 (version corrigée)
docker-run:
	xhost +local:docker
	docker run -it --rm \
		--net=host \
		--ipc=host \
		--pid=host \
		-e DISPLAY=$$DISPLAY \
		-e XAUTHORITY=$$XAUTHORITY \
		-e LIBGL_ALWAYS_SOFTWARE=1 \
		-e SDL_VIDEO_X11_DISABLE_SHM=1 \
		-e QT_X11_NO_MITSHM=1 \
		-e SDL_VIDEODRIVER=x11 \
		-v /tmp/.X11-unix:/tmp/.X11-unix:rw \
		-v $$HOME/.Xauthority:$$HOME/.Xauthority:ro \
		--device /dev/dri \
		$(DOCKER_IMAGE)

# Alternative avec isolation réseau (plus sécurisé)
docker-run-secure:
	xhost +local:docker
	docker run -it --rm \
		--shm-size=1g \
		-e DISPLAY=$$DISPLAY \
		-e LIBGL_ALWAYS_SOFTWARE=1 \
		-e SDL_VIDEO_X11_DISABLE_SHM=1 \
		-e SDL_VIDEODRIVER=x11 \
		-v /tmp/.X11-unix:/tmp/.X11-unix:rw \
		--device /dev/dri \
		$(DOCKER_IMAGE)

# Construire et lancer
docker: docker-build docker-run

# Memory check
memcheck: CFLAGS += $(DEBUG_FLAGS)
memcheck: $(NAME)
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(NAME)

# Show help
help:
	@echo "Cibles disponibles :"
	@echo " all                - Compiler le projet localement"
	@echo " run                - Lancer le jeu localement"
	@echo " clean              - Supprimer les fichiers compilés"
	@echo " fclean             - Supprimer aussi l'exécutable"
	@echo " re                 - Recompiler"
	@echo " docker             - Build + Run (version standard)"
	@echo " docker-build       - Construire l'image Docker"
	@echo " docker-run         - Lancer le jeu (version avec --net=host)"
	@echo " docker-run-secure  - Lancer le jeu (version plus sécurisée)"

# Phony targets
.PHONY: all run clean fclean re analyze memcheck help docker-run-secure

# Dependencies (auto-generated)
-include $(OBJECTS:.o=.d)

# Generate dependency files
$(OBJ_DIR)/%.d: %.c | $(OBJ_DIR)
	@$(CC) $(CFLAGS) -MM -MT $(OBJ_DIR)/$*.o $< > $@