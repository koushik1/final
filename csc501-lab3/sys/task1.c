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
	 

void testsem()
{
	int semaphore;
	int A, B, C;	
	semaphore = screate(1); 
	
	A = create(semaphore1, 2000, 10, "A", 1, semaphore);
        B = create(semaphore2, 2000, 20, "B", 1, semaphore);
        C = create(semaphore3, 2000, 30, "C", 1,semaphore);
	
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
				
}		


void lreader (char *msg, int lck)
{
        int     ret;

        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, READ, 20);
        kprintf ("  %s: acquired lock\n", msg);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void lwriter (char *msg, int lck)
{
        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, WRITE, 20);
        kprintf ("  %s: acquired lock, sleep 10s\n", msg);
        sleep (10);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void testlck()
{
	int ld;
	int rd1, rd2;
	int wr1;
	
	ld = lcreate(); 
	
	rd1 = create(lreader, 2000, 40, "readerA", 2, "reader A", ld);
        rd2 = create(lreader, 2000, 30, "readerB", 2, "reader B", ld);
        wr1 = create(lwriter, 2000, 10, "writer", 2, "writer", ld);
	
	kprintf("-start writer, then sleep 1s. lock granted to write (prio 10)\n");
        resume(wr1);
        sleep (1);
	kprintf("Priority of writer:%d\n", getprio(wr1));

        kprintf("-start reader A, then sleep 1s. reader A(prio 40) blocked on the lock\n");
        resume(rd1);
        sleep (1);
	kprintf("Priority of writer:%d\n", getprio(wr1));

        kprintf("-start reader B, then sleep 1s. reader B(prio 30) blocked on the lock\n");
        resume (rd2);
	sleep (1);
	kprintf("Priority of writer:%d\n", getprio(wr1));
				
}

int task1()
{
	kprintf("\nTest Results from Semaphore approach :-\n");
	testsem();
	kprintf("\nEnd of Test Result from Semaphore approach\n");
	kprintf("\nTest Results from Priority Inheritance approach :-\n");
	testlck();
	kprintf("\nEnd of Test Result Priority Inheritance approach\n");
	
}
