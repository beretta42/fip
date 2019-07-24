	;; imported
	import _main
	import __sectionbase_.bss__
	import __sectionlen_.bss__
	;; exported
	export start
	export _getc
	export _puts
	export _putc
	export _exit
	export _bzero
	export _memcpy
	export _strcmp
	export _bounce
	export _bounce_end
	export _exec
	export _di


	.area .bss
save	rmb   2
	rmb   256
mysp	
	
	.area .text
	
start:
	;; zero bss
	ldx #__sectionbase_.bss__
	ldd #__sectionlen_.bss__
	pshs d
	jsr _bzero
	puls d
	;; save BASIC stack frame
	sts save
	lds #mysp
	;; call main
	jsr _main
_exit:	lds save
	rts

_di:
	orcc #$50
	rts

	
_bzero:
	tfr x,d
	addd 2,s
	std end@+1
	bra end@
a@	clr ,x+
end@	cmpx #0
	bne a@
	rts

_memcpy:
	ldy 2,s
	tfr x,d
	addd 4,s
	std end@+1
	bra end@
a@	lda ,y+
	sta ,x+
end@	cmpx #0
	bne a@
	rts

_strcmp:
	pshs y
	ldy 4,s
a@	lda ,x+
	pshs cc
	cmpa ,y+
	bne err@
	puls cc
	bne a@
	ldx #0
	puls y,pc
err@	ldx #-1
	puls cc,y,pc

;;; get a char from input
_getc:
	jsr $a176
	tfr a,b
	rts
	
;;; put a char to screen
_putc:	tfr b,a
	jsr $a282
	rts

;;; put a string to the text screen
_puts:
a@	ldb ,x+
	beq b@
	bsr _putc
	bra a@
b@	rts


_exec:
	jmp	,x
	
_bounce
        ;; map in rest of kernel
        ldx     #$ffa9
        lda     #1
a@      sta     ,x+
        inca
        cmpa    #8
        bne     a@
        .db     $7e             ; jump
_bounce_end


	
	END start

