	;; imported
	import _main
	;; exported
	export start
	export _puts


	.area .bss
save	rmb   2
	
	.area .text
	
start:
	sts save
	lds $8000
	jsr _main
	lds save
	rts
	
;;; put a string to the text screen
_puts:  pshs u
	leax -1,x
	jsr $b99c
	puls u,pc

	END start
