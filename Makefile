
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic `pkg-config --cflags libpq`
LDFLAGS = `pkg-config --libs libpq`

ifeq ($(OS),Windows_NT)
    CFLAGS += -static
endif

EXEC = cshop

SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c,build/%.o,$(SRC))

.PHONY: all debug clean

all: CFLAGS += -O3
all: $(EXEC)

debug: CFLAGS += -g3 -fsanitize=undefined -fsanitize=address -static
debug: LDFLAGS += -fsanitize=undefined -fsanitize=address
debug: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p $@

clean:
	rm -rf cshop/build $(EXEC)
