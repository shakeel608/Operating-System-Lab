#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "mem_alloc_types.h"
#include "mem_alloc_standard_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"

#define ULONG(x)((long unsigned int)(x))

void add_node(mem_standard_free_block_t *n, mem_standard_free_block_t *nPrev,
		mem_standard_free_block_t *nNext);
		
void del_node(mem_standard_free_block_t *n);


/////////////////////////////////////////////////////////////////////////////

#ifdef STDPOOL_POLICY
    /* Get the value provided by the Makefile */
    std_pool_placement_policy_t std_pool_policy = STDPOOL_POLICY;
#else
    std_pool_placement_policy_t std_pool_policy = DEFAULT_STDPOOL_POLICY;
#endif

/////////////////////////////////////////////////////////////////////////////


void init_standard_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size) {
    /* TO BE IMPLEMENTED */

    p->start = my_mmap(size);
    p->first_free = p->start;
    p->end = (char *)p->start + size;
    printf("Block length: %lu\n",ULONG((char *)p->end - (char *)p->start));
    //printf("p->start %p\n",p->start);
    //printf("p->end %p\n",p->end);
    printf("Memory size is %ld\n", p->end - p->start );
    // Setting Header
    mem_standard_free_block_t *freeHeaderPtr = p->first_free;
    set_block_free(&(freeHeaderPtr->header));
    freeHeaderPtr->prev = NULL;
    freeHeaderPtr->next = NULL;
    
    //printf("freeHeaderPtr->prev %p\n",freeHeaderPtr->prev);
    //printf("freeHeaderPtr->next %p\n",freeHeaderPtr->next);

    printf("value at ist header is %lu \n",freeHeaderPtr->header.flag_and_size);
    set_block_size(&(freeHeaderPtr->header), size - 2 * sizeof(mem_standard_block_header_footer_t));
    printf("value at ist header is %lu \n",freeHeaderPtr->header.flag_and_size);

    //Setting Footer
    mem_standard_block_header_footer_t *footerPtr = (mem_standard_block_header_footer_t *)((char *)p->end - sizeof(mem_standard_block_header_footer_t));
    set_block_free(footerPtr);
    //printf("value at ist footer is %lu \n",footerPtr->flag_and_size);
    set_block_size(footerPtr, size - 2 * sizeof(mem_standard_block_header_footer_t));
    //printf("value at ist footer is %lu \n",footerPtr->flag_and_size);

    
   //printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
}


void * mem_alloc_standard_pool(mem_pool_t * pool, size_t size) {
/* TO BE IMPLEMENTED */

	mem_standard_free_block_t * freeHeaderPtr = pool->first_free;
	printf("-----Allocating-----\n");

	int counter =0;
	void * alloc_addr;
	size_t totalPayloadSize = 2 * sizeof(mem_standard_block_header_footer_t) + size;			
	
	if(size >= 0) { // more than 1 freeblock First fit
		int found = 0;
		while (found == 0 && freeHeaderPtr != NULL) { // while notFound and not end of freelist 
			counter++;
			if (size <= get_block_size( &(freeHeaderPtr->header))) {
				found = 1;

				printf("Address of found freeblock num %d at %lu\n",counter,ULONG(((char*)freeHeaderPtr) - ((char*)(pool->start))));
				// first smallest freeblock
				break;
			}
			freeHeaderPtr = freeHeaderPtr->next;

		} //endwhile
		if (found == 1) {

			mem_standard_block_header_footer_t *allocHeaderPtr = (mem_standard_block_header_footer_t *)freeHeaderPtr;
			
			alloc_addr = (void*)((char *)allocHeaderPtr + sizeof(mem_standard_block_header_footer_t));
						
			printf("totalPayloadSize %ld\n",totalPayloadSize);
					
			mem_standard_block_header_footer_t *allocFooterPtr = (mem_standard_block_header_footer_t *)((char *)allocHeaderPtr + sizeof(mem_standard_block_header_footer_t) + size);//allocPtrFooter
			
			size_t minFreeBlockSize = sizeof(mem_standard_free_block_t) + sizeof(mem_standard_block_header_footer_t);
			mem_standard_free_block_t *updFreePtr;
			
			printf("Min free block size:% ld\n",minFreeBlockSize);
			if((get_block_size(&(freeHeaderPtr->header))-totalPayloadSize) >= minFreeBlockSize){				//splitting				
				
				if(get_block_size(&(freeHeaderPtr->header)) > size){
					
					
					updFreePtr = (mem_standard_free_block_t *)((char *)freeHeaderPtr + totalPayloadSize);	
					
					
					//setting reduced free block's size
					set_block_size((&(updFreePtr->header)),(get_block_size((mem_standard_block_header_footer_t *)((char *)(&(freeHeaderPtr->header))))-totalPayloadSize));
					
					add_node(updFreePtr,freeHeaderPtr,freeHeaderPtr->next);//add updFreePtr bet 2 nodes				
					mem_standard_block_header_footer_t *newFreeFooterPtr = (mem_standard_block_header_footer_t *)((char *)updFreePtr + sizeof(mem_standard_block_header_footer_t) + get_block_size(&(updFreePtr->header)));
					
			
					set_block_size(newFreeFooterPtr,get_block_size(&(updFreePtr->header)));//setting freeblock footer		
					
					set_block_free(&(updFreePtr->header));
				
					
					set_block_free(newFreeFooterPtr);
					
					set_block_size(allocHeaderPtr,size);//setting allocated block's bits
				
					set_block_size(allocFooterPtr,size);//setting allocated block's bits
					
					if(pool->first_free == freeHeaderPtr){ 
						pool->first_free = updFreePtr;
					}
				}
				
				else if((get_block_size((mem_standard_block_header_footer_t *)((char *)(&(freeHeaderPtr->header)))) == size)){
					
					if(pool->first_free == freeHeaderPtr){	
						pool->first_free = freeHeaderPtr->next;
					}
					
				}

				
			}//endIf
			
			del_node(freeHeaderPtr);//delete old free node
								
			set_block_used(allocHeaderPtr);//allocated block header
			//allocated footer info		
			
			set_block_used(allocFooterPtr);//allocated block footer
				
									
		}
		else{
			printf("No free block found to accommodate request");
			exit(0);
		}
		
		printf("------------------------------------------------------------\n");
		return (void *) alloc_addr;
    }

     //printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
     return NULL;
}

