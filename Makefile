SOURCE = src
ASM = nasm
CC = gcc
BUILD_DIR = Build/OS
BOOT_DIR = $(SOURCE)/Boot

always: all

# List your C source files here
# C_SOURCES = $(wildcard $(OS_DIR)/*.c)
# C_OBJECTS = $(patsubst $(OS_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))

# # Floppy image
# FLOPPY_IMG = $(BUILD_DIR)/main_floppy.img

# # Ensure build directory exists
# $(BUILD_DIR):
# 	mkdir -p $(BUILD_DIR)

# # Compile C files
# $(BUILD_DIR)/%.o: $(OS_DIR)/%.c | $(BUILD_DIR)
# 	$(CC) -c $< -o $@

# # Assemble bootloader
# $(BOOT_BIN): $(BOOT_ASM) | $(BUILD_DIR)
# 	$(ASM) $< -f bin -o $@

# # Link object files into a flat binary (using a linker script)
# $(BUILD_DIR)/kernel.bin: $(C_OBJECTS)
# 	ld -T linker.ld -o $@ $(C_OBJECTS)

# # Concatenate bootloader and kernel
# $(BUILD_DIR)/main.bin: $(BOOT_BIN) $(BUILD_DIR)/kernel.bin
# 	cat $(BOOT_BIN) $(BUILD_DIR)/kernel.bin > $@

# Create floppy image
$(FLOPPY_IMG): $(BUILD_DIR)/main.bin
	cp $< $@
	truncate -s 1440k $@

# Default target
all: $(FLOPPY_IMG)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean