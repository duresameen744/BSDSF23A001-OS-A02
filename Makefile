# ============================================
# Makefile for LS Project (Feature 1)
# ============================================

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

# Paths
SRC = src/lsv1.0.0.c
BIN_DIR = bin
OBJ_DIR = obj
OUT = $(BIN_DIR)/ls

# Default target (when you just type "make")
all: dirs $(OUT)

# Create folders if missing
dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR) man

# Compile source file into the binary
$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

# Clean generated files
clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)
