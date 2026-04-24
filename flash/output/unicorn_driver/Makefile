#
# Makefile for AC780x EFLASH Driver Baremetal Test
# Target: ARM Cortex-M7 bare metal environment
#

# Toolchain configuration
CROSS_COMPILE = arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size

# Target configuration
TARGET = eflash_unicorn_test
MCU = cortex-m7

# Directory paths
BUILD_DIR = build
HAL_DIR = hal
DRIVER_DIR = driver
UNICORN_DIR = ../../../unicorn_env
OBJ_DIR = $(BUILD_DIR)

# Source files
STARTUP_SRC = $(UNICORN_DIR)/start.S
SYSTEM_SRC = $(wildcard $(UNICORN_DIR)/*.c)
DRIVER_SRC = $(wildcard $(HAL_DIR)/*.c) $(wildcard $(DRIVER_DIR)/*.c)
MAIN_SRC = multitask_eflash_unicorn.c

# All source files
SOURCES = $(STARTUP_SRC) $(SYSTEM_SRC) $(DRIVER_SRC) $(MAIN_SRC)

# Object files (derived from source files, placed in build directory)
OBJECTS = $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.o)))
OBJECTS := $(OBJECTS:.S=.o)

# Include directories
INCLUDES = -I. \
           -I../../../lib_drvintf \
		   -I$(UNICORN_DIR) \
           -I$(HAL_DIR) \
           -I$(DRIVER_DIR) \

# Compiler flags for Cortex-M7
CFLAGS = -mcpu=$(MCU) \
         -mthumb \
         -mfloat-abi=hard \
         -mfpu=fpv5-d16 \
         -O2 \
         -g \
         -Wall \
         -Wextra \
         -ffunction-sections \
         -fdata-sections \
         -std=c99 \
         $(INCLUDES)

# Assembler flags
ASFLAGS = -mcpu=$(MCU) \
          -mthumb \
          -mfloat-abi=hard \
          -mfpu=fpv5-d16 \
          -g

# Linker flags
LDFLAGS = -mcpu=$(MCU) \
          -mthumb \
          -mfloat-abi=hard \
          -mfpu=fpv5-d16 \
          -T $(LDSCRIPT) \
          -Wl,--gc-sections \
          -Wl,--print-memory-usage \
          -static \
          -specs=nosys.specs

# Linker script
LDSCRIPT = $(UNICORN_DIR)/unicorn_env.ld

# Default target
all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# Create build directory
$(BUILD_DIR):
	@echo "Creating build directory..."
	@mkdir -p $(BUILD_DIR)

# Build the ELF file
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT)
	@echo "Linking $(TARGET).elf..."
	$(CC) $(OBJECTS) $(LIB_DRVINTF) $(LDFLAGS) -o $@

# Generate HEX file for programming
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@echo "Creating $(TARGET).hex..."
	$(OBJCOPY) -O ihex $< $@

# Generate binary file
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@echo "Creating $(TARGET).bin..."
	$(OBJCOPY) -O binary $< $@
	$(OBJDUMP) -dxS $(BUILD_DIR)/$(TARGET).elf > $(BUILD_DIR)/Debuglist

# Show size information
size: $(BUILD_DIR)/$(TARGET).elf
	@echo "Size information:"
	$(SIZE) $(BUILD_DIR)/$(TARGET).elf

# Compile C source files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C source files from various directories
$(BUILD_DIR)/%.o: $(HAL_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(DRIVER_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(UNICORN_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly source files
$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	@echo "Assembling $<..."
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(UNICORN_DIR)/%.S | $(BUILD_DIR)
	@echo "Assembling $<..."
	$(CC) $(ASFLAGS) -c $< -o $@

# Generate disassembly listing
disasm: $(BUILD_DIR)/$(TARGET).elf
	@echo "Generating disassembly..."
	$(OBJDUMP) -dsX $(BUILD_DIR)/$(TARGET).elf > $(BUILD_DIR)/$(TARGET).lst

# Debug target - build with debug info and no optimization
debug: CFLAGS := $(subst -O2,-O0 -DDEBUG,$(CFLAGS))
debug: clean all

# Check if the cross-compiler is available
check-toolchain:
	@which $(CC) > /dev/null || (echo "Error: ARM cross-compiler not found. Please install arm-none-eabi-gcc." && exit 1)
	@echo "Toolchain check: OK"

# Check library dependencies
check-libs: $(LIB_DRVINTF)
	@echo "Library dependencies: OK"

$(LIB_DRVINTF):
	@echo "Error: Required library not found: $(LIB_DRVINTF)"
	@echo "Please ensure the lib_drvintf directory contains the required libraries."
	@exit 1

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)

# Create a memory usage report
memory-usage: $(BUILD_DIR)/$(TARGET).elf
	@echo "=== Memory Usage Report ==="
	@$(SIZE) -A $(BUILD_DIR)/$(TARGET).elf
	@echo ""
	@echo "=== Section Details ==="
	@$(OBJDUMP) -h $(BUILD_DIR)/$(TARGET).elf

# Programming targets (for various programming tools)
program-jlink: $(BUILD_DIR)/$(TARGET).hex
	@echo "Programming with J-Link..."
	@echo "Please connect J-Link programmer and run:"
	@echo "JLinkExe -device AC780x -if SWD -speed 4000 -autoconnect 1"
	@echo "Load file: $(BUILD_DIR)/$(TARGET).hex"

program-openocd: $(BUILD_DIR)/$(TARGET).hex
	@echo "Programming with OpenOCD..."
	@echo "openocd -f interface/stlink-v2.cfg -f target/ac780x.cfg -c \"program $(BUILD_DIR)/$(TARGET).hex verify reset exit\""

# Help target
help:
	@echo "Available targets:"
	@echo "  all           - Build ELF, HEX, and BIN files (default)"
	@echo "  debug         - Build with debug information and no optimization"
	@echo "  clean         - Remove all build artifacts (build directory)"
	@echo "  size          - Show memory usage information"
	@echo "  disasm        - Generate disassembly listing"
	@echo "  memory-usage  - Detailed memory usage report"
	@echo "  check-toolchain - Verify cross-compiler installation"
	@echo "  check-libs    - Check library dependencies"
	@echo "  program-jlink - Instructions for J-Link programming"
	@echo "  program-openocd - Instructions for OpenOCD programming"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Build output directory: $(BUILD_DIR)/"
	@echo "Target files:"
	@echo "  $(BUILD_DIR)/$(TARGET).elf - Executable with debug symbols"
	@echo "  $(BUILD_DIR)/$(TARGET).hex - Intel HEX format for programming"
	@echo "  $(BUILD_DIR)/$(TARGET).bin - Raw binary image"
	@echo "  $(BUILD_DIR)/$(TARGET).lst - Disassembly listing"

# Prevent make from removing intermediate files
.PRECIOUS: $(BUILD_DIR)/%.o

# Mark phony targets
.PHONY: all clean debug size disasm memory-usage check-toolchain check-libs program-jlink program-openocd help
