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

/* My Implementation */
static bool outstanding_priority (const struct list_elem *lhs, const struct list_elem *rhs, void *aux UNUSED);
/* == My Implementation */

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
  /* My Implementation */
  struct thread *wake_up, *curr;
  /* == My Implementation */
  enum intr_level old_level;

  ASSERT (sema != NULL);

  /* My Implementation */
  wake_up = NULL;
  curr = thread_current ();
  sort_thread_list (&sema->waiters);
  /* My Implementation */

  old_level = intr_disable ();
  if (!list_empty (&sema->waiters)) 
    /* Old Implementation 
    thread_unblock (list_entry (list_pop_front (&sema->waiters),
                                struct thread, elem)); */
    /* My Implementation */
    {
      wake_up = list_entry (list_pop_front (&sema->waiters), struct thread, elem);
      thread_unblock (wake_up);
    }
    /* == My Implementation */
  sema->value++;
  
  /* My Implementation */
  if (wake_up != NULL && wake_up->priority > curr->priority)
    thread_yield_head (curr);
  /* == My Implementation */
  
  intr_set_level (old_level);
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
  /* My Implementation */
  lock->lock_priority = PRI_MIN - 1;
  /* == My Implementation */
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
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  /* My Implementation */
  struct thread *curr, *thrd;
  struct lock *another;
  enum intr_level old_level;
  
  curr = thread_current ();
  thrd = lock->holder;
  curr->blocked = another = lock;
  
  old_level = intr_disable ();
  
  while (thrd != NULL && thrd->priority < curr->priority)
    {
      thrd->donated = true;
      thread_set_priority_other (thrd, curr->priority, false);
      if (another->lock_priority < curr->priority)
        another->lock_priority = curr->priority;
      if (thrd->status == THREAD_BLOCKED && thrd->blocked != NULL)
        {
          another = thrd->blocked;
          thrd = thrd->blocked->holder;
        }
      else
        break;
    }
  
  /* == My Implementation */

  sema_down (&lock->semaphore);
  /* Old Implementation
  lock->holder = thread_current (); */
  
  /* My Implementation */
  lock->holder = curr;
  curr->blocked = NULL;
  list_insert_ordered (&lock->holder->locks, &lock->holder_elem, outstanding_priority, NULL);
  
  intr_set_level (old_level);
  /* == My Implementation */
  
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
    /* Old Implementation
    lock->holder = thread_current (); */
    /* My Implementation */
    {
      lock->holder = thread_current ();
      list_push_back (&lock->holder->locks, &lock->holder_elem);
    }
    /* == My Implementation */
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  /* My Implementation */
  struct thread *curr;
  struct list_elem *l;
  struct lock *another;
  enum intr_level old_level;
  
  curr = thread_current ();
  
  ASSERT (curr->blocked == NULL);
  /* == My Implementation */
  
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  old_level = intr_disable ();

  lock->holder = NULL;
  sema_up (&lock->semaphore);
  
  /* My Implementation */
  list_remove (&lock->holder_elem);
  lock->lock_priority = PRI_MIN - 1;
  if (list_empty (&curr->locks))
    {
      curr->donated = false;
      thread_set_priority (curr->base_priority);
    }
  else /* Still holding locks */
    {
      l = list_back (&curr->locks); //, outstanding_priority, NULL);
      another = list_entry (l, struct lock, holder_elem);
      if (another->lock_priority != PRI_MIN - 1)
        thread_set_priority_other (curr, another->lock_priority, false);
      else
        thread_set_priority (curr->base_priority);
    }
    
  intr_set_level (old_level);
  /* == My Implementation */
  
}

/* My Implementation */
static bool
outstanding_priority (const struct list_elem *lhs, const struct list_elem *rhs, void *aux UNUSED)
{
  struct lock *l1, *l2;
  
  l1 = list_entry (lhs, struct lock, holder_elem);
  l2 = list_entry (rhs, struct lock, holder_elem);
  
  return (l1->lock_priority > l2->lock_priority);
}
/* == My Implementation */

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
    /* My Implementation */
    int sema_priority;                  /* priority for the semaphore_elem */
    /* == My Implementation */
  };

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
  /* My Implementation */
  cond->holder = NULL;
  /* == My Implementation */
}

/* My Implementation */
static bool
cond_priority (const struct list_elem *lhs, const struct list_elem *rhs, void *aux UNUSED)
{
  struct semaphore_elem *l, *r;
  
  l = list_entry (lhs, struct semaphore_elem, elem);
  r = list_entry (rhs, struct semaphore_elem, elem);
  
  return (l->sema_priority > r->sema_priority);
}
/* == My Implementation */

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
  /* My Implementation */
  waiter.sema_priority = thread_current ()->priority;
  list_insert_ordered (&cond->waiters, &waiter.elem, cond_priority, NULL);
  /* == My Implementation */
  /* Old Implementation
  list_push_back (&cond->waiters, &waiter.elem); */
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
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) 
    sema_up (&list_entry (list_pop_front (&cond->waiters),
                          struct semaphore_elem, elem)->semaphore);
                          
  /* My Implementation */
  //thread_yield_head (thread_current ());
  /* == My Implementation */
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
