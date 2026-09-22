CC ?= gcc
CFLAGS ?= -Wall -Wextra -Wpedantic -std=c99 -D_POSIX_C_SOURCE=200809L -O2 -Iinclude
LDFLAGS ?= 

BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj

CORE_SRCS = src/utils.c \
            src/logger.c \
            src/network.c \
            src/transceiver.c \
            src/device.c \
            src/diagnostics.c \
            src/monitoring.c

CORE_OBJS = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(CORE_SRCS))

TARGET = $(BIN_DIR)/netdiag
TEST_TARGET = $(BIN_DIR)/test_diagnostics

.PHONY: all clean test run demo dirs

all: dirs $(TARGET) $(TEST_TARGET)

dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR) logs

$(OBJ_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/main.o: src/main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/test_diagnostics.o: tests/test_diagnostics.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(CORE_OBJS) $(OBJ_DIR)/main.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Built NetDiag-C binary: $(TARGET)"

$(TEST_TARGET): $(CORE_OBJS) $(OBJ_DIR)/test_diagnostics.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Built NetDiag-C test binary: $(TEST_TARGET)"

test: $(TEST_TARGET)
	@echo "Running diagnostic unit test suite..."
	@./$(TEST_TARGET)

demo: $(TARGET)
	@./$(TARGET) --demo

run: $(TARGET)
	@./$(TARGET) --single

clean:
	rm -rf $(BUILD_DIR) logs/*.log
	@echo "Cleaned build artifacts and logs."
