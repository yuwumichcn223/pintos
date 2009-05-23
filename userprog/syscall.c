#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
/* My Implementation */
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/process.h"
/* == My Implementation */

static void syscall_handler (struct intr_frame *);

/* My Implementation */

typedef int pid_t;

static int sys_write (int fd, const void *buffer, unsigned length);
static int sys_exit (int status);
static int sys_halt (void);
static int sys_create (const char *file, unsigned initial_size);
static int sys_open (const char *file);
static int sys_close (int fd);
static int sys_read (int fd, void *buffer, unsigned size);
static int sys_exec (const char * cmd);
static int sys_wait (pid_t pid);

typedef int (*handler) (uint32_t, uint32_t, uint32_t);
static handler syscall_vec[128];
/* == My Implementation */

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  
  /* My Implementation */

  syscall_vec[SYS_EXIT] = (handler)sys_exit;
  syscall_vec[SYS_HALT] = (handler)sys_halt;
  syscall_vec[SYS_CREATE] = (handler)sys_create;
  syscall_vec[SYS_OPEN] = (handler)sys_open;
  syscall_vec[SYS_CLOSE] = (handler)sys_close;
  syscall_vec[SYS_READ] = (handler)sys_read;
  syscall_vec[SYS_WRITE] = (handler)sys_write;
  syscall_vec[SYS_EXEC] = (handler)sys_exec;
  syscall_vec[SYS_WAIT] = (handler)sys_wait;
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
  int *p;
  int ret;
  
  p = f->esp;
  
  if (!is_user_vaddr (p))
    goto terminate;
  
  if (*p < SYS_HALT || *p > SYS_INUMBER)
    goto terminate;
  
  h = syscall_vec[*p];
  
  if (!(is_user_vaddr (p + 1) && is_user_vaddr (p + 2) && is_user_vaddr (p + 3)))
    goto terminate;
  
  ret = h (*(p + 1), *(p + 2), *(p + 3));
  
  f->eax = ret;
  
  return;
  
terminate:
  sys_exit (-1);
  /* == My Implementation */
}

static int
sys_write (int fd, const void *buffer, unsigned length)
{
  if (fd == 1)
    putbuf (buffer, length);
  return length;
}

static int
sys_exit (int status)
{
  thread_current ()->ret_status = status;
  thread_exit ();
  return -1;
}

static int
sys_halt (void)
{
  power_off ();
}

static int
sys_create (const char *file, unsigned initial_size)
{
  return -1;
}

static int
sys_open (const char *file)
{
  return -1;
}

static int
sys_close(int fd)
{
  return -1;
}

static int
sys_read (int fd, void *buffer, unsigned size)
{
  return -1;
}

static int
sys_exec (const char * cmd)
{
  if (!cmd)
    return -1;
  return process_execute (cmd);
}

static int
sys_wait (pid_t pid)
{
  return process_wait (pid);
}
