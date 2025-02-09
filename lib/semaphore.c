// User-level Semaphore

#include "inc/lib.h"

struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{

	//TODO: [PROJECT'24.MS3 - #02] [2] USER-LEVEL SEMAPHORE - create_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_semaphore is not implemented yet");
	//Your Code is Here...
	//cprintf("semaphore.c create semaphore \n");
	struct semaphore res ={0};
	void* va =  smalloc(semaphoreName, sizeof(struct __semdata), 1);

	if (va == NULL)
		return res;

	struct __semdata* sema = (struct __semdata*) va;
	res.semdata = sema;

	strncpy(res.semdata->name, semaphoreName, sizeof(res.semdata->name) - 1);
	res.semdata->name[sizeof(res.semdata->name) - 1] = '\0';
	res.semdata->count = value;
	res.semdata->lock = 0;
	sys_init_queue(&res.semdata->queue);


	return res;

}
struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	//TODO: [PROJECT'24.MS3 - #03] [2] USER-LEVEL SEMAPHORE - get_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_semaphore is not implemented yet");
	//Your Code is Here...
	struct semaphore res = {0};
	//	cprintf("semaphore.c get semaphore \n");
	if (semaphoreName == NULL) {
		panic("Invalid semaphore name");
	}
	void *va = sget(ownerEnvID, semaphoreName);
	if (va == NULL) {
		panic("Failed to get shared memory for semaphore");
	}
	struct __semdata* sema = (struct __semdata *)va;
	res.semdata = sema;
	return res;
}
//void lock_acquire_release( uint32 *lock, int operation) {
//    if (operation == 1)
//    {
//    	uint32 key = 1;
//    	cprintf("lock inside semaphore acquire = %d\n", *lock);
//        while (xchg(lock, key) != 0){
//        	//cprintf("DEADLOCK\n");
//        }
//    }
//    else if (operation == 0)
//    {
//        *lock = 0;
//        cprintf("lock inside semaphore release = %d\n", *lock);
//    }
//}
void wait_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #04] [2] USER-LEVEL SEMAPHORE - wait_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("wait_semaphore is not implemented yet");
	//Your Code is Here...
//	cprintf("first semaphore.c wait semaphore \n");
//	lock_acquire_release(&sem.semdata->lock, 1);
//	cprintf("DONE sem lock acquire\n");
	uint32 key = 1;
//	cprintf("lock in wait before acquire = %d\n", sem.semdata->lock);
	while(xchg(&(sem.semdata->lock), key) != 0);
//	cprintf("lock in wait after acquire = %d\n", sem.semdata->lock);
//	do {
//		xchange(&key, &sem.semdata->lock);
//	} while (key != 0);
//	cprintf("count before = %d\n", sem.semdata->count);
	sem.semdata->count--;
//	cprintf("count after = %d\n", sem.semdata->count);

	if (semaphore_count(sem) < 0)
	{
//		cprintf("Calling this sys call\n");
		//System call to sleep the environment
		//sys_sleep_env(&sem.semdata->queue, &sem.semdata->lock, myEnv);

		struct Env* env = sys_get_cpu_proc();
		//cprintf("Going to sleep\n");
		sys_sem_env_enq(&(sem.semdata->queue), env,&(sem.semdata->lock));
		//cprintf("Returning from sleep\n");
		//cprintf("DONE sys_get_proc\n");
		//cprintf("env_id inside semaphore wait = %d\n", myEnv->env_id);
		//sys_sleep
		//sys_acquire_spinlock_pQ();
		//cprintf("DONE sys_acquire_spinlock_pQ\n");
		//cprintf("unlocking the lock in wait and sleeping = %d\n", sem.semdata->lock);

		//cprintf("after unlocking = %d\n", sem.semdata->lock);
//		cprintf("size before enqueue = %d\n", LIST_SIZE(&sem.semdata->queue));
//		struct Env tmp = *env;
		//tmp->env_status = ENV_BLOCKED;
//		myEnv->env_status = ENV_BLOCKED;
//		sem.semdata->lock = 0;

//		env->env_status = ENV_BLOCKED;
//		cprintf("after blocked------------\n");
//		//LIST_INSERT_HEAD(&sem.semdata->queue, &tmp);
//		cprintf("size after enqueue = %d\n", LIST_SIZE(&sem.semdata->queue));
		//sys_release_spinlock_pQ();

//		sys_enqueue(&sem.semdata->queue, env);
		//cprintf("DONE sys_enqueue\n");
		//LIST_INSERT_HEAD(&sem.semdata->queue, myEnv);
//		lock_acquire_release(&sem.semdata->lock, 0);
		//sys_sched();
		//
	}
//	lock_acquire_release(&sem.semdata->lock, 0);
	else
	{
		sem.semdata->lock = 0;
//		cprintf("unlocking the lock no sleep needed = %d\n", sem.semdata->lock);
	}
//	cprintf("ending semaphore.c wait semaphore \n");

}

void signal_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #05] [2] USER-LEVEL SEMAPHORE - signal_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("signal_semaphore is not implemented yet");
	//Your Code is Here...
//	cprintf("first in semaphore.c signal semaphore \n");
	uint32 key = 1;
	cprintf("DONE 1\n");
//	cprintf("lock in singal before acquire = %d\n", sem.semdata->lock);
	while(xchg(&sem.semdata->lock, key) != 0);
//	lock_acquire_release(&sem.semdata->lock, 1);
//	cprintf("lock in signal after acquire = %d\n", sem.semdata->lock);
	cprintf("DONE 2\n");
	sem.semdata->count++;
//	cprintf("count inside signal semaphore = %d\n", sem.semdata->count);
	if(semaphore_count(sem) <= 0)
	{
		cprintf("DONE 3\n");
//		cprintf("before deq in semaphore.c signal semaphore \n");
		//cprintf("LIST_SIZE inside = %d\n", LIST_SIZE(&sem.semdata->queue));
//		struct Env* currentEnv = sys_dequeue(&sem.semdata->queue);
//		cprintf("DONE 4\n");
		//cprintf("currentEnv_id = %d\n", currentEnv->env_id);
//		cprintf("after deq in semaphore.c signal semaphore \n");
		sys_sem_env_deq(&(sem.semdata->queue));
		cprintf("DONE 4\n");

//		sys_acquire_spinlock_pQ();
//		cprintf("DONE 5\n");
//		sys_sched_insert_ready(currentEnv);
//		cprintf("DONE 6\n");
//		sys_release_spinlock_pQ();
//		cprintf("DONE 7\n");
//		lock_acquire_release(&sem.semdata->lock, 0);
		//cprintf("after sched readdy in semaphore.c signal semaphore \n");
		//currentEnv->env_status = ENV_READY;
	}
	sem.semdata->lock = 0;
	cprintf("DONE 8\n");
	//	else lock_acquire_release(&sem.semdata->lock, 0);
//	cprintf("ending in semaphore.c signal semaphore \n");
}

int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}
