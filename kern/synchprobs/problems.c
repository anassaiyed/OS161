/*
 * Copyright (c) 2001, 2002, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Driver code for whale mating problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * 08 Feb 2012 : GWA : Driver code is in kern/synchprobs/driver.c. We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.

static int male_cnt=0, female_cnt=0, matchmaker_cnt=0;
static struct lock *lock;
static struct cv *male_cv, *female_cv, *matchmaker_cv;

//Variables for Buffalo Intersections
static struct lock *lock_quad[4], *lock_getlock;

void whalemating_init() {

  lock=lock_create("lock");
  if(lock==NULL) {
      panic("Can't create lock in whale mating.");
  }

  male_cv=cv_create("male_cv");
  if(male_cv==NULL) {
      panic("can't create male cv");
  }
  female_cv=cv_create("female_cv");
  if(female_cv==NULL) {
      panic("can't create female cv");
  }
  matchmaker_cv=cv_create("matchmaker_cv");
  if(matchmaker_cv==NULL) {
      panic("can't create matchmaker cv");
  }

}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void whalemating_cleanup() {
  lock_destroy(lock);
  cv_destroy(male_cv);
  cv_destroy(female_cv);
  cv_destroy(matchmaker_cv);
}

void
male(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  male_start();

	// Implement this function 
  lock_acquire(lock);
  male_cnt++;
  if(female_cnt>0 && matchmaker_cnt>0)
  {
    cv_signal(female_cv,lock);
    cv_signal(matchmaker_cv,lock);
  }
  else
  {
    cv_wait(male_cv,lock);
  }
  male_cnt--;
  lock_release(lock);

  male_end();

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
female(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  female_start();

	// Implement this function 
  lock_acquire(lock);
  female_cnt++;
  if(male_cnt>0 && matchmaker_cnt>0)
  {
    cv_signal(male_cv,lock);
    cv_signal(matchmaker_cv,lock);
  }
  else
  {
    cv_wait(female_cv,lock);
  }
  female_cnt--;
  lock_release(lock);

  female_end();
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
matchmaker(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  matchmaker_start();

	// Implement this function 
  lock_acquire(lock);
  matchmaker_cnt++;
  if(male_cnt>0 && female_cnt>0)
  {
    cv_signal(male_cv,lock);
    cv_signal(female_cv,lock);
  }
  else
  {
    cv_wait(matchmaker_cv,lock);
  }
  matchmaker_cnt--;
  lock_release(lock);

  matchmaker_end();
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

/*
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is,
 * of course, stable under rotation)
 *
 *   | 0 |
 * --     --
 *    0 1
 * 3       1
 *    3 2
 * --     --
 *   | 2 | 
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X
 * first.
 *
 * You will probably want to write some helper functions to assist
 * with the mappings. Modular arithmetic can help, e.g. a car passing
 * straight through the intersection entering from direction X will leave to
 * direction (X + 2) % 4 and pass through quadrants X and (X + 3) % 4.
 * Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in drivers.c.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.

void stoplight_init() {
  lock_quad[0]=lock_create("lock1");
  if(lock_quad[0]==NULL) {
      panic("Can't create lock 1 in buffalo intersection.");
  }
  lock_quad[1]=lock_create("lock2");
  if(lock_quad[1]==NULL) {
      panic("Can't create lock 2 in buffalo intersection.");
  }
  lock_quad[2]=lock_create("lock3");
  if(lock_quad[2]==NULL) {
      panic("Can't create lock 3 in buffalo intersection.");
  }
  lock_quad[3]=lock_create("lock4");
  if(lock_quad[3]==NULL) {
      panic("Can't create lock 4 in buffalo intersection.");
  }
  lock_getlock=lock_create("lockGetLock");
  if(lock_getlock==NULL) {
      panic("Can't create lock getLock in buffalo intersection.");
  }
  //return;
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void stoplight_cleanup() {
  lock_destroy(lock_quad[0]);
  lock_destroy(lock_quad[1]);
  lock_destroy(lock_quad[2]);
  lock_destroy(lock_quad[3]);
  lock_destroy(lock_getlock);
  //return;
}

void
gostraight(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  //(void)direction;
  
  lock_acquire(lock_getlock);
    lock_acquire(lock_quad[direction]);
    lock_acquire(lock_quad[(direction+3)%4]);
  lock_release(lock_getlock);
  inQuadrant(direction);                    //in first quadrant (from 2 quadrants)
  inQuadrant((direction+3)%4);              //in second quadrant (from 2 quadrants)
  leaveIntersection();                      //leave intersection
  lock_release(lock_quad[direction]);
  lock_release(lock_quad[(direction+3)%4]);

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}

void
turnleft(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  //(void)direction;

  lock_acquire(lock_getlock);
    lock_acquire(lock_quad[direction]);
    lock_acquire(lock_quad[(direction+3)%4]);
    lock_acquire(lock_quad[(direction+2)%4]);
  lock_release(lock_getlock);
  inQuadrant(direction);                    //in first quadrant (from 3 quadrants)
  inQuadrant((direction+3)%4);              //in second quadrant (from 3 quadrants)
  inQuadrant((direction+2)%4);              //in third quadrant (from 3 quadrants)
  leaveIntersection();                      //leave intersection
  lock_release(lock_quad[direction]);
  lock_release(lock_quad[(direction+3)%4]);
  lock_release(lock_quad[(direction+2)%4]);
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}

void
turnright(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  //(void)direction;

  lock_acquire(lock_getlock);
    lock_acquire(lock_quad[direction]);
  lock_release(lock_getlock);
  inQuadrant(direction);
  leaveIntersection();
  lock_release(lock_quad[direction]);

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}
