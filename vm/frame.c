/* This files maintains the frame table */

#include "vm/vm.h"
#include "threads/palloc.h"
#include "threads/pte.h"

#define PT_MAGIC (void *)0x67452301

static struct frame_table_entry *find_free_frame (void);
static bool is_frame_valid (struct frame_table_entry *f);

static struct frame_table_entry ft[FRAME_TABLE_SIZE];

void
vm_frame_init (void)
{
  int i;
  
  for (i = 0; i != FRAME_TABLE_SIZE - 1; ++i)
    ft[i].kpage = NULL;
  ft[FRAME_TABLE_SIZE - 1].kpage = PT_MAGIC;
}

/* Allocates a physical frame */
struct frame_table_entry *
vm_alloc_frame (void)
{
  struct frame_table_entry *_free;
  void *p;
  
  _free = find_free_frame ();
  ASSERT (is_frame_valid (_free));
  
  p = palloc_get_page (PAL_USER); /* from a user pool */
  _free->kpage = p;
  return _free;
}

/* Finds a free frame from the frame table,
 * if all frame is occupied, then evict some page
 */
static struct frame_table_entry *
find_free_frame (void)
{
  struct frame_table_entry *ret;
  
  ret = ft;
  while (ret->kpage && ret->kpage != PT_MAGIC)
    ++ret;
  return ret;
}

static bool
is_frame_valid (struct frame_table_entry *f)
{
  return f && f->kpage != PT_MAGIC;
}
