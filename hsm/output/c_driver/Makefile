# HSM C Driver Makefile
# This makefile builds the HSM HAL, Driver, and Test components

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
LDFLAGS =

# Directories
HAL_DIR = hal
DRIVER_DIR = driver
TEST_DIR = test

# Source files
HAL_SOURCES = $(HAL_DIR)/hsm_hal.c
DRIVER_SOURCES = $(DRIVER_DIR)/hsm_driver.c
TEST_SOURCES = $(TEST_DIR)/test_hsm_driver.c

# Object files
HAL_OBJECTS = $(HAL_SOURCES:.c=.o)
DRIVER_OBJECTS = $(DRIVER_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# Libraries
HAL_LIB = libhsm_hal.a
DRIVER_LIB = libhsm_driver.a

# Interface library (selected by DEBUG flag) - Use libraries from common lib_drvintf directory
LIB_DRVINTF_DIR = ../../../lib_drvintf
ifeq ($(DEBUG),1)
  LIBDRVINTF_LIB ?= $(LIB_DRVINTF_DIR)/libdrvintf_dbg.a
  CFLAGS += -DDEBUG=1 -O0
else
  LIBDRVINTF_LIB ?= $(LIB_DRVINTF_DIR)/libdrvintf.a
endif

# Test executable
TEST_EXEC = test_hsm_driver

# Include directories
INCLUDES = -I$(HAL_DIR) -I$(DRIVER_DIR) -I$(LIB_DRVINTF_DIR) -I../../

# Default target
all: $(TEST_EXEC)

# HAL library
$(HAL_LIB): $(HAL_OBJECTS)
	@echo "Creating HAL library: $@"
	ar rcs $@ $^

# Driver library
$(DRIVER_LIB): $(DRIVER_OBJECTS)
	@echo "Creating Driver library: $@"
	ar rcs $@ $^

# Test executable
$(TEST_EXEC): $(TEST_OBJECTS) $(DRIVER_LIB) $(HAL_LIB)
	@echo "Linking test executable: $@"
	@if [ ! -f "$(LIBDRVINTF_LIB)" ]; then \
		echo "Error: $(LIBDRVINTF_LIB) not found!"; \
		echo "Please ensure the interface library is present."; \
		exit 1; \
	fi
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBDRVINTF_LIB) $(LDFLAGS)

# Compile C files
%.o: %.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test target (called by top-level Makefile)
test: $(TEST_EXEC)
	@echo "Built HSM test executable successfully"
	@echo "Usage: ./$(TEST_EXEC) [base_address]"
	@echo "Example: ./$(TEST_EXEC) 0x40000000"

# Clean target
clean:
	@echo "Cleaning HSM driver build artifacts..."
	rm -f $(HAL_DIR)/*.o $(DRIVER_DIR)/*.o $(TEST_DIR)/*.o
	rm -f $(HAL_LIB) $(DRIVER_LIB) $(TEST_EXEC)
	rm -f *.o

# Install target (copy interface library)
install: $(LIBDRVINTF_LIB)

$(LIBDRVINTF_LIB):
	@echo "Interface library $(LIBDRVINTF_LIB) is required but not found"
	@echo "This will be copied by the top-level Makefile"

# Help target
help:
	@echo "HSM Driver Makefile Help"
	@echo "========================"
	@echo "Targets:"
	@echo "  all      - Build everything (default)"
	@echo "  test     - Build test executable"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  DEBUG=1  - Build with debug symbols and debug library"
	@echo "  CC       - Compiler (default: gcc)"
	@echo ""
	@echo "Examples:"
	@echo "  make test              # Build release version"
	@echo "  make test DEBUG=1      # Build debug version"
	@echo "  make clean             # Clean build files"

# Dependencies
$(HAL_DIR)/hsm_hal.o: $(HAL_DIR)/hsm_hal.c $(HAL_DIR)/hsm_hal.h
$(DRIVER_DIR)/hsm_driver.o: $(DRIVER_DIR)/hsm_driver.c $(DRIVER_DIR)/hsm_driver.h $(HAL_DIR)/hsm_hal.h
$(TEST_DIR)/test_hsm_driver.o: $(TEST_DIR)/test_hsm_driver.c $(DRIVER_DIR)/hsm_driver.h $(HAL_DIR)/hsm_hal.h

.PHONY: all test clean help install