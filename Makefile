all: main
	./main
main: main.c
	gcc main.c -o main -lncurses 

