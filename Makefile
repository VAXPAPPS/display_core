CC := gcc
WAYLAND_SCANNER := wayland-scanner

# ملف بروتوكول WLR Output Management
WLR_PROTOCOL_XML  := wlr-output-management-unstable-v1.xml
WLR_PROTOCOL_H    := include/services/wlr-output-management-unstable-v1-client-protocol.h
WLR_PROTOCOL_C    := src/services/wlr-output-management-unstable-v1-protocol.c

CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -Iinclude -I. \
          $(shell pkg-config --cflags gtk+-3.0 gio-2.0 xrandr x11 libpulse wayland-client)
LDLIBS := $(shell pkg-config --libs gtk+-3.0 gio-2.0 xrandr x11 libpulse wayland-client) -lm

TARGET := settingsx
THEME_DAEMON_TARGET := vaxp-theme-daemon
THEME_TESTER_TARGET := vaxp-theme-tester
SRC := $(sort $(wildcard src/*.c src/app/*.c src/domain/*.c src/services/*.c src/ui/*.c src/ui/pages/*.c) \
       $(WLR_PROTOCOL_C))
BUILD_DIR := build
OBJ := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC))

.PHONY: all clean run theme-daemon theme-client-check theme-tester generate-protocol

all: $(WLR_PROTOCOL_H) $(WLR_PROTOCOL_C) $(TARGET)

generate-protocol: $(WLR_PROTOCOL_H) $(WLR_PROTOCOL_C)

$(WLR_PROTOCOL_H): $(WLR_PROTOCOL_XML)
	@mkdir -p $(dir $@)
	$(WAYLAND_SCANNER) client-header $< $@

$(WLR_PROTOCOL_C): $(WLR_PROTOCOL_XML)
	@mkdir -p $(dir $@)
	$(WAYLAND_SCANNER) private-code $< $@

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

$(BUILD_DIR)/%.o: src/%.c $(WLR_PROTOCOL_H)
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
	rm -rf $(BUILD_DIR) $(TARGET) $(THEME_DAEMON_TARGET) $(THEME_TESTER_TARGET) \
	       $(WLR_PROTOCOL_H) $(WLR_PROTOCOL_C)
