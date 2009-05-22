#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

/* My Implementation */
static int sys_write (int fd, const void *buffer, unsigned length);

typedef int (*handler) (uint32_t, uint32_t, uint32_t);
static handler syscall_vec[128];
/* == My Implementation */

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  
  /* My Implementation */
  syscall_vec[SYS_WRITE] = (handler)sys_write;
  /* == My Implementation */
}

static void
syscall_handler (struct intr_frame *f /* Old Implementation UNUSED */) 
{
  /* Old Implementation 
  printf ("system call!\n");
  thread_exit (); */
  /* My Implementation */
  handler h;
  uint32_t *p;
  uint32_t ret;
  
  p = f->esp;
  h = syscall_vec[*p];
  
  ret = h (*(p + 1), *(p + 2), *(p + 3));
  
  f->eax = ret;
  /* == My Implementation */
}

static int
sys_write (int fd, const void *buffer, unsigned length)
{
  if (fd == 1)
    putbuf (buffer, length);
  return 1;
}
