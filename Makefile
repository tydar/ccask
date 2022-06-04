# Adapted very minorly from https://makefiletutorial.com/

TARGET_EXEC := ccask
TEST_EXEC := ccask_test

BUILD_DIR := ./build
SRC_DIRS := ./src

main=./build/./src/main.c.o
test=./build/./src/test/test.c.o

SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

MAIN_OBJS := $(filter-out $(test),$(OBJS)) 
TEST_OBJS := $(filter-out $(main),$(OBJS)) 

DEPS := $(MAIN_OBJS:.o=.d)
TEST_DEPS := $(TEST_OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CC=gcc
CFLAGS=-std=c99 -Werror $(INC_FLAGS) -MMD -MP

$(BUILD_DIR)/$(TARGET_EXEC): $(MAIN_OBJS)
	$(CC) $(MAIN_OBJS) -o $@ $(LDFLAGS)

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

build-test: CFLAGS += -g
build-test: $(TEST_OBJS)
	$(CC) $(TEST_OBJS) -o $(BUILD_DIR)/$(TEST_EXEC) $(LDFLAGS)

-include $(DEPS) $(TEST_DEPS)
