.POSIX:
.PHONY: comp, all, addpath

CC = gcc
CFLAGS = -std=c99 -Iinclude -O3 -fomit-frame-pointer -fwrapv -flto -march=native -mtune=native # -Wall -Wextra -pedantic

BINARY = adam
LIBRARY = lib$(BINARY).a
BUILD_DIR = ./build
BIN_PATH := $(BUILD_DIR)/$(BINARY)
LIB_PATH := $(BUILD_DIR)/$(LIBRARY)

STD_LIB = rng api
LIB_OBJ = $(STD_LIB:%=%.o)
CLI = ent test util cli
OBJ := $(CLI:%=%.o) $(LIB_OBJ)

all: comp cli lib addpath
	@rm $(OBJ)

comp:
	@printf "\033[1;36mCompiling sources...\033[m\n"

%.o: src/%.c 
	$(CC) $(CFLAGS) -c $^	

cli: $(OBJ)
	@printf "\n\033[1;36mBuilding ADAM CLI...\033[m\n"
	@mkdir -p $(BUILD_DIR)
	$(CC) -o $(BIN_PATH) $(OBJ) -lm
	@./scripts/size.sh $(BIN_PATH)
	
lib: $(LIB_OBJ)
	@printf "\n\033[1;36mBuilding ADAM library (standard API)...\033[m\n"
	ar rcs $(LIB_PATH) $(LIB_OBJ)
	@./scripts/size.sh $(LIB_PATH)
	@cp include/api.h $(BUILD_DIR)/adam.h
	@printf "\033[1;32mLibrary and header written to \033[m$(BUILD_DIR)\n"

addpath:
	@./scripts/path.sh $(BIN_PATH) $(BINARY)
	