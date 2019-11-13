/*
   Grok a Fuzix 16 bit filesystem, finds a kernel in in /boot, and
   loads it up into the usual kernel location in memory.

   some of this code is lifted from FUZIX for obvious reasons.

   In BASIC, set BT$ to the name of the kernel to load. Also, FX$ will
   be passed to the kernel as it's commandline.

   TODO:
   extended MBR partitioning support
   double indirect support (we're limited to reading ~140k files until done)
   figure out exactly how many temporary buffer madness
*/

#include <stdint.h>


#define BTTO 3      // Seconds to wait for CNTL key before autobooting
#define min(a,b) a < b ? a : b
#define NULL 0
#define NL putc('\r')

/* put string to console via BASIC (add your own NL/CR) */
void puts(char *s);
/* put charactor to console */
void putc(char c);
char getc(void);

int dw_init(uint8_t drive_no);
int dw_read(void *b, uint32_t blk);
void bzero(void *dest, uint16_t n);
void memcpy(void *dest, void *src, uint16_t n);
void exit(void);
void exec(uint16_t addr);
void di(void);
extern char *bounce;
extern char *bounce_end;
#define BOUNCE_LEN ((uint16_t)&bounce_end - (uint16_t)&bounce)
#define taskreg *((volatile uint8_t *)0xff91)

/* These structures define the FUZIX (uzi) filesystem */
#define ROOT 1
#define FILESYS_TABSIZE 50
#define MAGIC 0x31c6
#define VARTAB 0x1b
typedef struct super_s {
    uint16_t      s_mounted;
    uint16_t      s_isize;
    uint16_t      s_fsize;
    uint16_t      s_nfree;
    blkno_t       s_free[FILESYS_TABSIZE];
    int16_t       s_ninode;
    uint16_t      s_inode[FILESYS_TABSIZE];
    uint8_t       s_fmod;
    /* 0 is 'legacy' and never written to disk */
#define FMOD_GO_CLEAN   0       /* Write a clean to the disk (internal) */
#define FMOD_DIRTY      1       /* Mounted or uncleanly unmounted from r/w */
#define FMOD_CLEAN      2       /* Clean. Used internally to mean don't
				   update the super block */
    uint8_t       s_timeh;      /* bits 32-40: FIXME - wire up */
    uint32_t      s_time;
    blkno_t       s_tfree;
    uint16_t      s_tinode;
    uint8_t       s_shift;      /* Extent size */
} super_t;

typedef struct inode {
    uint16_t i_mode;
#define F_REG   0100000
#define F_DIR   040000
    uint16_t i_nlink;           /* Note we have 64K inodes so we never overflow
*/
    uint16_t i_uid;
    uint16_t i_gid;
    uoff_t    i_size;           /* Never negative */
    uint32_t   i_atime;         /* Breaks in 2038 */
    uint32_t   i_mtime;         /* Need to hide some extra bits ? */
    uint32_t   i_ctime;         /* 24 bytes */
    blkno_t  i_addr[20];
} inode_t;

typedef struct dirent {
    uint16_t inode;
    char name[30];
} dirent_t;

#define MBR_PART_OFF 446        /* offset into MBR block of partition entrys */
typedef struct {
    int8_t status;
    uint8_t chs_start[3];
    uint8_t type;
    uint8_t chs_last[3];
    uint8_t lba_first[4];
    uint8_t lba_last[4];
} mbr_part_t;


uint32_t swizzle32(uint8_t *b) {
    uint8_t s0, s1;
    s0 = b[0];
    s1 = b[1];
    b[0] = b[3];
    b[1] = b[2];
    b[2] = s1;
    b[3] = s0;
    return *(uint32_t *)b;
}

uint8_t mbr_magic(uint8_t *b) {
    if (b[510] != 0x55 || b[511] != 0xaa)
	return 1;
    return 0;
}


/* output routines / debugging stuff */

void putn(uint8_t n) {
    if (n > 9) n += 'A' - 10;
    else n += '0';
    putc(n);
}

void putb(uint8_t n) {
    putn(n >> 4);
    putn(n & 0xf);
}

void putw(uint16_t n) {
    putb(n >> 8);
    putb(n & 0xff);
}

void putl(uint32_t n) {
    putw(n >> 16);
    putw(n & 0xffff );
}

void dump(void *addr, int size) {
    char *a = (char *)addr;
    while (size--) {
	putb(*a++);
	putc(' ');
    }
    NL;
}



