#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include <kern/conc/sleeplock.h>
#include <kern/proc/user_environment.h>
#include "memory_manager.h"

// added code
int32 my_array[(KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE] = {0};
struct sleeplock kernel_alloc_lock;

//struct sleeplock *kernel_alloc_lock = NULL;
//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'24.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator
	// Write your code here, remove the panic and write your code
	//panic("initialize_kheap_dynamic_allocator() is not implemented yet...!!");
	//cprintf("Inside kheap init\n");
	/*Restore these*/
	if((daStart < KERNEL_HEAP_START || daStart >= KERNEL_HEAP_MAX) || (daLimit < KERNEL_HEAP_START || daLimit >= KERNEL_HEAP_MAX)){
		panic("initialize_kheap_dynamic_allocator() is not initialized with right start or end");
	}
	if(daLimit - daStart > DYN_ALLOC_MAX_SIZE){
		panic("initialize_kheap_dynamic_allocator() is not initialized with right size");
	}

	//init_sleeplock(kernel_alloc_lock, "kernel_alloc_lock");
	HARD_LIMIT = daLimit;

	//cprintf("\nKERNEL_HEAP_START %d\n", KERNEL_HEAP_START);
	//Initialize the array of pages in Page allocator
	//Initialize the Page Allocator free list
	//cprintf("DONE 1\n");
	uint32 sizeOfPageAllocator = KERNEL_HEAP_MAX - (HARD_LIMIT + PAGE_SIZE);
	num_of_pages = ROUNDDOWN(sizeOfPageAllocator, PAGE_SIZE);
	num_of_pages = num_of_pages/PAGE_SIZE;
	//cprintf("DONE 2\n");
	//cprintf("num_of_pages %d\n", num_of_pages);
	//init array

	init_sleeplock(&kernel_alloc_lock, "kernel_alloc_lock");
	my_array[0] = num_of_pages;
	my_array[num_of_pages-1] = num_of_pages;
	last_free = 0;
	initSizeToAllocate = ROUNDUP(initSizeToAllocate, PAGE_SIZE);
	brk = (void *)daStart;
	//cprintf("DONE 3\n");
	//cprintf("brk in KHEAP = %x\n", brk);
	void * brk = sbrk(initSizeToAllocate/PAGE_SIZE);
	initialize_dynamic_allocator(daStart, initSizeToAllocate);
	//cprintf("DONE 4\n");
	//cprintf("kernel heap init DONE\n");
//	struct PageBlockElement *elem;
//	cprintf("IAM HERE PLEASEEEE2222222\n");
//	LIST_INIT(&freePageBlocksList);
//	LIST_INSERT_HEAD(&freePageBlocksList, elem);
//	elem = LIST_FIRST(&freePageBlocksList);
//	cprintf("List Size in init %d\n",LIST_SIZE(&freePageBlocksList));
	return 0;
}

