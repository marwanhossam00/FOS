#include <inc/lib.h>

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
int32 user_array[(USER_HEAP_MAX - (USER_HEAP_START + (1<<11) + PAGE_SIZE)) / PAGE_SIZE] = {0};
int last_free = 0;

void *sbrk(int increment)
{
	//cprintf("sbrk syscall\n");
	void *return_va = sys_sbrk(increment);
	//cprintf("return_va = %x\n", return_va);
	return return_va;
	//return (void*) sys_sbrk(increment);
}
//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	//cprintf("1. MALLOC\n");
	//if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #12] [3] USER HEAP [USER SIDE] - malloc()
	// Write your code here, remove the panic and write your code
	//panic("malloc() is not implemented yet...!!");
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy
	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{
		void * return_va;
		if(sys_isUHeapPlacementStrategyFIRSTFIT())
		{
			//cprintf("2. Block Allocator\n");
			return_va = alloc_block_FF(size);
		}
		//Ht return NULL lw ml2ash aw el va
		return return_va;
	}
	if(myEnv->init_user_array == 0)
	{
		//cprintf("INIT user_array\n");
		user_array[0] = myEnv->num_of_pages;
		user_array[myEnv->num_of_pages-1] = myEnv->num_of_pages;
	}
	uint32 kamPage = ROUNDUP(size, PAGE_SIZE);
	kamPage = kamPage/PAGE_SIZE;


	//cprintf("INSIDE PAGE ALLOCATOR\n");

	//cprintf("kamPage = %d\n", kamPage);
	uint32 tmp, return_va;
	uint8 flag = 0;
	int i;
	//cprintf("num_of_pages = %d\n", myEnv->num_of_pages);
	//cprintf("array[0] = %d\n", user_array[0]);
	for(i = last_free; i < myEnv->num_of_pages;)
	{
		if(user_array[i] >= kamPage && user_array[i] > 0)
		{
			tmp = myEnv->user_heap_block_hard_limit + PAGE_SIZE + (i*PAGE_SIZE);
			flag = 1;
			//cprintf("flag = %d\n", flag);
			if(last_free == i && (i+kamPage) < myEnv->num_of_pages)
				last_free = (i+kamPage);
			else	last_free = MIN(last_free, i+kamPage);
			break;
		}
		if(user_array[i] > 0)	i += user_array[i];
		else if(user_array[i] < 0)	i += (user_array[i]*-1);
		else	i++;
	}
	if(flag == 0)	return NULL;

	return_va = tmp;
	//cprintf("i = %d\n", i);
	int32 val = user_array[i];
	user_array[i] = 0;
	user_array[i+val-1] = 0;
	if(val-kamPage > 0)
	{
		user_array[i+kamPage] = val-kamPage;
		user_array[i+kamPage+(val-kamPage)-1] = val-kamPage;
	}
	user_array[i] = -kamPage;
	sys_allocate_user_mem(tmp, size);
	return (void *)return_va;
}



