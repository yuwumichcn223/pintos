/* This files maintains the swap table */

#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/synch.h"
#include <bitmap.h>
#include "userprog/pagedir.h"
#include <string.h>

static struct lock swap_lock;
static struct bitmap *free_slots;

static disk_sector_t find_free_slot (void);

/* Initialize the swap slot */
void
vm_swap_init (void)
{
  lock_init (&swap_lock);
  fs = disk_get (0, 1);
  swap = disk_get (1, 1);
  if (swap)
    free_slots = bitmap_create (disk_size (swap));
  else
    free_slots = NULL;
}

/* Swap the page out */
bool
vm_swap_page (struct spte_t *page)
{
  disk_sector_t off;
  void *kpage;
  ASSERT (page && page->swap);
  
  if (page->swap->sector == SECTOR_ERROR && page->swap->sector == SECTOR_ZERO)
    {
      page->swap->sector = find_free_slot ();
      page->swap->disk = swap;
    }
    
  ASSERT (page->swap->sector != BITMAP_ERROR);
  
  lock_acquire (&swap_lock);
  kpage = page->fte->kpage;
  if (pagedir_is_dirty (page->spde->pd, page->upage)) /* if page is dirty */
    {
      for (off = 0; off != SLOT_SIZE; ++off)
        disk_write (page->swap->disk, page->swap->sector + off, kpage + PGSIZE * off / SLOT_SIZE);
    }
  vm_free_frame (page->fte);
  page->fte = NULL;
  lock_release (&swap_lock);
  pagedir_clear_page (page->spde->pd, page->upage);
  return true;
}

/* Load the page back to the frame */
bool
vm_load_page (struct spte_t *page)
{
  disk_sector_t off;
  ASSERT (page->swap->sector != SECTOR_ERROR);
  
  lock_acquire (&swap_lock);
  page->fte = vm_alloc_frame ();
  if (page->swap->sector != SECTOR_ZERO)
    {
      for (off = 0; off != SLOT_SIZE; ++off)
        disk_read (page->swap->disk, page->swap->sector + off, page->fte->kpage + PGSIZE * off / SLOT_SIZE);
    }
  else /* All zero page */
    memset (page->fte->kpage, PGSIZE, 0);
  lock_release (&swap_lock);
  pagedir_set_page (page->spde->pd, page->upage, page->fte->kpage, true);
  return true;
}

/* Free the slot to let other pages to swap in */
void
vm_free_slot (struct swap_t *swap)
{
  ASSERT (free_slots);
  
  if (swap->sector != SECTOR_ERROR && swap->sector != SECTOR_ZERO)
    {
      bitmap_set_multiple (free_slots, swap->sector, SLOT_SIZE, false);
      swap->sector = SECTOR_ERROR;
    }
}

static disk_sector_t
find_free_slot (void)
{
  disk_sector_t ret;
  ASSERT (swap && free_slots);
  
  lock_acquire (&swap_lock);
  ret = bitmap_scan_and_flip (free_slots, 0, SLOT_SIZE, false);
  lock_release (&swap_lock);
  return ret;
}
