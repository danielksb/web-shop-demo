CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic `pkg-config --cflags libpq` -pthread
LDFLAGS = `pkg-config --libs libpq` -pthread

TARGETS = displayorders addorder echo_server shop_server client


SRC = $(wildcard src/*.c)
# Filter out source files that have the same base name as the targets
DEPENDENCIES := $(filter-out $(addprefix src/, $(addsuffix .c, $(TARGETS))), $(SRC))
OBJ = $(patsubst src/%.c,build/%.o,$(DEPENDENCIES))

.PHONY: all debug clean

all: CFLAGS += -O3
all: $(TARGETS)

debug: CFLAGS += -g3 -fsanitize=undefined -fsanitize=address
debug: LDFLAGS += -fsanitize=undefined -fsanitize=address
debug: $(TARGETS)

# we assume that each executable has only one corresponding object file
define build_exec =
$(1): build/$(1).o $(OBJ)
	$(CC) $$^ -o $$@ $$(LDFLAGS)
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