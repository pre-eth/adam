.POSIX:
.PHONY: comp, addpath

CC = gcc
CFLAGS = -std=c99 -Iinclude -O3 -fomit-frame-pointer -fwrapv -flto -march=native -mtune=native # -Wall -Wextra -pedantic

BINARY = adam
LIBRARY = lib$(BINARY).a
BUILD_DIR = ./build
BIN_PATH := $(BUILD_DIR)/$(BINARY)
LIB_PATH := $(BUILD_DIR)/$(LIBRARY)

STD_LIB = rng api
LIB_OBJ = $(STD_LIB:%=src/%.o)
CLI = ent test util cli
OBJ := $(CLI:%=src/%.o) $(LIB_OBJ)

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
	@size=$$(ls -lh $(BIN_PATH) | grep -oE "\d+[KMB]"); \
	printf "\033[1;32mFinished building command line tool!\033[m (\033[1m%sB\033[m)\n" "$$size"
	
lib: $(LIB_OBJ)
	@printf "\n\033[1;36mBuilding ADAM library...\033[m\n"
	ar rcs $(LIB_PATH) $(LIB_OBJ)
	@size=$$(ls -lh $(LIB_PATH) | grep -oE "\d+[KMB]"); \
	printf "\033[1;32mFinished building $(LIBRARY)!\033[m (\033[1m%sB\033[m)\n" "$$size"
	@cp include/api.h $(BUILD_DIR)/adam.h
	@printf "\033[1;32mLibrary and header written to \033[m$(BUILD_DIR)\n"

addpath:
	@printf "\n\033[1;36mAdd ADAM to your PATH? [y/n]\033[m	" ; \
	read ans ; \
    if [[ $${ans:-Y} == Y || $${ans:-y} == y ]] ; then \
		sudo cp $(BIN_PATH) /usr/local/bin/$(BINARY) ; \
		printf "\033[1;32mADAM is now in your path. Run adam -h to get started!\033[m\n" ; \
    fi
	