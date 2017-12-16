a:
		org $100
		jmp start
		.ascii "FZX1"
		db 1
		dw 0
		dw 0
		dw 0
		dw 0
;p
		
p: swap
		 pulu d,x
		 pshu d
		 pshu x
		 rts
;p

p: dup
		ldd ,u
		pshu d
		rts
;p
		
p: drop
		leau 2,u
		rts
;p


p: 0=
		ldd  ,u
		beq  iszero
		ldd  #0
		std  ,u
		rts
iszero		ldd  #-1
		std  ,u
		rts
;p
				
p: lit
		puls x
		ldd  ,x++
		pshu d
		jmp  ,x
;p		

p: emit
		ldd #1
		pshs d
		leau 1,u
		pshs d,u
		ldx #1
		ldd #8
		swi
		leau 1,u
		leas 6,s
		rts
;p	


p: sys3
		pulu d,x
		ldy  2,u
		pshs y
		ldy  ,u
		pshs x,y
		leau 4,u
		swi
		leas 6,s
		bra rsc
;p	


p: sys2
		pulu d,x,y
		pshs x,y
		swi
		leas 4,s
		bra rsc
;p
		
		
p: sys1
		pulu d,x
		pshs d
		swi
		leas 2,s
		bra rsc
;p	

p: sys0
		pulu d
		pshs d
		swi
		leas 2,s
rsc		pshu x
		cmpd #0
		beq skip
		std errno
skip		rts
;p			

p: errno
		ldd #errno
		pshu d
		rts
errno		dw 0
;p

p: environ
		ldd #environ
		pshu d
		rts
environ		dw 0
;p		
			

p: bra
		puls x
		ldx ,x
		jmp ,x
;p
		
p: 0bra
		puls x
		ldd ,u++
		beq bra0
		leax 2,x
		jmp ,x
bra0		ldx ,x
		jmp ,x
;p

p: +
		pulu d
		addd ,u
		std ,u
		rts
;p

p: pull
		puls x,y
		pshu y
		jmp ,x
;p

p: push
		puls x
		pulu y
		pshs y
		jmp  ,x
;p

p: @
		pulu x
		ldx  ,x
		pshu x
		rts
;p

p: c@
		pulu x
		ldb  ,x
		clra
		pshu d
		rts
;p

p: shr
		ldd ,u
		lsra
		rorb
		std ,u
		rts
;p

p: shl
		ldd ,u
		lslb
		rola
		std ,u
		rts
;p		

p: bswp
		ldd ,u
		sta 1,u
		stb ,u
		rts
;p
				

p: xor
		pulu d
		eora ,u
		eorb 1,u
		std  ,u
		rts
;p

p: and
		pulu d
		anda ,u
		andb 1,u
		std  ,u
		rts
;p		

p: or
		pulu d
		ora ,u
		orb 1,u
		std ,u
		rts
;p

		
p: com
		ldd  ,u
		coma
		comb
		std  ,u
		rts
;p
		
p: cp0
		ldd #cp0
		pshu d
		rts
;p
	
p: dp0
		ldd #dp0
		pshu d
		rts
;p			

p: rp@
		leax 2,s
		pshu x
		rts
;p
		
p: sp@
		leax ,u
		pshu x
		rts
;p

p: sp!
		ldu ,u
		rts
;p	

p: rp!
		puls x
		pulu s
		jmp ,x
;p		


p: !
		pulu x,y
		sty ,x
		rts
;p

p: c!
		pulu x
		pulu d
		stb  ,x
		rts
;p

p: strcmp
		pulu x,y
		ldd ,x++
		cmpd ,y++
		bne no
		pshs x
		leax d,x
		stx  strcmp1+1
		puls x
loop		ldb ,x+
		cmpb ,y+
		bne no
strcmp1		cmpx #0
		bne loop
		ldd #0
		pshu d
		rts
no		ldd #-1
		pshu d
		rts
;p

p: exit
		puls d
		rts
;p

p: dofor
		puls x
		ldd  ,s
		beq  out
		addd #-1
		std  ,s
		leax 2,x
		jmp ,x
out		leas 2,s		
		ldx ,x  
		jmp ,x
;p						

: brk 30 sys1 ;
: sbrk 31 sys1 ;
: write 8 sys3 ;				
: read 7 sys3 ;
: open 1 sys3 ;
: sexit 0 sys1 ;
: bye 0 sexit ;	
: exec push ;		
: = xor 0= ;										
: c@+ dup 1 + swap c@ ;
: @+ dup 2 + swap @ ;
: slit  pull dup @+ + push ;
: cr    10 emit ;
: 2drop drop drop ;
: type  @+ for c@+ emit next drop ;
: neg com 1 + ;
: - neg + ;
: r@ rp@ 2 + @ ;
: over push dup pull swap ; 
: rot push swap pull swap ;
: nip push drop pull ;
: 2dup over over ;
: 0< 0x8000 and ;
: > swap - 0x8000 and ;
: u< 2dup xor 0< if nip 0< exit then - 0< ;
: u> swap u< ; 
: .n    0xf and dup 9 > if 0x27 + then 0x30 + emit ;			
: .b    0xff and dup shr shr shr shr  .n .n ;
: .u    dup bswp .b .b ;
: >name 5 + ;
: >flags 4 + ;
: >xt 2 + ;
: dovar pull ;
: dodoes pull dup 2 + swap @ push ;
: noop ;
0 var latest
0 var state
0 var startv  
: find  latest @ begin 2dup >name strcmp while 
	@ dup 0= if nip exit then repeat nip ;
