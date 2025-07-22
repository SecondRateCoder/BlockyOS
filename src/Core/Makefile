SRC_DIR = ./src/Core
BUILD_DIR = ./Build/Core

C_SOURCES = $(shell find $(SRC_DIR) -name "*.c")

C_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(C_SOURCES)))
BUILD_DIRS = $(BUILD_DIR)

CC = gcc
CFLAGS = -Wall -g -m32 -ffreestanding -nostdlib -T linker.ld


TARGET = Blocky

.PHONY: all clean print_vars

all: $(BUILD_DIRS) $(TARGET)

$(BUILD_DIRS):
	@echo "Creating build directory: $@"
	@mkdir -p $@

$(TARGET): $(C_OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) $^ -o $@

define compile_rule
$(BUILD_DIR)/$(notdir $(1:.c=.o)): $(1)
	@echo "Compiling $< to $@"
	$(CC) $(CFLAGS) -c $< -o $@
endef
# gcc  program.c -o program.bin
$(foreach src, $(C_SOURCES), $(eval $(call compile_rule,$(src))))

clean:
	@echo "Cleaning build directory and target..."
	rm -r $(BUILD_DIR) $(TARGET)

print_vars:
	@echo "SRC_DIR: $(SRC_DIR)"
	@echo "BUILD_DIR: $(BUILD_DIR)"
	@echo "C_SOURCES: $(C_SOURCES)"
	@echo "C_OBJECTS: $(C_OBJECTS)"
	@echo "BUILD_DIRS: $(BUILD_DIRS)"