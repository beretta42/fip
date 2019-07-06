	export _swi2
	export _swi2_handler
	export _os9go
	export _reg

	import _os9call

	.area .bss
_reg    rmb 2
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
	sts _reg
	lds #sstack		; kick off C with a fresh system stack
	jsr _os9call		; go do c: void os9call(void *sp)
	lds _reg
	rti
