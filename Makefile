INSTALL_DIR = ~/.local/bin
CC = @gcc

CFLAGS = -Iinclude -O3 -flto #-Wall -Wextra -Wpedantic -Werror
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
 
DEPS = util adam support cli ent test
HEADERS = $(DEPS:%=%.h)
FILES = adam cli ent test support main 
OBJ = $(FILES:%=src/%.o)

%.o: src/%.c 
	$(CC) $(CFLAGS) -c $^ $(HEADERS) $(SIMD_FLAGS)

adam: $(OBJ)
	@clang-format -i src/*.c
	@echo "\033[1;36mBuilding ADAM...\033[m"
	@mkdir -p ~/.local/bin
	$(CC) -o $(INSTALL_DIR)/adam $(OBJ)
	@rm $(OBJ)
	@echo "\033[1;32mFinished! Run adam -h to get started!\033[m"