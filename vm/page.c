/* This files maintains the supplemental page table */

#include "vm/vm.h"
#include <list.h>
#include "threads/synch.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"

struct spde_t /* page directory entry */
{
  uint32_t *pd;          /* page directory */
  struct list spt;       /* supplemental page table */
  struct lock mutex;     /* the lock */
  struct list_elem elem;
};

struct spte_t /* page table entry */
{
  struct frame_table_entry *fte;  /* kernel frame */
  void *upage;                    /* virtual address */
  struct list_elem elem;
};

static struct list pagedirs; /* page directories */
static struct lock pagelock; /* lock for pagedirs */

static struct spde_t *find_spde_by_pd (uint32_t *pd);

void
vm_init (void)
{
  vm_frame_init ();
  vm_page_init ();
}

void
vm_page_init (void)
{
  list_init (&pagedirs);
  lock_init (&pagelock);
}

/* create a page directory in the list
 * returns true when successfully done, false otherwise
 */
bool
vm_pagedir_create (uint32_t *pd)
{
  struct spde_t *spde;
  bool success;
  
  ASSERT (pd);
  ASSERT (!find_spde_by_pd (pd));
  
  success = false;
  lock_acquire (&pagelock);
  spde = malloc (sizeof (struct spde_t));
  if (!spde)
    goto done;
  spde->pd = pd;
  list_init (&spde->spt);
  lock_init (&spde->mutex);
  list_push_back (&pagedirs, &spde->elem);
  success = true;
  
done:
  lock_release (&pagelock);
  return success;
}

void
vm_pagedir_destroy (uint32_t *pd)
{
  struct spde_t *spde;
  struct list_elem *l;
  
  ASSERT (pd);
  spde = find_spde_by_pd (pd);
  ASSERT (spde);
  lock_acquire (&pagelock);
  list_remove (&spde->elem);
  for (l = list_begin (&spde->spt); l != list_end (&spde->spt); l = list_next (l))
    vm_page_destroy (list_entry (l, struct spte_t, elem));
  free (spde);
  lock_release (&pagelock);
}

struct spte_t *
vm_page_create (uint32_t *pd, void *vaddr)
{
  struct spte_t *ret;
  struct spde_t *spde;
  
  ASSERT (pd && vaddr);
  spde = find_spde_by_pd (pd);
  ASSERT (spde);
  
  lock_acquire (&spde->mutex);
  ret = malloc (sizeof (struct spte_t));
  if (!ret)
    goto done;
  ret->fte = vm_alloc_frame ();
  ret->upage = vaddr;
  if (!pagedir_set_page (pd, vaddr, ret->fte->kpage, true))
    {
      free (ret);
      goto done;
    }
  list_push_back (&spde->spt, &ret->elem);
  
done:
  lock_release (&spde->mutex);
  return ret;
}

void
vm_page_destroy (struct spte_t *spte)
{
  
}

static struct spde_t *find_spde_by_pd (uint32_t *pd)
{
  struct spde_t *ret;
  struct list_elem *l;
  
  for (l = list_begin (&pagedirs); l != list_end (&pagedirs); l = list_next (l))
    {
      ret = list_entry (l, struct spde_t, elem);
      if (ret->pd == pd)
        return ret;
    }
  
  return NULL;
}
