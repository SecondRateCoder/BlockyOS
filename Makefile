ASM nasm
BUILD_DIR Source/Build/OS
IOS_BUILD_DIR Source/Build/Programs
COS_DIR Source/Core-OS
IOS_DIR Source/Intermediary-OS

mkd

# Build to Floppy Image.
$(BUILD_DIR)/Main.bin: $/(SRC)/Boot/Boot.asm
	cp $(BUILD_DIR)/main.bin $(BUILD_DIR)/main_floppy.img	
	truncate -s 1440k $(BUILD_DIR)/main_floppy.img

# Build Core-OS.
$(BUILD_DIR)/main.bin: $(COS_DIR)/Boot/Boot.asm
	$(ASM) $(COS_DIR)/Boot/Boot.asm -f bin -o $(BUILD_DIR)/COS.bin

# Build Intermediary-OS
$(BUILD_DIR)/main.bin: $(IOS_DIR)/Boot/Boot.asm
	$(ASM) $(IOS_DIR)/Boot/Boot.asm -f bin -o $(BUILD_DIR)/IOS.bin