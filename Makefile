.POSIX:

BUILD_DIR := build
SRC_DIR := src
INC_DIR := inc

PREFIX := /usr/local
CFLAGS := -Wall -Ofast -I. -I$(INC_DIR)
LDLIBS := -lX11

BIN := dwmblocks
VPATH := $(SRC_DIR)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c))
OBJS += $(patsubst %.c,$(BUILD_DIR)/%.o,$(wildcard *.c))

all: $(BUILD_DIR)/$(BIN)

$(BUILD_DIR)/$(BIN): $(OBJS)
	$(LINK.o) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c config.h | $(BUILD_DIR)
	$(COMPILE.c) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	$(RM) -r $(BUILD_DIR)

install: $(BUILD_DIR)/$(BIN)
	install -D -m 755 $< $(DESTDIR)/$(PREFIX)/bin/$(BIN)

uninstall:
	$(RM) $(DESTDIR)/$(PREFIX)/bin/$(BIN)

.PHONY: all clean install uninstall
