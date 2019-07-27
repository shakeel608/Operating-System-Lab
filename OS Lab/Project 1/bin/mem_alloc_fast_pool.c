#include <assert.h>
#include <stdio.h>

#include "mem_alloc_fast_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"


void init_fast_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size) {
    /* TO BE IMPLEMENTED */

    //other fields setup by the init procedure
    
    if (my_mmap(size) != NULL ){
        p->start = my_mmap(size);
    }
    else {
	printf("Error allocating memory");
	exit(0);
    }
        
    printf("p->start init mem%p\n",p->start);
    //printf("size %ld\n",size);
    
    //printf("p end  %p\n",p->end );
    p->end = (char *) p->start + size;
    p->first_free = p->start;   

    int numBlocks = size/max_request_size;

    //printf("numBlocks = %d \n",numBlocks);
    mem_fast_free_block_t *headerPtr = p->start;

    for(int i = 1; i < numBlocks; i++) {                          // creates free list for numBlocks        
        headerPtr->next =  (mem_fast_free_block_t *)((char *)headerPtr + max_request_size);        // increments by block size *1
        headerPtr = headerPtr->next;
 
     
    }
    //printf("last pointer returned by mmap is  %p\n",headerPtr);
    headerPtr->next = NULL;
    
    
    //printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
}


void *mem_alloc_fast_pool(mem_pool_t *pool, size_t size) {
    /* TO BE IMPLEMENTED */
    mem_fast_free_block_t * headerPtr = pool->first_free;
    if(size >= 0){
	 
	 pool->first_free = headerPtr->next;
	 printf("%p:  pool->first_free in Alloc!\n", pool->first_free);
	 headerPtr->next = NULL;
    }
    else{
    	exit(0);
    }


	return (void *) headerPtr;  
}

void mem_free_fast_pool(mem_pool_t *pool, void *b) {

    mem_fast_free_block_t * header = (mem_fast_free_block_t * ) b;
    mem_fast_free_block_t * initialHeader = pool->first_free;
    pool->first_free = header;//LIFO
    header->next = initialHeader;
     

    
    //printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
}

size_t mem_get_allocated_block_size_fast_pool(mem_pool_t *pool, void *addr) {
    size_t res;
    res = pool->max_request_size;
    return res;
}
