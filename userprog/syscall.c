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
#include "threads/malloc.h"
#include "devices/input.h"
#include "threads/synch.h"
/* == My Implementation */

static void syscall_handler (struct intr_frame *);

/* My Implementation */

typedef int pid_t;
typedef int mapid_t;

static int sys_write (int fd, const void *buffer, unsigned length);
static int sys_halt (void);
static int sys_create (const char *file, unsigned initial_size);
static int sys_open (const char *file);
static int sys_close (int fd);
static int sys_read (int fd, void *buffer, unsigned size);
static int sys_exec (const char *cmd);
static int sys_wait (pid_t pid);
static int sys_filesize (int fd);
static int sys_tell (int fd);
static int sys_seek (int fd, unsigned pos);
static int sys_remove (const char *file);
static mapid_t sys_mmap (int fd, void *vaddr);
static void sys_munmap (mapid_t mapid);

static struct file *find_file_by_fd (int fd);
static struct fd_elem *find_fd_elem_by_fd (int fd);
static int alloc_fid (void);
static struct fd_elem *find_fd_elem_by_fd_in_process (int fd);

typedef int (*handler) (uint32_t, uint32_t, uint32_t);
static handler syscall_vec[128] = { NULL };
static struct lock file_lock;

struct fd_elem
  {
    int fd;
    struct file *file;
    struct list_elem elem;
    struct list_elem thread_elem;
  };
  
static struct list file_list;

#define REG_SYSCALL(CALL, HANDLER) syscall_vec[(CALL)] = (handler)(HANDLER)

/* == My Implementation */

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  
  /* My Implementation */

  REG_SYSCALL (SYS_EXIT, sys_exit);
  REG_SYSCALL (SYS_HALT, sys_halt);
  REG_SYSCALL (SYS_CREATE, sys_create);
  REG_SYSCALL (SYS_OPEN, sys_open);
  REG_SYSCALL (SYS_CLOSE, sys_close);
  REG_SYSCALL (SYS_READ, sys_read);
  REG_SYSCALL (SYS_WRITE, sys_write);
  REG_SYSCALL (SYS_EXEC, sys_exec);
  REG_SYSCALL (SYS_WAIT, sys_wait);
  REG_SYSCALL (SYS_FILESIZE, sys_filesize);
  REG_SYSCALL (SYS_SEEK, sys_seek);
  REG_SYSCALL (SYS_TELL, sys_tell);
  REG_SYSCALL (SYS_REMOVE, sys_remove);
  REG_SYSCALL (SYS_MMAP, sys_mmap);
  REG_SYSCALL (SYS_MUNMAP, sys_munmap);
  
  list_init (&file_list);
  lock_init (&file_lock);
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
  int ret;
  
  ret = -1;
  lock_acquire (&file_lock);
  if (fd == STDOUT_FILENO) /* stdout */
    putbuf (buffer, length);
  else if (fd == STDIN_FILENO) /* stdin */
    goto done;
  else if (!is_user_vaddr (buffer) || !is_user_vaddr (buffer + length))
    {
      lock_release (&file_lock);
      sys_exit (-1);
    }
  else
    {
      f = find_file_by_fd (fd);
      if (!f)
        goto done;
        
      ret = file_write (f, buffer, length);
    }
    
done:
  lock_release (&file_lock);
  return ret;
}

int
sys_exit (int status)
{
  /* Close all the files */
  struct thread *t;
  struct list_elem *l;
  
  t = thread_current ();
  while (!list_empty (&t->files))
    {
      l = list_begin (&t->files);
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
  if (!file)
    return sys_exit (-1);
  return filesys_create (file, initial_size);
}

static int
sys_open (const char *file)
{
  struct file *f;
  struct fd_elem *fde;
  int ret;
  
  ret = -1; /* Initialize to -1 */
  if (!file) /* file == NULL */
    return -1;
  if (!is_user_vaddr (file))
    sys_exit (-1);
  f = filesys_open (file);
  if (!f) /* Bad file name */
    goto done;
    
  fde = (struct fd_elem *)malloc (sizeof (struct fd_elem));
  if (!fde) /* Not enough memory */
    {
      file_close (f);
      goto done;
    }
    
  fde->file = f;
  fde->fd = alloc_fid ();
  list_push_back (&file_list, &fde->elem);
  list_push_back (&thread_current ()->files, &fde->thread_elem);
  ret = fde->fd;
done:
  return ret;
}

static int
sys_close(int fd)
{
  struct fd_elem *f;
  
  f = find_fd_elem_by_fd_in_process (fd);
  
  if (!f) /* Bad fd */
    goto done;
  file_close (f->file);
  list_remove (&f->elem);
  list_remove (&f->thread_elem);
  free (f);
  
done:
  return 0;
}

static int
sys_read (int fd, void *buffer, unsigned size)
{
  struct file * f;
  unsigned i;
  int ret;
  
  ret = -1; /* Initialize to zero */
  lock_acquire (&file_lock);
  if (fd == STDIN_FILENO) /* stdin */
    {
      for (i = 0; i != size; ++i)
        *(uint8_t *)(buffer + i) = input_getc ();
      ret = size;
      goto done;
    }
  else if (fd == STDOUT_FILENO) /* stdout */
      goto done;
  else if (!is_user_vaddr (buffer) || !is_user_vaddr (buffer + size)) /* bad ptr */
    {
      lock_release (&file_lock);
      sys_exit (-1);
    }
  else
    {
      f = find_file_by_fd (fd);
      if (!f)
        goto done;
      ret = file_read (f, buffer, size);
    }
    
done:    
  lock_release (&file_lock);
  return ret;
}

static int
sys_exec (const char *cmd)
{
  int ret;
  
  if (!cmd || !is_user_vaddr (cmd)) /* bad ptr */
    return -1;
  lock_acquire (&file_lock);
  ret = process_execute (cmd);
  lock_release (&file_lock);
  return ret;
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

static int
sys_filesize (int fd)
{
  struct file *f;
  
  f = find_file_by_fd (fd);
  if (!f)
    return -1;
  return file_length (f);
}

static int
sys_tell (int fd)
{
  struct file *f;
  
  f = find_file_by_fd (fd);
  if (!f)
    return -1;
  return file_tell (f);
}

static int
sys_seek (int fd, unsigned pos)
{
  struct file *f;
  
  f = find_file_by_fd (fd);
  if (!f)
    return -1;
  file_seek (f, pos);
  return 0; /* Not used */
}

static int
sys_remove (const char *file)
{
  if (!file)
    return false;
  if (!is_user_vaddr (file))
    sys_exit (-1);
    
  return filesys_remove (file);
}

static struct fd_elem *
find_fd_elem_by_fd_in_process (int fd)
{
  struct fd_elem *ret;
  struct list_elem *l;
  struct thread *t;
  
  t = thread_current ();
  
  for (l = list_begin (&t->files); l != list_end (&t->files); l = list_next (l))
    {
      ret = list_entry (l, struct fd_elem, thread_elem);
      if (ret->fd == fd)
        return ret;
    }
    
  return NULL;
}

static mapid_t
sys_mmap (int fd, void *vaddr)
{
  return 0;
}

static void
sys_munmap (mapid_t mapid)
{
  return;
}