/*
   Program variables - mostly for keeping state of the memory file and
   exactly one disk file
*/

char cmd_line[256];
char boot_file[256];
uint8_t drive_no;
uint8_t part_no;
uint32_t part_offset;
uint16_t tick_div;
uint8_t buf[512];        // temporary buffer
uint8_t tmp[512];
uint8_t mountf = 0;
/* these vars are the open file state */
uint8_t dbuf[512];       // data buffer
uint16_t idir[256];      // copy of single indirect block
inode_t i;               // copy of disk inode
uint16_t left;           // bytes left in data buffer
void *bpos;              // read position of data buffer
uint16_t spos;           // block position of file
uint16_t epos;           // end of blocks
uint16_t cd = ROOT;      // current directory
/* memory file */
uint16_t mleft;
char *maddr;
char *paddr;

int (*dev_init)(uint8_t drive_no) = dw_init;
int (*_dev_read)(void *b, uint32_t blk) = dw_read;


/* coco 3 BASIC/Hardware stuff */
#define mmu0 ((volatile uint8_t *)0xffa0)
#define mmu1 ((volatile uint8_t *)0xffa8)
#define casflg (*(volatile uint8_t *)0x11a)  // upper/lower case toggle
#define timer  (*(volatile uint16_t *)0x112) // BASIC's ticker
#define keyrol ((uint8_t *)0x0152)  // BASIC's keyroll over
#define piacol (*(volatile uint8_t *)0xff00) // keyboard columns (read this)
#define piarow (*(volatile uint8_t *)0xff02) // keyboard rows (write this)

/*
   this struct defines BASIC's idea a a name variable
   these structures exist in in an array
*/
typedef struct basvar {
    uint16_t name;
    uint8_t len;
    uint8_t res1;
    char *addr;
    uint8_t res2;
} basvar_t;

/* this struct is the 5 byte program BASIC .bin file section header */
struct binhdr_s {
    uint8_t flag;
#define PRE  0x00
#define POST  0xff
    uint16_t len;
    char *addr;
} h;


/* start with the basics */

static void _exit() {
    casflg = 0xff;
    exit();
}

uint16_t strlen(char *s) {
    uint16_t n = 0;
    while (*s++ != 0) n++;
    return ++n;
}


static strcpy( char *d, char *s) {
    memcpy(d,s,strlen(s)+1);
}


static dev_read(void *b, uint32_t blk) {
    return _dev_read(b, blk + part_offset);
}

/* copies string variable from BASIC to dst */
static int16_t getbas(char *dst, char *name) {
    uint16_t n = ((name[0] * 256) + name[1]) | 0x80;
    basvar_t *p = *(basvar_t **)VARTAB;
    for (; p->name != 0; p++) {
	if (n == p->name) {
	    memcpy(dst, p->addr, p->len);
	    dst[p->len] = 0;
	    return 0;
	}
    }
    dst[0] = 0;
    return -1;
}

/* fixme: these error message need clean up */
char *err_tab[] = {
    "NO ERROR",
    "READING 0 BYTES",
    "CANNOT OPEN KERNEL FILE",
    "SHORT READ OF KERNEL FILE",
    "SHORT READ OF KERNEL FILE",
    "ERROR READING KERNEL FILE",
    "ERROR READING DEVICE",
    "ERROR PREMATURE EOF",
    "BAD ERROR NO",
};


/* gernically handle errors */
static void error(uint8_t no) {
    puts("ERROR 0X");
    putb(no); NL;
    if (no > 7) error(8);
    puts(err_tab[no]); NL;
}

/* open the disk inode  --
   there's no closing an inode here, just keep opening up new ones */
static iopen(uint16_t no) {
   /* fixme: do a sanity check on inode no */
    dev_read(buf, 2 + (no >> 3));
    memcpy(&i, buf + (no & 0x7) * 64, sizeof(inode_t));
    dev_read(idir, i.i_addr[18]);
    left = 0;
    spos = 0;
    epos = (i.i_size + 0x1ff) >> 9;
}


/* helper to iread */
static uint16_t bmap(void) {
    if (spos < 18)
	return i.i_addr[spos];
    return idir[spos - 18];
}

/* read bytes from disk file put them in buffer */
static uint16_t iread(void *buf, uint16_t n) {
    // past end of data buffer ?
    if (left == 0) {
	if (spos == epos)
	    return 0;
	dev_read(dbuf, bmap());
	spos++;
	bpos = dbuf;
	//if (spos == epos - 1) left = i.i_size & 0x1ff;
	//else left = 512;
	left = 512;
    }
    n = min(n, left);
    if (n == 0) {
	error(1);
	return -1;
    }
    memcpy(buf, bpos, n);
    left -= n;
    bpos += n;
    return n;
}

