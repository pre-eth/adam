BUILD_DIR = ./build
CC = gcc

CFLAGS = -Iinclude -O2 -flto -march=native

STD_LIB = rng api
LIB_OBJ = $(STD_LIB:%=src/%.o)
CLI = ent test util cli
OBJ := $(CLI:%=src/%.o) $(LIB_OBJ)

all: comp cli lib
	@rm $(OBJ)

comp:
	@printf "\033[1;36mCompiling sources...\033[m\n"

%.o: src/%.c 
	$(CC) $(CFLAGS) -c $^

cli: $(OBJ)
	@printf "\n\033[1;36mBuilding ADAM CLI...\033[m\n"
	@mkdir -p $(BUILD_DIR)
	$(CC) -o $(BUILD_DIR)/adam $(OBJ) -lm
	@printf "\033[1;32mFinished! Run adam -h to get started!\033[m\n"

lib: $(LIB_OBJ)
	@printf "\n\033[1;36mBuilding ADAM library...\033[m\n"
	@ar rcs $(BUILD_DIR)/libadam.a $(LIB_OBJ)
	@cp include/api.h $(BUILD_DIR)/adam.h
	@printf "\033[1;32mLibrary and header written to \033[m$(BUILD_DIR)\n"

	
	
	