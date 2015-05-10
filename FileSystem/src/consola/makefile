NAME=ConsolaMDFS
LIBS= -lcommons 
MODS=-Wall -MMD -MP
CC=gcc
MV=mv -f
RM=rm -rf
GO=./$(NAME).o
SRCS=$(shell find ./ -iname *.c)

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
	valgrind --leak-check=full $(GO)
lval:
	valgrind --leak-check=full --log-file=$(NAME).v $(GO)
