	export _swi2
	export _swi2_handler
	export _reg
	export _os9go

	import _os9call

	.area .data
_reg	rmb 12
	
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
	;; copy regs from stack to data area
	;; so C get at them
	leax ,s
	ldy #_reg
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
	jsr _os9call
	;; translate back to regs here
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
