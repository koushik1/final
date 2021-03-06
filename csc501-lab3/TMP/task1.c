#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lock.h>

void semaphore1(int s){
        kprintf("Process A (writer) waiting for semaphore\n");
        int x = wait(s);
        kprintf("Process A (writer) acquires semaphore\n");
        sleep(7);
        kprintf("Process A (writer) signals\n");
        signal(s);
}

void semaphore2(int s){
        kprintf("Process B (reader) waiting for semaphore\n");
        int x = wait(s);
        kprintf("Process B (reader) acquires semaphore\n");
        kprintf("Process B (reader) signals\n");
        signal(s);
}

void semaphore3(int s){
        kprintf("Process C (reader) waiting for semaphore\n");
        int x = wait(s);
        kprintf("Process C (reader) acquires semaphore\n");
        kprintf("Process C (reader) signals\n");
        signal(s);
}
	 

void lock1(int l){
        kprintf("Process A (writer) waiting for lock\n");
        lock (l, WRITE, 10);
        kprintf("Process A (writer) acquires lock\n");
        sleep(7);
        kprintf("Process A (writer) releases lock\n");
        releaseall (1, l);
}

void lock2(int l){
        kprintf("Process B (reader) waiting for lock\n");
        lock (l, READ, 10);
        kprintf("Process B (reader) acquires lock\n");
        kprintf("Process B (reader) releases lock\n");
        releaseall (1, l);
}

void lock3(int l){
        kprintf("Process C (reader) waiting for lock\n");
        lock (l, READ, 10);
        kprintf("Process C (reader) acquires lock\n");
        kprintf("Process C (reader) releases lock\n");
        releaseall (1, l);
}


int task1()
{
	kprintf("\nSemaphore :-\n");

	int semaphore;
	int S_A, S_B, S_C;	
	semaphore = screate(1); 
	
	S_A = create(semaphore1, 2000, 10, "A", 1, semaphore);
        S_B = create(semaphore2, 2000, 20, "B", 1, semaphore);
        S_C = create(semaphore3, 2000, 30, "C", 1,semaphore);
	
	kprintf("Start process A (writer) with priority 10\n");
        resume(S_A);
        sleep (1);
	kprintf("Priority of A:%d\n", getprio(S_A));

        kprintf("Start process B (reader) with priority 20\n");
        resume(S_B);
        sleep (1);

        kprintf("Start process C (reader) with priority 30\n");
        resume (S_C);
	sleep (1);
	kprintf("\n");
	kprintf("Priority of A:%d\n", getprio(S_A));
	kprintf("\nSemahore Complete\n");


	kprintf("\nLocks with Priority Inheritence :-\n");

	int ld;
	int A, B, C;	
	ld = lcreate(); 
	
	A = create(lock1, 2000, 10, "A", 1, ld);
        B = create(lock2, 2000, 20, "B", 1, ld);
        C = create(lock3, 2000, 30, "C", 1, ld);
	
	kprintf("Start process A (writer) with priority 10\n");
        resume(A);
        sleep (1);
	kprintf("Priority of A:%d\n", getprio(A));

        kprintf("Start process B (reader) with priority 20\n");
        resume(B);
        sleep (1);

        kprintf("Start process C (reader) with priority 30\n");
        resume (C);
	sleep (1);
	kprintf("\n");

	kprintf("Priority of A:%d\n", getprio(A));	
        kprintf("\nLocks with Priority Inheritence Compplete\n");
	
}
