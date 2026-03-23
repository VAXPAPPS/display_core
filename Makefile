CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -Iinclude -I. $(shell pkg-config --cflags gtk+-3.0 gio-2.0 xrandr x11)
LDLIBS := $(shell pkg-config --libs gtk+-3.0 gio-2.0 xrandr x11) -lm

TARGET := display-settings
THEME_DAEMON_TARGET := vaxp-theme-daemon
THEME_TESTER_TARGET := vaxp-theme-tester
SRC := $(wildcard src/*.c src/app/*.c src/domain/*.c src/services/*.c src/ui/*.c src/ui/pages/*.c)
BUILD_DIR := build
OBJ := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC))

.PHONY: all clean run theme-daemon theme-client-check theme-tester

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

$(THEME_DAEMON_TARGET): vaxp_theme_daemon.c vaxp_theme_protocol.h
	$(CC) -Wall -Wextra -Wpedantic -std=c11 -O2 vaxp_theme_daemon.c \
		$$(pkg-config --cflags --libs glib-2.0 gio-2.0 x11) \
		-o $(THEME_DAEMON_TARGET)

$(THEME_TESTER_TARGET): vaxp_theme_tester.c
	$(CC) -Wall -Wextra -Wpedantic -std=c11 vaxp_theme_tester.c \
		$$(pkg-config --cflags --libs gtk+-3.0 gio-2.0) \
		-o $(THEME_TESTER_TARGET)

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

theme-daemon: $(THEME_DAEMON_TARGET)

theme-client-check: vaxp_theme_client.c vaxp_theme_client.h vaxp_theme_protocol.h
	$(CC) -Wall -Wextra -Wpedantic -std=c11 -c vaxp_theme_client.c \
		$$(pkg-config --cflags gio-2.0) \
		-o /tmp/vaxp_theme_client_check.o

theme-tester: $(THEME_TESTER_TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(THEME_DAEMON_TARGET) $(THEME_TESTER_TARGET)
