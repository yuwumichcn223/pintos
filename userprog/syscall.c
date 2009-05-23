#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
/* My Implementation */
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/process.h"
#include <list.h>
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/palloc.h"
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

static struct file *find_file_by_fd (int fd);
static struct fd_elem *find_fd_elem_by_fd (int fd);
static int alloc_fid (void);

typedef int (*handler) (uint32_t, uint32_t, uint32_t);
static handler syscall_vec[128];

struct fd_elem
  {
    int fd;
    struct file *file;
    struct list_elem elem;
    struct list_elem thread_elem;
  };
  
static struct list file_list;

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
  
  list_init (&file_list);
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
  struct file * f;
  
  if (fd == STDOUT_FILENO) /* stdout */
    putbuf (buffer, length);
  else if (fd == STDIN_FILENO) /* stdin */
    return -1;
  else
    {
      f = find_file_by_fd (fd);
      if (!f)
        return -1;
      return file_write (f, buffer, length);
    }
  return length;
}

static int
sys_exit (int status)
{
  /* Close all the files */
  struct thread *t;
  struct list_elem *l;
  
  t = thread_current ();
  while (!list_empty (&t->files))
    {
      l = list_pop_front (&t->files);
      sys_close (list_entry (l, struct fd_elem, thread_elem)->fd);
    }
  
  t->ret_status = status;
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
  struct file *f;
  struct fd_elem *fde;
  
  if (!file) /* file == NULL */
    return -1;
  
  f = filesys_open (file);
  if (!f) /* Bad file name */
    return -1;
    
  fde = (struct fd_elem *)palloc_get_page (0);
  if (!fde) /* Not enough memory */
    {
      file_close (f);
      return -1;
    }
    
  fde->file = f;
  fde->fd = alloc_fid ();
  list_push_back (&file_list, &fde->elem);
  list_push_back (&thread_current ()->files, &fde->thread_elem);
  return fde->fd;
}

static int
sys_close(int fd)
{
  struct fd_elem *f;
  
  f = find_fd_elem_by_fd (fd);
  
  if (!f) /* Bad fd */
    return -1;
  file_close (f->file);
  list_remove (&f->elem);
  list_remove (&f->thread_elem);
  palloc_free_page (f);
  return 0;
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

static struct file *
find_file_by_fd (int fd)
{
  struct fd_elem *ret;
  
  ret = find_fd_elem_by_fd (fd);
  if (!ret)
    return NULL;
  return ret->file;
}

static struct fd_elem *
find_fd_elem_by_fd (int fd)
{
  struct fd_elem *ret;
  struct list_elem *l;
  
  for (l = list_begin (&file_list); l != list_end (&file_list); l = list_next (l))
    {
      ret = list_entry (l, struct fd_elem, elem);
      if (ret->fd == fd)
        return ret;
    }
    
  return NULL;
}

static int
alloc_fid (void)
{
  static int fid = 2;
  return fid++;
}
