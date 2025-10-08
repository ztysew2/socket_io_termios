PROJECT := learning

CC ?= gcc
CSTD ?= gnu11

CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=$(CC) -Wall -Wextra -Werror
LDFLAGS ?= 
LDLIBS ?= -lphread

SRC_DIR := src
BUILD_DIR := build

SRC :=$(wildcard $(SRC_DIR)/*.c)
OBJ :=$(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))
DEP :=$(OBJ:.o=.d)

.PHONY:all
all:$(PROJECT)

$(PROJECT):$(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

-include $(DEP)

.PHONY: run clean disctclean debug release print

run: $(PROJECT)
	./$(PROJECT)

clean:
	$(RM) -r $(BUILD_DIR)

distclean: clean
	$(RM) $(PROJECT)

debug: CFLAGS += -O0 -g
debug: CPPFLAGS += -DDEBUG
debug: all

release: CFLAGS += -O3 -DNDEBUG
release: all

printf:
	@echo SRC=$(SRC)
	@echo OBJ=$(OBJ)
	@echo DEP=$(DEP)
