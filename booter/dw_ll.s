;;;  drivewire driver 
;;;
;;; todo: handle frame/read/ errors.
;;; 

	export	_dw_trans
	export	_dw_cksum

	.area	.text

_dw_cksum	rmb	2

	;; int dw_ll( char *s, int sz, char *r, int rz);
	;; do a drivewire transaction, send some stuff, rx some stuff
	;; PC sz r rz
	;; returns 0 = ok, 
_dw_trans:
	ldy	2,s
	orcc	#$50
	lbsr 	DWWrite
	ldx	4,s
	ldy	6,s
	lbsr	DWRead
	andcc	#~$50
	tfr	cc,b
	clra
	eorb	#4		; flip Z - Z set if error
	andb	#5		; mask only errors Z and C
	tfr	d,x
	sty	_dw_cksum
	rts
	
	
; Used by DWRead and DWWrite
IntMasks equ   $50
NOINTMASK equ  1

; Hardcode these for now so that we can use below files unmodified
H6309    equ 0
	IFNDEF BECKER	
BECKER   equ 1
	ENDC
        IFNDEF MULTICOMP
MULTICOMP equ 0
        ENDC
ARDUINO  equ 0
JMCPBCK  equ 0
BAUD38400 equ 0	

	include "dw-6809.def"
	include "dwread-6809.s"
	include "dwwrite-6809.s"