/* lookup name in an directory inode */
static int16_t lookup(uint16_t ino, char *name) {
    static dirent_t d;
    iopen(ino);
    if (i.i_mode & F_DIR == 0) {
	return -1;
    }
    while (iread(&d, sizeof(dirent_t))) {
	if (!strcmp(d.name, name))
	    return d.inode;
    }
    return -1;
}

/* break path apart and lookup each inode
 returns an inode no to ID the inode, or -1 on error */
static uint16_t dopath(char *s) {
    static char c[31];
    char *d;
    uint16_t ino = cd;
    if (*s == '/') {
	ino = ROOT;
	s++;
    }
    /* deal with unix pathnames */
    while(*s && ino != -1) {
	d = c;
	while (*s == '/') s++;
	while (*s != 0 && *s != '/') *d++ = *s++;
	*d = 0;
	ino = lookup(ino, c);
    }
    return ino;
}

/* open an inode by regular unix pathname */
static int8_t open(char *path) {
    uint16_t ino = dopath(path);
    if (ino == -1) return ino;
    iopen(ino);
    return 0;
}

/* outting wrapper around testing if anything is mounted */
static uint8_t mounted ( void ) {
    if (mountf == 0)
	puts("NOTHING MOUNTED\r");
    return mountf;
}

/* output each valid directory entry from a dir inode */
static void dir(uint16_t ino) {
    static dirent_t d;
    if (!mounted()) return;
    iopen(ino);
    while (iread(&d, sizeof(dirent_t))) {
	/* fixme: clarify unused directory entries are marked
	   with empty name string */
	if (d.name[0]) {
	    putw(d.inode);
	    putc(' ');
	    puts(d.name);
	    NL;
	}
    }
}

/* output contents of a file */
static void cat(char *p) {
    int n;
    int ino;
    char *c;
    if (!mounted()) return;
    if (open(p)) return;
    while (n = iread(buf, 512)) {
	c = buf;
	while(n--) {
	    if (*c == '\n') *c = '\r';
	    putc(*c++);
	}
    }
}

/* try to mount a filesystem */
static int8_t mount(uint16_t minor, uint8_t part) {
    super_t *s = (super_t *)buf;
    mbr_part_t *p = (mbr_part_t *)(buf + MBR_PART_OFF);
    uint8_t *c;
    if (dev_init(minor)) goto bad;
    /* load up first and check for sector for MBR */
    if (dev_read(buf, 0)) goto bad;
    if (mbr_magic(buf) == 0) {
	if (part) {
	    c = (uint8_t *)&p[minor-1].lba_first;
	    part_offset = swizzle32(c);
	}
    }
    else {
	puts("MBR NOT FOUND\r");
	part_offset = 0L;
	if (part > 0) goto bad;
    }
    if (dev_read(s, 1)) goto bad;
    /* fixme: should check fmod too? */
    if (s->s_mounted != MAGIC) goto bad;
    mountf = 1;
    cd = ROOT;
    return 0;
 bad:
    mountf = 0;
    return -1;
}


/* seek & map the physical memory file */
void mseek(char *addr) {
    uint16_t a = (uint16_t)addr;
    uint16_t off;
    paddr = addr;
    mmu0[1] = a >> 13;
    off = a & 0x1fff;
    mleft = 0x2000 - off;
    maddr = (char *)0x2000 + off;
}

/* write bytes to physical memory file */
void mwrite(void *buf, int no) {
    int max;
    while (no) {
	if (mleft == 0) mseek(paddr);
	max = min(mleft, no);
	memcpy(maddr, buf, max);
	mleft -= max;
	maddr += max;
	no -= max;
	paddr += max;
	buf += max;
    }
    return;
}

/* set driver to use */
// fixme: add dw_bit, sd
int8_t set_driver(char *b) {
    if (!strcmp(b,"dw")) {
	dev_init = dw_init;
	_dev_read = dw_read;
	return 0;
    }
    //if (!strcmp(b, "sdc")) {
	//dev_init = sdc_init;
	//dev_read = sdc_read;
    //	return 0;
    //}
    return -1;
}

/* display file loading to user here */
void ticker(uint16_t done) {
    static uint16_t tick_acc;
    tick_acc += done;
    if (tick_acc >= tick_div) {
	putc(0xaf);
	tick_acc -= tick_div;
    }
}

