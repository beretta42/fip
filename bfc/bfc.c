/*

This is a forth compiler.  It spits out asm source.  This compiler is
*not* forth, as it's input source cannot make immediate words.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define MAXDICT 256
#define MAXNAME 32

struct entry{
	char name[MAXNAME];
	unsigned char flags;
#define F_IMM 0x80
	unsigned int ref;
};


FILE *infile;
FILE *outfile;
char ibuff[256];   /* input line buffer */
char *ofile="f.out";
unsigned int labeli=0;   /* next label number */
struct entry dict[MAXDICT];
int dp=0;                /* next dict array to allocat */
int inline_dict=0;       /* spit out dictionary as we go? */
int mk_dict=1;           /* make dictionary after end of compiling? */
struct entry *lit;       /* how to do a literal */
struct entry *zbra;      /* 0branch ptr */
struct entry *bra;       /* branch ptr */
struct entry *slit;      /* string literal */
struct entry *dodoes;    /* does> doer */
struct entry *noop;      /* noop for filling in var's does */
struct entry *dofor;     /* for doing for */
struct entry *push;      /* for doing for */
unsigned int cstack[16]; /* control stack depth: 16 */
int cp=0;
unsigned int nstack[16]; /* number stack depth: 16 */
int np=0;
state=0;

void push_cntl( unsigned int i )
{
	cstack[cp++]=i;
}

unsigned int pull_cntl( void )
{
	return cstack[--cp];
}

void push_number( int i )
{
	nstack[np++]=i;
}

int pull_number( void )
{
	return nstack[--np];
}


void printe( char *s ){
	fprintf(stderr, s );
}


void printusage( void )
{
	printe( "usage: bfc srcfile\n" );
}

void quite( char *s )
{
	printe( "Error: " );
	printe( s );
	printe( "\n" );
	exit(1);
}

void quitn( void ) 
{
	fclose( outfile );
	fclose( infile );
	exit(0);
}


static char wordb[64];
int eof=0;

/* is a forth blank? */
int isfblank( char c )
{
	if( c <= ' ' ) return -1;
	return 0;
}

void comp_name( struct entry *e )
{
	int l=strlen(e->name);
	fprintf( outfile,"\t\tdw %d\n", l );
	fprintf( outfile,"\t\t.ascii \"%s\"\n", e->name );
}

/* compile a literal cell */
void comp_int( int i )
{
	fprintf( outfile, "\t\tdw $%x\t; compile: #\n",
		 i );
}

void comp_char( char c )
{
	fprintf( outfile, "\t\tdb $%x\t; compile: #\n",
		 c );
}
   

void doinline_dict( struct entry *e )
{
	comp_name( e );
	/* should do more here !!! */
}

void do_dict( )
{
	int i=0;
	int last=0;
	int me=0;
	for( i=0; i<dp; i++){
		struct entry *e=&(dict[i]);
		fprintf( outfile,"LAB%x:\t\t; do_dict: %s\n", labeli, e->name );
		if( i ) fprintf( outfile,"\t\tdw LAB%x\n", last);
		else comp_int( 0 );
		fprintf( outfile,"\t\tdw L%x\n", e->ref);
		fprintf( outfile,"\t\tdb $%x\n", e->flags );
		comp_name( e );
		last = labeli++;
	}
	fprintf( outfile, "dp0 equ LAB%x\n", last );
}

/* compile the action of entry */
void comp_entry( struct entry *e )
{
	fprintf( outfile, "\t\tjsr L%x\t; comp: %s\n", e->ref, e->name ); 
}


