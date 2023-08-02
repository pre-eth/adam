INSTALL_DIR=~/.local/bin
CC=gcc
CFLAGS=-O3 -flto -mavx -mavx2 -mrdseed
DEPS=util.h adam.h cli.h
HEADERS=$(DEPS:%=src/%)

%.o: src/%.c 
	$(CC) -c $^ $(HEADERS) $(CFLAGS)

adam: adam.o main.o
	@echo -e "\e[1;36mBuilding ADAM...\e[0m"
	$(CC) -o $(INSTALL_DIR)/adam adam.o main.o
	@echo -e "\e[1;32mFinished! Run adam -h to get started!\e[0m"
	rm adam.o main.o src/*.gch