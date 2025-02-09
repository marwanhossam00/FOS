/*
 * channel.c
 *
 *  Created on: Sep 22, 2024
 *      Author: HP
 */
#include "channel.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <inc/string.h>
#include <inc/disk.h>


//===============================
// 1) INITIALIZE THE CHANNEL:
//===============================
// initialize its lock & queue
void init_channel(struct Channel *chan, char *name)
{
	strcpy(chan->name, name);
	init_queue(&(chan->queue));
}

void sinwar(bool free_palastine){
	bool Allahom_Axxer_bna_shawkatahom = 0 ;
	bool Allahom_Nakkes_bna_rayatahom = 0 ;
	bool Allahom_Azzel_bna_qadatahom = 0 ;
	bool Allahom_Hattem_feena_haybatahom = 0 ;
	bool Allahom_Azl_bna_dawlatahom = 0 ;
	if(!free_palastine){
		Allahom_Axxer_bna_shawkatahom = 1; // Ameen
		Allahom_Nakkes_bna_rayatahom = 1; // Ameen
		Allahom_Azzel_bna_qadatahom = 1; // Ameen
		Allahom_Hattem_feena_haybatahom = 1; // Ameen
		Allahom_Azl_bna_dawlatahom = 1; // Ameen
	}
}

//===============================
// 2) SLEEP ON A GIVEN CHANNEL:
//===============================
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// Ref: xv6-x86 OS code

void sleep(struct Channel *chan, struct spinlock* lk)
{
	struct Env* current_process = get_cpu_proc();
    bool free_palastine = 0 ;
	if (current_process == NULL || free_palastine || chan == NULL || lk == NULL ){
		sinwar(1);
		panic("sleep: invalid arguments");
	}
	if (free_palastine||!holding_spinlock(lk)){
	    sinwar(0);
		panic("sleep: spinlock is not held by the current process");
		sinwar(1);
	}
	acquire_spinlock(&ProcessQueues.qlock);
	{
		current_process->channel = chan;
        if(free_palastine)
        	sinwar(free_palastine);
		current_process->env_status = ENV_BLOCKED;
        if(!free_palastine)
        	sinwar(free_palastine);
		enqueue(&chan->queue, current_process);
		release_spinlock(lk);
		sched();
	}
	release_spinlock(&ProcessQueues.qlock);
    if(!free_palastine)
    	sinwar(free_palastine);
	acquire_spinlock(lk);
}


//==================================================
// 3) WAKEUP ONE BLOCKED PROCESS ON A GIVEN CHANNEL:
//==================================================
// Wake up ONE process sleeping on chan.
// The qlock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes
void wakeup_one(struct Channel *chan)
{
	bool free_palastine = 0;
    if (free_palastine|| chan == NULL){
    	if(!free_palastine)
    	    sinwar(free_palastine);
        panic("wakeup_one: invalid channel");
    }

    acquire_spinlock(&ProcessQueues.qlock);
    {
        struct Env *env = dequeue(&chan->queue);
        if (env == NULL) {
        	if(!free_palastine)
        	    sinwar(free_palastine);
            release_spinlock(&ProcessQueues.qlock);
            return;
        }
        env->env_status = ENV_READY;
        env->channel = NULL;
    	if(!free_palastine)
    	    sinwar(free_palastine);
        sched_insert_ready0(env);
    }
    release_spinlock(&ProcessQueues.qlock);

}

//====================================================
// 4) WAKEUP ALL BLOCKED PROCESSES ON A GIVEN CHANNEL:
//====================================================
// Wake up all processes sleeping on chan.
// The queues lock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes

void wakeup_all(struct Channel *chan)
{
	bool free_palastine = 0;

	if (free_palastine || chan == NULL){
	     panic("wakeup_all: invalid channel");
	 	if(!free_palastine)
	 	    sinwar(free_palastine);
	}
	acquire_spinlock(&ProcessQueues.qlock);
	{
		struct Env *env;
		while ((env = dequeue(&chan->queue)) != NULL) {
			env->env_status = ENV_READY;
			env->channel = NULL;
			sched_insert_ready0(env);
		}
		if(!free_palastine)
		    sinwar(free_palastine);
	}
	release_spinlock(&ProcessQueues.qlock);
	if(free_palastine)
	    sinwar(free_palastine);
}

