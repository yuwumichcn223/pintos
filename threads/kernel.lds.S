#include "threads/loader.h"

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")
ENTRY(start)			/* Kernel starts at "start" symbol. */
SECTIONS
{
  /* Specifies the virtual address for the kernel base. */
  . = LOADER_PHYS_BASE + LOADER_KERN_BASE;

  _start = .;

  /* Kernel starts with code, followed by read-only data and writable data. */
  .text : { *(.start) *(.text) } = 0x90
  .rodata : { *(.rodata) *(.rodata.*) 
	      . = ALIGN(0x1000); 
	      _end_kernel_text = .; }
  .data : { *(.data) }

  /* BSS (zero-initialized data) is after everything else. */
  _start_bss = .;
  .bss : { *(.bss) }
  _end_bss = .;

  _end = .;
}
