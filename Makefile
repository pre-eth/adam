BUILD_DIR = ./build
CC = @gcc

CFLAGS = -Iinclude -O2 -flto -march=native	# -Wall -Wextra -Wpedantic -Werror

 
STD_LIB = rng api
LIB_OBJ = $(STD_LIB:%=src/%.o)
CLI = ent test util cli
OBJ := $(CLI:%=src/%.o) $(LIB_OBJ)

%.o: src/%.c 
	$(CC) $(CFLAGS) $(SIMD_FLAGS) -c $^ 

all: cli lib
	@clang-format -i src/*.c
	@rm $(OBJ)

cli: $(OBJ)
	@echo -e "\033[1;36mBuilding ADAM CLI...\033[m"
	@mkdir -p $(BUILD_DIR)
	$(CC) -o $(BUILD_DIR)/adam $(OBJ) -lm
	@echo -e "\033[1;32mFinished! Run adam -h to get started!\033[m"
	@cp ./build/adam ~/.local/bin/adam

lib: $(LIB_OBJ)
	@echo -e "\033[1;36mBuilding ADAM library...\033[m"
	@ar rcs $(BUILD_DIR)/libadam.a $(LIB_OBJ)
	@cp include/api.h $(BUILD_DIR)/adam.h
	@echo -e "\033[1;32mLibrary and header written to \033[m$(BUILD_DIR)"

	
	
	