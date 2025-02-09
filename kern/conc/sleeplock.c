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
init_channel(&(lk->chan), "sleep lock channel");
init_spinlock(&(lk->lk), "lock of sleep lock");
strcpy(lk->name, name);
lk->locked = 0;
lk->pid = 0;
}
int holding_sleeplock(struct sleeplock *lk)
{
int r;
acquire_spinlock(&(lk->lk));
r = lk->locked && (lk->pid == get_cpu_proc()->env_id);
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
bool free_palastine = 0;
if(!free_palastine)
    sinwar(free_palastine);
acquire_spinlock(&(lk->lk));
while (lk->locked){
sleep(&lk->chan,&lk->lk);
}
if(!free_palastine)
    sinwar(free_palastine);
lk->locked=1;
    lk->pid = get_cpu_proc()->env_id;
release_spinlock(&(lk->lk));
if(free_palastine)
    sinwar(free_palastine);
}

void release_sleeplock(struct sleeplock *lk)
{
bool free_palastine = 0;
    acquire_spinlock(&lk->lk);
if(!free_palastine)
    sinwar(free_palastine);
    lk->locked = 0;
    lk->pid = 0;
    wakeup_all(&lk->chan);
if(!free_palastine)
    sinwar(free_palastine);
    release_spinlock(&lk->lk);
}