void del_node(mem_standard_free_block_t *n){
	if(n->next == NULL && n->prev != NULL){//last node 
		printf("n->next == NULL && n->prev != NULL\n");
		n->prev->next = NULL;
	}
	else if(n->next != NULL && n->prev != NULL){//middle node 
		printf("n->next != NULL && n->prev != NULL\n");
		n->next->prev = n->prev;
		n->prev->next = n->next;
	}
	else if(n->next != NULL && n->prev == NULL){//node at head
		printf("n->next != NULL && n->prev == NULL\n");
		n->next->prev = NULL;
	}
}


void add_node(mem_standard_free_block_t *n, mem_standard_free_block_t *nPrev,
		mem_standard_free_block_t *nNext){
	if(nPrev!=NULL && nNext != NULL){//insert at middle 
		printf("None null\n");
		nNext->prev = n;
		n->next = nNext;
		n->prev = nPrev;
		nPrev->next = n;				
			
	}else if(nPrev!=NULL && nNext == NULL){//insert after last node 
		
		printf("next null prev not null\n");
		n->prev = nPrev;
		nPrev->next = n;
		printf("newFreePtr prev:%p\n",n->prev);
		printf("newFreePtr next:%p\n",n->next);			

	}
	else if(nNext !=NULL && nPrev == NULL){//insert before head
	
		printf("next not null prev null\n");
		n->next = nNext;
		nNext->prev = n;
		n->prev = NULL;
		
		printf("newFreePtr prev:%p\n",n->prev);
		printf("newFreePtr next:%p\n",n->next);			

	}
}

