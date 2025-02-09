#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/queue.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
struct Share* get_share(int32 ownerID, char* name);

//===========================
// [1] INITIALIZE SHARES:
//===========================
//Initialize the list and the corresponding lock
void sharing_init()
{
#if USE_KHEAP
	LIST_INIT(&AllShares.shares_list) ;
	init_spinlock(&AllShares.shareslock, "shares lock");
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

//==============================
// [2] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	//[PROJECT'24.MS2] DONE
	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//
	struct Share* ptr_share = get_share(ownerID, shareName);
	if (ptr_share == NULL)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return ptr_share->size;

	return 0;
}

//===========================================================


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] Create frames_storage:
//===========================
// Create the frames_storage and initialize it by 0
inline struct FrameInfo** create_frames_storage(int numOfFrames)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_frames_storage()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_frames_storage is not implemented yet");
	//Your Code is Here...
	int size_elframe_alloc = numOfFrames*(sizeof(struct FrameInfo*));
	struct FrameInfo** ptr_elframes =(struct FrameInfo**)kmalloc(size_elframe_alloc);
	if(ptr_elframes==NULL)
	{
		return NULL;
	}
	for(int i =0 ; i < numOfFrames;i++)
	{
	 	ptr_elframes[i] = 0;
	}
	return ptr_elframes;

}

//=====================================
// [2] Alloc & Initialize Share Object:
//=====================================
//Allocates a new shared object and initialize its member
//It dynamically creates the "framesStorage"
//Return: allocatedObject (pointer to struct Share) passed by reference
struct Share* get_share(int32 ownerID, char* name)
{
	//TODO: [PROJECT'24.MS2 - #17] [4] SHARED MEMORY - get_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_share is not implemented yet");
	//Your Code is Here...
	//cprintf("hello get share \n");
	 if (name == NULL) {
	     return NULL;
	 }
	 acquire_spinlock(&AllShares.shareslock);

	 struct Share *share_rn = LIST_FIRST(&AllShares.shares_list);
	 while (share_rn != NULL ) {
		 if (share_rn->ownerID == ownerID && strcmp(share_rn->name, name) == 0) {
	          release_spinlock(&AllShares.shareslock);
	          return share_rn;
	     }

	     share_rn = LIST_NEXT(share_rn);
	 }

	 release_spinlock(&AllShares.shareslock);
	 return NULL;
}


//=============================
// [3] Search for Share Object:
//=============================
//Search for the given shared object in the "shares_list"
//Return:
//	a) if found: ptr to Share object
//	b) else: NULL
struct Share* create_share(int32 ownerID, char* shareName, uint32 size, uint8 isWritable)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_share is not implemented yet");
	//Your Code is Here...
    struct Share* obj_gded_shared = (struct Share*)kmalloc(sizeof(struct Share));

    if (obj_gded_shared == NULL)
    {
        return NULL;
    }

    obj_gded_shared->ownerID = ownerID;
    strncpy(obj_gded_shared->name, shareName, 64);
    obj_gded_shared->size = size;
    obj_gded_shared->isWritable = isWritable;
    obj_gded_shared->references = 1;
    obj_gded_shared->ID = ((int32)obj_gded_shared) & 0x7FFFFFFF;
    int frameCount = ROUNDUP(size, PAGE_SIZE);
    frameCount = frameCount/PAGE_SIZE;

    struct FrameInfo** create_frames = create_frames_storage(frameCount);

    if(create_frames == NULL)	return NULL;
    // did not undo the allocations

    obj_gded_shared->framesStorage = create_frames;
    return obj_gded_shared;

}


//=========================
// [4] Create Share Object:
//=========================
int createSharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #19] [4] SHARED MEMORY [KERNEL SIDE] - createSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("createSharedObject is not implemented yet");
	//Your Code is Here...
    //cprintf("CREATEEEEEEEEEEEEE SHAREDDDDDDDDDDDD \n");
	struct Env* myenv = get_cpu_proc(); //The calling environment
	//cprintf("create_shared_obj 1\n");
	struct Share* existing_share = get_share(ownerID, shareName);

	if (existing_share != NULL)
	{
	    return E_SHARED_MEM_EXISTS;
	}
	//cprintf("2\n");
	struct Share* new_share = create_share(ownerID, shareName, size, isWritable);

	if (new_share == NULL)
	{
	   return E_NO_SHARE;
	}
	//cprintf("3\n");
	acquire_spinlock(&AllShares.shareslock);
	LIST_INSERT_HEAD(&AllShares.shares_list,new_share);
	release_spinlock(&AllShares.shareslock);

	//cprintf("4\n");
	uint32 total_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	uint32 va = (uint32)virtual_address;

	void* mapStart = kmalloc(ROUNDUP(size, PAGE_SIZE));
	//cprintf("5\n");
	if (mapStart == NULL)
	{
		//cprintf("returning null\n");
		return E_NO_SHARE;
	}

	for (uint32 i = 0; i < total_pages; i++)
	{
		uint32 *ptr_Page = NULL;
	   	get_page_table(ptr_page_directory,(uint32)mapStart,&ptr_Page);
	    struct FrameInfo* frame_info = get_frame_info(ptr_page_directory,(uint32)mapStart,&ptr_Page);
	    int perm = PERM_USER | PERM_WRITEABLE;
	    int  result = map_frame(myenv->env_page_directory, frame_info, va + (i * PAGE_SIZE),perm);

		new_share->framesStorage[i] = frame_info;
		if (result != 0)
		{
		   for (uint32 j = 0; j <= i; j++)
		   {
			   uint32 cleanup_va = va + (j * PAGE_SIZE);
			   unmap_frame(myenv->env_page_directory, cleanup_va);
		   }

		   return E_NO_SHARE;
		}
	}
	//cprintf("DONE create_shared_obj\n");
	return new_share->ID;

}


//======================
// [5] Get Share Object:
//======================
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("getSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); //The calling environment
	//cprintf("hello get shared obj\n");
    //cprintf("Found shared object with ID: %d\n", ownerID);

	struct Share* shared_obj = get_share(ownerID, shareName);
	if (shared_obj == NULL)
	{
	   return E_SHARED_MEM_NOT_EXISTS;
	}
	uint32 va = (uint32)virtual_address;
	uint32 numPages = ROUNDUP(shared_obj->size, PAGE_SIZE) / PAGE_SIZE;
    for (uint32 i = 0; i < numPages; i++)
    {
        struct FrameInfo* frame = shared_obj->framesStorage[i];
        if (frame == NULL)
	    {
        	return E_NO_SHARE;
	    }

	    int result = map_frame(
	    myenv->env_page_directory,
	    	frame,
	        va + (i * PAGE_SIZE),
	        shared_obj->isWritable? PERM_USER | PERM_WRITEABLE:  PERM_USER
	    );
	}

	shared_obj->references++;
	return shared_obj->ID;
	//===============================================================
}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//==========================
// [B1] Delete Share Object:
//==========================
//delete the given shared object from the "shares_list"
//it should free its framesStorage and the share object itself
void free_share(struct Share* ptrShare)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - free_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("free_share is not implemented yet");
	//Your Code is Here...

}
//========================
// [B2] Free Share Object:
//========================
int freeSharedObject(int32 sharedObjectID, void *startVA)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - freeSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("freeSharedObject is not implemented yet");
	//Your Code is Here...

}
