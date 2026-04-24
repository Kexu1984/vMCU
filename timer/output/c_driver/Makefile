# TIMER AC780x C Driver Makefile
# This makefile builds the TIMER Driver and Test components

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O0 -g
LDFLAGS =

# Directories
HAL_DIR = hal
DRIVER_DIR = driver
TEST_DIR = test

# Source files - Updated to match actual timer files
HAL_SOURCES = $(HAL_DIR)/ac780x_timer_hal.c
DRIVER_SOURCES = $(DRIVER_DIR)/ac780x_timer.c
TEST_SOURCES = $(TEST_DIR)/multitask_timer.c

# Object files
HAL_OBJECTS = $(HAL_SOURCES:.c=.o)
DRIVER_OBJECTS = $(DRIVER_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# Libraries
HAL_LIB = libac780x_timer_hal.a
DRIVER_LIB = libac780x_timer.a

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
TEST_EXEC = ac780x_timer_test

# Include directories
INCLUDES = -I$(HAL_DIR) -I$(DRIVER_DIR) -I$(LIB_DRVINTF_DIR) -I. -I../../

# Default target
all: $(TEST_EXEC)

# Alternative target for building just the libraries
libs: $(HAL_LIB) $(DRIVER_LIB)

# Alternative target for multitask test only
multitask: multitask_timer_standalone

# Standalone multitask executable (without wrapper)
multitask_timer_standalone: $(TEST_DIR)/multitask_timer.o $(DRIVER_LIB) $(HAL_LIB)
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

# HAL library (only if HAL sources exist)
ifneq ($(HAL_SOURCES),)
$(HAL_LIB): $(HAL_OBJECTS)
	@echo "Creating TIMER HAL library: $@"
	ar rcs $@ $^
else
$(HAL_LIB):
	@echo "No HAL sources found, skipping HAL library creation"
endif

# Driver library
$(DRIVER_LIB): $(DRIVER_OBJECTS)
	@echo "Creating TIMER Driver library: $@"
	ar rcs $@ $^

# Test executable
$(TEST_EXEC): $(TEST_OBJECTS) $(DRIVER_LIB) $(HAL_LIB)
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
	@echo "Built AC780x TIMER test executable successfully"
	@echo "Usage: ./$(TEST_EXEC) [base_address]"
	@echo "Example: ./$(TEST_EXEC) 0x40011000"

# Clean target
clean:
	@echo "Cleaning AC780x TIMER driver build artifacts..."
	rm -f $(HAL_DIR)/*.o $(DRIVER_DIR)/*.o $(TEST_DIR)/*.o
	rm -f $(HAL_LIB) $(DRIVER_LIB) $(TEST_EXEC) multitask_timer_standalone
	rm -f *.o

# Install target (copy interface library)
install: $(LIBDRVINTF_LIB)

$(LIBDRVINTF_LIB):
	@echo "Interface library $(LIBDRVINTF_LIB) is required but not found"
	@echo "This will be copied by the top-level Makefile"

# Help target
help:
	@echo "AC780x TIMER Driver Makefile Help"
	@echo "=================================="
	@echo "Targets:"
	@echo "  all         - Build everything (default)"
	@echo "  test        - Build test executable"
	@echo "  libs        - Build only Driver libraries"
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
	@echo "  make test               # Build with default settings"
	@echo "  make multitask          # Build standalone multitask test"
	@echo "  make test DEBUG=1       # Build debug version"
	@echo "  make test REAL_CHIP=1   # Build for real hardware"
	@echo "  make clean              # Clean build files"
	@echo ""
	@echo "Files built from:"
	@echo "  HAL:     $(HAL_SOURCES)"
	@echo "  Driver:  $(DRIVER_SOURCES)"
	@echo "  Test:    $(TEST_SOURCES)"

# Dependencies - Updated to match actual timer files
$(HAL_DIR)/ac780x_timer_hal.o: $(HAL_DIR)/ac780x_timer_hal.c $(HAL_DIR)/ac780x_timer_hal.h $(HAL_DIR)/ac780x_timer_reg.h
$(DRIVER_DIR)/ac780x_timer.o: $(DRIVER_DIR)/ac780x_timer.c $(DRIVER_DIR)/ac780x_timer.h $(HAL_DIR)/ac780x_timer_reg.h $(LIB_DRVINTF_DIR)/ac7805x.h $(HAL_DIR)/ac780x_timer_hal.h
$(TEST_DIR)/timer_test_wrapper.o: $(TEST_DIR)/timer_test_wrapper.c $(DRIVER_DIR)/ac780x_timer.h $(HAL_DIR)/ac780x_timer_hal.h
$(TEST_DIR)/multitask_timer.o: $(TEST_DIR)/multitask_timer.c $(DRIVER_DIR)/ac780x_timer.h $(LIB_DRVINTF_DIR)/ac7805x.h $(HAL_DIR)/ac780x_timer_reg.h

.PHONY: all test clean help install libs multitask
