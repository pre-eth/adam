INSTALL_DIR = ~/.local/bin
CC = @gcc

AVX512 = 0
CFLAGS = -Iinclude -O3 -flto #-Wall -Wextra -Wpedantic -Werror
UNAME_P := $(shell uname -p)
ifeq ($(UNAME_P), arm)
	SIMD_FLAGS = -mtune=native -march=native
else
	ifeq ($(AVX512), 0)
		SIMD_FLAGS = -mavx -mavx2 
	else
		SIMD_FLAGS = -mavx512f -mno-vzeroupper
	endif
endif
 
DEPS = util adam support cli ent
HEADERS = $(DEPS:%=%.h)
FILES = adam support cli ent main
OBJ = $(FILES:%=src/%.o)

%.o: src/%.c 
	$(CC) $(CFLAGS) -c $^ $(HEADERS) $(SIMD_FLAGS)

adam: $(OBJ)
	@clang-format -i src/*.c
	@echo "\033[1;36mBuilding ADAM...\033[m"
	$(CC) -o $(INSTALL_DIR)/adam $(OBJ) 
	@rm $(OBJ)
	@echo "\033[1;32mFinished! Run adam -h to get started!\033[m"