void* sbrk(int numOfPages)
{
	/* numOfPages > 0: move the segment break of the kernel to increase the size of its heap by the given numOfPages,
	 * 				you should allocate pages and map them into the kernel virtual address space,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * numOfPages = 0: just return the current position of the segment break
	 *
	 * NOTES:
	 * 	1) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, return -1
	 */
	//cprintf("-------------------INSIDE sbrk-------------------\n");
	//cprintf("address of sleep lock in kernel = %x\n", &kernel_alloc_lock);
	//acquire_sleeplock(&kernel_alloc_lock);
	//MS2: COMMENT THIS LINE BEFORE START CODING==========
	//return (void*)-1 ;
	//====================================================
	//cprintf("INSIDE KERNEL SBRK\n");
	//TODO: [PROJECT'24.MS2 - #02] [1] KERNEL HEAP - sbrk
	//Write your code here, remove the panic and write your code
	//panic("sbrk() is not implemented yet...!!");
	if(numOfPages == 0)
	{
		//cprintf("-------------------DONE sbrk-------------------\n");
		//release_sleeplock(&kernel_alloc_lock);
		return (void *) brk;
	}
//	cprintf("\nIn Function SBRK\n");
//	cprintf("brk %x\n", brk);
//	cprintf("num_of_pages in sbrk %d\n", numOfPages);
	//check if the new brk exceed the Hard_Limit
	if((uint32)brk + (numOfPages*PAGE_SIZE) > HARD_LIMIT)
	{
		//cprintf("-------------------Done sbrk 2-------------------\n");
		//release_sleeplock(&kernel_alloc_lock);
		return (void *)-1;
	}
	//cprintf("SURVIVED sbrk \n");
	uint32 tmp = (uint32)brk;
	//n7gz el pages
	for(int i = 0; i < numOfPages; ++i)
	{
		struct FrameInfo *ptr_frame_info = NULL;
		int s_OR_F = allocate_frame(&ptr_frame_info);
		//cprintf("SURVIVED sbrk1 \n");
		if(s_OR_F==E_NO_MEM)
		{
			//cprintf("-------------------DONE sbrk 3-------------------\n");
//			release_sleeplock(&kernel_alloc_lock);
			return (void *)-1;
		}
		ptr_frame_info->bufferedVA = tmp;
		s_OR_F = map_frame(ptr_page_directory , ptr_frame_info ,tmp ,PERM_WRITEABLE);
		//cprintf("SURVIVED sbrk2 \n");
		//???????????????????????
		/*what about the allocated frames in the previous iterations in the for loop*/
		/*Make for loop in the next conditions to free the previous frames*/
		//???????????????????????

		if(s_OR_F == E_NO_MEM)
		{
			//cprintf("sbrk ERROR\n");
			free_frame(ptr_frame_info);
			//cprintf("-------------------Done sbrk 4-------------------\n");
			//release_sleeplock(&kernel_alloc_lock);
			return (void *)-1;
		}
		else
		{
			tmp += PAGE_SIZE;
		}
	}
	//3dl el brk el gded
	//cprintf("brk = %x\n", brk);
	void * return_brk = brk;
	brk = brk + (numOfPages*PAGE_SIZE);

	//cprintf("\nOUT Function SBRK\n");
	//cprintf("-------------------Done sbrk 5-------------------\n");
	//release_sleeplock(&kernel_alloc_lock);
	return return_brk;
}

//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator

