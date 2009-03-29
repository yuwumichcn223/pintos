/* This file is fully designed and created by Christopher Xu
 * See README at root directory for details
 */

#ifndef THREADS_ALARM_H
#define THREADS_ALARM_H

#include "devices/timer.h"
#include <list.h>
#include "threads/interrupt.h"

/* an alarm */
struct alarm
  {
    int64_t ticks;
    struct thread *thrd; /* the waiting thread */
    
    struct list_elem elem; /* list element */
  };
  
void alarm_init(void); /* init the alarm list */

struct alarm* set_alarm(int64_t ticks); /* set alarm for current thread */
void dismiss_alarm(struct alarm*); /* dismiss the alarm */

#endif /* THREADS_ALARM_H */
