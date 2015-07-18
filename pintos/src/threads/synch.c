/***********************************************************
 *
 * $A2 150704 thinkhy  priority scheduler
 *
 ***********************************************************/

/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      list_push_back (&sema->waiters, &thread_current ()->elem);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;
  struct list_elem *e;                                              /* @A2A */
  int max_priority = -1;                                            /* @A2A */
  struct thread* next_holder = NULL;                                /* @A2A */

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (!list_empty (&sema->waiters)) 
  {
    /* thread_unblock (list_entry (list_pop_front (&sema->waiters),    @A2D */
    /*                             struct thread, elem));              @A2D */
    for ( e = list_begin (&sema->waiters);                          /* @A2A */
	      e != list_end(&sema->waiters); e = list_next(e) )     /* @A2A */ 
      {
	 struct thread *t = list_entry(e, struct thread, elem);     /* @A2A */
	 ASSERT(t != NULL);                                         /* @A2A */
	 int effective_priority = thread_get_effective_priority(t); /* @A2A */
	 if (effective_priority > max_priority)                     /* @A2A */
	  {
	       max_priority = effective_priority;                   /* @A2A */
	       next_holder = t;                                     /* @A2A */
	  }
      }

      ASSERT(next_holder != NULL);                                  /* @A2A */
      list_remove(&next_holder->elem);                              /* @A2A */
      thread_unblock (next_holder);                           

  }

  sema->value++;
  intr_set_level (old_level);

  if (next_holder && next_holder->effective_priority                /* @A2A */
		        > thread_current ()->effective_priority)    /* @A2A */
	   thread_yield ();                                         /* @A2A */
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  enum intr_level old_level;                        /* @A2A */
  struct thread *t = thread_current ();             /* @A2A */
  struct thread *holder = lock->holder;             /* @A2A */
  struct thread *waited_thread;                     /* @A2A */
  struct list_elem *e;                              /* @A2A */

  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  /* if lock holder is not NULL, put current thread
   * into lock holder's waiting list @A2A
   */
  if (holder != NULL) 
   {
     old_level = intr_disable ();                               /* @A2A */
     list_push_back (&holder->waiting_list, &t->waitelem);      /* @A2A */
     waited_thread = t->waited_thread = holder;                 /* @A2A */

     /* Updated waited threads's effective priority, which makes 
      * nested chain case passed              
      */
     while(waited_thread)                                       /* @A2A */
       {
             thread_set_dirty(waited_thread, true);             /* @A2A */
             waited_thread = waited_thread->waited_thread;      /* @A2A */
       } 
    //} /* End of if (holder != NULL)                              @A2A */

     intr_set_level (old_level);                                /* @A2A */

     //sema_down (&lock->semaphore);                            /* @A2A */

     /* Now we get the lock                                        @A2A */
     //old_level = intr_disable ();                             /* @A2A */

     /* Now current thread is owner of lock, remove itself from 
      * semaphore list  
      */
     //list_remove(&(t->waitelem));                             /* @A2A */
     //t->waited_thread = NULL;                                 /* @A2A */

     //intr_set_level (old_level);                              /* @A2A */
   }  
   //else                                     /* No holder         @A2A */
   //{
   sema_down (&lock->semaphore);         /* perform P primitive         */
   //}
     
   /* Copy waiters from last holder to current                 
    * thread's waiting list                                   
    */ 
   old_level = intr_disable ();                                      /* @A2A */
   for ( e = list_begin (&lock->semaphore.waiters);                  /* @A2A */
         e != list_end(&lock->semaphore.waiters); e = list_next(e) ) /* @A2A */ 
   {
     struct thread *waiter = list_entry(e, struct thread, elem);     /* @A2A */
     list_push_back (&t->waiting_list, &waiter->waitelem);           /* @A2A */
     waiter->waited_thread = t;                                      /* @A2A */
   }
    
   t->is_dirty = true;                                               /* @A2A */
   lock->holder = t;                                                 /* @A2C */
   intr_set_level (old_level);                                       /* @A2A */
}



/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void lock_release (struct lock *lock) 
{
  enum intr_level old_level;                                                /* @A2A */
  struct thread *holder = lock->holder;                                     /* @A2A */
  int old_effective_priority = holder->effective_priority;                  /* @A2A */
  struct list_elem *e1, *e2;                                                /* @A2A */ 
  struct list *sem_list = &(lock->semaphore.waiters);                       /* @A2A */
  bool dirty_flag = false;                                                  /* @A2A */

  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  /* Clear up waiting list of holder
   * TODO: Is necessary to close intr at here? 150709  
   */
  old_level = intr_disable ();                                              /* @A2A */

  /* Remove threads in semaphore->waiters from current 
   * thread's waiting list
   */
  for (e1 = list_begin (sem_list); e1 != list_end (sem_list);               /* @A2A */
		e1 = list_next (e1))                                        /* @A2A */
   {
      struct thread *t1 = list_entry (e1, struct thread, elem);             /* @A2A */
      list_remove(&(t1->waitelem));                                         /* @A2A */
      t1->waited_thread = NULL;                                             /* @A2A */
      if (holder->effective_priority >= t1->effective_priority)             /* @A2A */
             dirty_flag = true;                                             /* @A2A */
   }

  thread_set_dirty(holder, dirty_flag);                                     /* @A2A */
  lock->holder = NULL;

  /* Mark current thread dirty if effective priority is unequal 
   * with original priority
   */
  intr_set_level (old_level);                                               /* @A2A */

  sema_up (&lock->semaphore);

  if (holder->is_dirty)                                                     /* @A2A */
     thread_yield();                                                        /* @A2A */
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem 
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
  };

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  sema_init (&waiter.semaphore, 0);
  list_push_back (&cond->waiters, &waiter.elem);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  struct semaphore_elem *waiter;                                    /* @A2A */
  struct semaphore_elem *next_waiter;                               /* @A2A */
  struct list_elem *e;                                              /* @A2A */
  struct list_elem *te;                                             /* @A2A */
  struct thread    *t;                                              /* @A2A */
  int    max_priority = -1;                                         /* @A2A */
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) 
   { 
    /* sema_up (&list_entry (list_pop_front (&cond->waiters),         @A2D */
    /*                    struct semaphore_elem, elem)->semaphore);   @A2D */
    /* pick the highest priority in waiters list                      @A2A */
    for ( e = list_begin (&cond->waiters);                         /* @A2A */
	      e != list_end(&cond->waiters); e = list_next(e) )    /* @A2A */ 
     {
	waiter = list_entry (e, struct semaphore_elem, elem);      /* @A2A */
	ASSERT (waiter != NULL);                                   /* @A2A */
        te = list_begin (&waiter->semaphore.waiters);              /* @A2A */
	t  = list_entry (te, struct thread, elem);                 /* @A2A */
	int effective_priority = thread_get_effective_priority(t); /* @A2A */
	if (effective_priority > max_priority)                     /* @A2A */
	 {
	    max_priority = effective_priority;                     /* @A2A */
	    next_waiter = waiter;                                  /* @A2A */
	 }
     }
    
    list_remove(&next_waiter->elem);                               /* @A2A */
    sema_up(&next_waiter->semaphore);                              /* @A2A */
   } 
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}
