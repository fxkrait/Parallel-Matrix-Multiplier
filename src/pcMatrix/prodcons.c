/*
 *  prodcons module
 *  Producer Consumer module
 *
 *  Implements routines for the producer consumer module based on
 *  chapter 30, section 2 of Operating Systems: Three Easy Pieces
 *
 *  University of Washington, Tacoma
 *  TCSS 422 - Operating Systems
 *
 *
 *  Gregory Hablutzel
 *  TCSS 422-OS Fall '20 Hu
 */

// Include only libraries for this module
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "counter.h"
#include "matrix.h"
#include "pcmatrix.h"
#include "prodcons.h"


// Define Locks and Condition variables here
// Producer consumer data structures

// Bounded buffer bigmatrix defined in prodcons.h

int fill_ptr = 0; // where to put()
int use_ptr = 0; // where to get()
int count = 0; // current number of elements in producer/consumer queue
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;;
pthread_mutex_t bigmatrixMutex = PTHREAD_MUTEX_INITIALIZER; // mutex for get() and put()
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // General mutex for worker threads


// Initialize ProdConStats struct
void initProdCons(ProdConsStats * prc)
{
    prc->sumtotal = 0;
    prc->matrixtotal = 0;
    prc->multtotal = 0;
}

// Bounded buffer put() get()
// Adds value to shared buffer (NOTE: won't be called by producer unless there is space).
void put(Matrix * value)
{
    pthread_mutex_lock(&bigmatrixMutex);
    bigmatrix[fill_ptr] = value;
    fill_ptr = (fill_ptr + 1) % BOUNDED_BUFFER_SIZE;
    count++;
    pthread_mutex_unlock(&bigmatrixMutex);
}

// Retrieves a matrix from the front of the shared buffer
// (NOTE: Won't be called if shared buffer is empty)
Matrix * get()
{
    pthread_mutex_lock(&bigmatrixMutex);
    Matrix * gottenMatrix = bigmatrix[use_ptr];
    use_ptr = (use_ptr + 1) % BOUNDED_BUFFER_SIZE;
    count--;
    pthread_mutex_unlock(&bigmatrixMutex);
    return gottenMatrix;
}

// Matrix PRODUCER worker thread
// Creates matrices, and adds them to the shared buffer.
// Updates it's stats continuously using the passed in ProdConStats pointer object
void *prod_worker(void *arg)
{
    ProdConsStats * prodConsStats_ptr = (ProdConsStats* ) arg; // Get the passed in ProdConStats object (as a pointer)
    for (int i = 0; i < NUM_LOOPS_PER_THREAD; i++) {
        pthread_mutex_lock(&mutex);
        while (count == BOUNDED_BUFFER_SIZE) // if buffer is full, wait
            pthread_cond_wait(&empty, &mutex);
        Matrix * m;
        if (MATRIX_MODE == 0) // create a randomly sized matrix
        {
            m = GenMatrixRandom(); // produce a random matrix
        } else { // 1 to n (create a specified non-random matrix)
            m = GenMatrixBySize(MATRIX_MODE, MATRIX_MODE); // produced a set matrix
        }
        int sum = SumMatrix(m);
        prodConsStats_ptr->sumtotal = prodConsStats_ptr->sumtotal + sum; // increment sum of produced matrices.
        prodConsStats_ptr->matrixtotal = prodConsStats_ptr->matrixtotal + 1; // increment num produced matrices by 1
        put(m); // add matrix to buffer
        pthread_cond_signal(&full);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}


// Matrix CONSUMER worker thread
// Consumes matrices m1 m2, and multiplies them to get m3.
// If m1 m2 can't be multiplied, it finds another m2 such that it can.
// It prints the multiplication m1, m2, and m3 to the console.
// Updates it's stats continuously using the passed in ProdConStats pointer object
void *cons_worker(void *arg) {
    int i;
    // create your matrix pointers
    Matrix * m1 = NULL;
    Matrix * m2 = NULL;
    Matrix * m3 = NULL;

    ProdConsStats * prodConsStats_ptr = (ProdConsStats* ) arg; // Get the passed in ProdConStats object (as a pointer)

    for (i = 0; i < NUM_LOOPS_PER_THREAD; i++) {
        pthread_mutex_lock(&mutex);
        while (count == 0) // if nothing in buffer, then wait for signal (till something is in buffer)
            pthread_cond_wait(&full, &mutex);
        Matrix * receivedMatrix = get(); // get a matrix from buffer
        prodConsStats_ptr->matrixtotal = prodConsStats_ptr->matrixtotal + 1; // increment num consumed matrices by 1
        int sum = SumMatrix(receivedMatrix);
        prodConsStats_ptr->sumtotal = prodConsStats_ptr->sumtotal + sum; // increment sum of consumed matrices.
        if (m1 == NULL) // Haven't got first Matrix yet
        {
            m1 = receivedMatrix;
        } else if (m1 != NULL && m2 == NULL) // Got the first, but haven't got the second matrix yet
        {
            m2 = receivedMatrix;
            m3 = MatrixMultiply(m1, m2); // MULTIPLY
            if (m3 != NULL) // valid multiplication
            {
                prodConsStats_ptr->multtotal = prodConsStats_ptr->multtotal +1; // increment number of valid multiplied arrays.
                DisplayMatrix(m1, stdout); // output m1
                printf("    X\n");
                DisplayMatrix(m2, stdout); // output m2
                printf("    =\n");
                DisplayMatrix(m3, stdout); // output m3
                printf("\n");

                FreeMatrix(m1);
                FreeMatrix(m2);
                m1 = NULL;
                m2 = NULL;
                FreeMatrix(m3);
                m3 = NULL;
            } else // (m2 != null && m3 == null) invalid m2 for multplication. Reset m2, find a new (valid) m2
            {
                FreeMatrix(m2); // free memory after it is consumed.
                m2 = NULL;
                // m3 is already NULL, we don't need to free it.
            }
        } else { // should not happen
            printf("This should never occur!!!!!!!!!!!");
        }
        if( receivedMatrix != NULL) // receivedMatrix will already be freed above, just set it NULL to continue.
        {
            receivedMatrix = NULL;
        }
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}