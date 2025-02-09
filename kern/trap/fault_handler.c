/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <kern/cpu/cpu.h>
#include <kern/disk/pagefile_manager.h>
#include <kern/mem/memory_manager.h>

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//==================
// [1] MAIN HANDLER:
//==================
/*2022*/
uint32 last_eip = 0;
uint32 before_last_eip = 0;
uint32 last_fault_va = 0;
uint32 before_last_fault_va = 0;
int8 num_repeated_fault  = 0;

struct Env* last_faulted_env = NULL;
void fault_handler(struct Trapframe *tf)
{
	/******************************************************/
//	cprintf("Inside fault_handler\n");
	// Read processor's CR2 register to find the faulting address
	uint32 fault_va = rcr2();
//	cprintf("\n************Faulted VA = %x************\n", fault_va);
	//	print_trapframe(tf);
	/******************************************************/
	//If same fault va for 3 times, then panic
	//UPDATE: 3 FAULTS MUST come from the same environment (or the kernel)
	struct Env* cur_env = get_cpu_proc();
	if (last_fault_va == fault_va && last_faulted_env == cur_env)
	{
		num_repeated_fault++ ;
		if (num_repeated_fault == 3)
		{
			print_trapframe(tf);
			panic("Failed to handle fault! fault @ at va = %x from eip = %x causes va (%x) to be faulted for 3 successive times\n", before_last_fault_va, before_last_eip, fault_va);
		}
	}
	else
	{
		before_last_fault_va = last_fault_va;
		before_last_eip = last_eip;
		num_repeated_fault = 0;
	}
	last_eip = (uint32)tf->tf_eip;
	last_fault_va = fault_va ;
	last_faulted_env = cur_env;
	/******************************************************/
	//2017: Check stack overflow for Kernel
	int userTrap = 0;
	if ((tf->tf_cs & 3) == 3) {
		userTrap = 1;
	}
	if (!userTrap)
	{
		struct cpu* c = mycpu();
		//cprintf("trap from KERNEL\n");
		if (cur_env && fault_va >= (uint32)cur_env->kstack && fault_va < (uint32)cur_env->kstack + PAGE_SIZE)
			panic("User Kernel Stack: overflow exception!");
		else if (fault_va >= (uint32)c->stack && fault_va < (uint32)c->stack + PAGE_SIZE)
			panic("Sched Kernel Stack of CPU #%d: overflow exception!", c - CPUS);
#if USE_KHEAP
		if (fault_va >= KERNEL_HEAP_MAX)
			panic("Kernel: heap overflow exception!");
#endif
	}
	//2017: Check stack underflow for User
	else
	{
		//cprintf("trap from USER\n");
		if (fault_va >= USTACKTOP && fault_va < USER_TOP)
			panic("User: stack underflow exception!");
	}

	//get a pointer to the environment that caused the fault at runtime
	//cprintf("curenv = %x\n", curenv);
	struct Env* faulted_env = cur_env;
	if (faulted_env == NULL)
	{
		print_trapframe(tf);
		panic("faulted env == NULL!");
	}
	//check the faulted address, is it a table or not ?
	//If the directory entry of the faulted address is NOT PRESENT then
	if ( (faulted_env->env_page_directory[PDX(fault_va)] & PERM_PRESENT) != PERM_PRESENT)
	{
		// we have a table fault =============================================================
		//		cprintf("[%s] user TABLE fault va %08x\n", curenv->prog_name, fault_va);
		//		print_trapframe(tf);

		faulted_env->tableFaultsCounter ++ ;

		table_fault_handler(faulted_env, fault_va);
	}
	else
	{
		if (userTrap)
		{
//			cprintf("Inside usertrap\n");
//			cprintf("\n************Faulted VA = %x************\n", fault_va);
			/*============================================================================================*/
			//TODO: [PROJECT'24.MS2 - #08] [2] FAULT HANDLER I - Check for invalid pointers
			//(e.g. pointing to unmarked user heap page, kernel or wrong access rights),
			//your code is here
			/*DELETE COMMENTS HERE*/
			int el_perm = pt_get_page_permissions(faulted_env->env_page_directory,fault_va);
			if((fault_va >= USER_LIMIT))
			{
//				cprintf("fault_va = %x\n",va);
//				cprintf("USER_HEAP_START = %x, USTACKTOP = %x\n", USER_HEAP_START, USTACKTOP);
//				cprintf("NOT USER.....exit()\n");
				env_exit();
			}
			if((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) && !(el_perm & PERM_WRITEABLE))
			{
//				cprintf("USER BUT NOT WRITEABLE.....exit()\n");
				env_exit();
			}
			if((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) && !(el_perm & PERM_MARKED))
			{
//				cprintf("USER AND NOT MARKED.....exit()\n");
				env_exit();
			}
			if((el_perm & PERM_PRESENT) && !(el_perm & PERM_WRITEABLE))
			{
//				cprintf("PRESENT BUT NOT WRITEABLE.....exit()\n");
				env_exit();
			}
			/*============================================================================================*/
		}

		/*2022: Check if fault due to Access Rights */
		int perms = pt_get_page_permissions(faulted_env->env_page_directory, fault_va);
		if (perms & PERM_PRESENT)
			panic("Page @va=%x is exist! page fault due to violation of ACCESS RIGHTS\n", fault_va) ;
		/*============================================================================================*/


		// we have normal page fault =============================================================
		faulted_env->pageFaultsCounter ++ ;

		//		cprintf("[%08s] user PAGE fault va %08x\n", curenv->prog_name, fault_va);
		//		cprintf("\nPage working set BEFORE fault handler...\n");
		//		env_page_ws_print(curenv);

		if(isBufferingEnabled())
		{
			__page_fault_handler_with_buffering(faulted_env, fault_va);
		}
		else
		{
			//page_fault_handler(faulted_env, fault_va);
			page_fault_handler(faulted_env, fault_va);
		}
		//		cprintf("\nPage working set AFTER fault handler...\n");
		//		env_page_ws_print(curenv);


	}

	/*************************************************************/
	//Refresh the TLB cache
	tlbflush();
	/*************************************************************/
}

