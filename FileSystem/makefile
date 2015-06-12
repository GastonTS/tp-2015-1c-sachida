NAME=FileSystem
LIBS= -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0 -lcommons -lbson-1.0 -lmongoc-1.0 -lpthread
MODS=-Wall -MMD -MP
CC=gcc -g
MV=mv -f
RM=rm -rf
GO=./$(NAME).o
SRCS=$(shell find ./ -iname "*.c" ! -path "./tests/*")
UTILS=$(shell find ../utils/ -iname *.c)

all: clean dependents build
debug: CC += -DDEBUG -g -O0
debug: all
clean:
	$(RM) $(NAME).o $(NAME).d $(NAME).v
dependents:

build:
	$(CC) $(MODS) -o $(NAME).o $(SRCS) $(UTILS) $(LIBS)
go:
	$(GO)
val:
	valgrind --leak-check=full $(GO)
lval:
	valgrind --leak-check=full --log-file=$(NAME).v $(GO)
