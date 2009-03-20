#ifndef THREADS_LOADER_H
#define THREADS_LOADER_H

/* Constants fixed by the PC BIOS. */
#define LOADER_BASE 0x7c00      /* Physical address of loader's base. */
#define LOADER_END  0x7e00      /* Physical address of end of loader. */

/* Physical address of kernel base. */
#define LOADER_KERN_BASE 0x100000       /* 1 MB. */

/* Kernel virtual address at which all physical memory is mapped.

   The loader maps the 4 MB at the bottom of physical memory to
   this virtual base address.  Later, paging_init() adds the rest
   of physical memory to the mapping.

   This must be aligned on a 4 MB boundary. */
#define LOADER_PHYS_BASE 0xc0000000     /* 3 GB. */

/* Important loader physical addresses. */
#define LOADER_SIG (LOADER_END - LOADER_SIG_LEN)   /* 0xaa55 BIOS signature. */
#define LOADER_ARGS (LOADER_SIG - LOADER_ARGS_LEN)     /* Command-line args. */
#define LOADER_ARG_CNT (LOADER_ARGS - LOADER_ARG_CNT_LEN) /* Number of args. */
#define LOADER_RAM_PGS (LOADER_ARG_CNT - LOADER_RAM_PGS_LEN) /* # RAM pages. */

/* Sizes of loader data structures. */
#define LOADER_SIG_LEN 2
#define LOADER_ARGS_LEN 128
#define LOADER_ARG_CNT_LEN 4
#define LOADER_RAM_PGS_LEN 4

/* GDT selectors defined by loader.
   More selectors are defined by userprog/gdt.h. */
#define SEL_NULL        0x00    /* Null selector. */
#define SEL_KCSEG       0x08    /* Kernel code selector. */
#define SEL_KDSEG       0x10    /* Kernel data selector. */

#endif /* threads/loader.h */
