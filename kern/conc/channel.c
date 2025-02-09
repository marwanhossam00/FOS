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
	//cprintf("Inside chan init\n");
	strcpy(chan->name, name);
	//cprintf("DONE 1\n");
	init_queue(&(chan->queue));
	//cprintf("DONE chan init\n");
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
void enq_sem_deq(struct Env_Queue* queue)
{
	acquire_spinlock(&ProcessQueues.qlock);
	{
		struct Env * e = dequeue(queue);
		sched_insert_ready(e);
	}
	release_spinlock(&ProcessQueues.qlock);
}

void enq_sem_env(struct Env_Queue* queue, struct Env* env, uint32 *lock)
{
//	cprintf("inside enq_sem_env env->id = %d\n", env->env_id);
	acquire_spinlock(&ProcessQueues.qlock);
	{
//		cprintf("blocking \n");
		env->env_status = ENV_BLOCKED;
		//cprintf("", "");
		enqueue(queue,env);
//		cprintf("LIST_SIZE inside = %d\n", LIST_SIZE(queue));
//		cprintf("inserting in queue\n");
		//cprintf("Enabling Interrupt inside sleep\n");
//		cprintf("unlocking\n");
		*lock = 0;
		sched();
	}
	//cprintf("Enabling Interrupt inside sleep\n");
	release_spinlock(&ProcessQueues.qlock);
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
	if (current_process == NULL || chan == NULL || lk == NULL ){
		//sinwar(1);
		panic("sleep: invalid arguments");
	}
	if (!holding_spinlock(lk)){
	    //sinwar(0);
		panic("sleep: spinlock is not held by the current process");
		//sinwar(1);
	}
	//cprintf("acquiring spin lock in sleep\n");
	//cprintf("Disabling Interrupt inside sleep\n");
	acquire_spinlock(&ProcessQueues.qlock);
	{
		current_process->channel = chan;
		current_process->env_status = ENV_BLOCKED;
		enqueue(&chan->queue, current_process);
		//cprintf("Enabling Interrupt inside sleep\n");
		release_spinlock(lk);
		sched();
	}
	//cprintf("Enabling Interrupt inside sleep\n");
	release_spinlock(&ProcessQueues.qlock);
    //cprintf("Disabling Interrupt inside sleep\n");
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
    if (chan == NULL){
        panic("wakeup_one: invalid channel");
    }
    //cprintf("Disabling Interrupt inside sleep\n");
    acquire_spinlock(&ProcessQueues.qlock);
    {
        struct Env *env = dequeue(&chan->queue);
        if (env == NULL) {
        	//cprintf("Enabling Interrupt inside sleep\n");
            release_spinlock(&ProcessQueues.qlock);
            return;
        }
        env->env_status = ENV_READY;
        env->channel = NULL;
        sched_insert_ready(env);
    }
    //cprintf("Enabling Interrupt inside sleep\n");
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
	if (chan == NULL)
	     panic("wakeup_all: invalid channel");
	//cprintf("Disabling Interrupt inside sleep\n");
	acquire_spinlock(&ProcessQueues.qlock);
	{
		struct Env *env;
		while ((env = dequeue(&chan->queue)) != NULL) {
			env->env_status = ENV_READY;
			env->channel = NULL;
			sched_insert_ready(env);
		}
	}
	//cprintf("Enabling Interrupt inside sleep\n");
	release_spinlock(&ProcessQueues.qlock);
}

