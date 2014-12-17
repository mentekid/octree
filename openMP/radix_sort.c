#include "stdio.h"
#include "stdlib.h"
#include <string.h>

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

    //thread management
    extern int numThreads;
    extern int activeThreads;
    int startNewThreads = 0;
    //if there's space for new threads, set flag to 1 and add the new threads to the count
    //once calling is over, decrement count

    level_record[0] = lv; // record the level of the node

    if(N<=population_threshold || sft < 0) { // Base case. The node is a leaf
        memcpy(permutation_vector, index, N*sizeof(unsigned int)); // Copy the pernutation vector
        memcpy(sorted_morton_codes, morton_codes, N*sizeof(unsigned long int)); // Copy the Morton codes 

        return;
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

        
        //This is to make sure threads get the right arguments
        int sizes[MAXBINS];
        offset = 0;
        for (i=0; i<MAXBINS; i++) {
            sizes[i] = BinSizes[i] - offset;
            offset += sizes[i];
            //printf("sizes[%d]=%d\n", i, sizes[i]);
        }

        int offsets[MAXBINS];
        offset = 0;
        offsets[0] = 0;
        for(i = 0; i<MAXBINS-1; i++) {
            int size = BinSizes[i] - offset;
            offset +=size;
            offsets[i+1] = offset;
        }
        
        #pragma omp flush(activeThreads)
        //Allow creation of new threads? Only if the number has not been exceeded
        if (activeThreads < numThreads && 0 == startNewThreads){
            startNewThreads = 1; //allow creation of more threads
        }
        if (activeThreads > numThreads && 1 == startNewThreads){
            startNewThreads = 0; //stop creating more threads
        }
        

        #pragma omp flush(startNewThreads)
        omp_set_nested(startNewThreads);
        /* Call the function recursively to split the lower levels */
        #pragma omp parallel num_threads(numThreads)
        {
            #pragma omp for private(i) nowait\
            schedule(static)
            for(i=0; i<MAXBINS; i++){
                if (omp_get_nested()){
                    #pragma omp atomic
                    activeThreads ++; //account for new thread
                    #pragma omp flush(activeThreads)
                }
                truncated_radix_sort(&morton_codes[offsets[i]], 
                        &sorted_morton_codes[offsets[i]], 
                        &permutation_vector[offsets[i]], 
                        &index[offsets[i]], &level_record[offsets[i]], 
                        sizes[i], 
                        population_threshold,
                        sft-3, lv+1);
                if(omp_get_nested()){
                    #pragma omp atomic
                    activeThreads--;  //thread about to terminate
                    #pragma omp flush(activeThreads)
                }
            }
        }
    } 
}

