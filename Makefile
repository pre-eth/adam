INSTALL_DIR=~/.local/bin
CC=gcc
CFLAGS=-O3 -flto -mavx -mavx2 -mrdseed
DEPS=util adam cli
HEADERS=$(DEPS:%=src/%.h)

%.o: src/%.c 
	$(CC) -c $^ $(HEADERS) $(CFLAGS)

adam: adam.o cli.o main.o
	@echo -e "\e[1;36mBuilding ADAM...\e[m"
	$(CC) -o $(INSTALL_DIR)/adam adam.o cli.o main.o
	rm adam.o cli.o main.o src/*.gch
	@echo -e "\e[1;32mFinished! Run adam -h to get started!\e[m"