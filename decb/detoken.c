/*

detokenize a Color Computer BASIC source file

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>

#define TNO 53+25+20+23   /* number of primary tokens */
#define TNO2 20+14+6+5    /* number of secondary tokens */
#define MAXBUF 256        /* load buffer size */

char *tokens[TNO] = {
    "FOR",
    "GO",
    "REM",
    "'",
    "ELSE",
    "IF",
    "DATA",
    "PRINT",
    "ON",
    "INPUT",
    "END",
    "NEXT",
    "DIM",
    "READ",
    "RUN",
    "RESTOR",    /* s/b "RESTORE" ? */
    "RETURN",
    "STOP",
    "POKE",
    "CONT",   /* s/b "CONTINUE" ? */
    "LIST",
    "CLEAR",
    "NEW",
    "CLOAD",
    "CSAVE",
    "OPEN",
    "CLOSE",
    "LLIST",
    "SET",
    "RESET",
    "CLS",
    "MOTOR",
    "SOUND",
    "AUDIO",
    "EXEC",
    "SKIPF",
    "TAB(",
    "TO",
    "SUB",
    "THEN",
    "NOT",
    "STEP",
    "OFF",
    "+",
    "-",
    "*",
    "/",
    "^",
    "AND",
    "OR",
    ">",
    "=",
    "<",
    /* Extended Color BASIC */
    "DEL",
    "EDIT",
    "TRON",
    "TROF",
    "DEF",
    "LET",
    "LINE",
    "PCLS",
    "PSET",
    "PRESET",
    "SCREEN",
    "PCLEAR",
    "COLOR",
    "CIRCLE",
    "PAINT",
    "GET",
    "PUT",
    "DRAW",
    "PCOPY",
    "PMODE",
    "PLAY",
    "DLOAD",
    "RENUM",
    "FN",
    "USING",
    /* Disk Extended Color BASIC */
    "DIR",
    "DRIVE",
    "FIELD",
    "FILES",
    "KILL",
    "LOAD",
    "LSET",
    "MERGE",
    "RENAME",
    "RSET",
    "SAVE",
    "WRITE",
    "VERIFY",
    "UNLOAD",
    "DSKINI",
    "BACKUP",
    "COPY",
    "DSKI$",
    "DSKO$",
    "DOS",
    /* Super Extended Color BASIC */
    "WIDTH",
    "PALETTE",
    "HSCREEN",
    "LPOKE",
    "HCLS",
    "HCOLOR",
    "HPAINT",
    "HCIRCLE",
    "HLINE",
    "HGET",
    "HPUT",
    "HBUFF",
    "HPRINT",
    "ERR",
    "BRK",
    "LOCATE",
    "HSTAT",
    "HSET",
    "HRESET",
    "HDRAW",
    "CMP",
    "RGB",
    "ATTR",
};

char *second[TNO2] = {
    "SGN",
    "INT",
    "ABS",
    "USR",
    "RND",
    "SIN",
    "PEEK",
    "LEN",
    "STR$",
    "VAL",
    "ASC",
    "CHR$",
    "EOF",
    "JOYSTK",
    "LEFT$",
    "RIGHT$",
    "MID$",
    "POINT",
    "INKEY$",
    "MEM",
    /* Extended Color BASIC */
    "ATN",
    "COS",
    "TAN",
    "EXP",
    "FIX",
    "LOG",
    "POS",
    "SQR",
    "HEX$",
    "VARPTR",
    "INSTR",
    "TIMER",
    "PPOINT",
    "STRING$",
    /* Disk Extended Color BASIC */
    "CVN",
    "FREE",
    "LOC",
    "LOF",
    "MKN$",
    "AS",
    /* Super Extended Color BASIC */
    "LPEEK",
    "BUTTON",
    "HPOINT",
    "ERRNO",
    "ERLIN"
};

unsigned char buf[MAXBUF];
int len = 0;
int pos = 0;
int unixflg = 0;
int tflag = 0;

