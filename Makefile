ASM nasm
BUILD_DIR Build
SRC Source

$(BUILD_DIR)/Main.bin: $/(SRC)/Boot/Boot.asm
	cp $(BUILD_DIR)/main.bin $(BUILD_DIR)/main_floppy.img	
	truncate -s 1440k $(BUILD_DIR)/main_floppy.img

$(BUILD_DIR)/main.bin: $(SRC)/Boot/Boot.asm
	$(ASM) $(SRC)/Boot/Boot.asm -f bin -o $(BUILD_DIR)/main.bin