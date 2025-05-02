// sudoku_validator.c

/* 
How it works:

    1. Data model:
        + We store the 9×9 grid in the global array sudoku[N][N].

    2. Thread parameters:
        + Each thread receives a parameters struct telling it:

        + type: whether to check a row (0), a column (1), or a 3×3 subgrid (2).

        + row/col: the index (or starting indices for subgrids).

        + thread_num: its unique ID in [0..26], used to record results in valid[thread_num].

    3. Launching threads:

        + The first 9 threads check rows 0–8.

        + The next 9 check columns 0–8.

        + The last 9 scan each 3×3 block, at starting positions (0,0), (0,3), (0,6), (3,0), …, (6,6).

    4. Region validation:
        + Each thread zeroes out a local seen[1..9] array and iterates its assigned 9 cells.

        + If a number is out of range or already seen, it immediately marks its slot in valid[] as 0 and exits.
        
        + Otherwise it marks seen[num] = 1.
        
        + If all 9 are distinct and between 1–9, it sets valid[...] = 1.

    5. Collecting results:
        + After pthread_join-ing all 27 threads, the main thread scans valid[].
          If every entry is 1, the Sudoku is valid; otherwise it’s invalid. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define N 9
#define NUM_THREADS 27

// The Sudoku to validate:
int sudoku[N][N] = {
    {6,2,4,5,3,9,1,8,7},
    {5,1,9,7,2,8,6,3,4},
    {8,3,7,6,1,4,2,9,5},
    {1,4,3,8,6,5,7,2,9},
    {9,5,8,2,4,7,3,6,1},
    {7,6,2,3,9,1,4,5,8},
    {3,7,1,9,5,6,8,4,2},
    {4,9,6,1,8,2,5,7,3},
    {2,8,5,4,7,3,9,1,6}
};

// Shared array: valid[i] == 1 if thread i finds its region valid.
int valid[NUM_THREADS] = {0};

// Parameter struct for each thread
typedef struct {
    int thread_num;  // index into valid[]
    int type;        // 0 = row, 1 = column, 2 = subgrid
    int row;         // for row check: row index; for subgrid: start row
    int col;         // for column check: col index; for subgrid: start col
} parameters;

// Thread function: checks one row, column, or 3×3 box
void* check_region(void* args) {
    parameters* p = (parameters*) args;
    int seen[N+1] = {0};  // seen[1..9]

    if (p->type == 0) {
        // check row p->row
        for (int c = 0; c < N; c++) {
            int num = sudoku[p->row][c];
            if (num < 1 || num > 9 || seen[num]) {
                valid[p->thread_num] = 0;
                pthread_exit(NULL);
            }
            seen[num] = 1;
        }
    }
    else if (p->type == 1) {
        // check column p->col
        for (int r = 0; r < N; r++) {
            int num = sudoku[r][p->col];
            if (num < 1 || num > 9 || seen[num]) {
                valid[p->thread_num] = 0;
                pthread_exit(NULL);
            }
            seen[num] = 1;
        }
    }
    else {
        // check 3×3 subgrid starting at (p->row, p->col)
        for (int r = p->row; r < p->row + 3; r++) {
            for (int c = p->col; c < p->col + 3; c++) {
                int num = sudoku[r][c];
                if (num < 1 || num > 9 || seen[num]) {
                    valid[p->thread_num] = 0;
                    pthread_exit(NULL);
                }
                seen[num] = 1;
            }
        }
    }

    // if we reach here, region is valid
    valid[p->thread_num] = 1;
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    parameters params[NUM_THREADS];
    int thread_idx = 0;

    // 1) Create one thread per row
    for (int i = 0; i < N; i++) {
        params[thread_idx] = (parameters){ .thread_num = thread_idx,
                                           .type = 0,
                                           .row = i,
                                           .col = 0 };
        pthread_create(&threads[thread_idx], NULL, check_region, &params[thread_idx]);
        thread_idx++;
    }

    // 2) Create one thread per column
    for (int i = 0; i < N; i++) {
        params[thread_idx] = (parameters){ .thread_num = thread_idx,
                                           .type = 1,
                                           .row = 0,
                                           .col = i };
        pthread_create(&threads[thread_idx], NULL, check_region, &params[thread_idx]);
        thread_idx++;
    }

    // 3) Create one thread per 3×3 subgrid
    for (int row = 0; row < N; row += 3) {
        for (int col = 0; col < N; col += 3) {
            params[thread_idx] = (parameters){ .thread_num = thread_idx,
                                               .type = 2,
                                               .row = row,
                                               .col = col };
            pthread_create(&threads[thread_idx], NULL, check_region, &params[thread_idx]);
            thread_idx++;
        }
    }

    // 4) Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // 5) Aggregate results
    int all_valid = 1;
    for (int i = 0; i < NUM_THREADS; i++) {
        if (! valid[i]) {
            all_valid = 0;
            break;
        }
    }

    if (all_valid)
        printf("Sudoku solution is VALID.\n");
    else
        printf("Sudoku solution is INVALID.\n");

    return 0;
}
