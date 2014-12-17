#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "pthread.h"

#define DIM 3

struct morton_data {
    unsigned long int *mcodes;
    unsigned int *codes;
    int N;
    int max_level;
};


inline unsigned long int splitBy3(unsigned int a){
    unsigned long int x = a & 0x1fffff; // we only look at the first 21 bits
    x = (x | x << 32) & 0x1f00000000ffff;  // shift left 32 bits, OR with self, and 00011111000000000000000000000000000000001111111111111111
    x = (x | x << 16) & 0x1f0000ff0000ff;  // shift left 32 bits, OR with self, and 00011111000000000000000011111111000000000000000011111111
    x = (x | x << 8) & 0x100f00f00f00f00f; // shift left 32 bits, OR with self, and 0001000000001111000000001111000000001111000000001111000000000000
    x = (x | x << 4) & 0x10c30c30c30c30c3; // shift left 32 bits, OR with self, and 0001000011000011000011000011000011000011000011000011000100000000
    x = (x | x << 2) & 0x1249249249249249;
    return x;
}

unsigned long int mortonEncode_magicbits(unsigned int x, unsigned int y, unsigned int z){
    unsigned long int answer = 0;
    answer |= splitBy3(x) | splitBy3(y) << 1 | splitBy3(z) << 2;
    return answer;
}

void *pthread_morton(void *pthread_data)
{
    //assign values to arguments
    struct morton_data *data = (struct morton_data*) pthread_data;
    unsigned long int *mcodes = data->mcodes;
    unsigned int *codes = data->codes;
    int N = data->N;
    int max_level = data->max_level;

    //original function's body
    int i=0;
    for (i = 0; i < N; i++)
    {
        // Compute the morton codes using the magic bits method
        mcodes[i] = mortonEncode_magicbits(codes[i*DIM], codes[i*DIM + 1], codes[i*DIM + 2]);
    }

    pthread_exit(0);
}
/* The function that transform the morton codes into hash codes */
void morton_encoding(unsigned long int *mcodes, unsigned int *codes, int N, int max_level){
    extern int num_threads;
    int tc, rc, sum; //thread counter, return code, sum

    //thread stuff
    pthread_t threads[num_threads];
    pthread_attr_t joinable;
    pthread_attr_init(&joinable);
    pthread_attr_setdetachstate(&joinable, PTHREAD_CREATE_JOINABLE);
    void *status;

    //thread argument stuff
    struct morton_data args[num_threads];
    int offsets[num_threads];
    int sizes[num_threads];

    //assign a chunk of codes and mcodes to each thread
    offsets[0] = 0;
    for (tc = 0; tc < num_threads -1; tc++)
    {
        sizes[tc] = N/num_threads;
        sum+=sizes[tc];
        offsets[tc+1]=sum;
    }
    sizes[num_threads-1] = N - sum;

    for (tc = 0; tc < num_threads; tc++)
    {
        //put each thread's arguments inside the array and create the threads
        args[tc].mcodes = &mcodes[offsets[tc]];
        args[tc].codes = &codes[DIM*offsets[tc]]; //codes is 3x the size of mcodes
        args[tc].N = sizes[tc];
        args[tc].max_level = max_level;

        rc = pthread_create(&threads[tc], &joinable, pthread_morton, (void *)&args[tc]);

        if (rc) {
            printf("Error: pthread_create returned code %d\n", rc);
            return;
        }
    }

    //free up memory and wait for threads to finish before returning
    pthread_attr_destroy(&joinable);
    for (tc = 0; tc < num_threads; tc++)
    {
        rc = pthread_join(threads[tc], &status);
        if(rc) {
            printf("Error: pthread_join returned code %d\n", rc);
            return;
        }
    }
}
