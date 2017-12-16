// I guess this is a packet dumper I started on.
: ioctl 1d sys3 ;
: allot here + cp ! ;
create buf1 6 allot
create buf2 8 allot
create packet 2048 allot
0 RO icstr "/dev/dw2" open var dev
: poll buf1 f3 c!+ 1 c!+ 0 c!+ drop
   buf2 buf1 !+ 3 !+ buf1 !+ 2 !+ drop
   buf2 1 dev @ ioctl drop buf1 @ ;
: recv buf1 f3 c!+ 1 c!+ 1 c!+ drop
   buf2 buf1 !+ 3 !+ packet !+ swap !+ drop
   buf2 1 dev @ ioctl drop ;   
: pause 25 sys1 ;
: sleep 10 pause drop ;   


: doicmp 
      str "ICMP: " type
     packet 14 + @ .u space ;
: dotcp
      str "TCP: " type
      pa:
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
	
