CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic `pkg-config --cflags libpq` -g3 -fsanitize=undefined -fsanitize=address
LDFLAGS = `pkg-config --libs libpq` -fsanitize=undefined -fsanitize=address

TARGETS = displayorders addorder

SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c,build/%.o,$(SRC))

.PHONY: all debug clean

all: CFLAGS += -O3
all: $(TARGETS)

debug: CFLAGS += -g3 -fsanitize=undefined -fsanitize=address
debug: LDFLAGS += -fsanitize=undefined -fsanitize=address
debug: $(TARGETS)

# we assume that each executable has only one corresponding object file
define build_exec =
$(1): build/$(1).o
	$(CC) $$^ -o $$@ $(LDFLAGS)
endef

$(foreach target,$(TARGETS),$(eval $(call build_exec,$(target))))

$(EXEC): $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p $@

clean:
	rm -rf build $(EXEC)