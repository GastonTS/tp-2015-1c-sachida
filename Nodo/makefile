NAME=Nodo
LIBS= -lcommons -lpthread
MODS=-Wall -MMD -MP -D_FILE_OFFSET_BITS=64
CC=gcc -g
MV=mv -f
RM=rm -rf
CFG_FILE=config.cfg
GO=./$(NAME).o $(CFG_FILE)
SRCS=$(shell find ./ -iname *.c)

goi: CFG_FILE=config${i}.cfg
goi: go
vali: CFG_FILE=config${i}.cfg
vali: val
	
all: clean dependents build
debug: CC += -DDEBUG -g -O0
debug: all
clean:
	$(RM) $(NAME).o $(NAME).d $(NAME).v
dependents:

build:
	$(CC) $(MODS) -o $(NAME).o $(SRCS) $(LIBS)
go:
	$(GO)
val:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(GO)
lval:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --log-file=$(NAME).v $(GO)