void* kmalloc(unsigned int size)
{

	//TODO: [PROJECT'24.MS2 - #03] [1] KERNEL HEAP - kmalloc
	// Write your code here, remove the panic and write your code
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
	acquire_sleeplock(&kernel_alloc_lock);
//	cprintf("-------------------INSIDE KMALLOC-------------------\n");
	if(size == 0)
	{
		//cprintf("Returning Null because size = 0\n");
		release_sleeplock(&kernel_alloc_lock);
		//cprintf("-------------------Done Kmalloc 1-------------------\n");
		return NULL;
	}
	//cprintf("\nIN KMALLOC\n");
	/*Logic of Block Allocator is missing*/
	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{
		//cprintf("\nInside Block Allocator\n");
		void * return_va;
		if(isKHeapPlacementStrategyFIRSTFIT())
		{
			//acquire_sleeplock(&kernel_alloc_lock);
			return_va = alloc_block_FF(size);
//			release_sleeplock(&kernel_alloc_lock);
		}
		if(isKHeapPlacementStrategyBESTFIT())
		{
			//acquire_sleeplock(&kernel_alloc_lock);
			return_va = alloc_block_BF(size);
//			release_sleeplock(&kernel_alloc_lock);
		}
		//Ht return NULL lw ml2ash aw el va
		release_sleeplock(&kernel_alloc_lock);
		//cprintf("-------------------Done Kmalloc 2-------------------\n");
		//cprintf("returned address = %x\n", return_va);
		return return_va;
	}

	/*Logic of Page Allocator Done*/
	//M7tag Kam Page
	//cprintf("size before ROUNDUP= %d\n", size);
	uint32 kamPage = ROUNDUP(size, PAGE_SIZE);
	//cprintf("size after ROUNDUP= %d\n", kamPage);
	kamPage = kamPage/PAGE_SIZE;
	//cprintf("\nkamPage = %d\n", kamPage);
	uint32 tmp, return_va;
	uint8 flag = 0;
	for(int i = last_free; i < num_of_pages;)
	{
		//cprintf("last_free = %d\n", last_free);
		if(my_array[i] >= kamPage && my_array[i] > 0)
		{
			/*Open these comments for optimization*/
			//cprintf("\nInside for loop size %d\n", array[i]);
			//cprintf("i = %d\n", i);
			//set the array[i] & array[i+array[i]-1] = 0
			int32 val = my_array[i];
			my_array[i] = 0;
			my_array[i+val-1] = 0;

			if(val-kamPage > 0)
			{
				my_array[i+kamPage] = val-kamPage;
				my_array[i+kamPage+(val-kamPage)-1] = val-kamPage;
			}
			//cprintf("kamPage = %d\n", kamPage);
			//cprintf("i+kamPage = %d\n", i+kamPage);
			//cprintf("i+kamPage+(val-kamPage)-1 = %d\n", (i+kamPage+(val-kamPage)-1));
			my_array[i] = -kamPage;
			tmp = HARD_LIMIT + PAGE_SIZE + (i*PAGE_SIZE);
			flag = 1;
			if(last_free == i && (i+kamPage) < num_of_pages)
				last_free = (i+kamPage);
			else	last_free = MIN(last_free, i+kamPage);
			break;
		}
		if(my_array[i] > 0)	i += my_array[i];
		else if(my_array[i] < 0)	i += (my_array[i]*-1);
		else{
			//cprintf("I am here\n");
			i++;
		}
	}
	if(flag == 0)
	{
		release_sleeplock(&kernel_alloc_lock);
		//cprintf("Returning Null Because no enough memory\n");
		//cprintf("-------------------Done Kmalloc 3-------------------\n");
		return NULL;
	}
	return_va = tmp;
	for(int i = 0; i < kamPage; ++i)
	{
		struct FrameInfo *ptr_frame_info = NULL;
		int s_OR_F = allocate_frame(&ptr_frame_info);
		if(s_OR_F==E_NO_MEM)
		{
			release_sleeplock(&kernel_alloc_lock);
			//cprintf("Return Null because no memory 1\n");
			//cprintf("-------------------Done Kmalloc 4-------------------\n");
			return NULL;
		}
		ptr_frame_info->bufferedVA = tmp;
		s_OR_F = map_frame(ptr_page_directory , ptr_frame_info ,tmp ,PERM_WRITEABLE);
		//???????????????????????
		/*what about the allocated frames in the previous iterations in the for loop*/
		/*Make for loop in the next conditions to free the previous frames*/
		//???????????????????????
		if(s_OR_F == E_NO_MEM)
		{
			//cprintf("Return Null because no memory 2\n");
			free_frame(ptr_frame_info);
			release_sleeplock(&kernel_alloc_lock);
			//cprintf("-------------------Done Kmalloc 5-------------------\n");
			return NULL;
		}
		else
		{
			tmp += PAGE_SIZE;
		}
	}
	void * tmp1 = (void *) return_va;
	release_sleeplock(&kernel_alloc_lock);
	//cprintf("-------------------Done Kmalloc 6-------------------\n");
	//cprintf("Return Address = %x\n", return_va);
	return (void *)return_va;

//	cprintf("kamPage %d\n", kamPage);
//	struct PageBlockElement * elybeshawr;

//	cprintf("Size el list %d\n", LIST_SIZE(&freePageBlocksList));
//	uint8 Done = 0;
//	LIST_FOREACH(elybeshawr, &freePageBlocksList)
//	{
//		cprintf("Page BlocksList %d\n", elybeshawr->Pages);
//		// If pages fit in the contigous block
//		if(elybeshawr->Pages >= kamPage)
//		{
//			uint32 elba2y = elybeshawr->Pages - kamPage;
//			uint32 return_va = HARD_LIMIT + PAGE_SIZE + (PAGE_SIZE*elybeshawr->Pages);
//			uint32 tmp = return_va;
//			//Allocating Frames and mapping them
//			for(int i = 0; i < kamPage; ++i)
//			{
//				struct FrameInfo *ptr_frame_info =NULL;
//				int s_OR_F = allocate_frame(&ptr_frame_info);
//				if(s_OR_F==E_NO_MEM)
//				{
//					return NULL;
//				}
//				s_OR_F = map_frame(ptr_page_directory , ptr_frame_info ,tmp ,PERM_WRITEABLE|PERM_PRESENT);
//
//				//???????????????????????
//				/*what about the allocated frames in the previous iterations in the for loop*/
//				/*Make for loop in the next conditions to free the previous frames*/
//				//???????????????????????
//				if(s_OR_F == E_NO_MEM)
//				{
//					free_frame(ptr_frame_info);
//					return NULL;
//				}
//				else
//				{
//					tmp += PAGE_SIZE;
//				}
//			}
//			if(elba2y > 0)
//			{
//				struct PageBlockElement * newBlock = NULL;
//				newBlock->Pages = elba2y;
//				newBlock->num = elybeshawr->num + kamPage;
//				LIST_INSERT_AFTER(&freePageBlocksList, elybeshawr, newBlock);
//			}
//			//This array is used in kfree
//			arr[elybeshawr->num] = 1;
//			arr[(elybeshawr->num + kamPage) - 1] = 1;
//			LIST_REMOVE(&freePageBlocksList, elybeshawr);
//			return (void *)return_va;
//		}
//	}
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details

//	cprintf("-------LITTLE CHECK-------------\n");
//	int32 y = array[0];
//	cprintf("y = %d\n", y);
//	y = array[767];
//	cprintf("y = %d\n", y);
//	y = array[768];
//	cprintf("y = %d\n", y);
//	y = array[1023];
//	cprintf("y = %d\n", y);
//	y = array[1024];
//	cprintf("y = %d\n", y);
//	cprintf("-------END OF LITTLE CHECK------------\n");

	acquire_sleeplock(&kernel_alloc_lock);
//	cprintf("-------------------INSIDE KFREE-------------------\n");
	uint32 va = (uint32)virtual_address;
	if(va >= KERNEL_HEAP_START && va < HARD_LIMIT)
	{
		//cprintf("I AM IN BLOCK FREE\n");
		free_block(virtual_address);
		release_sleeplock(&kernel_alloc_lock);
		return;
	}
	if(va >= HARD_LIMIT + PAGE_SIZE && va < KERNEL_HEAP_MAX)
	{
		//cprintf("\nI AM INSIDE IN PAGE FREE\n");
		//TL3 el index fe el array
		int32 tmp = ROUNDDOWN(va, PAGE_SIZE);
		tmp = tmp - (HARD_LIMIT + PAGE_SIZE);
		tmp = tmp / PAGE_SIZE;
		//cprintf("the number of allocated pages = %d\n", array[tmp]);
		int32 left, right, curr = tmp;
		//cprintf("curr = %d\n", curr);
		if((curr-1) >= 0)
		{
			//cprintf("checking left\n");
			left = my_array[curr-1];
			//cprintf("left = %d\n", left);
		}
		if(curr+(my_array[curr]*-1) < num_of_pages)
		{
			//cprintf("checking right\n");
			right = my_array[curr + (my_array[curr]*-1)];
			//cprintf("right = %d\n", right);
		}
		int32 curr_pages = my_array[curr];
		//cprintf("the number of allocated pages2 = %d\n", curr_pages);
		if(curr_pages < 0)
		{
			curr_pages *= -1;
			int32 tmp_va = va;

			for(int i = 0; i < curr_pages; i++)
			{
				unmap_frame(ptr_page_directory, tmp_va);
				tmp_va += PAGE_SIZE;
			}
			if(left > 0 && right > 0)
			{
				last_free = MIN(last_free, (curr - left));
				int32 val = left + right + curr_pages;
				my_array[curr - left] = val;
				my_array[(curr + curr_pages + right)-1] = val;

				my_array[curr] = 0;
				my_array[curr-1] = 0;
				my_array[curr+curr_pages] = 0;
			}
			else if(left > 0)
			{
				last_free = MIN(last_free, (curr- left));
				int32 val = left + curr_pages;
				my_array[curr - left] = val;
				my_array[(curr + curr_pages)-1] = val;

				my_array[curr] = 0;
				my_array[curr-1] = 0;
			}
			else if(right > 0)
			{
				last_free = MIN(last_free, curr);
				int32 val = right + curr_pages;
				my_array[curr] = val;
				my_array[(curr + curr_pages + right)-1] = val;

				my_array[curr + curr_pages] = 0;
			}
			else
			{
				last_free = MIN(last_free, curr);
				my_array[curr] = curr_pages;
				my_array[(curr + curr_pages)-1] = curr_pages;
			}
		}
	}
	release_sleeplock(&kernel_alloc_lock);
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");
	//acquire_sleeplock(kernel_alloc_lock);
	/*Check boundaries of virtual address*/
	uint32 *ely_byshawr_3la_page_table = NULL;
	/*Check the (return of get_page_table)*/
	int ret = get_page_table(ptr_page_directory, virtual_address, &ely_byshawr_3la_page_table);
	if(ret == E_NO_MEM)
	{
//		release_sleeplock(kernel_alloc_lock);
		return 0;
	}
	if (ely_byshawr_3la_page_table != NULL)
	{
		// shift b 12 w hat el frame number
		//if it present or not
		unsigned int page = ely_byshawr_3la_page_table[PTX(virtual_address)];
		if((page & PERM_PRESENT) == PERM_PRESENT)
		{
			unsigned int rkm_frame = (page & 0xFFFFF000);
			unsigned int offset = (virtual_address & (PAGE_SIZE - 1));
			unsigned int res = (rkm_frame | offset);
//			release_sleeplock(kernel_alloc_lock);
			return res;
		}
//		release_sleeplock(kernel_alloc_lock);
		return 0;
	}
//	release_sleeplock(kernel_alloc_lock);
	return 0;
	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//acquire_sleeplock(kernel_alloc_lock);
	unsigned int offset = physical_address & (PAGE_SIZE - 1);

	struct FrameInfo *frame_tmp = to_frame_info(physical_address);
	if(frame_tmp->references != 0)
	{
//		release_sleeplock(kernel_alloc_lock);
		return (((frame_tmp->bufferedVA) & 0xFFFFF000) | (offset));
	}
	else
	{
//		release_sleeplock(kernel_alloc_lock);
		return 0;
	}
	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}