uint8_t iscntldown() {
    piarow = ~0x10;
    return ~piacol & 0x40;
}

uint8_t isspcdown() {
    piarow = (uint8_t)~0x80;
    return ~piacol & 0x08;
}

/* attempt to boot - takes .bin name and a kernel command line */
uint8_t boot(char *name, char *cmd) {
    int16_t todo;
    int m;
    int x;
    int ret;
    if (open(name)) {
	error(2);
	return 1;
    }
    tick_div = i.i_size / 32;
    while (1) {
	if (iread(&h, sizeof(h)) != sizeof(h)) {
	    error(3);
	    return 1;
	}
	if (h.flag != PRE) break;
	todo = h.len;
	mseek(h.addr);
	while(todo) {
	    if (iscntldown()) {
		bzero(keyrol,8);
		NL;
		return 1;
	    }
	    m = min(todo, mleft);
	    m = min(m,512);
	    /* if we can do direct device to memory */
	    if (m == 512 && left == 0) {
		if (spos == epos) {
		    error(7);
		    return 1;
		}
		ret = dev_read(maddr, bmap());
		if (ret) {
		    error(6);
		    return 1;
		}
		// duplicate mwrite
		mleft -= m;
		maddr += m;
		todo -= m;
		paddr += m;
		// duplicate iread
		spos++;
		ticker(512);
	    }
	    /* else use a tmp buffer */
	    else {
		ret = iread(tmp, todo);
		if (ret == 0) {
		    error(4);
		    return 1;
		}
		if (ret == -1) {
		    error(5);
		    return 1;
		}
		mwrite(tmp, ret);
		todo -= ret;
		ticker(ret);
	    }
	}
    }
    ticker(tick_div);
    /* copy bonce routine down to phys 0 */
    mseek(0);
    mwrite(&bounce, BOUNCE_LEN);
    mwrite(&h.addr,2);
    mseek((char *)0x88);
    mwrite(cmd,strlen(cmd)+1);
    /* move to task 1 */
    for (x = 0; x < 8; x++) {
	mmu1[x] = 0x38 + x;
    }
    taskreg = 1;
    /* map in bounce routine and exec it */
    mmu1[0] = 0;
    di();
    exec(0);
}

/*
  functions for interactive command-line mode
*/


/* fill a buffer with keystroke from user */
void getcmdline(char *buf, char *prompt) {
    int len = 0;
    char c;
    puts(prompt);
    while (1) {
	c = getc();
	if (c == '\r') {
	    buf[len] = 0;
	    putc(c);
	    break;
	}
	if (c == 8) {
	    if (len) {
		len--;
		putc(c);
	    }
	    continue;
	}
	if (c < ' ' || c > 0x7f) {
	    continue;
	}
	if (len < 255) {
	    buf[len] = c;
	    len++;
	    putc(c);
	}
    }
}

/* similar to official strtok - breaks chunks of a string down into
   space separately token */
char *rest;
char *strtok(char *line) {
    char *old;
    if (line != NULL) {
	rest = line;
    }
    if (*rest == 0) return NULL;
    old = rest;
    while(*rest == ' ') rest++;
    while(*rest != ' ' && *rest != 0) rest++;
    if (*rest != 0) *rest++ = 0;
    else rest = NULL;
    return old;
}

/* change the working directory */
void docd(void) {
    uint16_t ret;
    if (!mounted()) return;
    char *t = strtok(NULL);
    if (t == NULL) {
	cd = ROOT;
	return;
    }
    ret = dopath(t);
    if (ret == -1) {
	puts("DIR NOT FOUND\r");
	return;
    }
    iopen(ret);
    if (!(i.i_mode & F_DIR)) {
	puts("NOT A DIR\r");
	return;
    }
    cd = ret;
}


/* attempt a FUZIX filesystem mount */
void domount(void) {
    uint8_t drive = 0;
    uint8_t part = 0;
    char *t = strtok(NULL);
    if (t == NULL) goto bad;
    set_driver(t);
    if (t) {
	t = strtok(NULL);
	if (t) drive = *t - 0x30;
    }
    if (t) {
	t = strtok(NULL);
	if (t) part = *t - 0x30;
    }
    if (drive > 9 || part > 4) {
	puts("BAD DRIVE NO OR PART\r");
	goto bad;
    }
    if (mount(drive, part)) goto bad;
    return;
 bad:
    puts("CANNOT MOUNT\r");
    return;
}


