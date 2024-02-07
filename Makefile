BINARY_DIR = ./bin
LIBRARY_DIR = ./lib
CC = @gcc

CFLAGS = -Iinclude -O2 -flto # -Wall -Wextra -Wpedantic -Werror
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
 
FILES = adam ent test support cli
OBJ = $(FILES:%=src/%.o)

%.o: src/%.c 
	$(CC) $(CFLAGS) -c $^ $(SIMD_FLAGS)

adam: $(OBJ)
	@clang-format -i src/*.c
	@echo "\033[1;36mBuilding ADAM...\033[m"
	@mkdir -p bin lib
	@ar rcs $(LIBRARY_DIR)/libadam.a src/adam.o
	$(CC) -o $(BINARY_DIR)/adam $(OBJ)
	@rm $(OBJ)
	@echo "\033[1;32mFinished! Run adam -h to get started!\033[m"
	@cp bin/adam ~/.local/bin/adam