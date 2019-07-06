	export _swi2
	export _swi2_handler
	export _reg
	export _os9go

	import _os9call

	.area .bss
_reg	rmb 12
ustack  rmb 2
	rmb 256			; system/C stack
sstack
	.area .text

_swi2:
	stb p@
	swi2
p@:	.db 0
	rts

_os9go:
	tfr x,s
	rti

_swi2_handler:
	leax ,s			; copy from stack frame
	stx ustack		; save user stack pointer for later
	ldy #_reg		; copy 12 bytes over to regs struct
	ldd ,x++
	std ,y++
	ldd ,x++
	std ,y++
	ldd ,x++
	std ,y++
	ldd ,x++
	std ,y++
	ldd ,x++
	std ,y++
	ldd ,x
	std ,y
	lds #sstack		; kick off C with a fresh system stack
	jsr _os9call
	;; translate back to regs here
	ldy ustack		; reload user's stack and copy it all back over
	leas ,y
	leay ,s
	ldx #_reg
	ldd ,x++
	std ,y++
	ldd ,x++
	std ,y++
	ldd ,x++
	std ,y++
	ldd ,x++
	std ,y++
	ldd ,x++
	std ,y++
	ldd ,x
	std ,y
	rti