void dopart(void) {
    int x;
    mbr_part_t *p = (mbr_part_t *)(buf + MBR_PART_OFF);
    if (_dev_read(buf,0)) {
	puts("CANNOT READ DEVICE\r");
	return;
    }
    if (mbr_magic(buf)) {
	puts("MBR MAGIC NOT FOUND\r");
	return;
    }
    puts("STAT TYPE START    END\r");
    for (x = 0; x < 4; x++) {
	putb(p->status); puts("   ");
	putb(p->type); puts("   ");
	putl(swizzle32(p->lba_first)); putc(' ');
	putl(swizzle32(p->lba_last));
	NL;
	p++;
    }
}

/* yup. */
void dohelp(void) {
    puts("mount driver no\r");
    puts("part\r");
    puts("cd dir\r");
    puts("ls\r");
    puts("cat\r");
    puts("help\r");
    puts("exit\r");
    puts("bt <file> <driver> <no>\r");
    puts("fx <krn-cmd-len>\r");
    puts("boot\r");
}

/* manually give a boot file */
void dooverride(char *buf) {
    if (rest == NULL) {
	puts(buf);
	NL;
	return;
    }
    strcpy(buf,rest);
}

/* interactive get command from user */
void docmd(void) {
    char *t;
    puts("DROPPING TO COMMAND LINE\r");
    while(1) {
	getcmdline(tmp, ">");
	t = strtok(tmp);
	if (t == NULL) continue;
	if (!strcmp(t,"ls")) { dir(cd); continue; }
	if (!strcmp(t,"cd")) { docd(); continue; }
	if (!strcmp(t,"cat")) { cat(strtok(NULL)); continue; }
	if (!strcmp(t,"mount")) { domount(); continue; }
	if (!strcmp(t,"part")) { dopart(); continue; }
	if (!strcmp(t,"exit")) _exit();
	if (!strcmp(t,"quit")) _exit();
	if (!strcmp(t,"help")) { dohelp(); continue; }
	if (!strcmp(t,"boot")) { return; }
	if (!strcmp(t,"bt")) { dooverride(boot_file); continue; }
	if (!strcmp(t,"fx")) { dooverride(cmd_line); continue; }
	puts("BAD COMMAND\r");
    }
}

uint8_t togo;
void main(void) {
    char *t;
    casflg = 0;
    uint8_t time;
    togo = BTTO;
    puts("FUZIX BOOT\r");
    getbas(cmd_line, "FX");
    // we can autoboot if FX isn't set, but NOT BT (the boot file)
    if (getbas(boot_file, "BT")) {
	puts("BT$ NOT SET\r");
	docmd();
    }
    else {
	// wait a while for user to hit override
	puts("HIT CNTL FOR MANUAL BOOTING\r");
	puts("HIT SPACE TO BOOT\r");
	puts("BOOT IN "); putb(togo);
	while(togo--) {
	    time = timer + 60;
	    while(time > timer) {
		if (iscntldown()) {
		    NL;
		    docmd();
		    goto again;
		}
		if (isspcdown()) {
		    NL;
		    goto again;
		}
	    }
	    putc('\b');
	    putn(togo);
	}
	NL;
    }
 again:
    puts("BOOTING WITH:\r");
    puts("BT: "); puts(boot_file); NL;
    puts("FX: "); puts(cmd_line); NL;
    /* parse boot params */
    t = strtok(boot_file); // first time parses off kernel filename
    t = strtok(NULL); // second pares off drive no
    if (t) {
	if (set_driver(t)) {
	    puts("BAD DRIVER NAME\r");
	    docmd();
	    goto again;
	}
    }
    if (t) {
	t = strtok(NULL);
	if (t) drive_no = *t - 0x30;
	if (drive_no > 9) {
	    puts("BAD DRIVE NO\r");
	}
    }
    if (t) {
	t = strtok(NULL);
	if (t) part_no = *t - 0x30;
	if (part_no > 4) {
	    puts("BAD PARTITION NO\r");
	    part_no = 0;
	}
    }
    /* try to mount */
    while (mount(drive_no, part_no)) {
	puts("CANNOT MOUNT FILESYSTEM\r");
	docmd();
	goto again;
    }
    /* display boot message, and try to boot */
    cat("/boot/boot.msg");
    while (boot(boot_file, cmd_line)) {
	puts("CANNOT BOOT KERNEL\r");
	docmd();
	goto again;
    }
}
