/*
   A Simple IRC client for Fuzix

   TODO
    * add va to oprint.
    * select()
    * long input line handling
    * detect vt res
    * nocolor cmd line switch
    * word wrap
    * fix local messages

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "netdb.h"
#include <termios.h>

/* I shouldn't have to do this? */
#define AF_INET 1
#define SOCK_STREAM 3


struct sockaddr_in addr;
struct sockaddr_in laddr;

#define IMAX 512
char ibuf[IMAX];
char obuf[IMAX];
char *iptr = ibuf;
char *scrl = ibuf;
char nbuf[IMAX];
char pbuf[IMAX];
char *mes;
char *prefix;
char *com;
char *param[6];
int param_no;
int flags;
struct termios prev_term;
struct termios new_term;
int sfd=-1;
char channel[256];
char nick[256];
char dirty=1;
int cols=80;


static void my_exit( int val )
{
	fcntl(0, F_SETFL, flags );
	tcsetattr( 0, TCSANOW, &prev_term );
	exit(val);
}


static void alarm_handler( int signum )
{
	return;
}

static void int_handler( int signum )
{
	my_exit( 1 );
}

static void mvto( char y, char x )
{
	printf( "\033Y%c%c", y+' ', x+' ' );
}

static void set_fcolor( int color )
{
	printf( "\033b%c", 0x30 + color );
}

static char hash_nick( char *s )
{
	char c;
	while ( *s )
		c ^= *s++;
	c &= 7;
	if (c == 4 || !c )
		c++;
	return c;
}

/* setup input line */
static iprint( void )
{
	mvto( 24, 0 );
	*iptr = 0;
	printf( scrl );
	fflush( stdout );
}

static oprint( void )
{
	int len = strlen( obuf );
	mvto( 24, 0 );
	printf( "\033K");
	printf( obuf );
	putchar( '\n');
	iprint();
}

char *skip( char *p )
{
	for ( ; *p != 32 ; p++ )
		if( ! *p )
			return p;
	*p++ = 0;
	for ( ; *p == 32 && *p; p++ );
	return p;
}

/* processes the protocol buffer */
void proc_proto( ){
	char *p = pbuf;
	param_no = 0;
	prefix = 0;
	com = 0;
	mes = 0;

	if (*p == ':'){
		p++;
		prefix = p;
		p = skip(p);
	}
	com = p;
	p = skip(p);
//	if ( !strcmp( com, "004" )) return;
	if ( !strcmp( com, "005" )) return;
	while (1){
		if (*p == ':'){
			p++;
			mes = p;
			param[param_no++] = p;
			break;
		}
		if (*p == '\0')
			break;
		param[param_no++] = p;
		mes = p;
		p = skip(p);
	}
	if ( !strcmp( com, "PING")){
		write( sfd, "PONG\r\n", 6 );
		return;
	}
	if ( !strcmp( com, "JOIN")){
		strcpy( channel, param[0] );
		sprintf( obuf, "%8s joined channel %s", prefix, channel);
		oprint();
		return;
	}
	if ( !strcmp( com, "NICK")){
		sprintf( obuf, "%8s is now known as %s", prefix, param[0] );
		oprint();
		return;
	}
	if ( !strcmp( com, "PART")){
		sprintf( obuf, "%8s left channel", prefix );
		oprint();
		return;
	}
	if ( com[0] >= 0x30 && com[0] <= 0x39 ){
		sprintf( obuf, "%s %s", com, mes );
		oprint();
		return;
	}
	if ( !strcmp( com, "PRIVMSG" )){
		set_fcolor( hash_nick( prefix ) );
		sprintf( obuf, "%.8s \033b4%s", prefix, mes );
		oprint();
		return;
	}
	sprintf( obuf, "%s", mes );
	oprint();
}

/* fill protocol buffer from network buff */
void fill_proto( char c )
{
	static char *p = pbuf;
	if( c == '\n' ){
		*(p-1) = '\0';
		proc_proto();
		p = pbuf;
		return;
	}
	*p++ = c;
}

/* gets bytes from network */
int do_network( void )
{
	int i;
	char *p=nbuf;
	int ret = read( sfd, nbuf, IMAX );
	if ( ret < 1 ){
		return 0;
	}
	for( i=0; i<ret; i++){
		fill_proto( *p++ );
	}
	return -1;
}





void my_open( int argc, char *argv[])
{
	int port = 6667;    /* default port */
	struct hostent *h;

	if( argc == 3 )
		port = atol(argv[2] );

	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;

	h=gethostbyname( argv[1] );
	if( ! h ){
		write(2, "cannot resolve hostname\n", 24 );
		my_exit(1);
	}
	memcpy( &addr.sin_addr.s_addr, h->h_addr_list[0], 4 );
	int i;

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
		perror("af_inet sock_stream 0");
		my_exit(1);
	}

	signal( SIGALRM, alarm_handler );
	alarm(2);
	if (connect(sfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("connect");
		my_exit(1);
	}

	puts( "connected" );
	fcntl( sfd, F_SETFL, O_NDELAY );
}


/* my version of getc */
int mygetc( void )
{
	char c;
	int ret;
	ret = read( 0, &c, 1);
	if ( ret > 0 )
		return c;
	return ret;
}

/* Process input buffer */
void messproc( void ){
	char *p = ibuf;
	if ( *p == '/' ){
		if ( ! strcmp(p + 1, "exit") )
			my_exit(0);
		write( sfd, ibuf+1, strlen(ibuf)-1 );
	}
	else {
		write( sfd, "PRIVMSG ", 8);
		write( sfd, channel, strlen(channel) );
		write( sfd, " :", 2 );
		write( sfd, ibuf, strlen(ibuf) );
		sprintf( obuf, "me> %s", ibuf );
		oprint();
	}
	write( sfd, "\r\n", 2 );
}

/* Process newly received stdin keys */
int inproc( char c )
{
	int y,x;

	if ( iptr == ibuf + IMAX - 1 )
		return 0;
	if ( c == '\b' ){
		if ( iptr == ibuf )
			return 0;
		iptr -= 1;
		if( ! x ){
		    //	mvwprintw(iwin, --y, COLS-1, " ");
		    //	wmove(iwin, y, COLS-1);
			return 0;
		}
		else{
		    	printf("\b \b");
			fflush( stdout );
			return 0;
		}
	}
	if ( c == '\n' ){
		*iptr = '\0';
		iptr = ibuf;
		scrl = ibuf;
		return -1;
	}
	*iptr++ = c;
	if ( iptr - ibuf > 78 ){
		scrl++;
		iprint();
	}
	else{
		putchar( c );
		fflush( stdout );
	}
	return 0;
}


int main( int argc, char **argv )
{
	int c;
	int n;
	int res;

	flags = fcntl(0, F_GETFL, 0);

	if( tcgetattr( 0, &prev_term ) ){
		perror("getting termios");
		exit(1);
	}
	signal( SIGTERM, int_handler );
	tcgetattr( 0, &new_term );
	cfmakeraw( &new_term );
	if( tcsetattr( 0, TCSANOW,  &new_term ) ){
		perror("setting termios");
		exit(1);
	}
	fcntl(0, F_SETFL, flags | O_NDELAY);


	printf( "\033H\033J");
	mvto( 23, 0 );

	/*Set up tcp connection*/
	my_open( argc, argv );
	sprintf( obuf, "Opened connection." );
	oprint();
	do {
		c = mygetc();
		if( c > 0 ){
			if ( inproc( c ) )
				messproc();
		}
		n = do_network();
		if( !n && c == -1 )
			_pause( 1 );
	} while ( c != 27 );
	my_exit(0);
}
