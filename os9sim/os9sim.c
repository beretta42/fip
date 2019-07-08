/* 
   A os9 simulator for FUZIX

TODO:
   separate C and os9 stack
   work on blasting away discardable code
   translate fuzix error returns to os9 error codes
   reuse f$chain stuff for init ?
   cat and pass all C args to os9
*/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <cpu_ioctl.h>

#define DEBUG

void swi2_handler(void);
uint8_t swi2(uint8_t func);
void os9go(uint16_t stackptr);
uint16_t mybrk;
uint16_t mytop;

/* this struct is for passing os9 register
   snapshot to C 
*/
struct regs_s {
    uint8_t  cc;
    uint8_t  a;
    uint8_t  b;
    uint8_t  dp;
    uint16_t x;
    uint16_t y;
    uint16_t u;
    uint16_t pc;
};

extern struct regs_s *reg;
uint16_t datasize;
int8_t path[256];
int term;

/* os9 module header struct (executable modules) 
   fixme: struct field packing may be incorrect!
*/
struct header_s {
    uint16_t sync;
    uint16_t size;
    uint16_t name;
    uint8_t type;
    uint8_t attr;
    uint8_t check;
    uint16_t exec;
    uint16_t dsize;
};

struct pathoptions_s {
    uint8_t  type;
    uint8_t  tcase;
    uint8_t  back;
    uint8_t  delete;
    uint8_t  echo;    
    uint8_t  autolf;
    uint8_t  eof;
    uint8_t  pause;
    uint8_t  lpp;
    uint8_t  bschar;
    uint8_t  delchar;
    uint8_t  eorchar;
    uint8_t  eofchar;
    uint8_t  rprchar;
    uint8_t  dupchar;
    uint8_t  pauchar;
    uint8_t  intchar;
    uint8_t  quichar;
    uint8_t  bsechar;
    uint8_t  belchar;
    uint8_t  hwcntl;
    uint8_t  rate;
    uint16_t name;
    uint8_t  xonchar;
    uint8_t  xofchar;
    uint8_t  szx;
    uint8_t  szy;
} po;


static uint16_t mysbrk(int16_t inc) {
    uint16_t old = mybrk;
    if (mybrk + inc > mytop)
	return -1;
    mybrk += inc;
    return old;
}

static dump(char *p, int c) {
    int x;
    for (x = 0; x < c; x++) {
	fprintf(stderr,"%x ", p[x]);
    }
    fprintf(stderr,"\n");
}

static parsepath(void) {
    int8_t *p = (char *)reg->x;
    int8_t *d = path;
    /* skip leading spaces */
    while (*p == ' ') p++;
    /* skip past pathname */
    while (*p != ' ' && *p > 0 )
	*d++ = *p++;
    if (*p < 0)
	*d++ = (*p++ & 0x7f);
    *d = 0;
    /* skip past trailing spaces */
    while (*p == ' ') p++;
    reg->x = (uint16_t)p;
}

static mode_t getmode(uint16_t os9mod) {
    mode_t mod;
    if (os9mod & 01) mod |= 0400;
    if (os9mod & 02) mod |= 0200;
    if (os9mod & 04) mod |= 0100;
    if (os9mod & 010) mod |= 0044;
    if (os9mod & 020) mod |= 0022;
    if (os9mod & 040) mod |= 0011;
    return mod;
}

void os9icpt(void) {
    return;
}

void os9create(void) {
    int ret;
    int mod = getmode(reg->b);
    int flags;
    parsepath();
    flags = (reg->a & 3) + 1;
    flags |= O_CREAT;
    ret = open(path, flags, mod);
    if (ret < 0) {
	reg->cc |= 1;
	reg->b = errno;
	return;
    }
    reg->a = ret;
}
 
void os9open(void) {
    int ret;
    int mod;
    parsepath();
#ifdef DEBUG
    fprintf(stderr,"I$Open: \"%s\"\n", path);
#endif
    if (!strcmp(path, "/TERM") ) {
	term = reg->a = dup(0);
	return;
    }
    mod = (reg->a & 3) - 1;
    ret = open(path, mod);
    if (ret < 0) {
#ifdef DEBUG
	perror("open");
#endif
	reg->cc |= 1;
	reg->b = -1;
	return;
    }
    reg->a = ret;
}

void os9getstat(void) {
#ifdef DEBUG
    fprintf(stderr, "I$GetStt: %x %x\n", reg->a, reg->b);
#endif
    if (reg->b == 0) {
	memcpy((char *)reg->x, &po, 32);
	return;
    }
    reg->cc |= 1;
    reg->b = -1;
}

void os9setstat(void) {
#ifdef DEBUG
    fprintf(stderr, "I$SetStt: %x %x\n", reg->a, reg->b);
#endif
    if (reg->b == 0) {
	dump((char *)reg->x, 32);
    }
    reg->cc |= 1;
    reg->b = -1;
}

