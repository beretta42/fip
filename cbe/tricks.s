;;;
;;;
;;; tricks to patch, and run ColorBASIC
;;;
;;;
	.globl _doBASIC
	.globl _setStack
	.globl _dfd
	.globl _prior
	.globl _myexit

DEST	equ	$d900		; where  we are copying code to
	
	.area	.header
reserved
	rmb	0x2000		; FIXME: need this much?
	

	.area 	.text

;;;  This code below is called from BASIC - 
;;;  This code should be PIC, to keep it easy to relocate, and
;;;  it should be free of calls to the c runtime.
	
cstart				; start of copied code

_dfd
dfd	rmb	2		; fd of disk image
off	rmb	4		; byte offset of sector
_prior	rmb	20		; termios structure
	
	;; a new dskcon interface for BASIC
dskcon
	pshs	d,x
	ldb	$ea
	cmpb	#1
	bls	out@
	cmpb	#3
	bhi	out@
	addb	#5
	stb	DEST+(a@-cstart)+2
	;; get byte address from sector
	lda	$ec		; get track no.
	ldb	#18		; 18 sectors per track
	mul			; D = track offset
	pshs	d		; save
	clra			; D = sector no
	ldb	$ed		;
	decb			; sector nos start a 1
	addd	,s++		; D = lsn
	clr	DEST+(off-cstart)	; store in 32 bit offset
	std	DEST+(off-cstart)+1	;
	clr	DEST+(off-cstart)+3	;
	;; seek
	ldx	#0		; whence: SEEK_SET (I hope)
	ldd	#DEST+(off-cstart)	; offset
	pshs	d,x		; push onto call stack
	ldx	DEST+(dfd-cstart)	; X = fd
	pshs	x		; push a dummy
	ldd	#9		; lseek
	swi			; call fuzix
	leas	6,s		; clean up params
	cmpd	#0		; test for failure
	bne	fail@
	ldx	#256		; length of read: 1 coco sector
	ldd	$ee		; transfer address
	pshs	d,x		; push onto call stack
	ldx	DEST+(dfd-cstart)	; X = fd
	pshs	x		; push a dumy
a@	ldd	#7		; read  <---- self modifying code
	swi			; call fuzix
	leas	6,s		; clean up params
	cmpd	#0
	bne	fail@
out@	clr	$f0		; clear BASIC DSK return status
	puls	d,x,pc		; restore, return
fail@	ldb	#1
	stb	$f0		; set error (which one?)
	puls	d,x,pc

	;; console out replacement code
	;;  takes A: char to put
	;;  returns: nothing
conout
	tst	$6f		; test dev num
	bne	dsk@		; is not screen? 0 ?
	pshs	d,x		; save regs
	cmpa	#13
	bne	a@
	clr	$6c		; set DEVPOS
	lda	#10
	sta	,s
	bra	b@
a@	inc	$6c
b@	ldd	#1		; push length
	pshs	d
	leax	2,s		; point to char in stack
	pshs	x
	ldx	#2		; stderr
	pshs	x
	ldd	#8		; write
	swi			; call fuzix
	leas	6,s		; remove args
	puls	d,x		; restore regs
	leas	2,s		; drop return address
out@	rts			; return to normal console out
dsk@	jmp	$cc1c		; do disk console out
	
	;; console in replacement code
	;;  takes: nothing
	;;  returns: A
conin
	tst	$6f		; test dev num
	bne	dsk@		; is not screen? 0 ?
	clr	$70		; force clear EOF
	pshs	d,x		; save regs
	ldd	#1		; push length
	pshs	d
	leax	2,s		; point to char in stack
	pshs	x
	ldx	#0		; stdin
	pshs	x
	ldd	#7		; read
	swi			; call fuzix
	cmpx	#0
	beq	exit
	leas	6,s		; remove args
	puls	d,x		; restore regs
	cmpa	#$1b
	beq	exit
	cmpa	#$0a		; nl?
	bne	b@
	lda	#$0d		; turn into CR
b@	bsr	toupper
	leas	2,s		; drop return address
