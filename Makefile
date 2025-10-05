CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
SRC = src/lsv1.0.0.c   # <--- must match filename in your repo
BIN_DIR = bin
OUT = $(BIN_DIR)/ls

all: dirs $(OUT)

dirs:
	@mkdir -p $(BIN_DIR) obj man

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -rf $(BIN_DIR) obj
