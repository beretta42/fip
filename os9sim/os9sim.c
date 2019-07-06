/* 
   A os9 simulator for FUZIX

TODO:
   separate C and os9 stack

*/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <6809/sys.h>

void swi2_handler(void);
uint8_t swi2(uint8_t func);
void os9go(uint8_t *stackptr);

/* this struct is for passing os9 register
   snapshot to C 
*/
struct regs_t {
    uint8_t  cc;
    uint8_t  a;
    uint8_t  b;
    uint8_t  dp;
    uint16_t x;
    uint16_t y;
    uint16_t u;
    uint16_t pc;
};

extern struct regs_t reg;

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
} header;


void os9open(void) {
    int ret;
#ifdef DEBUG
    fprintf(stderr,"I$Open: \"%s\"\n", (char *)reg.x);
#endif
    ret = open((char *)reg.x, O_RDWR);
    if (ret < 0) {
	reg.cc |= 1;
	reg.b = -1;
    }
    else
	reg.a = ret;
}

void os9read(void) {
    int ret;
#ifdef DEBUG  
    fprintf(stderr, "I$Read: %x %x %x\n", reg.a, reg.x, reg.y);
#endif
    ret = read(reg.a, (char *)reg.x, reg.y);
    if (ret < 0) {
	reg.cc |= 1;
	reg.b = -1;
	return;
    }
    reg.y = ret;
}

void os9write(void) {
    int ret;
#ifdef DEBUG
    fprintf(stderr, "I$Write: %x %x %x\n", reg.a, reg.x, reg.y);
#endif
    ret = write(reg.a, (char *)reg.x, reg.y);
    if (ret < 0) {
	reg.cc |= 1;
	reg.b = -1;
	return;
    }
    reg.y = ret;
}

void os9close(void) {
    int ret;
#ifdef DEBUG   
    fprintf(stderr, "I$Close: %x\n", reg.a);
#endif    
    ret = close(reg.a);
    if (ret < 0) {
	reg.cc |= 1;
	reg.b = errno;
    }
    reg.b = ret;
}

void os9exit(void) {
#ifdef DEBUG    
    fprintf(stderr, "F$Exit: %x\n", reg.b);
#endif    
    exit(reg.b);
}

    
void os9call(void) {
    uint8_t op = *(uint8_t *)reg.pc;
    reg.pc += 1;
    reg.cc &= ~0x1;
    switch(op) {
    case 0x84:
	os9open();
	break;
    case 0x89:
	os9read();
	break;
    case 0x8a:
	os9write();
	break;
    case 0x8f:
	os9close();
	break;
    case 0x06:
	os9exit();
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
    struct regs_t *s;
    
    /* all the pointers os9 needs */
    uint8_t *ibot;    /* bottom of code area */
    uint8_t *itop;    /* top of code area */
    uint8_t *dtop;    /* top of data area */
    uint8_t *sbot;    /* bottom of stack area */
    uint8_t *pbot;    /* bottom of parameter area */
    uint8_t *ipc;     /* initial PC of program */
    uint16_t psize;   /* size of parameter area */
    uint16_t size;    /* total size of I + D */

    static struct _uzisysinfoblk i;

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
    ret = ioctl(fd, IOC_SET_SWI2, swi2_handler);
    if (ret < 0) {
	perror("setting swi2");
	exit(1);
    }
    close(fd);
    
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
    ret = read(fd, &header, sizeof(struct header_s));
    if (ret == -1) {
	perror("read");
	exit(1);
    }
    if (header.sync != 0x87cd) {
	fprintf(stderr,"bad header sync bytes\n");
	exit(1);
    }
    
    /* fixme: filter out other bad module stuff here (wrong type) */
    
    /* calculate areas and alloc total sbrk needed */
    ibot = sbrk(0);
    itop = ibot + header.size;
    itop = (uint8_t *)(((uint16_t)itop + 0xff) & ~0xff);
    dtop = itop + header.dsize;
    dtop = (uint8_t *)(((uint16_t)dtop + 0xff) & ~0xff);
    psize = strlen(argv[2]);
    pbot = dtop - psize;
    sbot = pbot - 12;    /* 12 bytes of registers push onto stack */
    size = dtop - ibot;
    ipc = ibot + header.exec;

#ifdef DEBUG    
    printf("h  size: %x\n", header.size);
    printf("h  exec: %x\n", header.exec);
    printf("h dsize: %x\n", header.dsize);
    printf("ibot: %x\n", ibot);
    printf("itop: %x\n", itop);
    printf("dtop: %x\n", dtop);
    printf("psize: %x\n", psize);
    printf("pbot: %x\n", pbot);
    printf("sbot: %x\n", sbot);
    printf("size: %x\n", size);
    printf("ipc: %x\n", ipc);
#endif
    
    ret = (uint16_t) sbrk(size);
    if (ret == -1) {
	perror("sbrk");
	exit(1);
    }

    /* load binary to code area */
    ret = lseek(fd, 0, SEEK_SET);
    if (ret < 0) {
	perror("lseek");
	exit(1);
    }
    todo = header.size;
    b = ibot;
    do {
	ret = read(fd, b, todo);
	if (ret < 0) {
	    perror("read");
	    exit(1);
	}
	todo -= ret;
	b += ret;
    } while (todo);
    close(fd);
    
    /* copy fuzix cmd line to os9 parameter area */
    strcpy(pbot, argv[2]);

    /* make the initial interrupt stack */
    s = (struct regs_t *)sbot;
    s->cc = 0x80;
    *(uint16_t *)&s->a = (uint16_t) psize;
    s->dp = (uint16_t) itop >> 8;
    s->x = (uint16_t) pbot;
    s->y = (uint16_t) dtop;
    s->u = (uint16_t) itop;
    s->pc = (uint16_t) ipc;
    os9go(sbot);
}
