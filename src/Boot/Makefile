BUILD_DIR = ./Build
ASM = NASM

EXE_PATH = ./Build/Core/Blocky.exe

CURR_DIR = ./src/Boot/
BUILD_DIR = ./Build
FLOPPY_TARGET = executable.img

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(FLOPPY_TARGET):	$(BUILD_DIR)/Boot1.bin $(BUILD_DIR)/Boot2.bin $(EXE_PATH)
	cp $(BUILD_DIR)/Boot1.bin $(BUILD_DIR)/$(FLOPPY_TARGET)
	truncate -s 1440k $(BUILD_DIR)/$(FLOPPY_TARGET)
	mcopy -i $@ $< ::BOOT1.BIN
	mcopy -i $@ $< ::BOOT2.BIN
	mcopy -i $@ $(EXE_PATH) ::BLOCKY.EXE

$(EXE_PATH):
	make -C ./src/Core/Makefile all

$(BUILD_DIR)/Boot1.bin:	$(CURR_DIR)/Boot1/Boot.asm
	$(ASM) $(CURR_DIR)/Boot1/Boot.asm -f bin -o $(BUILD_DIR)/Boot1.bin

$(BUILD_DIR)/Boot2.bin:	$(CURR_DIR)/Boot2/Boot.asm
	$(ASM) $(CURR_DIR)/Boot2/Boot.asm -f bin -o $(BUILD_DIR)/Boot1.bin

all:	$(BUILD_DIR)/$(FLOPPY_TARGET)

clean:
	@echo "Cleaning build directory and target..."
	@rm -f $(FLOPPY_TARGET)
	@make -C ./src/Core/Makefile clean

.PHONY:	all clean