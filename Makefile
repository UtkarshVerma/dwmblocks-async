.POSIX:

BIN := dwmblocks
BUILD_DIR := build
SRC_DIR := src
INC_DIR := inc

VERBOSE := 0

PREFIX := /usr/local
CFLAGS := -Wall -Wextra -Ofast -I. -I$(INC_DIR)
CFLAGS += -Wall -Wextra -Wno-missing-field-initializers
LDLIBS := -lX11

VPATH := $(SRC_DIR)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c))
OBJS += $(patsubst %.c,$(BUILD_DIR)/%.o,$(wildcard *.c))

INSTALL_DIR := $(DESTDIR)$(PREFIX)/bin

# Prettify output
PRINTF := @printf "%-8s %s\n"
ifeq ($(VERBOSE), 0)
	Q := @
endif

all: $(BUILD_DIR)/$(BIN)

$(BUILD_DIR)/$(BIN): $(OBJS)
	$(PRINTF) "LD" $@
	$Q$(LINK.o) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c config.h | $(BUILD_DIR)
	$(PRINTF) "CC" $@
	$Q$(COMPILE.c) -o $@ $<

$(BUILD_DIR):
	$(PRINTF) "MKDIR" $@
	$Qmkdir -p $@

clean:
	$(PRINTF) "CLEAN" $(BUILD_DIR)
	$Q$(RM) $(BUILD_DIR)/*

install: $(BUILD_DIR)/$(BIN)
	$(PRINTF) "INSTALL" $(INSTALL_DIR)/$(BIN)
	$Qinstall -D -m 755 $< $(INSTALL_DIR)/$(BIN)

uninstall:
	$(PRINTF) "RM" $(INSTALL_DIR)/$(BIN)
	$Q$(RM) $(INSTALL_DIR)/$(BIN)

.PHONY: all clean install uninstall
