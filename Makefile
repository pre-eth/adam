BUILD_DIR = ./build
CC = @gcc

CFLAGS = -Iinclude -O2 -flto	# -Wall -Wextra -Wpedantic -Werror
UNAME_P := $(shell uname -p)

ifeq ($(UNAME_P), arm)
	SIMD_FLAGS = -march=native
else
	AVX512 := $(grep -o avx512 /proc/cpuinfo)
	ifeq ($(AVX512), avx512)
		SIMD_FLAGS = -mavx512f -mno-vzeroupper
	else
		SIMD_FLAGS = -mavx -mavx2 
	endif
endif
 
STD_LIB = rng api
LIB_OBJ = $(STD_LIB:%=src/%.o)
CLI = ent test support cli
OBJ := $(CLI:%=src/%.o) $(LIB_OBJ)

all: cli lib
	@clang-format -i src/*.c
	@rm $(OBJ)

%.o: src/%.c 
	$(CC) $(CFLAGS) $(SIMD_FLAGS) -c $^ 

cli: $(OBJ)
	@echo "\033[1;36mBuilding ADAM CLI...\033[m"
	@mkdir -p $(BUILD_DIR)
	$(CC) -o $(BUILD_DIR)/adam $(OBJ) 
	@echo "\033[1;32mFinished! Run adam -h to get started!\033[m"

lib: $(LIB_OBJ)
	@echo "\033[1;36mBuilding ADAM library (standard API)...\033[m"
	@ar rcs $(BUILD_DIR)/libadam.a $(LIB_OBJ)
	@cp include/api.h $(BUILD_DIR)/adam.h
	@echo "\033[1;32mLibrary and header written to \033[m$(BUILD_DIR)"

	
	
	