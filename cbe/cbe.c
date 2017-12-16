/*  
    A Color Basic Loader
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

#define PREFIX "/usr/share/roms"

void doBASIC( void );
void setStack( void );
void myexit( int );
extern int dfd;
extern struct termios prior;

struct termios new;


void printe( char *mess )
{
    if( mess ){
	write( 2, mess, strlen(mess) );
	write( 2, "\n", 1 );
    }
}

void exit_err( char *mess )
{
    printe( mess );
    exit( 1 );    
}


void doload( )
{
    int fd;
    char *p;
        /* FIXME Do basic sanity check on ROM image */
    /* Load CB ROM Image */
    fd = open( PREFIX"/basic.rom", O_RDONLY );
    if ( fd < 0 )
	exit_err("cannot open CB ROM image");
    for ( p = (char *)0xa000; p < (char *)0xc000; p += 512 )
	if ( read( fd, p, 512 ) < 1 )
	    exit_err("cannot load CB ROM image");
    close( fd );
    /* FIXME Do CRC check on CB */
    /* Load ECB ROM Image */
    fd = open( PREFIX"/ecb.rom", O_RDONLY );
    if ( fd < 0 )
	goto cont;
    for ( p = (char *)0x8000; p < (char *)0xa000; p += 512 )
	if ( read( fd, p, 512 ) < 1 )
	    exit_err("cannot load ECB ROM image");
    close( fd );
    /* FIXME Do CRC check on ECB */
    /* Load ECB ROM Image */
    fd = open( PREFIX"/disk.rom", O_RDONLY );
    if ( fd < 0 )
	goto cont;
    for ( p = (char *)0xc000; p < (char *)0xd900; p += 512 )
	if ( read( fd, p, 512 ) < 1 ){
	    fprintf(stderr,"base: %x\n", p );
	    exit_err("cannot load DECB ROM image");
	}
    close( fd );
    /* FIXME Do CRC check on ECB */
    /* fix up terminal */
 cont:
    tcgetattr( 0, &prior );
    tcgetattr( 0, &new );
    cfmakeraw( &new );
    new.c_iflag |= INLCR;
    new.c_oflag |= OPOST|ONLCR;
    tcsetattr( 0, TCSANOW, &new );
    /* install sig handler */
    signal( SIGTERM, myexit );
    /* Do what basic does to RAM */
    memset( (char *)0x0001, 0, 1023);
    /* hand off the asm */
    doBASIC();
}


int main( int argc, char **argv )
{
    /* 
       after we reset our stack
       we'll no longer have access to main's
       arguments or autovars
    */
    if (argc > 1){
	dfd = open(argv[1], O_RDWR );
    }
    setStack();
    doload();
}
