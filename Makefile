main: main.c
	gcc main.c -O3 -o main -lncurses 

run: main
	./main
