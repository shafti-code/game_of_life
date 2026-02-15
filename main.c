#include "ncurses.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <locale.h>
#include <stdbool.h>
#include <time.h>

typedef struct {
    int capacity;
    int size;
    int* indexes;
} GrowingArray;

GrowingArray dead_to_check; 

int height,width;
int* curr;
int* next;

void seed();

void clean_exit(int signum){
    endwin();
    free(next);   
    free(curr);
    free(dead_to_check.indexes);
    char goodbye[] = "exiting with the pointers freed";
    write(1,goodbye,sizeof(goodbye) - 1);
    _exit(0);
}

void draw_game(){
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            if (curr[(i * width) + j]== 0){
                printw("  ");
            } else {
                printw("██");
            }
        }
        move(i+1,0);
    }
    refresh();
}


int is_already_tagged(int index){
    for (int i = 0; i < dead_to_check.size; i++){
        if (index == dead_to_check.indexes[i]){
            return 1;
        }
    }
    return 0;
}

int find_neighbours(int y, int x, int append_dead){
    int neighbours = 0;

    int dy[8] = { -1, -1, -1,  0, 0,  1, 1, 1 };
    int dx[8] = { -1,  0,  1, -1, 1, -1, 0, 1 };

    for (int d = 0; d < 8; d++) {
        int ny = y + dy[d];
        int nx = x + dx[d];

        if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
            int index = ny * width + nx;
            if (curr[index] == 1) {
                neighbours++;
            } else if (curr[index] == 0 && append_dead == 1) {
                if (!is_already_tagged(index)){
                    if (dead_to_check.size != dead_to_check.capacity){
                        dead_to_check.indexes[dead_to_check.size] = index;
                        dead_to_check.size++;
                    } else {
                        dead_to_check.capacity += 8;
                        dead_to_check.indexes = realloc(dead_to_check.indexes, dead_to_check.capacity * sizeof(int));
                        if (dead_to_check.indexes == NULL){
                            char err_msg[] = "failed to allocate mem\n";
                            write(2, err_msg, sizeof(err_msg));
                            return -1;
                        }
                        dead_to_check.indexes[dead_to_check.size] = index;
                        dead_to_check.size++;
                    }
                }
            }
        }
    }
    return neighbours;
}
    // Any live cell with fewer than two live neighbours dies, as if by underpopulation.
    // Any live cell with two or three live neighbours lives on to the next generation.
    // Any live cell with more than three live neighbours dies, as if by overpopulation.
    // Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
void process_board(){
    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            if (curr[width * y + x] == 1){
                int ngbrs = find_neighbours(y,x,1);
                if (ngbrs >= 2 && ngbrs <= 3) {
                    next[width * y + x] = 1;
                } else if (ngbrs < 2) {
                    next[width * y + x] = 0;
                } else if (ngbrs >= 4) {
                    next[width * y + x] = 0;
                }
            }
        }
    }
    for (int i = 0; i < dead_to_check.size; i++){
        int y,x;
        y = dead_to_check.indexes[i] / width;
        x = dead_to_check.indexes[i] % width;
        int ngbrs = find_neighbours(y,x,0);
        if (ngbrs == 3){
            next[width * y + x] = 1;
        }
    }
    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            curr[width * y + x] = next[width * y + x];
        }
    }
    dead_to_check.size = 0;
}


int repeats = 0;

void count_cells(){
    static int last_cells = 0;
    int cells = 0;
    for (int i = 0; i < height; i++){
        for (int j = 0;j < width; j++){
            if (curr[i * width + j] == 1){
                cells++;
            }
        }
    }
    if (cells == last_cells){
        repeats++;
    } else {
        last_cells = cells;
    }
}

void pre_sim(){
    mousemask(ALL_MOUSE_EVENTS, NULL);

    MEVENT event;
    int finished = 0;

    while(!finished){
        int press = getch();
        if (press == KEY_MOUSE){
            if(getmouse(&event) == OK){
                if(event.bstate & BUTTON1_CLICKED){
                    curr[width * event.y + (event.x/2)] = 1;
                    draw_game();
                }
            }
        } else if (press == 32){
            finished = 1;
        }
    }
}

void seed(){
    srand(time(NULL));
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            curr[i * width + j] = 0;
            next[i * width + j] = 0;
        }
    }
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            curr[i * width + j] = rand() % 2;
        }
    }
}

int main(int argc, char *argv[])
{
    bool random = false;
    bool restart = false;
    char c;
    for (int i = 0; i < argc; i++){
        c = getopt(argc,argv,"srh");
        switch(c) {
            case 'r':
                restart = true;
                break;
            case 's':
                random = true;
                break;
            case 'h':
                printf("simple game of life\n");
                printf("-r\t\tseed the board randomly\n");
                printf("-h\t\tshow this help\n");
                break;
        }
    }
    dead_to_check.size     = 0;
    dead_to_check.capacity = 4;
    dead_to_check.indexes = malloc(dead_to_check.capacity * sizeof(int));
    signal(SIGINT,clean_exit);
    setlocale(LC_ALL, "");
    initscr();
    keypad(stdscr,true);
    getmaxyx(stdscr,height,width);
    width /=2;
    curr = malloc((height * width) * sizeof(int));
    next = malloc((height * width) * sizeof(int));

    if (curr == NULL || next == NULL){
        free(curr);
        free(next);
        char err_msg[] = "failed to allocate mem\n";
        write(2, err_msg, sizeof(err_msg));
        return -1;
    }

    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            curr[(y * width) + x] = 0;
        }
    }
    // curr[(5 * width) + 5] = 1;
    // curr[(6 * width) + 6] = 1;
    // curr[(7 * width) + 6] = 1;
    // curr[(7 * width) + 5] = 1;
    // curr[(7 * width) + 4] = 1;

    draw_game();

    if (!random){
        pre_sim();
    } else {
        seed();
    }
    while (1){
        clear();
        draw_game();
        process_board();
        if(restart){
            count_cells();
            if (repeats > 40){
                seed();
                repeats = 0;
            }
        }
        usleep(100000);
    }
    endwin();

    free(curr);
    free(next);
    free(dead_to_check.indexes);
    return 0;
}