//=========================
// [2] TABLE FAULT HANDLER:
//=========================
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//=========================
// [3] PAGE FAULT HANDLER:
//=========================
void page_fault_handler(struct Env * faulted_env, uint32 fault_va)
{
//	cprintf("Maximum WS size for this env = %d\n", faulted_env->page_WS_max_size);
//	env_page_ws_print(faulted_env);
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(faulted_env->page_WS_list));
#else
		int iWS =faulted_env->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(faulted_env);
#endif

	if(wsSize < (faulted_env->page_WS_max_size))
	{
//		cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
		//TODO: [PROJECT'24.MS2 - #09] [2] FAULT HANDLER I - Placement
		// Write your code here, remove the panic and write your code
		//panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
		int ret = pf_read_env_page(faulted_env, (void *)fault_va);
		if(ret == E_PAGE_NOT_EXIST_IN_PF)
		{
			//(fault_va > USTACKBOTTOM && fault_va < (USTACKTOP - faulted_env->initNumStackPages * PAGE_SIZE)) ||
			if((fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)||(fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX))
			{
				/*try without saving on disk*/
				struct FrameInfo *ptr_frame = NULL;
				int ret = allocate_frame(&ptr_frame);
				if(ret == E_NO_MEM)	return;
				ret = map_frame(faulted_env->env_page_directory,ptr_frame,fault_va,PERM_PRESENT|PERM_USER|PERM_WRITEABLE);
				if(ret == E_NO_MEM)
				{
					free_frame(ptr_frame);
					return;
				}
				struct WorkingSetElement * elem = env_page_ws_list_create_element(faulted_env, fault_va);
				LIST_INSERT_TAIL(&faulted_env->page_WS_list,elem);
//				cprintf("LIST_SIZE = %d\n",LIST_SIZE(&faulted_env->page_WS_list));
//				elem->idx = LIST_SIZE(&faulted_env->page_WS_list);
				if(LIST_SIZE(&faulted_env->page_WS_list) == faulted_env->page_WS_max_size)
				{
					if(faulted_env->page_last_WS_element == NULL)
					{
						faulted_env->page_last_WS_element = LIST_FIRST(&faulted_env->page_WS_list);
						faulted_env->page_last_WS_index = 1;
					}
				}
//				pf_update_env_page(faulted_env,fault_va,ptr_frame);
				return;
			}
			else
			{
				env_exit();
				return;
			}
		}
		struct FrameInfo *ptr_frame = NULL;
		ret = allocate_frame(&ptr_frame);
		if(ret == E_NO_MEM)	return;
		ret = map_frame(faulted_env->env_page_directory,ptr_frame,fault_va,PERM_PRESENT|PERM_USER|PERM_WRITEABLE);
		if(ret == E_NO_MEM)
		{
			free_frame(ptr_frame);
			return;
		}
		struct WorkingSetElement * elem = env_page_ws_list_create_element(faulted_env, fault_va);
		LIST_INSERT_TAIL(&faulted_env->page_WS_list,elem);
//		cprintf("LIST_SIZE = %d\n",LIST_SIZE(&faulted_env->page_WS_list));
//		elem->index = LIST_SIZE(&faulted_env->page_WS_list);
		if(LIST_SIZE(&faulted_env->page_WS_list) == faulted_env->page_WS_max_size)
		{
			if(faulted_env->page_last_WS_element == NULL)
			{
				faulted_env->page_last_WS_element = LIST_FIRST(&faulted_env->page_WS_list);
				faulted_env->page_last_WS_index = 1;
			}
		}
//		cprintf("PAGE FAULT DONE\n");
//		pf_update_env_page(faulted_env,fault_va,ptr_frame);
		return;
//		refer to the project presentation and documentation for details
	}
	else
	{
//		cprintf("Allocated Pages = %d\n",pf_calculate_allocated_pages(faulted_env));
		//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
		//refer to the project presentation and documentation for details
		//TODO: [PROJECT'24.MS3] [2] FAULT HANDLER II - Replacement
		// Write your code here, remove the panic and write your code
		//panic("page_fault_handler() Replacement is not implemented yet...!!");
//		cprintf("\n--------------BEFORE------------------\n");
//		env_page_ws_print(faulted_env);
//		cprintf("\n--------------BEFORE------------------\n");
//		cprintf("Inside replacement\n");
		struct WorkingSetElement * mx_element = faulted_env->page_last_WS_element;
				struct WorkingSetElement * elybeshawr;
				/*Edit the page_WS_max_sweeps from uint32 -> int */
				uint8 modif = 0, flag = 0, done = 0;
				int32 mx = -1, it_index = faulted_env->page_last_WS_index;
				int32 mx_index = it_index;
				if(page_WS_max_sweeps < 0)	modif = 1;
				int i = 0;
				LIST_FOREACH(elybeshawr, &(faulted_env->page_WS_list))
				{
					int perm = pt_get_page_permissions(faulted_env->env_page_directory, elybeshawr->virtual_address);
					if(perm & PERM_USED)
					{
						pt_set_page_permissions(faulted_env->env_page_directory, elybeshawr->virtual_address, 0, PERM_USED);
						elybeshawr->sweeps_counter = 0;
					}
				}
				LIST_FOREACH(elybeshawr, &(faulted_env->page_WS_list))
				{

//					cprintf("it_index = %d, i = %d\n", it_index, i);
		//			if((it_index) == elybeshawr->index)
		//			{
		//				mx = elybeshawr->sweeps_counter;
		//				cprintf("elybeshawr->sweeps_counter = %d\n", elybeshawr->sweeps_counter);
		//				cprintf("elybeshawr->index = %d\n", elybeshawr->index);
		//				cprintf("mx = %d\n", mx);
		//			}
					if(flag || ((it_index) == i))
					{
						/*Dah el it*/ /*Check MAX*/
		//				cprintf("it_index = %d, elybeshawr->index = %d\n", it_index, elybeshawr->index);
						flag = 1;
						int perm = pt_get_page_permissions(faulted_env->env_page_directory, elybeshawr->virtual_address);
						if(modif && (perm & PERM_MODIFIED) && ((elybeshawr->sweeps_counter != 0) &&((elybeshawr->sweeps_counter-1) > mx)))
						{
		//					cprintf("elybeshawr->sweeps_counter-1 = %d\n", elybeshawr->sweeps_counter-1);
							mx = (elybeshawr->sweeps_counter-1);
							mx_element = elybeshawr;
							mx_index = i;
						}
						else if((elybeshawr->sweeps_counter > mx))
						{
							if((modif && !(perm & PERM_MODIFIED)))
							{
								mx = (elybeshawr->sweeps_counter);
								mx_element = elybeshawr;
								mx_index = i;
							}
							else if(!modif)
							{
								mx = (elybeshawr->sweeps_counter);
								mx_element = elybeshawr;
								mx_index = i;
							}
						}
					}
					i++;
				}
				i = 0;
				LIST_FOREACH(elybeshawr, &(faulted_env->page_WS_list))
				{
//					cprintf("i = %d\n", i);
					if(it_index == i)
					{
						done = 1;
						//break;
					}
					if(!done)
					{
						int perm = pt_get_page_permissions(faulted_env->env_page_directory, elybeshawr->virtual_address);
						if(modif && (perm & PERM_MODIFIED) && ((elybeshawr->sweeps_counter != 0) &&((elybeshawr->sweeps_counter-1) > mx)))
						{
		//					cprintf("mx in 2nd iteration = %d\n", mx);
							mx = (elybeshawr->sweeps_counter-1);
		//					cprintf("mx in 2nd iteration = %d\n", mx);
							mx_element = elybeshawr;
							mx_index = i;
						}
						else if((elybeshawr->sweeps_counter > mx))
						{
							if((modif && !(perm & PERM_MODIFIED)))
							{
								mx = (elybeshawr->sweeps_counter);
								mx_element = elybeshawr;
								mx_index = i;
							}
							else if(!modif)
							{
								mx = (elybeshawr->sweeps_counter);
								mx_element = elybeshawr;
								mx_index = i;
							}
						}
					}
					i++;
				}
				int addnum, addnum1;
				if(page_WS_max_sweeps < 0)
				{
					addnum = ((-1*page_WS_max_sweeps) - mx);
					addnum1 = addnum + 1;
				}
				else
				{
					addnum = (page_WS_max_sweeps - mx);
					addnum1 = addnum + 1;
				}
		//		cprintf("\n--------------------------------\n");
		//		cprintf("mx = %d\n", mx);
		//		cprintf("mx_index = %d\n", mx_index);
		//		cprintf("it_index = %d\n", it_index);
		//		cprintf("addnum = %d\n", addnum);
		//		cprintf("addnum1 = %d\n", addnum1);
		//		cprintf("\n--------------------------------\n");
				flag = 0;
				int i2 = 0;
				LIST_FOREACH(elybeshawr, &(faulted_env->page_WS_list))
				{
					if(mx_index == it_index)
					{
						if(i2 != mx_index)
						{
		//					cprintf("sweeps_counter = %d\n", elybeshawr->sweeps_counter);
							elybeshawr->sweeps_counter += addnum;
		//					cprintf("sweeps_counter = %d\n", elybeshawr->sweeps_counter);
						}
					}
					else if((mx_index > it_index))
					{
						if((i2 >= it_index) && (i2 <= mx_index))
							elybeshawr->sweeps_counter += addnum1;
						else
							elybeshawr->sweeps_counter += addnum;
					}
					else
					{
						if((i2 <= mx_index) || (i2 >= it_index))
							elybeshawr->sweeps_counter += addnum1;
						else
							elybeshawr->sweeps_counter += addnum;
					}
					i2++;
				}
				/*Putting the modified replaced page in disk*/
				uint32 *env_page_table = NULL;
				struct FrameInfo *ptr_frame = get_frame_info(faulted_env->env_page_directory, (uint32) mx_element->virtual_address, &env_page_table);
				int perm = pt_get_page_permissions(faulted_env->env_page_directory, mx_element->virtual_address);
				//		cprintf("ret = %d\n", ret);
		//		cprintf("virtual_add = %x\n", fault_va);
				if((perm & PERM_MODIFIED))
				{
		//			cprintf("page is modified\n");
					pf_update_env_page(faulted_env, mx_element->virtual_address,ptr_frame);
				}
				/*Interchanging the info of this WS*/
		//		cprintf("Inserting WS element in list\n");
				ptr_frame = NULL;
				allocate_frame(&ptr_frame);
				map_frame(faulted_env->env_page_directory,ptr_frame,fault_va,PERM_PRESENT|PERM_USER|PERM_WRITEABLE);
				struct WorkingSetElement * elem = env_page_ws_list_create_element(faulted_env, fault_va);
				i2 = mx_index;

				if(mx_index == 0)
				{
		//			cprintf("i am inserting in head\n");
					LIST_INSERT_HEAD(&faulted_env->page_WS_list, elem);
				}
				else if(mx_index == faulted_env->page_WS_max_size-1)
				{
		//			cprintf("\n--------------------------------\n");
		//			cprintf("i am inserting in tail\n");
					LIST_INSERT_TAIL(&faulted_env->page_WS_list, elem);
		//			cprintf("\n--------------------------------\n");
				}
				else
				{
		//			cprintf("i am inserting in middle\n");
					LIST_INSERT_BEFORE(&faulted_env->page_WS_list, mx_element, elem);
				}
				pf_read_env_page(faulted_env,(void *)fault_va);
		//		if(ret == E_PAGE_NOT_EXIST_IN_PF)
		//		{
		//			cprintf("PAGE does not exist\n");
		////			cprintf("fault_va = %x\n", fault_va);
		//			pf_update_env_page(faulted_env, fault_va ,ptr_frame);
		//		}
		//		cprintf("\n--------------------------------\n");
		//		env_page_ws_print(faulted_env);
		//		cprintf("\n--------------------------------\n");
		//		cprintf("\nmx_element->virtul_address = %x\n", mx_element->virtual_address);
				/*Editing the page_last_WS_element*/
				if(mx_index == faulted_env->page_WS_max_size-1)
				{
					faulted_env->page_last_WS_element = LIST_FIRST(&(faulted_env->page_WS_list));
					faulted_env->page_last_WS_index = 0;
				}
				else
				{
					struct WorkingSetElement * nxt_to_mx = LIST_NEXT(mx_element);
					faulted_env->page_last_WS_element = nxt_to_mx;
					faulted_env->page_last_WS_index = mx_index+1;
				}
				/*Removing the MAX WS element*/
				LIST_REMOVE(&faulted_env->page_WS_list, mx_element);
				unmap_frame(faulted_env->env_page_directory,mx_element->virtual_address);
		//		cprintf("\n--------------AFTER------------------\n");
		//		env_page_ws_print(faulted_env);
		//		cprintf("\n--------------AFTER------------------\n");
			}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	//[PROJECT] PAGE FAULT HANDLER WITH BUFFERING
	// your code is here, remove the panic and write your code
	panic("__page_fault_handler_with_buffering() is not implemented yet...!!");
}