void myerror(int val) {
    if (val) {
	fprintf(stderr,"error %d: ", val);
	switch (val) {
	case 1: fprintf(stderr,"premature end of file\n"); break;
	case 2: fprintf(stderr,"unknown block type\n"); break;
	case 3: fprintf(stderr,"cannot rewind input file\n"); break;
	case 4: fprintf(stderr,"cannot open output file\n"); break;
	case 5: fprintf(stderr,"cannot open input file\n"); break;
	default: fprintf(stderr,"unknown error\n"); break;
	}
    }
    exit(val);
}

int mygetc(void) {
    if (pos == len) {
	pos = 0;
	len = read(0, buf, MAXBUF);
	if (len == 0) return -1;
    }
    return buf[pos++];
}

uint8_t mygetc_err(void) {
    int c;
    c = mygetc();
    if (c < 0) myerror(1);
    return c;
}

uint16_t mygetw(void) {
    uint16_t a;
    a = mygetc_err() << 8;
    return a + mygetc_err();
}


int reset(void) {
    int ret;
    pos = 0;
    len = 0;
    ret = lseek(0, 0, SEEK_SET);
    if (ret) {
	perror("lseek");
	return 1;
    }
    return 0;
}


void copy(void) {
    int c;

    if (reset()) myerror(3);
    while( (c = mygetc()) >= 0 ) {
	if (unixflg && c == '\r')
	    c = '\n';
	printf("%c", c);
    }
    exit(0);
}


int myprintf(int flag, const char *format, ...) {
    va_list arg;

    va_start(arg, format);
    if (flag)
	return vprintf(format, arg);
    va_end(arg);
}

/* try to convert the file, and possibly print it */
int foo(int pflag) {
    int flag = 0;
    int c;
    uint16_t ln;
    uint16_t len;

    if (reset()) return 1;
    /* check header */
    c = mygetc_err();
    if (c != 0xff)
	copy();
    len = mygetw();
    if (len > (0x8000 - 200 - 256 - 0x2600))
	fprintf(stderr,"suspicious length: %d\n", len);

    /* interate through each line */
    while(1) {
	/* drop next line link */
	if (mygetw() == 0) break;
	/* get line no. */
	ln = mygetw();
	myprintf(pflag, "%d ", ln);
	while(1) {
	    /* get next byte */
	    c = mygetc_err();
	    if (flag) {
		c &= 0x7f;
		myprintf(pflag, "%s", second[c]);
		flag = 0;
		continue;
	    }
	    if (c == 0xff) {
		flag = 1;
		continue;
	    }
	    if (c & 0x80) {
		c &= 0x7f;
		myprintf(pflag, "%s", tokens[c]);
		continue;
	    }
	    if (c == 0) {
		myprintf(pflag, "%c", unixflg ? '\n' : '\r');
		break;
	    }
	    myprintf(pflag, "%c", c);
	}
    }
    return 0;
}

void usage(void) {
    fprintf(stderr,"usage: detoken -hlo file ...\n");
    fprintf(stderr,"-h : print help\n");
    fprintf(stderr,"-l : convert line ending to unix\n");
    fprintf(stderr,"-o : set output file\n");
    fprintf(stderr,"-t : exit 0 if binary, don't output\n");
    exit(1);
}

int main(int argc, char **argv) {
    uint16_t len;
    char *arg;
    int c;
    int i;
    int ret;

    /* check args */
    for (i = 1; i < argc; i++) {
	arg = argv[i];
	if (*arg == '-') {
	    for (arg++; *arg; arg++) {
		switch (*arg) {
		case 'l' :
		    unixflg = 1;
		    break;
		case 'h' :
		    usage();
		    break;
		case 'o' :
		    arg++;
		    if (freopen(*arg ? arg : argv[++i], "w+", stdout) == NULL)
			myerror(4);
		    goto next;
		case 't':
		    tflag = 1;
		    break;
		default:
		    fprintf(stderr,"unknown option: %c\n", *arg);
		    usage();
		}
	    }
	}
	else {
	    close(0);
	    if (open(arg, O_RDONLY) < 0) {
		perror(arg);
		myerror(5);
	    };
	    foo(0);
	    foo(1);
	}
    next:
	continue;
    }
    exit(0);
}
