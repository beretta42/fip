/*
  A LZW 'uncompress' for Fuzix
  
  set MAXBITS, BUFZ:

  MAXBITS - how many bits per symbol.
  BUFZ    - I/O buffer size (make it multiple of disk sectors)

*/

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define MAXBITS 13
#define CLEAR 256
#define DICTZ (1<<MAXBITS)
#define BUFZ  1024
#define NAMZ  256

/* bits used for passing commandline options */
uint8_t cflg = 0;
uint8_t fflg = 0;
uint8_t vflg = 0;

/* bits used for decompression algo */
uint8_t app[DICTZ];
uint16_t ref[DICTZ];
uint16_t nexti;
uint8_t bits;
uint16_t maxbits;

/* bits used for in / out buffering */
char infile[NAMZ];
char outfile[NAMZ];
int eof;
int in = 0;  /* in and out fd's */
int out = 1;
uint8_t ibuf[BUFZ];
uint16_t ipos;
uint16_t isize;
uint8_t obuf[BUFZ];
uint16_t opos;

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
    if (ret != opos ) {
	myabort("Writing outfile failed.");
    }
    opos = 0;
}

void myabort( char *m )
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


void cleardict ( void )
{
    nexti = 257;
    bits = 9;
}

uint8_t getbit( void )
{
    static uint8_t num = 0;
    static uint8_t c;
    uint8_t x;
    int ret;

    if (num == 0){
	c = ret = inbyte();
	if (ret < 0){
	    eof = 1;
	    return 0;
	}
	else
	    num = 8;
    }
    num--;
    x = c & 1;
    c = c / 2;
    return x;
}

uint16_t getsym( void )
{
    uint16_t acc = 0;
    int x;
    for (x = 0; x < bits; x++){
	acc = acc | (getbit()<<x);
    }
    return acc;
}

void printsym( uint16_t sym )
{
    if (sym < 256){
	outbyte(sym);
	return;
    }
    printsym(ref[sym]);
    outbyte(app[sym]);
}

/* find first charactor in a symbol string */
uint8_t first( uint16_t sym )
{
    if (sym < 256 ) return sym;
    return first(ref[sym]);
}

/* add next dictionary entry */
void adddict( uint16_t prefix, uint8_t suffix )
{
    if (nexti == (1<<bits))
	return;
    ref[nexti] = prefix;
    app[nexti] = suffix;
    nexti++;
    if (nexti >= (1<<bits) &&
	bits < maxbits ) {
	bits++;
    }
    return;
}

void myexit( void )
{
    outflush();
    exit(0);
}


int decompress( void )
{
    uint8_t header[3];
    int ret;

    ipos = 0;
    isize = 0;
    opos = 0;
    eof = 0;

    ret = read(in, &header, 3);
    if (ret < 3 || header[0] != 0x1f || header[1] != 0x9d )
	myabort("Bad id in header");
    
    maxbits = header[2] & 0x1f;
    
    if (maxbits > MAXBITS){
	prints("Error: encoded in ");
	printi(maxbits);
	prints(" bits, but I can only do ");
	printi(MAXBITS);
	myabort(". ");
    }

    cleardict();

    /* All the main logic loop for decoding is down here */
    uint16_t cur;
    uint16_t prev;
 again:
    prev = getsym();
    printsym(prev);
    while (!eof) {
	cur = getsym();
	if (eof) break; 
	if (cur > nexti){
	    myabort("Bad input stream");
	}
	if (cur == CLEAR){
	    cleardict();
	    goto again;
	}
	if (cur == nexti)  	/* <- weird case here */
	    adddict( prev, first(prev) );
	else
	    adddict( prev, first(cur) );
	printsym(cur);
	prev = cur;
    }
    outflush();
    return 0;
}

void printusage( void )
{
    prints("uncompress [-cfv] [file...]\n");
    exit(1);
}

void pipeit( void )
{
    in = 0;
    out = 1;
    decompress();
    myexit();
}


int main( int argc, char *argv[] )
{
    int i,j;
    char *p;
    char c;
    int ret;
    struct stat s;

    /* no args, just use stdin/stdout */
    if (argc == 1)
	pipeit();
    for (i = 1; i < argc; i++){
	/* parse options */
	if (argv[i][0] == '-'){
	    if (strlen(argv[i]) == 1)
		pipeit();
	    for( j = 1; j < strlen(argv[i]); j++ ) {
		switch (argv[i][j]){
		case 'f':
		    fflg = 1;
		    break;
		case 'v':
		    vflg = 1;
		    break;
		case 'c':
		    cflg = 1;
		    break;
		default:
		    printusage();
		}
	    }
	    continue;
	}
	/* parse filenames */
	infile[0] = outfile[0] = 0;
	p = argv[i];
	p += strlen(p) - 2;
	/* formulate in and out filenames */
	if (p[0] == '.' && p[1] == 'Z' ){
	    strncat(infile, argv[i], NAMZ);
	    *p = 0;
	    strncat(outfile, argv[i], NAMZ);
	}
	else {
	    strncat(infile, argv[i], NAMZ);
	    strncat(infile, ".Z", NAMZ);
	    strncat(outfile, argv[i], NAMZ);
	}
	in = open(infile,O_RDONLY);
	if (in < 0){
	    perror("open");
	    exit(1);
	}
	ret = fstat(in, &s);
	if (ret<0)
	    myabort("Can not stat() input file");
	
	if (!fflg && !access(outfile,F_OK)){
	    if (isatty(0)){
		while(1){
		    prints(outfile);
		    prints(": File exists.  Overwrite? ");
		    ret = read(0,&c,1);
		    if (ret < 1)
			myabort("End of input");
		    if (c == 'y' || c == 'Y')
			break;
		    if (c == 'n' || c == 'N')
			myabort("User cancelled.");
		}
	    }
	    else{
		myabort("File already exists.");
	    }
	}

	if (cflg) {
	    out = 1;
	    strncat(outfile,"stdout", NAMZ);
	}
	else {
	    out = open(outfile,O_WRONLY|O_CREAT, 0600);
	    if (out < 0){
		perror("open");
		exit(1);
	    }
	}
	ret = fchown(out, s.st_uid, s.st_gid);
	if (ret<0)
	    myabort("Cannot chown output file.");
	ret = fchmod(out, s.st_mode);
	if (ret<0)
	    myabort("Cannot chmod output file.");

	if (vflg) {
	    prints(infile);
	    prints(" -> ");
	    prints(outfile);
	    prints("\n");
	}

	decompress();

	close(in);
	if (!cflg) 
	    close(out);
    }
    myexit();
}