out@	rts			; return to normal console out
dsk@	jmp	$c5bc		; do disk console in
	
	;; convert keyboard input to uppercase
toupper	tst	$11a
	beq	out@
	cmpa	#$61
	blo	out@
	cmpa	#$7a
	bhi	out@
	suba	#$20
out@	rts

	;; clear the screen (vt52 for now)
cls
	pshs	y
	ldy	#4		; 4 bytes worth
	leax	a@,pcr
	pshs	d,x,y		; D is dummy
	ldx	#2
	ldd	#8		; write
	swi
	leas	6,s
	puls	y,pc
a@	.db	$1b,'H,$1b,'J

	;; exit
_myexit
exit
	;; fix tty
	leax	_prior,pcr	; termios struct
	ldd	#2		; TCSETS
	pshs	d,x		; push onto call stack
	ldx	#0		; tty no
	pshs	x		; dummy push
	ldd	#29		; ioctl syscall no
	swi			; go!
	leas	6,s		; drop params + dummy
	;; call exit
	ldx	#0
	ldd	#0		; exit
	swi			; goodbye!

	
cend				; end of copied code
	
	;; patch up BASIC's console in/out ram vectors
icept_disk
	ldd	#$c138		; X = DECB banner
	ldx	#$c0e7		; DECB warm start address
	bra	a@
icept_basic
	ldd	#$a146		; X = CB banner
	ldx	#$a0e8		; CB warm start address
	bra	a@
icept_ecb
	ldd	#$80e7		; X = ECB banner
	ldx	#$80c0		; ECB warm start address
a@
	pshs	d,x
	ldx	#$167		; point to out
	lda	#$7e		; jmp for con out
	sta	,x
	sta	3,x		; jmp for con in
	ldd	#DEST+(conout-cstart)
	std	1,x
	ldd	#DEST+(conin-cstart)
	std	4,x
	puls	x		; pull banner address
	jsr	$b99c		; print it
	puls	x		; pull warm start vector
	stx	$72		; save it
	lda	#$55		; enable warm start
	sta	$71		;
	jmp	$ac73		; jump to main loop
	

_doBASIC
	;; move runtime code
	ldx	#DEST		; dest ~end of DECB
	ldu	#cstart
a@	ldd	,u++
	std	,x++
	cmpu	#cend
	blo	a@
	;; finish what BASIC does
	ldx	#$600		; start of basic program
	clr	,x+		; clear first byte (just like cold start)
	stx	$19	        ; and save this pointer to RAM ( like cold )
	;; Patch Cold start: dont play with interrupts
	;; and make cold start call back to us just before printing banner
	lda	#$7e		; jump op
	sta	$a0d5		; intercept BASIC
	sta	$80b0		; intercept ECB
	sta	$c0d1
	ldd	#icept_basic
	std	$a0d6
	ldd	#icept_ecb
	std	$80b1
	ldd	#icept_disk
	std	$c0d2
	;; Patch dskcon to point to our fuzix driver
	lda	#$7e		; jump op
	sta	$d75f
	ldd	#DEST+(dskcon-cstart)
	std	$d760
	;; Patch references to $0000, ENDFLG
	lda	#$76	        ; new location of ENDFLG
	sta	$adb7		; CB's refs
	sta	$ae12
	sta	$ae28
	sta	$82d2		; ECB's only ref
	;; defang Break key checking FIXME: :)
	lda	#$39	        ; RTS op
	sta	$adeb		;
	;; always print CR
	lda	#$20		; BRA op
	sta	$b963		;
	;; patch CLS
	ldx	#$a928
	lda	#$7e
	sta	,x+
	ldd	#DEST+(cls-cstart)
	std	,x
	lda	#$12	        ; no op
	sta	$a927
	;; set CPU regs and call BASIC's cold start
	clra			; make sure BP is cleared
	tfr	a,dp	
	lds	#$8000		; bye-bye C stack
	tfr	s,x
	jmp	$a093		; jump to middle of BASIC's cold start


	;; Move C stack down to something usuable
	;; while we locate DECB
_setStack
	puls	x		; return address
	lds	#$8000
	jmp	,x		; return to basic
	
	