/* make an internal dictionary entry */
void mkentry( void )
{
	struct entry *e=&(dict[dp++]);
	if( !strcmp( wordb, "lit" ) ) lit=e;
	if( !strcmp( wordb, "0bra" ) ) zbra=e;
	if( !strcmp( wordb, "bra" ) ) bra=e;
	if( !strcmp( wordb, "slit" ) ) slit=e;
	if( !strcmp( wordb, "dodoes" )) dodoes=e;
	if( !strcmp( wordb, "noop" )) noop=e;
	if( !strcmp( wordb, "dofor" )) dofor=e;
	if( !strcmp( wordb, "push" )) push=e;
	strncpy( e->name, wordb, 32 );
	e->ref=labeli++;
	e->flags=0;
	if( inline_dict ) doinline_dict( e );
	fprintf( outfile, "L%x\t\t; mkentry: %s\n", e->ref, e->name );
}

/* find an entry */
struct entry *findentry( )
{
	struct entry *e;
	int i=labeli;
	for( ; i; i--){
		e=&(dict[i-1]);
		if( !strcmp( e->name, wordb ) ) return e;
	}
	return NULL;
}

/* fills word buffer from input */
int word( void )
{
	int c;
	char *p=wordb;
	/* remove leading white-space */
	while( 1 ){
		c=fgetc( infile );
		if( c == EOF ) return -1;
		if( ! isfblank( c ) ) break;
	}
	while( c!=EOF ){
		if( isfblank(c) ) break;
		*p++=c;
		c=fgetc( infile );
	}
	*p=0;
	return 0;
}

void doasm( void )
{
	char buff[256];
	while( fgets( buff, 256, infile ) ){
		if( !strncmp( buff, ";p", 2 ) )
			return;
		fputs( buff, outfile );
	}
}

void doprim( void )
{
        word();
	mkentry();
	doasm( );
}

void docolon( void )
{
	word();
	mkentry();
	state=1;
}

void dosemi( void )
{
	fprintf( outfile, "\t\trts\t; exit\n" );
	state=0;
}

void doif( void )
{
	comp_entry( zbra );
	fprintf( outfile, "\t\tdw L%x\n", labeli );
	push_cntl( labeli++ );
}

void dodofor( void )
{
	unsigned int i=labeli++;
	comp_entry( push );
	fprintf( outfile, "L%x\n", i );
	comp_entry( dofor );
	fprintf( outfile, "\t\tdw L%x\n", labeli );
	push_cntl( labeli++ );
	push_cntl( i );
}

void donext( void )
{
	doagain();
	dothen();
}

void dothen( void )
{
	fprintf( outfile, "L%x\n", pull_cntl() );
}

void dobegin( void )
{
	fprintf( outfile, "L%x\n", labeli );
	push_cntl( labeli++ );
}

void doagain( void )
{
	comp_entry( bra );
	fprintf( outfile, "\t\tdw L%x\n", pull_cntl() );
}

void dountil( void )
{
	comp_entry( zbra );
	fprintf( outfile, "\t\tdw L%x\n", pull_cntl() );
}

void dowhile( void )
{
	unsigned int i=pull_cntl();
	comp_entry( zbra );
	fprintf( outfile, "\t\tdw L%x\n", labeli );
	push_cntl( labeli++ );
	push_cntl( i );
}

void dorepeat( void )
{
	doagain();
	fprintf( outfile, "L%x\n", pull_cntl() );
}

void doelse( void )
{
	unsigned int i=pull_cntl();
	comp_entry( bra );
	fprintf( outfile, "\t\tdw L%x\n", labeli );
	push_cntl( labeli++ );
	fprintf( outfile, "L%x\n", i );
}


/* build a zero termed string in wordb */
int wordb_str( void )
{
	int c;
	int d;
	char *p=wordb;
	/* remove leading white-space */
	while( 1 ){
		c=fgetc( infile );
		if( c == EOF ) quite( "EOF before string delimiter" );
		if( ! isfblank( c ) ) break;
	}
	d = c;
	c=fgetc( infile );
	if( c == EOF ) quite( "EOF during string" );
	while( 1 ){
		if( c== EOF ) quite( "EOF during string" );
		if( c == d  ) break;
		*p++=c;
		c=fgetc( infile );
	}
	*p=0;
	return d;
}

