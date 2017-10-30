/*	Michael DeSimone
	mtd13@my.fsu.edu
	COP4610
	Project 2
*/

// added test

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>

#define NUMBER_THREADS 10

static struct lock *shared_lock;

static void init()
{
	shared_lock = lock_create("shared_lock");

	if(shared_lock == NULL)
		panic("new_test failure: init\n");
}



static void thread_join_test(void *na, unsigned long num_thread)
{
	(void) na;
	kprintf("%ld\n", num_thread);
}

static void lock_holding_test(void *na, unsigned long num_thread)
{
	(void) na;
	(void) num_thread;

	if(lock_do_i_hold(shared_lock) != 0)
		panic("new_test failure: lock_holding_test\n");
	else
	{
		lock_acquire(shared_lock);

		if(lock_do_i_hold(shared_lock) != 0)
		{
			thread_yield();

			if(lock_do_i_hold(shared_lock) != 0)
			{
				lock_release(shared_lock);

				if(lock_do_i_hold(shared_lock) != 0)
					panic("new_test failure: lock_holding_test\n");
				else
					return;

			}
			if(lock_do_i_hold(shared_lock) == 0)
				panic("new_test failure: lock_holding_test\n");

		}
		else
			panic("new_test failure: lock_holding_test\n");

	}
}

static int lock_holding_testpass()
{

	struct semaphore *sem_test = sem_create("sem_test", 0);

	for(int num_thread = 0; num_thread < NUMBER_THREADS; num_thread++)
	{
		thread_fork("lock_holding_test", NULL, lock_holding_test, NULL, num_thread);
		V(sem_test);
	}

	for(int num_thread = 0; num_thread < NUMBER_THREADS; num_thread++)
		P(sem_test);

	kprintf("Lock Holding Test passed\n");
	return 0;
}

static int cv_testpass()
{
	kprintf("CV Test passed\n");
	return 0;
}

static int thread_join_testpass()
{
	for(int i = 0; i < NUMBER_THREADS; i++)
	{
		kprintf("Creating Thread %d\n", i);
		thread_fork("thread_join_test", NULL, thread_join_test, NULL, i);
	}
	thread_join();

	kprintf("Thread Join Test passed\n");
	return 0;
}


int new_test(int nargs, char **args)
{
	kprintf("New_test start!\n");

	init(nargs, args);
	KASSERT(lock_holding_testpass(nargs, args) == 0);
	KASSERT(thread_join_testpass(nargs, args) == 0);
	KASSERT(cv_testpass(nargs, args) == 0);

	kprintf("New_test done!\n");

	return 0;
}

/*
#include <test.h>
int new_test(int argc, char ** args)
{
	(void)argc;
	(void)args;
	return 0;
}
*/
