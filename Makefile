# Adapted very minorly from https://makefiletutorial.com/

TARGET_EXEC := ccask

BUILD_DIR := ./build
SRC_DIRS := ./src

SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CC=gcc
CFLAGS=-std=c99 -Werror $(INC_FLAGS) -MMD -MP

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

.PHONY: style
style:
	astyle --style=java --recursive $(SRC_DIRS)/*.c,*.h

debug: CFLAGS += -g
debug: $(BUILD_DIR)/$(TARGET_EXEC)


-include $(DEPS)
