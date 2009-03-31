/* This file is fully designed and created by Christopher Xu
 * See README at root directory for details
 */
 
#include <list.h>
#include <stdlib.h>
#include <stdbool.h>
#include "threads/thread.h"
#include "threads/alarm.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include <string.h>
#include <limits.h>
#include <stdio.h>

#define ALARM_MAGIC 0x67452301 /* magic number for ALARM_MAGIC */

static struct list alarm_list;
static int64_t prev_ticks;

static bool is_alarm (struct alarm *);
static void dismiss_alarm (struct alarm *);

/* Initialize the alarm list */
void
alarm_init (void)
{
  list_init (&alarm_list);
  prev_ticks = timer_ticks ();
  ASSERT (list_empty (&alarm_list));
}

/* Initialize the alarm object */
void
set_alarm (int64_t t)
{
  struct thread *thrd;
  struct alarm *alrm;
  
  thrd = thread_current (); /* the current thread */
  
  /* if t == 0 then do nothing */
  if (t == 0)
    return;
  
  ASSERT (thrd->status == THREAD_RUNNING);
  
  alrm = &thrd->alrm;
  
  alrm->thrd = thrd;
  
  alrm->ticks = t + timer_ticks (); /* set the tick count to t plus current timer ticks */
  alrm->magic = ALARM_MAGIC;
  
  /* add to alarm_list, critical section */
  intr_disable ();
  list_push_back (&alarm_list, &alrm->elem);
  
  /* block the thread */
  thread_block ();
  intr_enable ();
}

/* Check if the alrm is actually an alarm */
static bool
is_alarm (struct alarm *alrm)
{
  return (alrm != NULL && alrm->magic == ALARM_MAGIC);
}

/* dismiss the alarm and unblock the thread */
static void
dismiss_alarm (struct alarm *alrm)
{
  enum intr_level old_level;
  
  ASSERT (is_alarm (alrm));
  
  /* remove from alarm_list, critical section */
  old_level = intr_disable ();
  list_remove (&alrm->elem);
  
  thread_unblock (alrm->thrd); /* unblock the thread */
  intr_set_level (old_level);
}

void
alarm_check (void)
{
  int64_t curr_ticks, diff;
  struct list_elem *tmp, *next;
  struct alarm *alrm;
  
  tmp = list_begin (&alarm_list);
  
  while (tmp != list_end (&alarm_list))
    { 
      alrm = list_entry (tmp, struct alarm, elem);
      next = list_next (tmp);
      if (alrm->ticks <= timer_ticks ())
        {
          dismiss_alarm (alrm);
          list_remove (tmp);
        }
      tmp = next;
    }
}
