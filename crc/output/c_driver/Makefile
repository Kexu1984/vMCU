# Makefile for CRC Driver C Implementation
# This makefile builds the HAL layer, Driver layer, and test program

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -O2 -g
INCLUDES = -I. -Ihal -Idriver -I../../../lib_drvintf

# Directories
HAL_DIR = hal
DRIVER_DIR = driver
TEST_DIR = test
BUILD_DIR = build

# External dependencies - Use libraries from common lib_drvintf directory
LIB_DRVINTF_DIR = ../../../lib_drvintf
LIBDRVINTF_LIB = $(LIB_DRVINTF_DIR)/libdrvintf.a
LIBDRVINTF_DBG_LIB = $(LIB_DRVINTF_DIR)/libdrvintf_dbg.a

# Select library based on DEBUG flag
ifeq ($(DEBUG),1)
  LIBDRVINTF_SELECTED = $(LIBDRVINTF_DBG_LIB)
  CFLAGS += -DDEBUG=1 -O0
else
  LIBDRVINTF_SELECTED = $(LIBDRVINTF_LIB)
endif

# Source files
HAL_SOURCES = $(HAL_DIR)/crc_hal.c
DRIVER_SOURCES = $(DRIVER_DIR)/crc_driver.c
TEST_SOURCES = $(TEST_DIR)/test_crc_driver.c

# Object files
HAL_OBJECTS = $(BUILD_DIR)/crc_hal.o
DRIVER_OBJECTS = $(BUILD_DIR)/crc_driver.o
TEST_OBJECTS = $(BUILD_DIR)/test_crc_driver.o

# Target executables
TEST_EXECUTABLE = $(BUILD_DIR)/test_crc_driver

# Static libraries
HAL_LIB = $(BUILD_DIR)/libcrc_hal.a
DRIVER_LIB = $(BUILD_DIR)/libcrc_driver.a

# Default target
all: directories $(HAL_LIB) $(DRIVER_LIB) $(TEST_EXECUTABLE)

# Create build directory
directories:
	@mkdir -p $(BUILD_DIR)

# Build HAL library
$(HAL_LIB): $(HAL_OBJECTS)
	@echo "Creating HAL library..."
	ar rcs $@ $^
	@echo "HAL library created: $@"

# Build Driver library
$(DRIVER_LIB): $(DRIVER_OBJECTS) $(HAL_LIB)
	@echo "Creating Driver library..."
	ar rcs $@ $(DRIVER_OBJECTS)
	@echo "Driver library created: $@"

# Build test executable
$(TEST_EXECUTABLE): $(TEST_OBJECTS) $(DRIVER_LIB) $(HAL_LIB) $(LIBDRVINTF_SELECTED)
	@echo "Linking test executable..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(TEST_OBJECTS) $(DRIVER_LIB) $(HAL_LIB) $(LIBDRVINTF_SELECTED)
	@echo "Test executable created: $@"

# Compile HAL objects
$(BUILD_DIR)/crc_hal.o: $(HAL_SOURCES) | directories
	@echo "Compiling HAL source..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $(HAL_SOURCES) -o $@

# Compile Driver objects
$(BUILD_DIR)/crc_driver.o: $(DRIVER_SOURCES) | directories
	@echo "Compiling Driver source..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $(DRIVER_SOURCES) -o $@

# Compile test objects
$(BUILD_DIR)/test_crc_driver.o: $(TEST_SOURCES) | directories
	@echo "Compiling test source..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $(TEST_SOURCES) -o $@

# Check for libdrvintf.a dependency
$(LIBDRVINTF_LIB) $(LIBDRVINTF_DBG_LIB):
	@if [ ! -f "$@" ]; then \
		echo "Error: $@ not found in $(LIB_DRVINTF_DIR)!"; \
		echo "Please ensure the interface library is built in the lib_drvintf directory."; \
		exit 1; \
	fi

