/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
__inline__ uint32 get_block_size(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (*curBlkMetaData) & ~(0x1);
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
//
__inline__ int8 is_free_block(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
//	cprintf("---- Address in free block --- %x", curBlkMetaData);
//	cprintf("---- the result --- %d", ~(*curBlkMetaData));
	return (~(*curBlkMetaData) & 0x1) ;
}

//===========================
// TRANSFER DATA:
//===========================
//

int8 transfer_data(void * va_old, void * va_new, uint32 old_size, uint32 new_size){
	//Sizes including metadata

	//Mynf3sh tn2l feh mkan 2as8r mnk

	if(old_size > new_size)
	{
		return -1;
	}
	char * it_old = (char *) va_old;
	char * it_new = (char *) va_new;

	for(uint32 i = 1; i <= (old_size - (1<<3)); i++)
	{
		(*it_new) = (*it_old);
		it_old++;
		it_new++;
	}
	return 1;
}


//===========================
// 3) ALLOCATE BLOCK:
//===========================

void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockElement* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d, va: %x)\n", get_block_size(blk), is_free_block(blk), blk) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//cprintf("\n INSIDE DBA INIT\n");
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (initSizeOfAllocatedSpace % 2 != 0) initSizeOfAllocatedSpace++; //ensure it's multiple of 2
		if (initSizeOfAllocatedSpace == 0)
			return ;
		is_initialized = 1;
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #04] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("initialize_dynamic_allocator is not implemented yet");
	//Your Code is Here...
	//Initialize the List
	LIST_INIT(&freeBlocksList);
	uint32* BEG = (uint32*) daStart ;
	*BEG = 1;
	uint32* END = (uint32*) (daStart +  initSizeOfAllocatedSpace - sizeof(int));
	*END = 1;
	uint32* HEADER = (uint32*) (daStart + sizeof(int));
	*HEADER = initSizeOfAllocatedSpace - 2*sizeof(int);
	uint32* FOOTER = (uint32*) (daStart +  initSizeOfAllocatedSpace - 2*sizeof(int));
	*FOOTER = initSizeOfAllocatedSpace - 2*sizeof(int);
	struct BlockElement *firstFree;
	uint32* firstFreePtr = (uint32 *)(daStart + 2*sizeof(int)); // next_pointer after (BEG & Header)
	int8 check = is_free_block((void *)firstFreePtr);
	uint32 sssize = get_block_size((void *)firstFreePtr);
	firstFree = (struct BlockElement *) firstFreePtr;
	LIST_INSERT_HEAD(&freeBlocksList, firstFree);

//	cprintf("---- is free --- %d\n", check);
//	cprintf("---- firstFree --- %x\n", firstFree);
//	cprintf("---- Size --- %d\n", sssize);
//	cprintf("---- Header Content ---%d\n", *HEADER);
}
//==================================
// [2] SET BLOCK HEADER & FOOTER:
//==================================
void set_block_data(void* va, uint32 totalSize, bool isAllocated)
{
	//TODO: [PROJECT'24.MS1 - #05] [3] DYNAMIC ALLOCATOR - set_block_data
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("set_block_data is not implemented yet");
	//Your Code is Here...
	//Check For Minimum Block Size
	if(totalSize < (1 << 4)) // Minimum Block size is 16
		totalSize = (1 << 4);
	if(totalSize%2) // If total size is not even. make it even to utilize the LSB.
		totalSize++;
	uint32 * tmpPtrHeader = ((uint32 *)va - 1); //Going to Header
	uint32 * tmpPtrFooter = (uint32 *)(va + totalSize - (2*sizeof(int))); //Going to Footer
	//Info including meta data
	uint32 info = totalSize + isAllocated; //This the info that is assigned in Header & Footer
	(*tmpPtrHeader) = info;
	(*tmpPtrFooter) = info;
}