void os9read(void) {
    int ret;
#ifdef DEBUG  
    fprintf(stderr, "I$Read: %x %x %x\n", reg->a, reg->x, reg->y);
#endif
    ret = read(reg->a, (char *)reg->x, reg->y);
    if (ret < 0) {
#ifdef DEBUG
	perror("read");
#endif
	reg->cc |= 1;
	reg->b = -1;
	return;
    }
    reg->y = ret;
}

void os9write(void) {
    int ret;
#ifdef DEBUG
    fprintf(stderr, "I$Write: %x %x %x\n", reg->a, reg->x, reg->y);
    dump((char *)reg->x, reg->y);
#endif
    ret = write(reg->a, (char *)reg->x, reg->y);
    if (ret < 0) {
	reg->cc |= 1;
	reg->b = -1;
	return;
    }
    reg->y = ret;
}

void os9close(void) {
    int ret;
#ifdef DEBUG   
    fprintf(stderr, "I$Close: %x\n", reg->a);
#endif    
    ret = close(reg->a);
    if (ret < 0) {
	reg->cc |= 1;
	reg->b = errno;
	return;
    }
    reg->b = ret;
}

void os9exit(void) {
#ifdef DEBUG    
    fprintf(stderr, "F$Exit: %x\n", reg->b);
#endif    
    exit(reg->b);
}

void os9seek(void) {
    int ret;
    uint32_t pos = ((uint32_t)reg->x << 16) + reg->u;
#ifdef DEBUG
    fprintf(stderr, "I$Seek: %d\n", reg->u);
#endif
    ret = lseek(reg->a, pos, SEEK_SET);
    if (ret < 0) {
	reg->cc |= 1;
	reg->b = ret;
	return;
    }
}


void os9mkdir(void) {
    int ret;
    mode_t mod = getmode(reg->b);
    parsepath();
    /* fixme: convert os9 mode to unix mode here */
    ret = mkdir(path, mod);
    if (ret < 0) {
	reg->cc |= 1;
	reg->b = ret;
	return;
    }
}

void os9dup(void) {
    int ret;
    ret = dup(reg->a);
    if (ret < 0) {
	reg->cc |= 1;
	reg->b = errno;
	return;
    }
}

void os9delete(void) {
    int ret;
    parsepath();
    ret = unlink(path);
    if (ret < 0){
	reg->cc |= 1;
	reg->b = errno;
	return;
    }
}

void os9chdir(void) {
    int ret;
    parsepath();
    /* fixme: execution path? */
    ret = chdir(path);
    if (ret < 0) {
	reg->cc |= 1;
	reg->b = errno;
	return;
    }
}

void os9perr(void) {
    fprintf(stderr, "ERROR #%d\n", reg->b);
}

void os9mem(void) {
    int ret;
    uint16_t new = *(uint16_t *)&reg->a;
    new = (new + 0xff) & 0xff00;
    uint16_t diff = new - datasize;
#ifdef DEBUG
    fprintf(stderr, "F$Mem: %x %x\n", new, diff);
#endif
    ret = (int)mysbrk(diff);
    if (ret < 0) {
#ifdef DEBUG
	fprintf(stderr,"bad sbrk\n");
#endif
	reg->cc |= 1;
	reg->b = errno;
	return;
    }
    datasize = new;
    reg->y = mybrk;
    *(uint16_t *)&reg->a = new;
    reg->b = 0;
#ifdef DEBUG
    fprintf(stderr,"%x %x\n", datasize, reg->y);
#endif
}
    
void os9call(void) {
    uint8_t op = *(uint8_t *)reg->pc;
    reg->pc += 1;
    reg->cc &= ~0x1;
    /* fixme: prob need a table or something */
    switch(op) {
    case 0x09:
	os9icpt();
	break;
    case 0x82:
	os9dup();
	break;
    case 0x83:
	os9create();
	break;
    case 0x84:
	os9open();
	break;
    case 0x85:
	os9mkdir();
	break;
    case 0x86:
	os9chdir();
	break;
    case 0x87:
	os9delete();
	break;
    case 0x088:
	os9seek();
	break;
    case 0x89:
	os9read();
	break;
    case 0x8a:
	os9write();
	break;
    case 0x8d:
	os9getstat();
	break;
    case 0x8e:
	os9setstat();
	break;
    case 0x8f:
	os9close();
	break;
    case 0x90:
	os9delete();
	break;
    case 0x06:
	os9exit();
	break;
    case 0x07:
	os9mem();
	break;
    case 0x0f:
	os9perr();
	break;
    default:
	fprintf(stderr,"unhandled syscall 0x%x\n", op);
	exit(1);
    }
}

