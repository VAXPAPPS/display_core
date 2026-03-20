CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -Iinclude $(shell pkg-config --cflags gtk+-3.0 xrandr x11)
LDLIBS := $(shell pkg-config --libs gtk+-3.0 xrandr x11) -lm

TARGET := display-settings
SRC := $(wildcard src/*.c src/app/*.c src/domain/*.c src/services/*.c src/ui/*.c src/ui/pages/*.c)
BUILD_DIR := build
OBJ := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
