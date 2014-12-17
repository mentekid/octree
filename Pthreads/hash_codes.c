#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "float.h"
#include "pthread.h"

#define DIM 3

unsigned int compute_code(float, float, float);

struct quantize_data {
    unsigned int *codes;
    float *X;
    float *low;
    float step;
    int N;
};

void *pthread_quantize(void* pthread_data)
{
    //assign values to arguments
    struct quantize_data *data = (struct quantize_data *) pthread_data;
    unsigned int *codes = data->codes;
    float *X = data->X;
    float *low = data->low;
    float step = data->step;
    int N = data->N;

    //initialize stuff
    int i, j;
    for(i=0; i<N; i++)
    {
        for (j=0; j<DIM; j++)
        {
            codes[i*DIM+j] = compute_code(X[i*DIM + j], low[j], step);
        }
    }
    pthread_exit(0);
}

unsigned int compute_code(float x, float low, float step)
{

    return floor((x - low) / step);

}


/* Function that does the quantization */
void quantize(unsigned int *codes, float *X, float *low, float step, int N)
{
    //variables
    int i = 0, j = 0, sum=0, tc, rc;
    extern int num_threads;

    //thread stuff
    pthread_t threads[num_threads]; //array of threads
    pthread_attr_t joinable; //attribute: joinable - this is for portability
    pthread_attr_init(&joinable);
    pthread_attr_setdetachstate(&joinable, PTHREAD_CREATE_JOINABLE);
    void *status;

    //thread arguments stuff
    struct quantize_data args[num_threads]; //arguments array
    int offsets[num_threads]; //offsets
    int sizes[num_threads]; //sizes


    offsets[0] = 0; //first thread starts at the top
    for(tc = 0; tc < num_threads-1; tc++)
    {
        sizes[tc] = N/num_threads;
        sum+=sizes[tc];
        offsets[tc+1]=DIM*sum; //array is DIM*N, so sub-arrays for each thread should also be of that size
    }
    sizes[num_threads-1] = N-sum; //last thread gets a slightly bigger chunk

    for(tc = 0; tc < num_threads; tc++)
    {
        //each thread will receive its respective arguments
        args[tc].codes = &codes[offsets[tc]];
        args[tc].X = &X[offsets[tc]];
        args[tc].low = low;
        args[tc].step = step;
        args[tc].N = sizes[tc];


        rc = pthread_create(&threads[tc], &joinable, pthread_quantize, (void *)&args[tc]);

        if(rc)
        {
            printf("Error: pthread_create returned code %d\n", rc);
            return;
        }
    }

    //join threads
    pthread_attr_destroy(&joinable);
    for (tc = 0; tc < num_threads; tc++)
    {
        rc = pthread_join(threads[tc], &status);
        if (rc)
        {
            printf("Error: pthread_join returned code %d\n", rc);
            return;
        }
    }

    //old code
    /*

    for(i=0; i<N; i++){
        for(j=0; j<DIM; j++){
            codes[i*DIM + j] = compute_code(X[i*DIM + j], low[j], step);
        }
    }
    */

}

float max_range(float *x){

    int i=0;
    float max = -FLT_MAX;
    for(i=0; i<DIM; i++){
        if(max<x[i]){
            max = x[i];
        }
    }

    return max;

}

void compute_hash_codes(unsigned int *codes, float *X, int N,
			int nbins, float *min,
            float *max){

    float range[DIM];
    float qstep;

    int i = 0;
    for(i=0; i<DIM; i++){
        range[i] = fabs(max[i] - min[i]); // The range of the data
        range[i] += 0.01*range[i]; // Add somthing small to avoid having points exactly at the boundaries
    }

    qstep = max_range(range) / nbins; // The quantization step

    quantize(codes, X, min, qstep, N); // Function that does the quantization

}



