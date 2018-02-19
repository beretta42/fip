/*

   A LZW 'compress' for Fuzix

*/

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define CLEAR 256
#define BUFZ 512
#define NAMZ 256


int cflg = 0;
int fflg = 0;
int vflg = 1;

/* bits used for I/O */
int in = 0;
int out = 1;
uint8_t ibuf[BUFZ];
uint16_t ipos;
uint16_t isize;
uint8_t obuf[BUFZ];
uint16_t opos;
uint8_t num = 0;
uint8_t bbuf = 0;

/* bits used for decompression algo */
uint8_t *app;
uint16_t *ref;
uint16_t nexti;
uint8_t bits;
uint8_t mymaxbits;




void myabort( char *m );

void prints( char *s)
{
    write(2,s,strlen(s));
}

void printi( uint16_t n )
{
    char c;
    int a;
    if ( n > 9 ){
	a = n / 10;
	n = n % 10;
	printi(a);
    }
    c = '0' + n;
    write(2, &c, 1);
}


void outflush( void )
{
    int ret = write(out, &obuf, opos);
    if (ret != opos){
	myabort("Writing outfile failed.");
    }
    opos = 0;
}

void myabort( char *m)
{
    outflush();
    prints("Abort: ");
    prints( m );
    prints("\n");
    exit(1);
}

void outbyte( uint8_t c )
{
    obuf[opos++] = c;
    if (opos == BUFZ)
	outflush();
}

int16_t inbyte( void )
{
    uint8_t c; 
    if (isize == 0) {
	isize = read(in, &ibuf, BUFZ);
	ipos = 0;
	if (isize <= 0)
	    return -1;
    }
    c = ibuf[ipos++];
    if (ipos == isize) 	isize = 0;
    return c;
}

void flushbits( void )
{
    fprintf(stderr,"%d\n", num);
    if (num)
	outbyte(bbuf);
    num = 0;
    outflush();
}

void putbit( uint16_t b )
{

    bbuf |= (b & 1) << num++;
    if (num == 8){
	outbyte(bbuf);
	num = 0;
	bbuf = 0;
    }
}


void outsym( uint16_t s)
{
    int x = bits;
    while (x--){
	putbit(s);
	s >>= 1;
    }
}

void cleardict ( void )
{
    nexti = 257;
    bits = 9;
}

/* add next dictionary entry */
int adddict( uint16_t prefix, uint8_t suffix )
{
    if (nexti == (1<<bits)){ 
	if (bits < mymaxbits ){
	    bits++;
	    fprintf(stderr,"inc bits to %d\n", bits);
	}
	else{
	    fprintf(stderr,"clear dict\n");
	    return 1;
	}
    }
    *(ref + nexti ) = prefix;
    *(app + nexti ) = suffix;
    nexti++;
    fprintf(stderr,".");
    return 0;
}

void myexit( void )
{
    flushbits();
    exit(0);
}

void myalloc( void )
{
    uint16_t *a;
    uint16_t *b;
    uint8_t *c;
    intptr_t *st;
    uint16_t size;

    mymaxbits = sizeof(int) * 8 - 2;
    size = 1<<mymaxbits;
    st = sbrk(0);
    for (; mymaxbits > 8; mymaxbits--, size>>=1){
	brk(st);
	a = sbrk( size );
	if ( a == (void *)-1 ) 
	    continue;
	b = sbrk( size );
	if ( b == (void *)-1 )
	    continue;
	c =  sbrk( size );
	if ( c == (void *)-1 )
	    continue;
	mymaxbits = 9;
	if (vflg) {
	    prints("Max encoded bits: ");
	    printi(mymaxbits);
	    prints("\n");
	}
	ref = a;
	app = c;
	return;
    }
    myabort("Out of memory");
}

uint8_t *buildstr(uint8_t *p, uint16_t *size, uint16_t entry)
{
    p = p + 256;
    uint8_t *e = p;
    while (1){
	if (entry < 256){
	    *--p = entry;
	    break;
	}
	*--p = *(app + entry);
	entry = *(ref + entry);
    }
    *size = e - p;
    return p;
}

uint8_t st[257];
uint8_t st1[256];

int exist( uint16_t prefix, uint8_t suffix )
{
    uint8_t *s;
    uint8_t *s1;
    uint16_t z;
    uint16_t z1;
    uint16_t h;

    st[256] = suffix;
    s = buildstr(st, &z, prefix);
    z++;
    //write(2,s,z);
    //write(2,"<!\n",3);

    for (h = 257; h < nexti; h++){
	s1 = buildstr(st1, &z1, h);
	//write(2,s1,z1);
	//write(2,"<\n",2);
	if (z != z1) continue;
	if (!memcmp(s,s1,z))
	    return h;
    }
    return -1;
}


void compress( void )
{
    int ret;
    int16_t ch;
    uint16_t w;
    int16_t n;
    uint8_t max = mymaxbits | 0x80;

    ipos = 0;
    isize = 0;
    opos = 0;

    ret= write(1,"\x1f\x9d", 2);
    ret |= write(1,&max,1); 
    if (ret<0)
	myabort("Cannot write header.");

    cleardict();

    w = inbyte();
    while ((ch = inbyte()) >= 0 ){
	n = exist(w,ch);
	if (n >= 0){
	    w = n;
	}
	else {
	    if (adddict(w,ch)){
		outsym(CLEAR);
		cleardict();
	    } 
	    else {
		outsym(w);
		w = ch;
	    }
	}
    }
    outsym(w);
    return;
}

int main( int argc, char *argv[] )
{
    myalloc();
    compress();
    myexit();
}