//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, if moved to another loc: the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
	//Write your code here, remove the panic and write your code
//	acquire_sleeplock(kernel_alloc_lock);
	if(virtual_address == NULL)
	{
//		release_sleeplock(kernel_alloc_lock);
		return kmalloc(new_size);
	}
	if(new_size == 0)
	{
//		release_sleeplock(kernel_alloc_lock);
		kfree(virtual_address);
		return NULL;
	}
	//Inside block allocator
	if((uint32)virtual_address >= KERNEL_HEAP_START && (uint32)virtual_address < HARD_LIMIT)
	{
		if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		{
			// Block allocator to Block allocator
			//release_sleeplock(kernel_alloc_lock);
			return realloc_block_FF(virtual_address, new_size);
		}
		else
		{
			//Going from block allocator to page allocator
			void * new_va = kmalloc(new_size);
			if(new_va == NULL)
			{
				//release_sleeplock(kernel_alloc_lock);
				return virtual_address;
			}
			int32 oldSize = get_block_size(virtual_address);
			transfer_data(virtual_address,new_va,oldSize,new_size);
			free_block(virtual_address);
			//release_sleeplock(kernel_alloc_lock);
			return new_va;
		}
	}
	else if((uint32)virtual_address >= (HARD_LIMIT + PAGE_SIZE) && (uint32)virtual_address < KERNEL_HEAP_MAX)
	{
		int32 curr = ROUNDDOWN((uint32)virtual_address, PAGE_SIZE);
		curr = curr - (HARD_LIMIT + PAGE_SIZE);
		curr = curr / PAGE_SIZE;
		int32 curr_pages = my_array[curr]*-1;
		int32 right = 0, right_index = 0;
		int32 new_size_num_of_pages = ROUNDUP(new_size, PAGE_SIZE);
		new_size_num_of_pages = new_size_num_of_pages / PAGE_SIZE;

		if(new_size_num_of_pages <= curr_pages)
		{
			my_array[curr] = -new_size_num_of_pages;
			my_array[curr + new_size_num_of_pages] = -(curr_pages - new_size_num_of_pages);
			//release_sleeplock(kernel_alloc_lock);
			kfree(virtual_address + new_size);
			return virtual_address;
		}

		//page allocator to block allocator
		if(new_size <= DYN_ALLOC_MAX_SIZE)
		{
			void * new_va = kmalloc(new_size);
			if(new_va == NULL)
			{
				//release_sleeplock(kernel_alloc_lock);
				return virtual_address;
			}
			//transfer_data(virtual_address,new_va,oldSize,new_size);
			char * it_old = (char *) virtual_address;
			char * it_new = (char *) new_va;
			for(uint32 i = 1; i <= new_size; i++)
			{
				(*it_new) = (*it_old);
				it_old++;
				it_new++;
			}
			//free_block(&freeBlocksList,(struct BlockElement *)virtual_address);
			//release_sleeplock(kernel_alloc_lock);
			kfree(virtual_address);
			return new_va;
		}
		else
		{
			//Page allocator to page allocator
			//Checking right next to me first
			if(curr+(my_array[curr]*-1) < num_of_pages)
			{
				right_index = curr + (my_array[curr]*-1);
				right = my_array[right_index];
				int32 needed_pages = new_size_num_of_pages - curr_pages;
				int32 new_right_number_of_pages = right - needed_pages;
				if(right > 0)
				{
					if(new_right_number_of_pages > 0)
					{
						my_array[right_index] = 0;
						my_array[right_index + right] = 0;
						int32 new_freed_index = right_index + needed_pages;
						my_array[new_freed_index] = -new_right_number_of_pages;
						my_array[curr] = -new_size_num_of_pages;
						kfree(virtual_address + new_size);
					}
					else if(new_right_number_of_pages == 0)
					{
						my_array[curr] = -new_size_num_of_pages;
						my_array[right_index] = 0;
						my_array[curr + new_size_num_of_pages-1] = 0;
					}
					//release_sleeplock(kernel_alloc_lock);
					return virtual_address;
				}
			}
			else
			{
				//relocate to another place
				void * va_new = kmalloc(new_size);
				if(va_new == NULL)
				{
					//release_sleeplock(kernel_alloc_lock);
					return virtual_address;
				}
				int32 oldSize = curr_pages*PAGE_SIZE;
				transfer_data(virtual_address, va_new,oldSize, new_size);
				//release_sleeplock(kernel_alloc_lock);
				kfree(virtual_address);
				return va_new;
			}
		}
	}
	//release_sleeplock(kernel_alloc_lock);
	return NULL;
}