void dostring( void ){
	int d=wordb_str();
	comp_entry( slit );
	fprintf( outfile, "\t\tdw %d\n", strlen(wordb) );
	fprintf( outfile, "\t\t.ascii %c%s%c\n", d, wordb, d );
}

void docstring( void ){
	int d=wordb_str();
	comp_entry( slit );
	fprintf( outfile, "\t\tdw %d\n", strlen(wordb)+1 );
	fprintf( outfile, "\t\t.asciz %c%s%c\n", d, wordb, d );
}

void dodovar( void ){
	word();
	mkentry();
	comp_entry( dodoes );
	fprintf( outfile, "\t\tdw L%x\n", noop->ref );
	fprintf( outfile, "\t\tdw %d\n", pull_number() );
}

void dorbrack( void ){
	state = 1;
}

void doallot( void ){
	int i=pull_number();
	while( i-- ){
		comp_char( 0 );
	}		
}

void doimm( void ){
	struct entry *e=&(dict[dp-1] );
	e->flags |= F_IMM;
}

void dolcom( void ){
	char c;
	do{
		c=fgetc( infile );
	} while( c != '\n' && c != -1 );      
}

void process_file( void )
{
	while( !word() ){
		/* host immediate words */
		if( !strcmp( wordb, "p:" ) ){ doprim(); continue; }
		if( !strcmp( wordb, "a:" ) ){ doasm(); continue; }
		if( !strcmp( wordb, ":" ) ) { docolon(); continue; }
		if( !strcmp( wordb, ";" ) ) { dosemi(); continue; }
		if( !strcmp( wordb, "if" ) ){ doif(); continue; }
		if( !strcmp( wordb, "then")){ dothen(); continue; }
		if( !strcmp( wordb, "begin")){ dobegin(); continue; }
		if( !strcmp( wordb, "again")){ doagain(); continue; }
		if( !strcmp( wordb, "until")){ dountil(); continue; }
		if( !strcmp( wordb, "while")){ dowhile(); continue; }
		if( !strcmp( wordb, "repeat")){dorepeat(); continue; }
		if( !strcmp( wordb, "else")){ doelse(); continue; }
		if( !strcmp( wordb, "$")){ dostring(); continue; }
		if( !strcmp( wordb, "c$")){ docstring(); continue; }
		if( !strcmp( wordb, "var")){ dodovar(); continue; }
		if( !strcmp( wordb, "]" )){ dorbrack(); continue; }
		if( !strcmp( wordb, "allot")){ doallot(); continue; }
		if( !strcmp( wordb, "for")){ dodofor(); continue; }
		if( !strcmp( wordb, "next")){ donext(); continue; }
		if( !strcmp( wordb, "imm")){ doimm(); continue; }
		if( !strcmp( wordb, "#")){ dolcom(); continue; }

		/* compile it */
		struct entry *e=findentry();
		if( e ){ 
			comp_entry( e );
			continue;
		}
		/* compile a literal */
		char *end;
	        int x=strtol( wordb, &end, 0 );
		if( !*end ){
			if( state ){
				comp_entry( lit );
				comp_int( x );
			}
			else push_number( x );
			continue;
		}
		/* quit unfound word */
		printe( wordb );
		printe( "\n");
		quite( "word not found.\n");
	}
	fclose( infile );
}

uint main( int argc, char *argv[] )
{
	if( argc<2 ){
		printusage();
		quitn();
	}
	
	infile=fopen( argv[1], "r" );
	if( !infile ) quite( "Cannot open input file." );
	
	outfile=fopen( ofile, "w" );
	if( !outfile ) quite( "Cannot open output file." );


	process_file( );
	if( mk_dict ) do_dict();
	fprintf( outfile, "cp0 equ *\n" );
	quitn();
}
