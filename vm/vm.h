/* This file is to manage virtual memory */

#ifndef VM_VM_H
#define VM_VM_H

#include "threads/pte.h"
#include <list.h>
#include "threads/synch.h"
#include "devices/disk.h"

#define FRAME_TABLE_SIZE 1024
#define SLOT_SIZE (PGSIZE / DISK_SECTOR_SIZE)
#define SECTOR_ERROR (disk_sector_t)-1
#define SECTOR_ZERO (disk_sector_t)-2

struct frame_table_entry
{
  void *kpage;   /* physical address to the frame */
  bool occupied; /* whether used or not */
};

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
  struct spde_t *spde;
  struct swap_t *swap;            /* where it is swapped at */
};

struct swap_t
{
  struct spte_t *page;  /* which page it refers to */
  struct disk *disk;    /* which disk to swap to */
  disk_sector_t sector; /* sector on the disk */
};

struct disk *fs;    /* disk used by the file system */
struct disk *swap;  /* disk used by swap slots */

void vm_init (void);

void vm_frame_init (void);
struct frame_table_entry *vm_alloc_frame (void);
void vm_free_frame (struct frame_table_entry *f);

void vm_page_init (void);
bool vm_pagedir_create (uint32_t *pd);
void vm_pagedir_destroy (uint32_t *pd);
struct spte_t *vm_page_create (uint32_t *pd, void *vaddr, struct disk *disk, disk_sector_t sector);
void vm_page_destroy (struct spte_t *spte);
struct spte_t *vm_page_find_by_vaddr (uint32_t *pd, void *vaddr);

void vm_swap_init (void);
bool vm_load_page (struct spte_t *page);
bool vm_swap_page (struct spte_t *page);
void vm_free_slot (struct swap_t *swap);
#endif