void mem_free_standard_pool(mem_pool_t *pool, void *addr) {

	
	mem_standard_used_block_t *allocHeaderPtr = (mem_standard_used_block_t *)((char *)addr - sizeof(mem_standard_block_header_footer_t));
		
	size_t allocBlockSize = get_block_size((mem_standard_block_header_footer_t *)allocHeaderPtr);
	
	printf("--allocBlocksize to free: %ld\n",allocBlockSize);		
	
	size_t totalNewFreeSize = 0;
	
	mem_standard_free_block_t *newFreePtr;
		

	//Initialise newFreePtr to allocHeaderPtr
	newFreePtr = (mem_standard_free_block_t *)allocHeaderPtr;
	//size to free is allocated blk's size
	totalNewFreeSize = allocBlockSize;
	
	printf("--totalNewFreeSize : %ld\n: ",totalNewFreeSize);
	
	
	mem_standard_block_header_footer_t *prevBlk = NULL;
	mem_standard_block_header_footer_t *nextBlk = NULL;
	
	if(newFreePtr > (mem_standard_free_block_t *)pool->first_free){//free block may be above newFreePtr

		prevBlk = (mem_standard_block_header_footer_t *)((char *)newFreePtr - sizeof(mem_standard_block_header_footer_t));
	}
	
	if(((char *)newFreePtr + allocBlockSize + 2* sizeof(mem_standard_block_header_footer_t)) != pool->end){//free block may be below newFreePtr
		nextBlk = (mem_standard_block_header_footer_t *)((char *)newFreePtr + allocBlockSize + 2* sizeof(mem_standard_block_header_footer_t));
	}
	mem_standard_block_header_footer_t *newFreeFooterPtr = (mem_standard_block_header_footer_t *)((char *) newFreePtr + sizeof(mem_standard_block_header_footer_t)+ totalNewFreeSize);	
	//coalesce
	if(prevBlk != NULL && is_block_free(prevBlk) == 1){//check if prevBlk is free
		totalNewFreeSize = allocBlockSize + get_block_size(prevBlk) + (2* sizeof(mem_standard_block_header_footer_t));
		printf("Coalesce previous totalNewFreeSize%ld\n ",totalNewFreeSize);
			
		newFreePtr = (mem_standard_free_block_t *)((char *)prevBlk  - (get_block_size(prevBlk) + sizeof(mem_standard_block_header_footer_t)));
		
		printf("newFreePtr%lu\n", ULONG(((char*)&(newFreePtr->header) )- ((char*)(pool->start))));

		//del_node(tmpPtr->prev);
		
				
		set_block_size( &(newFreePtr->header), totalNewFreeSize);
		
		newFreeFooterPtr = (mem_standard_block_header_footer_t *)((char *) newFreePtr + sizeof(mem_standard_block_header_footer_t)+ totalNewFreeSize);
	
		set_block_size(newFreeFooterPtr, totalNewFreeSize);

		
		if(nextBlk != NULL && is_block_free(nextBlk)){//check if next blk is free
			totalNewFreeSize = totalNewFreeSize + get_block_size(nextBlk) + 2 * sizeof(mem_standard_block_header_footer_t);
			printf("block_size of next free block %ld\n ",get_block_size(nextBlk));	
			printf("Coalesce next totalNewFreeSize%ld\n ",totalNewFreeSize);

				
			//del_node(nextBlk);			

			set_block_size(&(newFreePtr->header), totalNewFreeSize);
			
			newFreeFooterPtr = (mem_standard_block_header_footer_t *)((char *) newFreePtr + sizeof(mem_standard_block_header_footer_t)+ totalNewFreeSize);
			
	
			set_block_size(newFreeFooterPtr, totalNewFreeSize);
		}

	}	
	else if(nextBlk != NULL && is_block_free(nextBlk)){//coalesce next
		totalNewFreeSize = allocBlockSize + get_block_size(nextBlk) + (2* sizeof(mem_standard_block_header_footer_t));
		printf("Coalesce next totalNewFreeSize%ld\n ",totalNewFreeSize);
	
		//del_node(tmpPtr);
	
						
		set_block_size( &(newFreePtr->header), totalNewFreeSize);
		
		newFreeFooterPtr = (mem_standard_block_header_footer_t *)((char *) newFreePtr + sizeof(mem_standard_block_header_footer_t)+ totalNewFreeSize);
	
		set_block_size(newFreeFooterPtr, totalNewFreeSize);
		

	}
	
	mem_standard_free_block_t * ptrToInsertFree = pool->first_free;


	if(newFreePtr <= (mem_standard_free_block_t *)pool->first_free || pool->first_free == NULL){//to be freed block is before first free		
				
		if(newFreePtr < (mem_standard_free_block_t *)pool->first_free && pool->first_free != NULL){//first free exists

			add_node(newFreePtr,NULL,pool->first_free);						
			
		}else if(newFreePtr == (mem_standard_free_block_t *)pool->first_free){
			add_node(newFreePtr,NULL,ptrToInsertFree->next);
		}else{

			add_node(newFreePtr,NULL,NULL);		
		}
		
		pool->first_free = newFreePtr;	


	}
	else{//to be freed block is after first free
		
		while(newFreePtr > ptrToInsertFree && ptrToInsertFree != NULL){	
	
			ptrToInsertFree->prev = ptrToInsertFree;
			ptrToInsertFree = ptrToInsertFree->next;

		}

		if(ptrToInsertFree == NULL){//last node

			add_node(newFreePtr,ptrToInsertFree->prev,NULL);
				
		}
		if(ptrToInsertFree->prev != NULL){//middle node 

			//update pointers after insert in last entry in free list after to be freed block
				
			add_node(newFreePtr,ptrToInsertFree->prev,ptrToInsertFree);			
		}
			
	}
	
		
	set_block_free(&(newFreePtr->header));		
		
	set_block_free(newFreeFooterPtr);
	
	printf("block_size of freed block: %ld\n",totalNewFreeSize);
	
	printf("pool->first_free :%lu\n",ULONG(((char*)pool->first_free )- ((char*)(pool->start))));
	
	
	printf("-------------------------------------------------------\n");
	
    /* TO BE IMPLEMENTED */
    
    //printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
}

size_t mem_get_allocated_block_size_standard_pool(mem_pool_t *pool, void *addr) {
    /* TO BE IMPLEMENTED */
    mem_standard_block_header_footer_t *headerAddr = (mem_standard_block_header_footer_t *)((char *)addr - sizeof(mem_standard_block_header_footer_t));
    size_t res;
    res = get_block_size(headerAddr);
    
    //printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
    return res;
}



