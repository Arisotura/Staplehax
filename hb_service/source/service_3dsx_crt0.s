@---------------------------------------------------------------------------------
@ 3DS processor selection
@---------------------------------------------------------------------------------
	.cpu mpcore
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
	.section ".crt0"
	.global _start, __service_ptr, __apt_appid, __heap_size, __linear_heap_size, __system_arglist, __system_runflags
@---------------------------------------------------------------------------------
	.align 2
	.arm
@---------------------------------------------------------------------------------
_start:
@---------------------------------------------------------------------------------
	b startup
	.ascii "_prm" @ 3DSX startup params. We don't need them for this service.
__service_ptr:
	.word 0 @ Pointer to service handle override list -- if non-NULL it is assumed that we have been launched from a homebrew launcher
__apt_appid:
	.word 0 @ Program APPID
__heap_size:
	.word 0 @ Default heap size
__linear_heap_size:
	.word 0 @ Default linear heap size
__system_arglist:
	.word 0 @ Pointer to argument list (argc (u32) followed by that many NULL terminated strings)
__system_runflags:
	.word 0 @ Flags to signal runtime restrictions to ctrulib
startup:
	@ Clear the BSS section
	ldr r0, =__bss_start__
	ldr r1, =__bss_end__
	sub r1, r1, r0
	bl  ClearMem

	@ Jump to user code
	ldr r3, =main
	bx  r3

@---------------------------------------------------------------------------------
@ Clear memory to 0x00 if length != 0
@  r0 = Start Address
@  r1 = Length
@---------------------------------------------------------------------------------
ClearMem:
@---------------------------------------------------------------------------------
	mov  r2, #3     @ Round down to nearest word boundary
	add  r1, r1, r2 @ Shouldn't be needed
	bics r1, r1, r2	@ Clear 2 LSB (and set Z)
	bxeq lr         @ Quit if copy size is 0

	mov	r2, #0
ClrLoop:
	stmia r0!, {r2}
	subs  r1, r1, #4
	bne   ClrLoop

	bx lr
