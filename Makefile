CC = gcc
CFLAGS = -Wall -Wextra -Werror -I./include
LDFLAGS =

# Lista explícita dos fontes
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

all: kvstore

# Regra genérica para compilar qualquer .c da pasta src para .o
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kvstore: $(OBJ) main.c
	$(CC) $(CFLAGS) -o kvstore main.c $(OBJ) $(LDFLAGS)

test: $(OBJ)
	$(CC) $(CFLAGS) -o run_tests tests/unit/*.c tests/main.c $(OBJ) $(LDFLAGS)
	DATA_PATH=./tests/data MEMTABLE_SIZE=1024 ./run_tests
	rm -rf run_tests ./tests/data

clean:
	rm -f src/*.o kvstore run_tests
	rm -f *.dat L*/*.dat