//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	//cprintf("\nInside Dynamic Block Allocator\n");
	if(size == 0)	return NULL;
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE ;
		if (!is_initialized)
		{
			//cprintf("init\n");
			uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
			uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
			uint32 da_break = (uint32)sbrk(0);
			initialize_dynamic_allocator(da_start, da_break - da_start);
		}
	}
	//==================================================================================
	//==================================================================================
	//TODO: [PROJECT'24.MS1 - #06] [3] DYNAMIC ALLOCATOR - alloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("alloc_block_FF is not implemented yet");
	//Your Code is Here...
	/*1 input size less than 8 = 8 (----Done----)
      2 to do input size should added to 8 (header footer) (----Done----)
	  3 loop on list (for each)(----Done----)
      4 law el first block size akbr mn el required size
      5 to handle if (block size - required size >= 16) blocks will be separated (----DONE----)
	  6 else (block size - required size < 16) will be internal fragmentation
      7 move with pointer and edit headers and footers  (----DONE----)
	  9 NULL if the requested size is 0.*/

	uint32 requiredSize = size + 2*sizeof(int);
	struct BlockElement *elybeshawr;
	//cprintf("SIZE = %d\n", size);
	//max block size in the list to save iterating in the loop
	LIST_FOREACH(elybeshawr, &freeBlocksList)
	{
		//cprintf("No SBRK needed\n");
		void * tmp = (void *) elybeshawr;
		uint32 _7agm_Block = get_block_size(tmp);
		uint8 block_Fadya_Wla_La = is_free_block(tmp);
		if(_7agm_Block >= requiredSize && block_Fadya_Wla_La == 1){
			uint32 elba2y = _7agm_Block - requiredSize;
			if(elba2y >= (1<<4))
			{
				set_block_data(tmp, requiredSize, 1);
				void * returnPtr = tmp;
				//e3ml block element ll elba2y
				tmp = (void *)((uint32)tmp + requiredSize);
				struct BlockElement *block_ll_elba2y = (struct BlockElement *) tmp;

				set_block_data(tmp, elba2y, 0);
				LIST_INSERT_AFTER(&freeBlocksList, elybeshawr, block_ll_elba2y);
				LIST_REMOVE(&freeBlocksList,elybeshawr);
				return returnPtr;
			}
			else
			{
				set_block_data(tmp, _7agm_Block, 1);
				LIST_REMOVE(&freeBlocksList, elybeshawr);
				return tmp;
			}
		}
	}
	//Call sbrk
	//msh m7tag a7sb m7tag kam page la2n logically m4 h7tag aktr mn page wa7da
	//Law et7gz f3lan
	//cprintf("sbrk needed\n");
	void *new_brk = sbrk(1);
	if(new_brk != (void *)-1)
	{
		//cprintf("\nIN SBRK DBA\n");
		//ghz el list w el END Block
		uint32 *END_block = (uint32 *)((uint32)new_brk + PAGE_SIZE - sizeof(int));
		//cprintf("NEW END = %x\n", END_block);
		*END_block = 1;
		//cprintf("Done END BLOCK\n");
		set_block_data(new_brk, PAGE_SIZE, 0);
		//cprintf("Done SET BLOCK DATA\n");
		free_block(new_brk);
		//cprintf("Done FREE BLOCK\n");
		//cprintf("Done 1\n");

		struct BlockElement *lastElem;
		lastElem = LIST_LAST(&freeBlocksList);
		uint32 size_last_elem = get_block_size((void *)lastElem);
		uint32 elba2y = size_last_elem - requiredSize;
		//cprintf("Done 2\n");
		set_block_data((void *)lastElem, requiredSize, 1);

		void * tmp = (void *)lastElem;

		lastElem = (void *)((uint32)lastElem + requiredSize);
		struct BlockElement *block_ll_elba2y = (struct BlockElement *) lastElem;
		//cprintf("Done 3\n");
		set_block_data(lastElem, elba2y, 0);
		LIST_INSERT_AFTER(&freeBlocksList, (struct BlockElement *)tmp, block_ll_elba2y);
		LIST_REMOVE(&freeBlocksList,(struct BlockElement *)tmp);
		//cprintf("Done 4\n");
		//cprintf("tmp %x\n", tmp);
		//cprintf("\nOUT SBRK DBA\n");
		return tmp;
	}
	else
		return NULL;
	//cprintf("\nOut Dynamic Block Allocator\n");
}
//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'24.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("alloc_block_BF is not implemented yet");
	//Your Code is Here...
	//==================================================================================

	{
			if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
			if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
				size = DYN_ALLOC_MIN_BLOCK_SIZE ;
			if (!is_initialized)
			{
				uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
				uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
				uint32 da_break = (uint32)sbrk(0);
				initialize_dynamic_allocator(da_start, da_break - da_start);
			}
		}
	if (size == 0) return NULL;
	uint32 requiredSize = size + 2 * sizeof(int);
	struct BlockElement *bestFitBlock = NULL;
	uint32 smallestFitSize = 4294967295; // max in uint32 in worst case
	struct BlockElement *elybeshawr;

	LIST_FOREACH(elybeshawr, &freeBlocksList) {

		uint32 _7agm_Block = get_block_size(elybeshawr);

		uint8 block_Fadya_Wla_La = is_free_block(elybeshawr);

		if (_7agm_Block >= requiredSize && block_Fadya_Wla_La && _7agm_Block < smallestFitSize) {

				smallestFitSize = _7agm_Block;
				bestFitBlock = elybeshawr;

		}
	}

	if (bestFitBlock == NULL) {
		//sbrk(); NOT IMPLEMENTED YET
		return NULL;
	}

	void *tmp = (void *) bestFitBlock;
	uint32 _7agm_Block = get_block_size(tmp);
	uint32 elba2y = _7agm_Block - requiredSize;

	if (elba2y >= (1 << 4)) {

		set_block_data(tmp, requiredSize, 1);
		void *returnPtr = tmp;
//		cprintf("va must be %x\n", bestFitBlock);
//		cprintf("requiredSize %d\n", requiredSize);
//		cprintf("elba2y %d\n", elba2y);
//		cprintf("7agm el block %d\n", _7agm_Block);
//		cprintf("------\n");
		tmp = (void *)((uint32)tmp + requiredSize);
		struct BlockElement *block_ll_elba2y = (struct BlockElement *) tmp;
		set_block_data(tmp, elba2y, 0);

		LIST_INSERT_AFTER(&freeBlocksList, bestFitBlock, block_ll_elba2y);
		LIST_REMOVE(&freeBlocksList, bestFitBlock);

		return returnPtr;
	} else {
		set_block_data(tmp, _7agm_Block, 1);
		LIST_REMOVE(&freeBlocksList, bestFitBlock);
		return tmp;
	}
	return NULL;

}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'24.MS1 - #07] [3] DYNAMIC ALLOCATOR - free_block
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("free_block is not implemented yet");
	//Your Code is Here...
	if(va == NULL)	return;

	//cprintf("\nIN FREE BLOCK\n");
	uint32 blockSize = get_block_size(va);
	//cprintf("current block size = %d\n", blockSize);
	//cprintf("Done get block size 1\n");
	//Law 3nd El BEG aw El END
	if(blockSize == 0)	return;

	void * footerLeft = (void *) (va - sizeof(int));
	void * headerRight = (void *) (va + blockSize);

	uint32 leftBlockSize = get_block_size(footerLeft);
	//cprintf("Done get block size 2\n");
	uint32 rightBlockSize = get_block_size(headerRight);
	//cprintf("HEADER RIGHT =  %x\n", headerRight);
	//cprintf("Done get block size 3\n");
	//cprintf("right block size = %d\n", rightBlockSize);

	int8 isLeftFree = is_free_block(footerLeft);
	//cprintf("Done is free block 1\n");
	int8 isRightFree = is_free_block(headerRight);
	//cprintf("Done is free block 2\n");
	//cprintf("right free? = %d\n", isRightFree);

	void * va_Left = (void *)((uint32)va - leftBlockSize);
	void * va_Right = (void *)((uint32)va + blockSize);

	//cprintf("va_Right = %x\n", va_Right);
	// 11
	// 01
	// 10
	// 00
	if(isLeftFree == 0 && isRightFree == 0)
	{
		if(LIST_SIZE(&freeBlocksList) == 0)
		{
			LIST_INSERT_HEAD(&freeBlocksList,(struct BlockElement *) va);
			//cprintf("Done LIST INSERT\n");
			set_block_data(va, blockSize, 0);
			//cprintf("Done set block data 1\n");
			return;
		}
		struct BlockElement * iterator;
		struct BlockElement * tmp = (struct BlockElement *) va;
		uint8 done = 0;
		LIST_FOREACH(iterator, &freeBlocksList)
		{
			if((uint32)iterator > (uint32)tmp && done == 0)
			{
				LIST_INSERT_BEFORE(&freeBlocksList, iterator, tmp);
				//cprintf("Done LIST INSERT\n");
				done = 1;
			}
		}
		if(done == 0)
		{
			LIST_INSERT_TAIL(&freeBlocksList,tmp);
			//cprintf("Done LIST INSERT\n");
		}
		set_block_data(tmp, blockSize, 0);
		//cprintf("Done set block data 2\n");
	}
	else if(isLeftFree == 1 && isRightFree == 0)
	{
		blockSize += leftBlockSize;
		set_block_data(va_Left, blockSize, 0);
		//cprintf("Done set block data 3\n");
	}
	else if(isLeftFree == 0 && isRightFree == 1 && rightBlockSize != 0)
	{
		blockSize += rightBlockSize;
		set_block_data(va, blockSize, 0);
		//cprintf("Done set block data 4\n");
		//m7tag a3dl al list
		LIST_INSERT_BEFORE(&freeBlocksList, (struct BlockElement *)va_Right, (struct BlockElement * ) va);
		//cprintf("Done LIST INSERT\n");
		LIST_REMOVE(&freeBlocksList, (struct BlockElement *) va_Right);
		//cprintf("Done LIST REMOVE\n");
	}
	else if(isLeftFree && isRightFree)
	{
//		cprintf("I should be here\n");
//		cprintf("Block Size of the block %d\n", blockSize);
		blockSize = (blockSize + leftBlockSize + rightBlockSize);
//		cprintf("Full Merged Block Size %d\n", blockSize);
//		cprintf("Right Block Size %d\n", rightBlockSize);
//		cprintf("Left Block Size %d\n", leftBlockSize);
//		cprintf("Left va %x\n", va_Left);
		LIST_REMOVE(&freeBlocksList, (struct BlockElement *)va_Right);
		//cprintf("Done LIST INSERT\n");
		set_block_data(va_Left, blockSize, 0);
		//cprintf("Done set block data 5\n");
		void *footerInfo = (void *)((uint32)va_Left + blockSize);
		uint32 * tmpFooterInfo = (uint32 *)((uint32 *)footerInfo - 2);
		//cprintf("footerInfo %d\n", *tmpFooterInfo);
	}
}

