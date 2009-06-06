/* This file is to manage virtual memory */

#ifndef VM_VM_H
#define VM_VM_H

#include "threads/pte.h"

#define FRAME_TABLE_SIZE 32

struct frame_table_entry
{
  void *kpage; /* physical address to the frame */
};


void vm_init (void);

void vm_frame_init (void);
struct frame_table_entry *vm_alloc_frame (void);

void vm_page_init (void);
bool vm_pagedir_create (uint32_t *pd);
void vm_pagedir_destroy (uint32_t *pd);
struct spte_t *vm_page_create (uint32_t *pd, void *vaddr);
void vm_page_destroy (struct spte_t *spte);

#endif
