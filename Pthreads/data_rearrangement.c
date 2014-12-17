#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pthread.h"

#define DIM 3

struct dataRearr_data{
    float *Y;
    float *X;
    unsigned int *permutation_vector;
    int N;
};

void *pthread_rearrangement(void *args)
{
    struct dataRearr_data *arguments = (struct dataRearr_data *) args;
    float *X = arguments->X;
    float *Y = arguments->Y;
    unsigned int *permutation_vector = arguments->permutation_vector;
    int N = arguments->N;

    int i;
    for(i=0; i<N; i++)
    {
        memcpy(&Y[i*DIM], &X[permutation_vector[i]*DIM], DIM*sizeof(float));
    }

    pthread_exit(0);
}

void data_rearrangement(float *Y, float *X, unsigned int *permutation_vector, int N)
{
    extern int num_threads;
    int rc, tc, sum;

    int offsets[num_threads];
    int sizes[num_threads];
    struct dataRearr_data arguments[num_threads];

    pthread_t threads[num_threads];
    pthread_attr_t joinable;
    pthread_attr_init(&joinable);
    pthread_attr_setdetachstate(&joinable, PTHREAD_CREATE_JOINABLE);
    void *status;

    offsets[0] = 0;
    for (tc = 0; tc < num_threads - 1; tc++)
    {
       sizes[tc] = N/num_threads;
       sum+=sizes[tc];
       offsets[tc+1]=sum;
    }

    sizes[num_threads-1] = N - sum;

    for(tc = 0; tc < num_threads; tc++)
    {
        arguments[tc].Y = &Y[DIM*offsets[tc]];
        arguments[tc].X = X;
        arguments[tc].permutation_vector = &permutation_vector[offsets[tc]];
        arguments[tc].N = sizes[tc];

        rc = pthread_create(&threads[tc], &joinable, pthread_rearrangement, (void *)&arguments[tc]);

        if(rc) {
            printf("Error: pthread_create returned code: %d\n", rc);
        }
    }

    pthread_attr_destroy(&joinable);
    for (tc = 0; tc < num_threads; tc++)
    {
        rc = pthread_join(threads[tc], &status);
        if (rc)
        {
            printf("Error: pthread_join returned code: %d\n", rc);
        }
    }


}