0 var wordb 32 allot
: isblank? 33 - 0< ;
: c!+ over c! 1 + ;
: !+ over ! 2 + ;



: RO 0 ;
0 var sstack ] 0 0 0 ;
0 var sptr
: spush sptr @ ! sptr @ 2 + sptr ! ;
: spull sptr @ 2 - sptr ! sptr @ @ ;
0 var source
: key 0 sp@ 1 + 1 swap source @ read 
    0= if drop spull source ! 0xa then ;
: sinit sstack sptr ! ;
: include 0 swap RO swap open dup 0x8000 and if $ "error" type cr exit then
  source dup @ spush ! ;

: isdig? 
    dup 0x30 u< if drop 0 exit then
    dup 0x66 u> if drop 0 exit then
    dup 0x39 u> swap 0x61 u< and if drop 0 exit then
    -1 ;

: todig 0x30 - dup 0x9 > if 0x27 - then ;
    
: word 
    begin key dup isblank? while drop repeat
    wordb 2 + swap
    begin c!+ key dup isblank? until 
    drop wordb - 2 - wordb !
;

: >num
   0 wordb @+ for c@+ dup isdig?  if
   todig rot shl shl shl shl + swap 
   else 2drop drop 0 pull drop exit then
   next drop -1 ;

0 var cp 
: here cp @ ;
: c, here c! here 1 + cp ! ;
: , here ! here 2 + cp ! ;
: do >xt @ exec ;
: compile 0xbd c, >xt @ , ;
: [ 0 state ! ; imm
: ; 0x39 c, [ ; imm
: header here swap
    latest @ ,
    here swap 0 ,
    0 c,
    @+ dup , for c@+ c, next drop here swap ! latest ! ;
: : word wordb header 1 state ! ;
: imm 0x80 latest @ >flags c! ;
: space 0x20 emit ;
: THEN here swap ! ;
: bra, $ "bra" find compile ;
: 0bra, $ "0bra" find compile ;
: ba, here 0 , ;
: IF 0bra, ba, ;
: AGAIN bra, , ;
: if IF ; imm
: then THEN ; imm
: else bra, ba, swap THEN ; imm			
: begin here ; imm
: until 0bra, , ; imm
: again AGAIN ; imm
: while IF ; imm
: repeat swap AGAIN THEN ; imm
: for $ "push" find compile here $ "dofor" find compile here 0 , ; imm
: next swap AGAIN THEN ; imm
: // begin key 0xa = until ; imm
: str $ "slit" find compile here 0 , here
    begin key dup isblank? while drop repeat
    begin key 2dup - while c, repeat 2drop here swap - swap ! ; imm
: create word wordb header 
    $ "dodoes" find compile 
    $ "noop" find >xt @ , ;
: DOES> pull @ latest @ >xt @ 3 + ! ; 
: does> $ "DOES>" find compile here 2 + , ; imm

: s>c @+ drop ;
: cstr $ "slit" find compile here 0 , here
    begin key dup isblank? while drop repeat
    begin key 2dup - while c, repeat 2drop 0 c, here swap - swap ! 
    $ "s>c" find compile ; imm

: icstr here 256 + dup
     begin key dup isblank? while drop repeat push
     begin key dup r@ - while c!+ repeat drop
     pull drop 0 c!+ drop ;




: WO 1 ;
: CREAT 256 ;
: clone 
     509 swap WO CREAT or swap open 
     256 here over - swap
     begin over while 2dup 3 write 
     push swap rp@ @ -
     swap pull + repeat 2drop 
;
: ctype begin c@+ dup while emit repeat 2drop ;
: dump dup .u cr 16 for 16 for c@+ .b space next cr next ;

: words latest @ begin dup while dup >name type space @ repeat drop cr ;
: quit
    begin word wordb find dup if 
       dup >flags @ 0< if do else 
       state @ if compile else do then then 
    else
	drop
	>num if state @ if $ "lit" find  compile , then 
	else wordb type $ " : word not found" type cr then
    then 
    again ;   

: welcome $ "Spiffy Forth." type cr ;

: printargs for @+ ctype cr next drop ;
		
p: main
start		leau -64,s
		puls d,x
		pshu x
		pshu d
		leax ,s
		stx  environ
;p 
]
# 0x5d00 brk drop
# 0x5f00 0x108 !
sinit
startv @ dup if exec bye then drop
dp0 latest !
cp0 cp !
welcome quit bye  

