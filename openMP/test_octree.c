#include "stdio.h"
#include "stdlib.h"
#include "sys/time.h"
#include "utils.h"
#include <omp.h>

#define DIM 3

int main(int argc, char** argv){

    // Time counting variables 
    struct timeval startwtime, endwtime;
    double hashAvg=0, mortonAvg=0, sortingAvg=0, rearrangeAvg=0;

    //openMP variables
    omp_set_nested(1); //don't allow nested threads
    extern int numThreads;
    extern int activeThreads;

    activeThreads=0;

    if (argc != 7) { // Check if the command line arguments are correct 
        printf("Usage: %s N dist pop rep P numThreads\n where\n N:number of points\n dist: distribution code (0-cube, 1-Plummer)\n pop: population threshold\n rep: repetitions\n L: maximum tree height.\n numThreads: maximum threads allowed\n", argv[0]);
        return (1);
    }

    // Input command line arguments
    int N = atoi(argv[1]); // Number of points
    int dist = atoi(argv[2]); // Distribution identifier 
    int population_threshold = atoi(argv[3]); // populatiton threshold
    int repeat = atoi(argv[4]); // number of independent runs
    int maxlev = atoi(argv[5]); // maximum tree height
    numThreads = atoi(argv[6]); // number of threads allowed
    if ( numThreads <= 0 ) numThreads = omp_get_num_procs(); //default

    printf("Running for %d particles with maximum height: %d\n", N, maxlev);

    float *X = (float *) malloc(N*DIM*sizeof(float));
    float *Y = (float *) malloc(N*DIM*sizeof(float));

    unsigned int *hash_codes = (unsigned int *) malloc(DIM*N*sizeof(unsigned int));
    unsigned long int *morton_codes = (unsigned long int *) malloc(N*sizeof(unsigned long int));
    unsigned long int *sorted_morton_codes = (unsigned long int *) malloc(N*sizeof(unsigned long int));
    unsigned int *permutation_vector = (unsigned int *) malloc(N*sizeof(unsigned int)); 
    unsigned int *index = (unsigned int *) malloc(N*sizeof(unsigned int));
    unsigned int *level_record = (unsigned int *) calloc(N,sizeof(unsigned int)); // record of the leaf of the tree and their level

    // initialize the index
    int i = 0;
    for(i=0; i<N; i++){
        index[i] = i;
    }

    /* Generate a 3-dimensional data distribution */
    create_dataset(X, N, dist);

    /* Find the boundaries of the space */
    float max[DIM], min[DIM];
    find_max(max, X, N);
    find_min(min, X, N);

    int nbins = (1 << maxlev); // maximum number of boxes at the leaf level

    int it = 0; 
    // Independent runs
    for(it = 0; it<repeat; it++){

        gettimeofday (&startwtime, NULL); 

        compute_hash_codes(hash_codes, X, N, nbins, min, max); // compute the hash codes

        gettimeofday (&endwtime, NULL);

        double hash_time = (double)((endwtime.tv_usec - startwtime.tv_usec)
                /1.0e6 + endwtime.tv_sec - startwtime.tv_sec);
        hashAvg+=hash_time;

        printf("Time to compute the hash codes: %f\n", hash_time);


        gettimeofday (&startwtime, NULL); 

        morton_encoding(morton_codes, hash_codes, N, maxlev); // computes the Morton codes of the particles

        gettimeofday (&endwtime, NULL);


        double morton_encoding_time = (double)((endwtime.tv_usec - startwtime.tv_usec)
                /1.0e6 + endwtime.tv_sec - startwtime.tv_sec);
        mortonAvg+=morton_encoding_time;

        printf("Time to compute the morton encoding: %f\n", morton_encoding_time);


        gettimeofday (&startwtime, NULL); 

        // Truncated msd radix sort
        truncated_radix_sort(morton_codes, sorted_morton_codes, 
                permutation_vector, 
                index, level_record, N, 
                population_threshold, 3*(maxlev-1), 0);

        gettimeofday (&endwtime, NULL);


        double sort_time = (double)((endwtime.tv_usec - startwtime.tv_usec)
                /1.0e6 + endwtime.tv_sec - startwtime.tv_sec);
        sortingAvg+=sort_time;
        printf("Time for the truncated radix sort: %f\n", sort_time);

        gettimeofday (&startwtime, NULL); 

        // Data rearrangement
        data_rearrangement(Y, X, permutation_vector, N);

        gettimeofday (&endwtime, NULL);


        double rearrange_time = (double)((endwtime.tv_usec - startwtime.tv_usec)
                /1.0e6 + endwtime.tv_sec - startwtime.tv_sec);
        rearrangeAvg+=rearrange_time;

        printf("Time to rearrange the particles in memory: %f\n", rearrange_time);

        /* The following code is for verification */ 
        // Check if every point is assigned to one leaf of the tree
        int pass = check_index(permutation_vector, N); 

        if(pass){
            printf("Index test PASS\n");
        }
        else{
            printf("Index test FAIL\n");
        }

        // Check is all particles that are in the same box have the same encoding. 
        pass = check_codes(Y, sorted_morton_codes, 
                level_record, N, maxlev);

        if(pass){
            printf("Encoding test PASS\n");
        }
        else{
            printf("Encoding test FAIL\n");
        }

    }

    double totalTime = hashAvg+mortonAvg + sortingAvg + rearrangeAvg;
    hashAvg=hashAvg/repeat;
    mortonAvg=mortonAvg/repeat;
    sortingAvg=sortingAvg/repeat;
    rearrangeAvg=rearrangeAvg/repeat;

    printf("Average time for hashing: %f \t %f %%\n", hashAvg, hashAvg/totalTime);
    printf("Average time for encoding: %f\n \t %f%%\n", mortonAvg, mortonAvg/totalTime);
    printf("Average time for sorting: %f\n \t %f%%\n", sortingAvg, sortingAvg/totalTime);
    printf("Average time for rearranging: %f\n \t %f%%\n", rearrangeAvg, rearrangeAvg/totalTime);
    /* clear memory */
    free(X);
    free(Y);
    free(hash_codes);
    free(morton_codes);
    free(sorted_morton_codes);
    free(permutation_vector);
    free(index);
    free(level_record);
}





