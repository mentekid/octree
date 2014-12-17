#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include "pthread.h"
#define MAXBINS 8


void swap_long(unsigned long int **x, unsigned long int **y){

    unsigned long int *tmp;
    tmp = x[0];
    x[0] = y[0];
    y[0] = tmp;

}

void swap(unsigned int **x, unsigned int **y){

    unsigned int *tmp;
    tmp = x[0];
    x[0] = y[0];
    y[0] = tmp;

}
void truncated_radix_sort(unsigned long int*, unsigned long int*, unsigned int*, unsigned int*, int*, int, int, int, int);

struct radix_arguments {
    unsigned long int *morton_codes;
    unsigned long int *sorted_morton_codes;
    unsigned int *permutation_vector;
    unsigned int *index;
    int *level_record;
    int N;
    int population_threshold;
    int sft;
    int lv;
};

void *pthread_radix_sort(void *pthread_arguments)
{

    struct radix_arguments *args = (struct radix_arguments *) pthread_arguments;
    unsigned long int *morton_codes = args->morton_codes;
    unsigned long int *sorted_morton_codes = args->sorted_morton_codes;
    unsigned int *permutation_vector = args->permutation_vector;
    unsigned int *index = args->index;
    int *level_record = args->level_record;
    int N = args->N;
    int population_threshold = args->population_threshold;
    int sft = args->sft;
    int lv = args->lv;

    struct  radix_arguments arguments[MAXBINS];
    //original code
    int BinSizes[MAXBINS] = {0};
    unsigned int *tmp_ptr;
    unsigned long int *tmp_code;

    level_record[0] = lv; // record the level of the node

    if(N<=population_threshold || sft < 0) { // Base case. The node is a leaf
        memcpy(permutation_vector, index, N*sizeof(unsigned int)); // Copy the pernutation vector
        memcpy(sorted_morton_codes, morton_codes, N*sizeof(unsigned long int)); // Copy the Morton codes

        return NULL;
    }
    else{

        // Find which child each point belongs to
        int j = 0;
        for(j=0; j<N; j++){
            unsigned int ii = (morton_codes[j]>>sft) & 0x07;
            BinSizes[ii]++;
        }


        // scan prefix (must change this code)
        int offset = 0, i = 0;
        for(i=0; i<MAXBINS; i++){
            int ss = BinSizes[i];
            BinSizes[i] = offset;
            offset += ss;
        }

        for(j=0; j<N; j++){
            unsigned int ii = (morton_codes[j]>>sft) & 0x07;
            permutation_vector[BinSizes[ii]] = index[j];
            sorted_morton_codes[BinSizes[ii]] = morton_codes[j];
            BinSizes[ii]++;
        }

        //swap the index pointers
        swap(&index, &permutation_vector);

        //swap the code pointers
        swap_long(&morton_codes, &sorted_morton_codes);

        /* Call the function recursively to split the lower levels */
        offset = 0;
        for(i=0; i<MAXBINS; i++){

            int size = BinSizes[i] - offset;
            arguments[i].morton_codes = &morton_codes[offset];
            arguments[i].sorted_morton_codes = &sorted_morton_codes[offset];
            arguments[i].permutation_vector = &permutation_vector[offset];
            arguments[i].index = &index[offset];
            arguments[i].level_record = &level_record[offset];
            arguments[i].N = size;
            arguments[i].population_threshold = population_threshold;
            arguments[i].sft = sft-3;
            arguments[i].lv = lv+1;

            offset += size;
            pthread_radix_sort( (void *) &arguments[i]);
        }
    }
}

void truncated_radix_sort(unsigned long int *morton_codes,
        unsigned long int *sorted_morton_codes,
        unsigned int *permutation_vector,
        unsigned int *index,
        int *level_record,
        int N,
        int population_threshold,
        int sft, int lv){



    int BinSizes[MAXBINS] = {0};
    unsigned int *tmp_ptr;
    unsigned long int *tmp_code;


    level_record[0] = lv; // record the level of the node

    if(N<=population_threshold || sft < 0) { // Base case. The node is a leaf
        memcpy(permutation_vector, index, N*sizeof(unsigned int)); // Copy the pernutation vector
        memcpy(sorted_morton_codes, morton_codes, N*sizeof(unsigned long int)); // Copy the Morton codes

        return;
    }
    else{

        //extern int num_threads;

        int offsets[MAXBINS], sizes[MAXBINS];
        int rc, tc, sum;

        pthread_t threads[MAXBINS];
        pthread_attr_t joinable;
        pthread_attr_init(&joinable);
        pthread_attr_setdetachstate(&joinable, PTHREAD_CREATE_JOINABLE);
        void *status;

        //thread arguments
        struct  radix_arguments args[MAXBINS];

        // Find which child each point belongs to
        int j = 0;


        for(j=0; j<N; j++){
            unsigned int ii = (morton_codes[j]>>sft) & 0x07;
            BinSizes[ii]++;
        }


        // scan prefix (must change this code)
        int offset = 0, i = 0;
        for(i=0; i<MAXBINS; i++){
            int ss = BinSizes[i];
            BinSizes[i] = offset;
            offset += ss;
        }

        for(j=0; j<N; j++){
            unsigned int ii = (morton_codes[j]>>sft) & 0x07;
            permutation_vector[BinSizes[ii]] = index[j];
            sorted_morton_codes[BinSizes[ii]] = morton_codes[j];
            BinSizes[ii]++;
        }

        //swap the index pointers
        swap(&index, &permutation_vector);

        //swap the code pointers
        swap_long(&morton_codes, &sorted_morton_codes);


        //This is to make sure threads get the right arguments
        offsets[0] = 0;
        offset = 0;
        for (i=0; i<MAXBINS; i++) {
            sizes[i] = BinSizes[i] - offset;
            offset += sizes[i];
        }

        offsets[0] = 0;
        offset = 0;
        for(i = 0; i<MAXBINS-1; i++) {
            int size = BinSizes[i] - offset;
            offset +=size;
            offsets[i+1] = offset;
        }

        /* Call the function recursively to split the lower levels */
        for(i=0; i<MAXBINS; i++){
            args[i].morton_codes = &morton_codes[offsets[i]];
            args[i].sorted_morton_codes = &sorted_morton_codes[offsets[i]];
            args[i].permutation_vector = &permutation_vector[offsets[i]];
            args[i].index = &index[offsets[i]];
            args[i].level_record = &level_record[offsets[i]];
            args[i].N = sizes[i];
            args[i].population_threshold = population_threshold;
            args[i].sft = sft-3;
            args[i].lv = lv+1;

            rc = pthread_create(&threads[i], &joinable, pthread_radix_sort,(void *)&args[i]);
            if (rc)
            {
                printf("Error: pthread_create returned code %d\n", rc);
                return;
            }
        }

        pthread_attr_destroy(&joinable);
        for(i = 0; i < MAXBINS; i++)
        {
            rc = pthread_join(threads[i], &status);
            if (rc) {
                printf("Error: pthread_join returned code %d\n", rc);
                return;
            }
        }
    }
}

