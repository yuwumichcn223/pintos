/* This files adds mmap mechanism to virtual memory */

#include "vm/vm.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "devices/disk.h"
#include <list.h>
#include "threads/malloc.h"
#include <stdio.h>

struct map_elem
{
  mapid_t id;
  struct list pages;
  struct list_elem elem;
};

static mapid_t alloc_mapid (void);
static struct map_elem *find_map_elem_by_mapid (mapid_t id);
static bool check_mapped (uint32_t *pd, void *vaddr);

static struct list maps;

void
vm_mmap_init (void)
{
  list_init (&maps);
}

/* Memory map the files */
mapid_t
vm_mmap (uint32_t *pd, struct file *file, void *vaddr)
{
  struct inode *ino;
  disk_sector_t sector;
  mapid_t ret;
  struct map_elem *map;
  struct spte_t *spte;
  int i, size;
  disk_sector_t off;
  
  ret = MAPID_ERROR;
  
  if (check_mapped (pd, vaddr))
    goto done;
  
  ino = file_get_inode (file);
  sector = inode_get_inumber (ino) + 1;
  
  size = file_length (file) / PGSIZE;
  if (file_length (file) % PGSIZE)
    ++size;
  
  map = malloc (sizeof (struct map_elem));
  if (!map)
    goto done;
  
  list_init (&map->pages);
  
  for (i = 0; i != size; ++i)
    {
      spte = vm_page_create (pd, vaddr, fs, sector);
      for (off = 0; off != SLOT_SIZE; ++off)
        disk_read (spte->swap->disk, spte->swap->sector + off, spte->fte->kpage + PGSIZE * off / SLOT_SIZE);
      list_push_back (&map->pages, &spte->mmap_elem);
      spte->mmapped = true;
      vaddr += PGSIZE;
      sector += SLOT_SIZE;
    }
  ret = alloc_mapid ();
  map->id = ret;
  list_push_back (&maps, &map->elem);
done:
  return ret;
}

/* Unmap the memory area */
void
vm_munmap (mapid_t id)
{
  struct map_elem *m;
  struct list_elem *l;
  struct spte_t *spte;
  
  m = find_map_elem_by_mapid (id);
  if (!m)
    return;
  while (!list_empty (&m->pages))
    {
      l = list_begin (&m->pages);
      spte = list_entry (l, struct spte_t, mmap_elem);
      vm_page_destroy (spte);
    }
  list_remove (&m->elem);
  free (m);
}

static struct map_elem *
find_map_elem_by_mapid (mapid_t id)
{
  struct list_elem *l;
  struct map_elem *ret;
  
  for (l = list_begin (&maps); l != list_end (&maps); l = list_next (l))
    {
      ret = list_entry (l, struct map_elem, elem);
      if (ret->id == id)
        return ret;
    }
    
  return NULL;
}

static bool
check_mapped (uint32_t *pd, void *vaddr)
{
  struct list_elem *l, *r;
  struct map_elem *m;
  struct spte_t *s;
  void *round;
  
  round = pg_round_down (vaddr);
  
  if (pagedir_get_page (pd, vaddr))
    return true;
  
  for (l = list_begin (&maps); l != list_end (&maps); l = list_next (l))
    {
      m = list_entry (l, struct map_elem, elem);
      for (r = list_begin (&m->pages); r != list_end (&m->pages); r = list_next (r))
        {
          s = list_entry (r, struct spte_t, mmap_elem);
          if (s->upage == round)
            return true;
        }
    }
    
  return false;
}

static mapid_t
alloc_mapid (void)
{
  static mapid_t id = 0;
  return id++;
}
