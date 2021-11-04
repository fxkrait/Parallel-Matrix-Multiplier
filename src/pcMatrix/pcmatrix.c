/*
 *  pcmatrix module
 *  Primary module providing control flow for the pcMatrix program
 *
 *  Producer consumer bounded buffer program to produce random matrices in parallel
 *  and consume them while searching for valid pairs for matrix multiplication.
 *  Matrix multiplication requires the first matrix column count equal the
 *  second matrix row count.
 *
 *  A matrix is consumed from the bounded buffer.  Then matrices are consumed
 *  from the bounded buffer, one at a time, until an eligible matrix for multiplication
 *  is found.
 *
 *  Totals are tracked using the ProdConsStats Struct for:
 *  - the total number of matrices multiplied (multtotal from consumer threads)
 *  - the total number of matrices produced (matrixtotal from producer threads)
 *  - the total number of matrices consumed (matrixtotal from consumer threads)
 *  - the sum of all elements of all matrices produced and consumed (sumtotal from producer and consumer threads)
 *
 *  Correct programs will produce and consume the same number of matrices, and
 *  report the same sum for all matrix elements produced and consumed.
 *
 *  Each thread produces a total sum of the value of
 *  randomly generated elements.  Producer sum and consumer sum must match.
 *
 *  University of Washington, Tacoma
 *  TCSS 422 - Operating Systems
 *
 *
 *  Gregory Hablutzel
 *  TCSS 422-OS Fall '20 Hu
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include "matrix.h"
#include "counter.h"
#include "prodcons.h"
#include "pcmatrix.h"



int main (int argc, char * argv[]) {
    // Process command line arguments
    int numw = NUMWORK;
    if (argc == 1) // 0 args
    {
        BOUNDED_BUFFER_SIZE = MAX;
        NUMBER_OF_MATRICES = LOOPS;
        MATRIX_MODE = DEFAULT_MATRIX_MODE;
        printf("USING DEFAULTS: worker_threads=%d bounded_buffer_size=%d matricies=%d matrix_mode=%d\n", numw,
               BOUNDED_BUFFER_SIZE, NUMBER_OF_MATRICES, MATRIX_MODE);
    } else {
        if (argc == 2) // 1 arg
        {
            numw = atoi(argv[1]);
            BOUNDED_BUFFER_SIZE = MAX;
            NUMBER_OF_MATRICES = LOOPS;
            MATRIX_MODE = DEFAULT_MATRIX_MODE;
        }
        if (argc == 3) // 2 args
        {
            numw = atoi(argv[1]);
            BOUNDED_BUFFER_SIZE = atoi(argv[2]);
            NUMBER_OF_MATRICES = LOOPS;
            MATRIX_MODE = DEFAULT_MATRIX_MODE;
        }
        if (argc == 4) // 3 args
        {
            numw = atoi(argv[1]);
            BOUNDED_BUFFER_SIZE = atoi(argv[2]);
            NUMBER_OF_MATRICES = atoi(argv[3]);
            MATRIX_MODE = DEFAULT_MATRIX_MODE;
        }
        if (argc == 5) // 4 args
        {
            numw = atoi(argv[1]);
            BOUNDED_BUFFER_SIZE = atoi(argv[2]);
            NUMBER_OF_MATRICES = atoi(argv[3]);
            MATRIX_MODE = atoi(argv[4]);
        }
        printf("USING: worker_threads=%d bounded_buffer_size=%d matricies=%d matrix_mode=%d\n", numw,
               BOUNDED_BUFFER_SIZE, NUMBER_OF_MATRICES, MATRIX_MODE);
    }
    NUM_LOOPS_PER_THREAD = (NUMBER_OF_MATRICES / numw); // calculate and set NUM_LOOPS_PER_THREAD
    printf("NUM_LOOPS_PER_THREAD (per worker): %d\n", NUM_LOOPS_PER_THREAD); // print this out

    time_t t;
    // Seed the random number generator with the system time
    srand((unsigned) time(&t));


    // --------------
    // Majority of MY CODE (below)


    // ---------------------------
    // Initial Execution Output

    printf("\n");
    printf("Producing %d matrices in mode %d\n", LOOPS, MATRIX_MODE);
    printf("Using a shared buffer of size=%d\n", BOUNDED_BUFFER_SIZE);
    printf("With %d producer and consumer threads\n", numw);
    printf("\n");
    // -----------------------------


    bigmatrix = (Matrix **) malloc(sizeof(Matrix *) * BOUNDED_BUFFER_SIZE); // create shared buffer to store matrices

    pthread_t pthread_array[numw*2]; // array of worker threads
    ProdConsStats prodConsStats_array[numw*2]; // create array of ProdConStats stat tracker for each worker thread.

    // create threads and ProdConStats, initialize arrays.
    // EX: if num workers (numw) is 2, this will iterate:0 1 2 3
    for (int i = 0; i < numw*2; i++)
    {
        pthread_t t; // create blank thread
        pthread_array[i] = t; // assign thread to array index
        ProdConsStats prc; // create blank ProdConStats
        initProdCons(&prc); // initialize it's values to 0
        prodConsStats_array[i] = prc; // assign to index
        if (i < numw) // producer half
        {
            pthread_create(&pthread_array[i], NULL, prod_worker, &prodConsStats_array[i]);  // CREATE MATRIX PRODUCER THREAD
        } else { // consumer half
            pthread_create(&pthread_array[i], NULL, cons_worker, &prodConsStats_array[i]);  // CREATE MATRIX CONSUMER THREAD
        }
    }

    // join threads (make main thread wait until worker threads are finished:
    for (int i = 0; i < numw*2; i++)
    {
        pthread_join(pthread_array[i], NULL);// wait for producers/consumers to finish before main exits
    }

    // -------------------
    // Summary Stats:

    /*
     * Internal Variables (producer/consumer)
     *   • Internal prodcons struct containing:
     *   ◦ #sumtotal (prod/con, sum of all elements in matrices produced and consumed)
     *   ◦ #multtotal (con, # matrices multiplied)
     *   ◦ #matrixtotal (prod/con, # matrices produced or consumed)(
    */

    int producer_sum = 0;
    int consumer_sum = 0;
    int producer_matrixTotal = 0;
    int consumer_matrixTotal = 0;
    int consumer_multTotal = 0;
    for (int i = 0; i < numw*2; i++)
    {
        if (i < numw) // producer half
        {
            producer_sum += prodConsStats_array[i].sumtotal;
            producer_matrixTotal += prodConsStats_array[i].matrixtotal;
        } else { // consumer half
            consumer_sum += prodConsStats_array[i].sumtotal;
            consumer_matrixTotal += prodConsStats_array[i].matrixtotal;
            consumer_multTotal += prodConsStats_array[i].multtotal;
         }
    }
    printf("Sum of Matrix elements --> Produced=%d = Consumed=%d\n", consumer_sum, producer_sum);
    printf("Matrices produced=%d consumed=%d multiplied=%d\n", producer_matrixTotal, consumer_matrixTotal, consumer_multTotal);

    return 0;
}