# Run tests
test: $(TEST_EXECUTABLE)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# Install headers and libraries (optional)
install: all
	@echo "Installing CRC driver..."
	@mkdir -p /usr/local/include/crc_driver
	@mkdir -p /usr/local/lib
	cp $(HAL_DIR)/crc_hal.h /usr/local/include/crc_driver/
	cp $(DRIVER_DIR)/crc_driver.h /usr/local/include/crc_driver/
	cp $(HAL_LIB) /usr/local/lib/
	cp $(DRIVER_LIB) /usr/local/lib/
	@echo "Installation complete."

# Uninstall
uninstall:
	@echo "Uninstalling CRC driver..."
	rm -rf /usr/local/include/crc_driver
	rm -f /usr/local/lib/libcrc_hal.a
	rm -f /usr/local/lib/libcrc_driver.a
	@echo "Uninstallation complete."

# Format code (if clang-format is available)
format:
	@echo "Formatting code..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find . -name "*.c" -o -name "*.h" | grep -v build | xargs clang-format -i -style="{BasedOnStyle: Linux, IndentWidth: 4, TabWidth: 4, UseTab: Never}"; \
		echo "Code formatting complete."; \
	else \
		echo "clang-format not available, skipping formatting."; \
	fi

# Static analysis (if cppcheck is available)
analyze:
	@echo "Running static analysis..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c99 --suppress=missingIncludeSystem $(HAL_DIR) $(DRIVER_DIR) $(TEST_DIR); \
		echo "Static analysis complete."; \
	else \
		echo "cppcheck not available, skipping analysis."; \
	fi

# Generate documentation (if doxygen is available)
docs:
	@echo "Generating documentation..."
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen -g && \
		echo "INPUT = $(HAL_DIR) $(DRIVER_DIR)" >> Doxyfile && \
		echo "OUTPUT_DIRECTORY = docs" >> Doxyfile && \
		doxygen Doxyfile && \
		echo "Documentation generated in docs/ directory."; \
	else \
		echo "doxygen not available, skipping documentation generation."; \
	fi

# Show help
help:
	@echo "CRC Driver Makefile"
	@echo "==================="
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build all libraries and test executable (default)"
	@echo "  test         - Build and run tests"
	@echo "  test-valgrind- Run tests with valgrind memory checking"
	@echo "  clean        - Remove all build artifacts"
	@echo "  install      - Install headers and libraries to /usr/local"
	@echo "  uninstall    - Remove installed headers and libraries"
	@echo "  format       - Format code using clang-format"
	@echo "  analyze      - Run static analysis using cppcheck"
	@echo "  docs         - Generate documentation using doxygen"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Build artifacts:"
	@echo "  $(HAL_LIB)  - HAL static library"
	@echo "  $(DRIVER_LIB) - Driver static library"
	@echo "  $(TEST_EXECUTABLE) - Test executable"
	@echo ""
	@echo "Example usage:"
	@echo "  make all     # Build everything"
	@echo "  make test    # Build and run tests"
	@echo "  make clean   # Clean build directory"

# Debug target - prints variables
debug:
	@echo "Makefile Debug Information"
	@echo "=========================="
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "INCLUDES: $(INCLUDES)"
	@echo "HAL_SOURCES: $(HAL_SOURCES)"
	@echo "DRIVER_SOURCES: $(DRIVER_SOURCES)"
	@echo "TEST_SOURCES: $(TEST_SOURCES)"
	@echo "HAL_OBJECTS: $(HAL_OBJECTS)"
	@echo "DRIVER_OBJECTS: $(DRIVER_OBJECTS)"
	@echo "TEST_OBJECTS: $(TEST_OBJECTS)"
	@echo "HAL_LIB: $(HAL_LIB)"
	@echo "DRIVER_LIB: $(DRIVER_LIB)"
	@echo "TEST_EXECUTABLE: $(TEST_EXECUTABLE)"

# Phony targets
.PHONY: all directories test test-valgrind clean install uninstall format analyze docs help debug

# Dependencies
$(DRIVER_OBJECTS): $(HAL_DIR)/crc_hal.h
$(TEST_OBJECTS): $(HAL_DIR)/crc_hal.h $(DRIVER_DIR)/crc_driver.h
