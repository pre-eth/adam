INSTALL_DIR = ~/.local/bin
CC = gcc

AVX512 = 0
CFLAGS = -O3 -flto
ifeq ($(AVX512), 0)
	SIMD_FLAGS = -mrdseed -mavx -mavx2 
else
	SIMD_FLAGS = -mrdseed -mavx512f -mno-vzeroupper
endif

DEPS = util adam cli
HEADERS = $(DEPS:%=src/%.h)

%.o: src/%.c 
	$(CC) -c $^ $(HEADERS) $(CFLAGS) $(SIMD_FLAGS)

adam: adam.o cli.o main.o
	@echo -e "\e[1;36mBuilding ADAM...\e[m"
	$(CC) -o $(INSTALL_DIR)/adam adam.o cli.o main.o
	rm adam.o cli.o main.o src/*.gch
	@echo -e "\e[1;32mFinished! Run adam -h to get started!\e[m"