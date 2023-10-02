INSTALL_DIR = ~/.local
CC = gcc

AVX512 = 0
CFLAGS = -O3 -flto
UNAME_P := $(shell uname -p)
ifeq ($(UNAME_P), arm)
	SIMD_FLAGS = -mcpu=native
else
	ifeq ($(AVX512), 0)
		SIMD_FLAGS = -mrdseed -mavx -mavx2 
	else
		SIMD_FLAGS = -mrdseed -mavx512f -mno-vzeroupper
	endif
endif

DEPS = util adam cli
HEADERS = $(DEPS:%=src/%.h)

%.o: src/%.c 
	$(CC) -c $^ $(HEADERS) $(CFLAGS) $(SIMD_FLAGS)

adam: adam.o cli.o main.o
	@echo "\033[1;36mBuilding ADAM...\033[m"
	$(CC) -o $(INSTALL_DIR)/adam adam.o cli.o main.o
	@rm adam.o cli.o main.o src/*.gch
	@echo "\033[1;32mFinished! Run adam -h to get started!\033[m"