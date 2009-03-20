#### The loader needs to have some way to know the kernel's entry
#### point, that is, the address to which it should jump to start the
#### kernel.  We handle this by writing the linker script kernel.lds.S
#### so that this module appears at the very beginning of the kernel
#### image, and then using that as the entry point.

.section .start
	
.globl start
.func start
	# Call main.
start:	call main

	# main() should not return, but if it does, spin.
1:	jmp 1b
.endfunc
