
// This is level two - build in the target forth itself.

: marker // ( -- ) create a word to reset dictionary to before itself
	here latest @ create , , does> @+ latest ! @ cp ! ;
: ] // ( -- ) go to compile mode
	1 state  ! ;


: adump // ( a -- ) dump some text from memory
	dup .u cr 10 for 10 for
	c@+ dup 20 u< if drop 2e then emit next cr next drop ;

: var // ( x "name" -- ) ( -- addr ) make a variable
	create , does> ;
: const // ( x "name" -- ) ( -- x ) make a constant
	create , does> @ ;
: defer // ( "name" -- ) ( i*j -- i*j ) make a defered word
	create noop , does> @ exec ;
		
sp@ var sp0
: depth! sp@ sp0 ! ;
: depth sp@ sp0 @ swap - shr ;
: . .u cr ;
: s. depth for sp@ r@ shl + @ .u space next cr ;

// Our start word
here ] 
	2 u< if welcome depth! quit bye then
	@+ drop begin @+ dup while include quit repeat drop bye	
; startv !

// Catch and throw - probably shouldn't stay here
0 var handler
: catch // ( xt -- x ) catch exceptions, return x
   sp@ push handler @ push rp@ handler ! exec pull handler ! pull drop 0 ;
: throw // throw an exception
   dup 0= if drop exit then
   handler @ rp! pull handler ! pull swap push sp! drop pull ;
      

here latest @
: reset // ( -- ) resets dictionary to fresh
	lit [ , ] lit [ , ] latest ! cp ! ;

// Clone ourself
icstr "spiffy" clone 
icstr "cloned" ctype cr
bye	