//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{

    if(va != NULL && new_size == 0){
        free_block(va);
        return NULL;
    }
    if(va == NULL && new_size > 0){
        va = alloc_block_FF(new_size);
        return va;
    }
    if(va == NULL && new_size == 0){
        va = alloc_block_FF(new_size);
        return va;
    }

    uint32 oldSize = get_block_size(va);

    if(oldSize == new_size){
        return va;
    }
    //print_blocks_list(freeBlocksList);
    new_size += (1<<3); //Size Bta3 el meta data

	//Header ely 3la ymyny
    void * va_Right = (void *)((uint32 )va + oldSize);

    //elly 3la ymeny fady wla la2
    int8 isRightFree = is_free_block(va_Right);
    //Msa7t elly 3la ymyny
    uint32 rightSize = get_block_size(va_Right);

    //cprintf("Inside function vaRight %x\n",va_Right);

	//el limit bta3y 16
	if(new_size < (1 << 4))
		new_size = (1 << 4);
	if(new_size%2)	new_size++;

    //Handle el awl decrease size
    if(new_size < oldSize){
		uint32 elba2y = oldSize - new_size;
		if(elba2y < (1<<4)){  // [Internal Fragmentation]
			return va;
		}
		else{
			// Hna hyb2a feh ba2y block
			// free block kda kda fe el 7altyn elly 3la ymyny fay wla la2
			// el function free block ht handle el kalam dah

			//Hn3dl el info
			set_block_data(va, new_size, 1);
			//Hro7 ll 7tah el free
			void * tmp = (void *)((uint32) va + new_size);
			set_block_data(tmp, elba2y, 0);
			//H3ml free ll block
			free_block(tmp);
			return va;
		}
    }
    if(new_size > oldSize){
		uint32 needed = new_size - oldSize;
		int32 newRightSize = rightSize - needed;

//		cprintf("Inside function newSize %d\n", new_size);
//		cprintf("Inside function rightSize %d\n", rightSize);
//		cprintf("Inside function oldSize %d\n", oldSize);
//		cprintf("Inside function newRightSize %d\n", newRightSize);
//		cprintf("Inside function isRightFree = %d\n", isRightFree);
//		cprintf("Inside function va %x\n", va);
//		cprintf("Inside function va_right = %x\n", va_Right);
//		cprintf("Inside function va Free? = %d\n", is_free_block(va));
    	if(isRightFree && newRightSize >= 0){
    		//cprintf("relocate in same place\n");
    		if(newRightSize < (1 << 4)){ // INTERNAL FRAG
    			//cprintf("print blocks list before LIST_REMOVE\n");
    			//print_blocks_list(freeBlocksList);
    			set_block_data(va, new_size, 1);
    			LIST_REMOVE(&freeBlocksList,(struct BlockElement *)	va_Right);
    			//cprintf("print blocks list after LIST_REMOVE\n");
    			//print_blocks_list(freeBlocksList);
    		}
    		else{
    			void * tmp = (void *)((uint32) va + new_size);
    			set_block_data(va, new_size, 1);
    			LIST_INSERT_AFTER(&freeBlocksList,(struct BlockElement *)va_Right ,(struct BlockElement *)tmp);
    			LIST_REMOVE(&freeBlocksList,(struct BlockElement *)	va_Right);
    			set_block_data(tmp, newRightSize, 0);
    		}
    		return va;
    	}
    	else{
    		//cprintf("LAZM relocate another place\n");
    		//Lazm Relocate ba3edan 3an en ely b3dy fady wla la2
    		//kda kda ht3ml free ll block bta3t el rellocate
    		//el mafrood t3ml save ll data 34an el rellocate
    		// bb3t el new_size na2s el meta data 34an hya btdaf fe el function
    		void * va_new = alloc_block_FF(new_size - (1<<3));

    		//Law m3rftsh a7gz mkan gded 7ta b3d sbrk
    		if(va_new == NULL)	return va;

    		//transfer el data
    		transfer_data(va, va_new, oldSize, new_size);
    		//H3dl el block el old
    		set_block_data(va, oldSize, 0);
    		free_block(va);

    		return va_new;
    	}
    }
    return va;
}

/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/
//=========================================
// [7] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [8] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}
