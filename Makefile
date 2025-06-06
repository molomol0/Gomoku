NAME = gomoku

PYTHON = python3
SRC = gui.py
TIME = -m cProfile

all: $(NAME)

$(NAME):
	$(PYTHON) $(SRC)

complete_analyze:
	$(PYTHON) $(TIME) $(SRC) > analyze.txt
	@echo "Analysis complete, results saved in analyze.txt"

analyze:
	$(PYTHON) $(TIME) $(SRC) | awk 'BEGIN {skip=1} \
	/filename:lineno\(function\)/ {skip=0; header=$$0; next} \
	{if (!skip && $$2 >= 0.01) print $$0}' | sort -k2,2nr | awk -v h="    ncalls  tottime  percall  cumtime  percall filename:lineno(function)" 'BEGIN {print h} {print}' > analyze.txt
	@echo "Short sorted analysis complete, results saved in analyze.txt"

clean:
	@py3clean .
	@echo "pycache cleaned"

fclean: clean

re: fclean all