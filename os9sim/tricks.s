	export _swi2_handler
	export _os9go
	export _reg

	import _os9call

	.area .bss
_reg    rmb 2
sstack  rmb 2
	.area .text

_os9go: sts sstack
	tfr x,s
	rti

_swi2_handler:
	sts _reg
	lds sstack		; load up saved C/system stack
	jsr _os9call		; go do c: void os9call(void *sp)
	lds _reg
	rti