//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #14] [3] USER HEAP [USER SIDE] - free()
	// Write your code here, remove the panic and write your code
	//panic("free() is not implemented yet...!!");

	uint32 va = (uint32)virtual_address;
	uint32 curr_limit = myEnv->user_heap_block_hard_limit;
	uint32 curr_start = myEnv->user_heap_block_start;
	if(va >= USER_HEAP_START && va < curr_limit)
	{
		//cprintf("I AM IN FREE BLOCK\n");
		free_block(virtual_address);
		return;
	}
	int32 curr = ROUNDDOWN(va, PAGE_SIZE);
	curr = curr - (curr_limit + PAGE_SIZE);
	curr = curr / PAGE_SIZE;
	int curr_pages;
	if(va >= curr_limit + PAGE_SIZE && va < USER_HEAP_MAX)
	{
		//cprintf("I AM IN PAGE FREE\n");
		curr_pages = user_array[curr];
		//cprintf("curr = %d\n", curr);
		//cprintf("curr_pages = %d\n", user_array[curr]);
		sys_free_user_mem(va, curr_pages*-4096);
		int32 left = 0, right = 0;
		curr_pages *= -1;
		if((curr-1) >= 0)
		{
			//cprintf("checking left\n");
			left = user_array[curr-1];
			//cprintf("left = %d\n", left);
		}
		if(curr+(user_array[curr]*-1) < myEnv->num_of_pages)
		{
			//cprintf("checking right\n");
			right = user_array[curr + (user_array[curr]*-1)];
			//cprintf("right = %d\n", right);
		}
		if(left > 0 && right > 0)
		{
			last_free = MIN(last_free, (curr - left));
			int32 val = left + right + curr_pages;
			user_array[curr - left] = val;
			user_array[(curr + curr_pages + right)-1] = val;
			user_array[curr] = 0;
			user_array[curr-1] = 0;
			user_array[curr+curr_pages] = 0;
		}
		else if(left > 0)
		{
			last_free = MIN(last_free, (curr - left));
			int32 val = left + curr_pages;
			user_array[curr - left] = val;
			user_array[(curr + curr_pages)-1] = val;
			user_array[curr] = 0;
			user_array[curr-1] = 0;
		}
		else if(right > 0)
		{
			last_free = MIN(last_free, curr);
			int32 val = right + curr_pages;
			user_array[curr] = val;
			user_array[(curr + curr_pages + right)-1] = val;
			user_array[curr + curr_pages] = 0;
		}
		else
		{
			last_free = MIN(last_free, curr);
			//cprintf("LEFT AND RIGHT NOT FREE\n");
			user_array[curr] = curr_pages;
			user_array[(curr + curr_pages)-1] = curr_pages;
		}
		//cprintf("DONE FREE MEM\n");
	}
	return;
}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	//cprintf("INSIDE smalloc 1\n");
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #18] [4] SHARED MEMORY [USER SIDE] - smalloc()
	// Write your code here, remove the panic and write your code
	//panic("smalloc() is not implemented yet...!!");
	 if (myEnv->init_user_array == 0)
	 {
		 user_array[0] = myEnv->num_of_pages;
		 user_array[myEnv->num_of_pages - 1] = myEnv->num_of_pages;
	 }
	 //cprintf("2\n");
	 uint32 kamPage = ROUNDUP(size, PAGE_SIZE);
	 kamPage = kamPage / PAGE_SIZE;
	 if(kamPage>myEnv->num_of_pages)return NULL;
	 uint32 tmp, return_va;
	 uint8 flag = 0;
	 int i;
	 //cprintf("3\n");
	 if (sys_isUHeapPlacementStrategyFIRSTFIT())
	 {
		 //cprintf("4\n");
		 for (i = last_free; i < myEnv->num_of_pages;)
		 {
			 if (user_array[i] >= kamPage && user_array[i] > 0)
			 {
				 tmp = myEnv->user_heap_block_hard_limit + PAGE_SIZE + (i * PAGE_SIZE);
				 flag = 1;
				 if(last_free == i && (i+kamPage) < myEnv->num_of_pages)
					 last_free = (i+kamPage);
				 else	last_free = MIN(last_free, i+kamPage);
				 break;
			 }
			 if (user_array[i] > 0) i += user_array[i];
			 else if (user_array[i] < 0) i += (user_array[i] * -1);
			 else i++;
		 }
	 }
	 //cprintf("5\n");
	 if (flag == 0)
	 {
		 //cprintf("Error\n");
		 // cprintf("mfesh block 3dl found in smalloc.\n");
		 return NULL;
	 }

	 return_va = tmp;
	 //cprintf(" block  index %d, address rag3: %p\n", i, (void*)return_va);

	 int32 val = user_array[i];
	 user_array[i] = 0;
	 user_array[i + val - 1] = 0;

	 if (val - kamPage > 0)
	 {
		 user_array[i + kamPage] = val - kamPage;
		 user_array[i + kamPage + (val - kamPage) - 1] = val - kamPage;
		 //cprintf(" block  : user_array[%d] = %d\n", i + kamPage, user_array[i + kamPage]);
	 }
	 //cprintf("6\n");
	 user_array[i] = -kamPage;
	 sys_allocate_user_mem(tmp, size);
	 int ss = sys_createSharedObject(sharedVarName, size, isWritable, (void*)tmp);
	 if(ss < 0)return NULL;

	    //cprintf(" el address el rag33333333: %p\n", return_va);
	 if(ss >= 0)
	 {
		 //cprintf("DONE 1\n");
		 return (void*) return_va;
	 }
	 else
	 {
		 //cprintf("DONE 2\n");
		 return NULL;
	 }

}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//TODO: [PROJECT'24.MS2 - #20] [4] SHARED MEMORY [USER SIDE] - sget()
	// Write your code here, remove the panic and write your code
	//panic("sget() is not implemented yet...!!");
	 uint32 size = sys_getSizeOfSharedObject(ownerEnvID, sharedVarName);

	 if (size == 0) {
	       return NULL;
	 }
	 if (myEnv->init_user_array == 0)
	 {
		 user_array[0] = myEnv->num_of_pages;
	     user_array[myEnv->num_of_pages - 1] = myEnv->num_of_pages;
	 }

	 uint32 kamPage = ROUNDUP(size, PAGE_SIZE);
	 kamPage = kamPage / PAGE_SIZE;
	 uint32 tmp, return_va;
	 uint8 flag = 0;
	 int i;

	 if (sys_isUHeapPlacementStrategyFIRSTFIT())
	 {
	   for (i = 0; i < myEnv->num_of_pages;)
	   {
	      if (user_array[i] >= kamPage && user_array[i] > 0)
	      {
	    	  tmp = myEnv->user_heap_block_hard_limit + PAGE_SIZE + (i * PAGE_SIZE);
	          flag = 1;
	          break;
	      }
	      if (user_array[i] > 0) i += user_array[i];
	      else if (user_array[i] < 0) i += (user_array[i] * -1);
	      else i++;
	   }
	 }

	 if (flag == 0) {
		return NULL;
	 }
	 return_va = tmp;

	 //cprintf(" block at index %d, returning address: %p\n", i, (void*)return_va);

	 int32 val = user_array[i];
	 user_array[i] = 0;
	 user_array[i + val - 1] = 0;

	 if (val - kamPage > 0)
	 {
	    user_array[i + kamPage] = val - kamPage;
	    user_array[i + kamPage + (val - kamPage) - 1] = val - kamPage;
	    //cprintf(" block el m2som : user_array[%d] = %d\n", i + kamPage, user_array[i + kamPage]);
	 }

	 user_array[i] = -kamPage;
     //cprintf(" before---------\n");

	 sys_allocate_user_mem(tmp, size);
     //cprintf(" after---------\n");

	 int ss = sys_getSharedObject(ownerEnvID, sharedVarName, (void*)tmp);



	 //cprintf("Returning sget address: %p\n", return_va);
	 if(ss >=0){
		 return (void*)return_va;
	 }
	 return NULL;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [USER SIDE] - sfree()
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//[PROJECT]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}