void main(int argc, char **argv) {
    int fd;
    int ret;
    uint16_t todo;
    uint8_t *b;
    struct regs_s *s;
    struct header_s *h;
    
    /* all the pointers os9 needs */
    uint16_t ibot;    /* bottom of code area */
    uint16_t itop;    /* top of code area */
    uint16_t dtop;    /* top of data area */
    uint16_t sbot;    /* bottom of stack area */
    uint16_t pbot;    /* bottom of parameter area */
    uint16_t ipc;     /* initial PC of program */
    uint16_t psize;   /* size of parameter area */
    uint16_t size;    /* total size of I + D */
    
    static struct _uzisysinfoblk i;

    /* brk() all we can from fuzix, and dyi brk 
       once we brk() everything we can get, then we can move
       our SP around
     */
    mybrk = (uint16_t) sbrk(0);
    ret = brk(&s - 512);
    if (ret < 0) {
	perror("brk");
	exit(1);
    }
    mytop = (uint16_t) sbrk(0);
#ifdef DEBUG
    fprintf(stderr, "mybrk %x, mytop %x\n", mybrk, mytop);
#endif  
    /* verify we're on a 6809 */
    ret = _uname(&i, sizeof(i));
    if (ret < 0) {
	perror("uname");
	exit(1);
    }

    if (i.cputype != 1) {
	fprintf(stderr,"incorect cpu, need 6809\n");
	exit(1);
    }

    /* install swi2 vector */
    fd = open("/dev/sys",O_RDONLY);
    if (fd < 0) {
	perror("/dev/sys");
	exit(1);
    }
    ret = ioctl(fd, CPUIOC_6809SWI2, swi2_handler);
    if (ret < 0) {
	perror("setting swi2");
	exit(1);
    }
    close(fd);

    /* init pathoptions */
    bzero(&po, 32);
    po.echo = 1;
    po.lpp = 24;
    po.bschar = 0x08;
    po.delchar = 0x7f;
    po.eorchar = 0x0a;
    po.eofchar = 0x04;
    po.szx = 80;
    po.szy = 24;
    
    if (argc < 2) {
	fprintf(stderr,"os9sim module [params]...\n");
	exit(1);
    }
    /* load look at header */
    fd = open(argv[1],O_RDONLY);
    if (fd < 0) {
	perror(argv[1]);
	exit(1);
    }
    ret = read(fd, (char *)mybrk, sizeof(struct header_s));
    if (ret == -1) {
	perror("read");
	exit(1);
    }
    h = (struct header_s *)mybrk;
    if (h->sync != 0x87cd) {
	fprintf(stderr,"bad header sync bytes\n");
	exit(1);
    }
    
    /* fixme: filter out other bad module stuff here (wrong type) */
    
    /* calculate areas and alloc total sbrk needed */
    ibot = mybrk;
    itop = ibot + h->size;
    itop = (itop + 0xff) & ~0xff;
    dtop = itop + h->dsize;
    dtop = (dtop + 0xff) & ~0xff;
    psize = strlen(argv[2]);
    pbot = dtop - psize;
    sbot = pbot - 12;    /* 12 bytes of registers push onto stack */
    size = dtop - ibot;
    ipc = ibot + h->exec;
    datasize = dtop - itop;

#ifdef DEBUG    
    printf("h  size: %x\n", h->size);
    printf("h  exec: %x\n", h->exec);
    printf("h dsize: %x\n", h->dsize);
    printf("ibot: %x\n", ibot);
    printf("itop: %x\n", itop);
    printf("dtop: %x\n", dtop);
    printf("psize: %x\n", psize);
    printf("pbot: %x\n", pbot);
    printf("sbot: %x\n", sbot);
    printf("size: %x\n", size);
    printf("ipc: %x\n", ipc);
#endif
    
    ret = mysbrk(size);
    if (ret == -1) {
	perror("mysbrk");
	exit(1);
    }

    todo = h->size - sizeof(struct header_s);
    b = (char *)ibot + sizeof(struct header_s);
    do {
	ret = read(fd, b, INT_MAX);
	if (ret < 0) {
	    perror("read");
	    exit(1);
	}
	if (ret == 0){
	    fprintf(stderr,"error: module size too small\n");
	    exit(1);
	}
	todo -= ret;
	b += ret;
    } while (todo);
    close(fd);
    
    /* copy fuzix cmd line to os9 parameter area */
    strcpy((char *)pbot, argv[2]);

    /* make the initial interrupt stack */
    s = (struct regs_s *)sbot;
    s->cc = 0x80;
    *(uint16_t *)&s->a = psize;
    s->dp = itop >> 8;
    s->x = pbot;
    s->y = dtop;
    s->u = itop;
    s->pc = ipc;
    os9go(sbot);
}
