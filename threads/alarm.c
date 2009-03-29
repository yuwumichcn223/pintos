/* This file is fully designed and created by Christopher Xu
 * See README at root directory for details
 */
 
#include <list.h>
#include "threads/thread.h"
#include "threads/alarm.h"

static struct list alarm_list;

void alarm_init(void)
{
  list_init (&alarm_list);
}
