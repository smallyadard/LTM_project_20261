CC      = gcc
CFLAGS  = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -g -I.
BUILD   = build

COMMON_SRC = $(wildcard common/*.c)
COMMON_OBJ = $(patsubst %.c,$(BUILD)/%.o,$(COMMON_SRC))

TEST_SRC = $(wildcard tests/test_*.c)
TEST_BIN = $(patsubst tests/%.c,$(BUILD)/%,$(TEST_SRC))

.PHONY: all test clean
all: $(COMMON_OBJ)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/test_%: tests/test_%.c $(COMMON_OBJ)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $< $(COMMON_OBJ) -o $@

test: $(TEST_BIN)
	@for t in $(TEST_BIN); do echo "== $$t"; ./$$t || exit 1; done

clean:
	rm -rf $(BUILD)
