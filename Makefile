# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS = -lreadline        # Link against GNU Readline

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Target executable
TARGET = $(BIN_DIR)/myshell

# Source and object files
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/shell.c $(SRC_DIR)/execute.c
OBJS = $(OBJ_DIR)/main.o $(OBJ_DIR)/shell.o $(OBJ_DIR)/execute.o

# Default rule: build the shell
all: $(TARGET)

# Link object files to create final executable
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile each .c file to .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Rebuild everything
rebuild: clean all

# Run the shell
run: all
	$(TARGET)

# Phony targets
.PHONY: all clean rebuild run

