# EFLASH AC780x C Driver Makefile
# This makefile builds the EFLASH Driver and Test components

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O0 -g
LDFLAGS =

# Directories
HAL_DIR = hal
DRIVER_DIR = driver
TEST_DIR = test

# Source files - Updated to match actual files
HAL_SOURCES = 
DRIVER_SOURCES = $(DRIVER_DIR)/ac780x_eflash.c
TEST_SOURCES = $(TEST_DIR)/multitask_eflash.c

# Object files
HAL_OBJECTS = $(HAL_SOURCES:.c=.o)
DRIVER_OBJECTS = $(DRIVER_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)


# Interface library (selected by DEBUG flag) - Use libraries from common lib_drvintf directory
LIB_DRVINTF_DIR = ../../../lib_drvintf
ifeq ($(DEBUG),1)
  LIBDRVINTF_LIB ?= $(LIB_DRVINTF_DIR)/libdrvintf_dbg.a
  CFLAGS += -DDEBUG=1 -O0
else
  LIBDRVINTF_LIB ?= $(LIB_DRVINTF_DIR)/libdrvintf.a
endif

# Real chip compilation support
ifeq ($(REAL_CHIP),1)
  CFLAGS += -DREAL_CHIP=1
  LIBDRVINTF_LIB =  # No interface library needed for real chip
endif

# Test executable
TEST_EXEC = ac780x_eflash_test

# Include directories
INCLUDES = -I$(HAL_DIR) -I$(DRIVER_DIR) -I$(LIB_DRVINTF_DIR) -I. -I../../

# Default target
all: $(TEST_EXEC)

# Alternative target for multitask test only
multitask: multitask_eflash_standalone

# Standalone multitask executable (without wrapper)
multitask_eflash_standalone: $(TEST_DIR)/multitask_eflash.o $(DRIVER_OBJECTS) $(HAL_OBJECTS)
	@echo "Linking standalone multitask executable: $@"
ifeq ($(REAL_CHIP),1)
	@echo "Building for real chip - no interface library needed"
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)
else
	@if [ ! -f "$(LIBDRVINTF_LIB)" ]; then \
		echo "Error: $(LIBDRVINTF_LIB) not found!"; \
		exit 1; \
	fi
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBDRVINTF_LIB) $(LDFLAGS)
endif

# Test executable
$(TEST_EXEC): $(TEST_OBJECTS) $(DRIVER_OBJECTS) $(HAL_OBJECTS)
	@echo "Linking test executable: $@"
ifeq ($(REAL_CHIP),1)
	@echo "Building for real chip - no interface library needed"
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)
else
	@if [ ! -f "$(LIBDRVINTF_LIB)" ]; then \
		echo "Error: $(LIBDRVINTF_LIB) not found!"; \
		echo "Please ensure the interface library is present."; \
		exit 1; \
	fi
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBDRVINTF_LIB) $(LDFLAGS)
endif

# Compile C files
%.o: %.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test target (called by top-level Makefile)
test: $(TEST_EXEC)
	@echo "Built AC780x EFLASH test executable successfully"
	@echo "Usage: ./$(TEST_EXEC) [base_address]"
	@echo "Example: ./$(TEST_EXEC) 0x40000000"

# Clean target
clean:
	@echo "Cleaning AC780x EFLASH driver build artifacts..."
	rm -f $(HAL_DIR)/*.o $(DRIVER_DIR)/*.o $(TEST_DIR)/*.o
	rm -f $(HAL_OBJECTS) $(DRIVER_OBJECTS) $(TEST_EXEC) multitask_eflash_standalone
	rm -f *.o

# Install target (copy interface library)
install: $(LIBDRVINTF_LIB)

$(LIBDRVINTF_LIB):
	@echo "Interface library $(LIBDRVINTF_LIB) is required but not found"
	@echo "This will be copied by the top-level Makefile"

# Help target
help:
	@echo "AC780x EFLASH Driver Makefile Help"
	@echo "================================"
	@echo "Targets:"
	@echo "  all         - Build everything (default)"
	@echo "  test        - Build test executable with wrapper"
	@echo "  libs        - Build only HAL and Driver libraries"
	@echo "  multitask   - Build standalone multitask test"
	@echo "  clean       - Remove build artifacts"
	@echo "  help        - Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  DEBUG=1     - Build with debug symbols and debug library"
	@echo "  CC          - Compiler (default: gcc)"
	@echo "  REAL_CHIP=1 - Build for real chip (direct memory access)"
	@echo ""
	@echo "Examples:"
	@echo "  make test               # Build with wrapper (recommended)"
	@echo "  make multitask          # Build standalone multitask test"
	@echo "  make test DEBUG=1       # Build debug version"
	@echo "  make test REAL_CHIP=1   # Build for real hardware"
	@echo "  make clean              # Clean build files"
	@echo ""
	@echo "Files built from:"
	@echo "  HAL:     $(HAL_SOURCES)"
	@echo "  Driver:  $(DRIVER_SOURCES)"
	@echo "  Test:    $(TEST_SOURCES)"

# Dependencies - Updated to match actual files
$(DRIVER_DIR)/ac780x_eflash.o: $(DRIVER_DIR)/ac780x_eflash.c $(DRIVER_DIR)/ac780x_eflash.h $(LIB_DRVINTF_DIR)/ac7805x.h $(HAL_DIR)/ac780x_eflash_reg.h
$(TEST_DIR)/multitask_eflash.o: $(TEST_DIR)/multitask_eflash.c $(DRIVER_DIR)/ac780x_eflash.h $(LIB_DRVINTF_DIR)/ac7805x.h

.PHONY: all test clean help install libs multitask
