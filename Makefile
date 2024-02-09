BUILD_DIR = ./build
CC = @gcc

CFLAGS = -Iinclude -O2 -flto 	# -Wall -Wextra -Wpedantic -Werror
UNAME_P := $(shell uname -p)

ifeq ($(UNAME_P), arm)
	SIMD_FLAGS = -mtune=native -march=native
else
	AVX512 := $(grep -o avx512 /proc/cpuinfo)
	ifeq ($(AVX512), avx512)
		SIMD_FLAGS = -mavx512f -mno-vzeroupper
	else
		SIMD_FLAGS = -mavx -mavx2 
	endif
endif
 
LIBRARY = rng adam 
LIB_OBJ = $(LIBRARY:%=src/%.o)
CLI = $(LIBRARY) ent test support cli
OBJ = $(CLI:%=src/%.o)

%.o: src/%.c 
	$(CC) $(CFLAGS) -c $^ $(SIMD_FLAGS)

adam: $(OBJ)
	@clang-format -i src/*.c
	@echo "\033[1;36mBuilding ADAM...\033[m"
	@mkdir -p $(BUILD_DIR)
	@ar rcs $(BUILD_DIR)/libadam.a $(LIB_OBJ)
	$(CC) -o $(BUILD_DIR)/adam $(OBJ)
	@rm $(OBJ)
	@echo "\033[1;32mFinished! Run adam -h to get started!\033[m"
	@cp include/adam.h build/adam.h
	@cp $(BUILD_DIR)/adam ~/.local/bin/adam