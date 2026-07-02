CC = gcc
CFLAGS = -Wall -Wextra -Werror -fsanitize=address -I./include
LDFLAGS = -fsanitize=address

# Lista explícita dos fontes
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

all: kvstore

# Regra genérica para compilar qualquer .c da pasta src para .o
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@


stress:
	$(CC) -Wall -Wextra -Werror -g -fsanitize=thread -pthread -I./include \
		-o run_stress $(SRC) tests/unit/*.c tests/stress/*.c tests/main.c
	DATA_PATH=./tests/data MEMTABLE_SIZE=1024 MAX_LOG_LEN=128 DEBUG_LOGS=0 ./run_stress --stress
	rm -rf run_stress ./tests/data


kvstore: $(OBJ) main.c
	$(CC) $(CFLAGS) -pthread  -o kvstore main.c $(OBJ) $(LDFLAGS)

test: $(OBJ)
	$(CC) $(CFLAGS) -fsanitize=address -g -o run_tests tests/unit/*.c tests/stress/*.c tests/main.c $(OBJ) $(LDFLAGS)
	DATA_PATH=./tests/data MEMTABLE_SIZE=1024 MAX_LOG_LEN=128 DEBUG_LOGS=1 ./run_tests
	rm -rf run_tests ./tests/data

clean:
	rm -f src/*.o kvstore run_tests
	rm -f *.dat L*/*.dat