#include "inc/types.h"
#include "inc/x86.h"
#include "inc/memlayout.h"
#include "inc/mmu.h"
#include "inc/environment_definitions.h"
#include "inc/assert.h"
#include "inc/string.h"
#include "sleeplock.h"
#include "channel.h"
#include "../cpu/cpu.h"
#include "../proc/user_environment.h"

void init_sleeplock(struct sleeplock *lk, char *name)
{
	//cprintf("Inside sleep lock init\n");
	init_channel(&(lk->chan), "sleep lock channel");
	//cprintf("DONE 1\n");
	init_spinlock(&(lk->lk), "lock of sleep lock");
	strcpy(lk->name, name);
	lk->locked = 0;
	lk->pid = 0;
	//cprintf("Done sleep lock init\n");
}

int holding_sleeplock(struct sleeplock *lk)
{
	int r;
	//cprintf("acquire spin lock inside holding sleep lock\n");
	acquire_spinlock(&(lk->lk));
	r = lk->locked && (lk->pid == get_cpu_proc()->env_id);
	//cprintf("release spin lock inside holding sleep lock\n");
	release_spinlock(&(lk->lk));
	return r;
}
//==========================================================================
//void sinwar(bool free_palastine){
//int Allahom_Axxer_bna_shawkatahom = 0 ;
//int Allahom_Nakkes_bna_rayatahom = 0 ;
//int Allahom_Azzel_bna_qadatahom = 0 ;
//int Allahom_Hattem_feena_haybatahom = 0 ;
//int Allahom_Azl_bna_dawlatahom = 0 ;
//if(!free_palastine){
//Allahom_Axxer_bna_shawkatahom = 1; // Ameen
//Allahom_Nakkes_bna_rayatahom = 1; // Ameen
//Allahom_Azzel_bna_qadatahom = 1; // Ameen
//Allahom_Hattem_feena_haybatahom = 1; // Ameen
//Allahom_Azl_bna_dawlatahom = 1; // Ameen
//}
//}
void acquire_sleeplock(struct sleeplock *lk)
{
	//cprintf("Inside acquire sleep lock\n");
	bool free_palastine = 0;
	//cprintf("DONE 1\n");
//	if(!free_palastine)
//		sinwar(free_palastine);
	//cprintf("DONE 2\n");
	//cprintf("Acquiring spin lock in acquire sleep lock\n");
	//cprintf("Disabling interrupt inside acquire sleep lock\n");
	acquire_spinlock(&(lk->lk));
	while (lk->locked){
		sleep(&lk->chan,&lk->lk);
	}
	//cprintf("DONE 3\n");
//	if(!free_palastine)
//		sinwar(free_palastine);
	lk->locked = 1;
	//cprintf("DONE 4\n");
	//cprintf("lk address = %x\n", &lk);
	struct Env *tmp = get_cpu_proc();
//    if(!(tmp == NULL))
//    {
//    	cprintf("env_id inside acquire = %d\n", tmp->env_id);
//    }
//    else	cprintf("env is NULL!!\n");
    //cprintf("DONE 5\n");
    //cprintf("release spin lock in acquire sleep lock\n");
    release_spinlock(&(lk->lk));
    //cprintf("DONE 6\n");
//    if(free_palastine)
//    	sinwar(free_palastine);
    //cprintf("Done acquire sleep lock\n");
}

void release_sleeplock(struct sleeplock *lk)
{
	//cprintf("Inside release sleep lock\n");
	bool free_palastine = 0;
	//cprintf("acquire spin lock in sleep lock\n");
    acquire_spinlock(&lk->lk);
//    if(!free_palastine)
//    	sinwar(free_palastine);
    lk->locked = 0;
    lk->pid = 0;
    wakeup_all(&lk->chan);
//    if(!free_palastine)
//    	sinwar(free_palastine);
    release_spinlock(&lk->lk);
    //cprintf("DONE release sleep lock\n");
}
