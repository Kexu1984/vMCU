# DMAv2 Driver Makefile
#
# This Makefile builds the DMAv2 driver following the three-layer architecture:
# - HAL layer (dmav2_hal.h/.c)
# - Driver layer (dmav2_driver.h/.c)
# - Test layer (dmav2_test.c)

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
INCLUDES = -I./hal -I./driver -I../../../lib_drvintf

# Library paths and names
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
HAL_LIB = libdmav2_hal.a
DRIVER_LIB = libdmav2_driver.a
TEST_EXECUTABLE = dmav2_test

# Support for real chip compilation
ifdef REAL_CHIP
    CFLAGS += -DREAL_CHIP
    LIBS =
else
    LIBS = -L. -ldrvintf
endif

# Directory structure
HAL_DIR = hal
DRIVER_DIR = driver
TEST_DIR = test
BUILD_DIR = build

# Source files
HAL_SOURCES = $(HAL_DIR)/dmav2_hal.c
DRIVER_SOURCES = $(DRIVER_DIR)/dmav2_driver.c
TEST_SOURCES = $(TEST_DIR)/dmav2_test.c

# Object files
HAL_OBJECTS = $(BUILD_DIR)/dmav2_hal.o
DRIVER_OBJECTS = $(BUILD_DIR)/dmav2_driver.o
TEST_OBJECTS = $(BUILD_DIR)/dmav2_test.o

# Default target
all: directories $(TEST_EXECUTABLE)

# Create build directory
directories:
	@mkdir -p $(BUILD_DIR)

# Build test executable
$(TEST_EXECUTABLE): $(TEST_OBJECTS) $(DRIVER_LIB) $(HAL_LIB) $(LIBDRVINTF_SELECTED)
	@echo "Linking test executable..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(TEST_OBJECTS) $(DRIVER_LIB) $(HAL_LIB) $(LIBDRVINTF_SELECTED)
	@echo "Test executable created: $@"

# Build driver library
$(DRIVER_LIB): $(DRIVER_OBJECTS) $(HAL_LIB)
	@echo "Creating driver library..."
	ar rcs $@ $(DRIVER_OBJECTS)
	@echo "Driver library created: $@"

# Build HAL library
$(HAL_LIB): $(HAL_OBJECTS)
	@echo "Creating HAL library..."
	ar rcs $@ $(HAL_OBJECTS)
	@echo "HAL library created: $@"

# Compile HAL objects
$(BUILD_DIR)/dmav2_hal.o: $(HAL_SOURCES) | directories
	@echo "Compiling HAL source..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $(HAL_SOURCES) -o $@

# Compile Driver objects
$(BUILD_DIR)/dmav2_driver.o: $(DRIVER_SOURCES) | directories
	@echo "Compiling Driver source..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $(DRIVER_SOURCES) -o $@

# Compile test objects
$(BUILD_DIR)/dmav2_test.o: $(TEST_SOURCES) | directories
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
	@echo "Running DMAv2 driver tests..."
	./$(TEST_EXECUTABLE) 0x40000000

# Run tests with valgrind (if available)
test-valgrind: $(TEST_EXECUTABLE)
	@echo "Running DMAv2 driver tests with valgrind..."
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_EXECUTABLE) 0x40000000; \
	else \
		echo "valgrind not available, running normal test."; \
		./$(TEST_EXECUTABLE) 0x40000000; \
	fi

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(HAL_LIB) $(DRIVER_LIB) $(TEST_EXECUTABLE)
	@echo "Clean completed"

# Install libraries (optional)
install: $(HAL_LIB) $(DRIVER_LIB)
	@echo "Installing libraries..."
	@mkdir -p /usr/local/lib
	@mkdir -p /usr/local/include/dmav2
	cp $(HAL_LIB) /usr/local/lib/
	cp $(DRIVER_LIB) /usr/local/lib/
	cp $(HAL_DIR)/dmav2_hal.h /usr/local/include/dmav2/
	cp $(DRIVER_DIR)/dmav2_driver.h /usr/local/include/dmav2/
	@echo "Installation completed"

# Uninstall libraries (optional)
uninstall:
	@echo "Uninstalling libraries..."
	rm -f /usr/local/lib/$(HAL_LIB)
	rm -f /usr/local/lib/$(DRIVER_LIB)
	rm -rf /usr/local/include/dmav2
	@echo "Uninstall completed"

# Format code (if clang-format available)
format:
	@echo "Formatting code..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find . -name "*.c" -o -name "*.h" | xargs clang-format -i; \
		echo "Code formatting completed"; \
	else \
		echo "clang-format not available, skipping formatting"; \
	fi

# Static analysis (if cppcheck available)
analyze:
	@echo "Running static analysis..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --suppress=missingIncludeSystem $(HAL_SOURCES) $(DRIVER_SOURCES) $(TEST_SOURCES); \
		echo "Static analysis completed"; \
	else \
		echo "cppcheck not available, skipping analysis"; \
	fi

# Generate documentation (if doxygen available)
docs:
	@echo "Generating documentation..."
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile 2>/dev/null || echo "Doxyfile not found, using default settings"; \
		echo "Documentation generated in docs/"; \
	else \
		echo "doxygen not available, skipping documentation generation"; \
	fi

# Help target
help:
	@echo "DMAv2 Driver Build System"
	@echo "========================"
	@echo "Available targets:"
	@echo "  all          - Build everything (default)"
	@echo "  test         - Run tests with default base address (0x40000000)"
	@echo "  test-valgrind- Run tests with valgrind memory checking"
	@echo "  clean        - Remove all build artifacts"
	@echo "  install      - Install libraries to system directories"
	@echo "  uninstall    - Remove libraries from system directories"
	@echo "  format       - Format code using clang-format"
	@echo "  analyze      - Run static analysis using cppcheck"
	@echo "  docs         - Generate documentation using doxygen"
	@echo "  debug        - Show build configuration"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Build modes:"
	@echo "  make                    - Build for simulator (default)"
	@echo "  make REAL_CHIP=1        - Build for real hardware"
	@echo ""
	@echo "Examples:"
	@echo "  make test                           # Run tests in simulator mode"
	@echo "  make REAL_CHIP=1 all               # Build for real hardware"
	@echo "  ./dmav2_test 0x50000000            # Run tests with custom base address"

# Debug target to show configuration
debug:
	@echo "Build Configuration:"
	@echo "==================="
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
	@echo "LIBS: $(LIBS)"
ifdef REAL_CHIP
	@echo "Mode: Real Hardware"
else
	@echo "Mode: Simulator"
endif

# Phony targets
.PHONY: all directories test test-valgrind clean install uninstall format analyze docs help debug

# Dependencies
$(DRIVER_OBJECTS): $(HAL_DIR)/dmav2_hal.h
$(TEST_OBJECTS): $(HAL_DIR)/dmav2_hal.h $(DRIVER_DIR)/dmav2_driver.h