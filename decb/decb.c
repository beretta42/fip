/*

A Radio Shack / Tandy BASIC filesystem manipulation program

TODOS: 
* better checking of argc,argv

*/



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>


#define SECZ    256          /* RSDOS sector size */
#define GRANZ   SECZ * 9L    /* Granule size */
#define FATZ    68           /* size of FAT table */
#define FATST   307L*SECZ    /* start offset of FAT */
#define DIRST   308L*SECZ    /* start offset of directory */
#define LABEL   322L*SECZ    /* disk label sector */

/* RSDOS filesystem directory entry */
struct dirent {
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t type;
    uint8_t aflag;
    uint8_t first;
    uint8_t remainder[2];
    uint8_t unused[16];
};


struct dirent entry;         /* copy of on-disk dir entry */
off_t entry_off;             /* device file offset of loaded dir entry */
uint8_t fat[FATZ];           /* copy of FAT table */
uint8_t buf[GRANZ];          /* scratch buffer */
int fd;                      /* fd of device/image file */
int estate = 0;              /* state of  getentry */
int deftype = 0;             /* default file type (BASIC) */
int defaflag = 0;            /* default ascii flag (bin/crunched)*/
int trans = 0;               /* translate lf */
int mbr = 0;                 /* protect mbr */
int boot = 0;                /* protect boot track */
off_t offset = 0;            /* offset into image */


/* fixme: delete me after debugging */
void printm( char *p, int l)
{
    while( l--)
	printf("%02x ", *p++);
}


int printusage( void )
{
    fprintf(stderr,"decb [opts] command img src dst\n");
    fprintf(stderr,"opts: -ab0123lmp -o diskno -v offset\n");
    fprintf(stderr,"commands: mkfs, ls, rm, get, put, putboot, getboot, map, label\n");
    exit(1);
}


int myopen( char *name, int flags, mode_t mode){
    int ret;
    ret = open(name, flags, mode);
    if (ret < 0){
	perror("open");
	exit(1);
    }
    return ret;
}

/* fills and returns buf with C style string */
char *getstring( char *p, int l ){
    char *d = buf;
    while (l--){
	*d++ = *p++;	
    }
    *d = 0;
    return buf;
}

void saveentry( void ){
    int ret;
    off_t off;
    off = lseek(fd, entry_off, SEEK_SET);
    if (off < 0){
	perror("seek");
	exit(1);
    }
    ret = write(fd, &entry, 32);
    if (ret != 32){
	perror("write");
	exit(1);
    }
}

/* fills in entry with next directory entry */
int getentry( void ){
    off_t off;
    int ret;
    static int count;
    switch( estate ){
    case 0:
	off = lseek(fd, DIRST+offset, SEEK_SET);
	if (off < 0){
	    perror("seek");
	    exit(1);
	}
	estate = 1;
	count = 0;
    case 1:
	while (1){
	    entry_off = lseek(fd, 0L, SEEK_CUR);
	    if (entry_off < 0){
		perror("dir seek");
		exit(1);
	    }
	    ret = read(fd, &entry, 32 );
	    if (ret != 32){
		perror("dir read");
		exit(1);
	    }
	    if (++count > 72){
		estate = 0;
		return 0;
	    }
	    return 1;
	} 
    }
    return 0;
}

/* seek device file to start of FAT */
void seekfat( void )
{
    off_t ret;
    ret = lseek(fd, FATST+offset, SEEK_SET);
    if (ret < 0){
	perror("seek fat");
	exit(1);
    }
}

/* load fat from device file */
void loadfat( void )
{
    int ret;
    seekfat();
    ret = read(fd, fat, FATZ);
    if (! ret){
	fprintf(stderr,"image not big enough\n");
	exit(1);
    }
    if (ret != FATZ){
	perror("fat read");
	exit(1);
    }
}

/* save fat to device file */
void savefat( void )
{
    int ret;
    seekfat();
    ret = write(fd, fat, FATZ);
    if (ret != FATZ){
	perror("fat write");
	exit(1);
    }
}

/* fill buf with RSDOS formatted name */
void stringconv( char *name )
{
    char *d = buf;
    char *p = name;
    /* translate name to entry format */
    memset(buf,0x20, 11 );
    while (*p){
	if( *p == '.' ){
	    p++;
	    d = &buf[8];
	    continue;
	}
	*d++ = *p++;
    }
}

/* scan directory entries to find name */
int find( char *name )
{
    stringconv(name);
    /* look thru entries for match */
    while (getentry()){
	if (entry.name[0] == 0xff) return 0;
	if (!memcmp(&entry,&buf,11)) return 1;
    }
    return 0;
}

/* Find a first free granule */
int findgran( void )
{
    int x;
    for ( x=0; x < FATZ; x++){
	if (fat[x] == 0xff){
	    fat[x] = 0xc0;  /* top two bits = last gran in file */
	    return x;
	}
    }
    return -1;
}

/* seek device fd to a certain ganule */
void seekgran(int gran)
{
    off_t ret;
    if (gran > 33) gran +=2;
    ret = lseek(fd, GRANZ*gran+offset, SEEK_SET);
    if (ret < 0){
	perror("gran seek");
	exit(1);
    }
}

/* load buf with 1 granule of data */
void loadgran(int gran)
{
    int ret;
    seekgran(gran);
    ret = read(fd, buf, GRANZ);
    if (! ret){
	fprintf(stderr,"image not big enough\n");
	exit(1);
    }
    if (ret != GRANZ){
	perror("gran read");
	exit(1);
    }
}


/* open on image */
void openimg( char *name )
{
    int ret;
    fd = open(name, O_RDWR);
    if (fd < 0){
	perror("image open");
	exit(1);
    }
    loadfat();
}

/* delete a granule chain */
void delchain( int first )
{
    int next;
    while ((first & 0xc0) != 0xc0){
	next = fat[first];
	fat[first] = 0xff;
	first = next;
    }
}

/* count granules in file */
int countchain( int first )
{
    int c = 0;
    while ((first & 0xc0) != 0xc0 ){
	first = fat[first];
	c++;
    }
    return c;
}

/* get free granules */
int granfree( void )
{
    int x = 0;
    int c = 0;
    for (; x < FATZ; x++){
	if (fat[x] == 0xff) c++;
    }
    return c;
}


/* translate newlines */
void nl2cr( void ){
    int x;
    for (x = 0; x < GRANZ; x++)
	if (buf[x] == '\n') buf[x] = '\r';
}

/* translate newlines */
void cr2nl( void ){
    int x;
    for (x = 0; x < GRANZ; x++)
	if (buf[x] == '\r') buf[x] = '\n';
}


/* make an empty disk */
int mkfs( int argc, char *argv[] )
{
    int x;
    char *p;
    int ret; 
    off_t o;

    /* check arguments here */
    /* set buffer to all 0xff's */
    p = buf;
    for (x = 0; x < GRANZ; x++) *p++ = 0xff;
    /* write the buffer to disk to make a standard 35 track disk */
    fd = myopen(argv[0], O_RDWR|O_CREAT, 0666);
    o = lseek(fd, offset, SEEK_SET);
    if (o < 0){
	perror("lseek");
	exit(1);
    }
    for (x = 0; x < 70; x++){
	ret = write(fd, buf, GRANZ);
	if (ret != GRANZ){
	    perror("write");
	    exit(1);
	}
    }
    loadfat();
    if (mbr)
	fat[0] = 0xc0;
    if (boot)
	fat[FATZ - 2] = fat[FATZ - 1] = 0xc0;
    savefat();
    return 0;
}

/* list contents of disk */
int list( int argc, char *argv[] )
{
    int ret;
    int x;
    int no = 0;
    off_t o;
    
    /* fixme: check args here */
    openimg(argv[0]);
    /* print disk label if any */
    o = lseek(fd, LABEL + offset, SEEK_SET);
    ret = read(fd, buf, 256);
    if (buf[0] != 0 && buf[0] != 0xff){
        printf("%s\n", buf);
    }
    /* list the files */
    while (getentry()){
	if (entry.name[0] == 0xff) break;
	if (entry.name[0] != 0) {
	    printf("%s ", getstring(entry.name, 8));
	    printf("%s %d ", getstring(entry.ext, 3),
		   entry.type);
	    if (entry.aflag)
		printf("A");
	    else
		printf("B");
	    printf(" %2d", countchain(entry.first) );
	    printf(" %x\n", entry.first );
	    no++;
	}
    }
    printf("%d files, %d free\n", no, granfree());
    return 0;
}

/* copy disk file to host filesystem */
int get( int argc, char *argv[] )
{
    int ret;
    int x;
    int dst;
    int gran;
    int size;
    /* fixme: check args here */
    openimg(argv[0]);
    if (!find(argv[1])){
	fprintf(stderr,"file not found\n");
	exit(1);
    }
    /* found name */
    dst = myopen(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0666);
    gran = entry.first;
    size = GRANZ;
    while (size == GRANZ){
	if ((fat[gran] & 0xc0) == 0xc0){
	    size = (fat[gran] & ~0xc0) - 1;
	    size *= SECZ;
	    size += (entry.remainder[0]<<8) + entry.remainder[1];
	}
	loadgran(gran & ~0x80 );
	if (trans) cr2nl();
	ret = write( dst, buf, size );
	if (ret != size){
	    perror("write");
	    exit(1);
	}
	gran = fat[gran];
    }
    return 0;
}

/* put a file */
int put( int argc, char *argv[] )
{
    int src;
    int x;
    int size;
    int ret;
    openimg(argv[0]);
    /* check for file existance */
    if (find(argv[2])){
	fprintf(stderr,"file exists\n");
	exit(1);
    }
    estate = 0;
    /* find empty directory entry */
    while (getentry()){
	if (entry.name[0] == 0 ||
	    entry.name[0] == 0xff ){
	    goto found;
	}
    }
    fprintf(stderr,"out of free directory entries\n");
    exit(1);
 found:
    src = myopen(argv[1], O_RDONLY, 0);
    x = entry.first = findgran();
    if (entry.first < 0){
	fprintf(stderr,"out of granules");
	exit(1);
    }
    size = GRANZ;
    while (size == GRANZ){
	size = read(src, buf, size);
	if (size < 0){
	    perror("read");
	    exit(1);
	}
	if (trans) nl2cr();
	seekgran(x);
	ret = write(fd, buf, size);
	if (ret < 0){
	    perror("write");
	    exit(1);
	}
	if (size < GRANZ){
	    break;
	}
	x = fat[x] = findgran();
	if (x < 0){
	    fprintf(stderr,"out of granules\n");
	    delchain(entry.first);
	    exit(1);
	}
    }
    entry.remainder[0] = (size % SECZ) >> 8;
    entry.remainder[1] = (size % SECZ) & 0xff;
    fat[x] = 0xc0 | ((size + SECZ-1) / SECZ);
    stringconv(argv[2]);
    memcpy(&entry, buf, 11);
    /* fixme: need options to choose type, ascii flag */
    entry.type = deftype;
    entry.aflag = defaflag;
    saveentry();
    savefat();
    return 0;
}

/* remove a file */
int rm( int argc, char *argv[] )
{
    openimg(argv[0]);
    if (!find(argv[1])){
	fprintf(stderr,"file not found");
	exit(1);
    }
    delchain(entry.first);
    entry.name[0] = 0;
    savefat();
    saveentry();
    return 0;
}

/* write file to boot track */
int putb( int argc, char *argv[] )
{
    int src;
    int size;
    int ret;
    int x;
    openimg(argv[0]);
    /* check to see if anything refs boot track */
    while (getentry()){
	if (entry.name[0] == 0xff) break;
	if (entry.name[0] != 0){
	    x = entry.first;
	    while ((x & 0xc0) != 0xc0){
		if ((x == FATZ - 2) ||
		    (x == FATZ - 1)){
		    fprintf(stderr,"error: boot track already used");
		}
		x = fat[x];
	    }
	}
    }
    /* open source file */
    src = myopen(argv[1], O_RDONLY, 0);
    /* copy first 4608 bytes from src to boot track */
    for (x = FATZ - 2; x < FATZ; x++){
	size = read(src, buf, GRANZ);
	if (size < 0){
	    perror("read");
	    exit(1);
	}
	seekgran(x);
	ret = write(fd, buf, size);
	if (ret < 0){
	    perror("write");
	    exit(1);
	}
    }
    /* protect boot track */
    fat[FATZ-2] = fat[FATZ-1] = 0xc0;
    savefat();
    return 0;
}

/* write boot track to file */
int getb( int argc, char *argv[] )
{
    int dst;
    int x;
    int ret;
    openimg(argv[0]);
    dst = myopen(argv[1], O_WRONLY|O_CREAT, 0666);
    for (x = FATZ-2; x < FATZ; x++){
	seekgran(x);
	ret = read(fd, buf, GRANZ);
	if (ret < GRANZ){
	    perror("read");
	    exit(1);
	}
	ret = write(dst, buf, GRANZ);
	if (ret < GRANZ){
	    perror("write");
	    exit(1);
	}
    }
    return 0;
}

/* print the fat map */
int map( int argc, char *argv[] )
{
    int i,j;
    uint8_t *p = fat;
    openimg(argv[0]);
    for (i = 0; i < (FATZ+7)/8; i++){
	printf("%2x: ", i/2);
	for(j = 0; j < 8 && p-fat < FATZ; j++){
	    switch (*p){
	    case 0xc0:
		printf("XX ");
		break;
	    case 0xff:
		printf(".. ");
		break;
	    default:
		printf("%02x ", *p);
	    }
	    p++;
	}
	printf("\n");
    }
    return 0;
}

int label( int argc, char *argv[] )
{
    off_t o;
    int ret;
    openimg(argv[0]);
    o = lseek(fd, LABEL + offset, SEEK_SET);
    if (o < 0){
	perror("seek");
	exit(1);
    }
    memset(buf, 0, SECZ);
    strncpy(buf, argv[1], SECZ-1);
    ret = write(fd, buf, 256);
    if (ret < 0){
	perror("write");
	exit(1);
    }
    return 0;
}


/* "And the kickoff!" */
int main( int argc, char *argv[] )
{
    int pargc;
    char **pargv;
    int ret;
    while (1){
	ret = getopt(argc, argv, "ab0123lmpo:v:");
	if (ret < 0) break;
	switch (ret){
	case 'a':
	    defaflag = 0xff;
	    break;
	case 'b':
	    defaflag = 0x00;
	    break;
	case '0':
	    deftype = 0x0;
	    break;
	case '1':
	    deftype = 0x1;
	    break;
	case '2':
	    deftype = 0x2;
	    break;
	case '3':
	    deftype = 0x3;
	    break;
	case 'l':
	    trans = 1;
	    break;
	case 'm':
	    mbr = 1;
	    break;
	case 'p':
	    boot = 1;
	    break;
	case 'o':
	    offset = SECZ * 35L * 18 * strtol(optarg,NULL,0);
	    break;
	case 'v':
	    offset = strtol(optarg,NULL,0);
	    break;
	case '?':
	    printusage();
	}
    }
    pargc = argc - optind - 1;
    pargv = &argv[optind+1];
    if (pargv[-1] == NULL) printusage();
    if ( ! strcmp(pargv[-1],"mkfs") ) exit(mkfs(pargc, pargv));
    if ( ! strcmp(pargv[-1],"ls") ) exit(list(pargc, pargv));
    if ( ! strcmp(pargv[-1],"get") ) exit(get(pargc, pargv));
    if ( ! strcmp(pargv[-1],"put") ) exit(put(pargc, pargv));
    if ( ! strcmp(pargv[-1],"rm") ) exit(rm(pargc, pargv));
    if ( ! strcmp(pargv[-1],"putboot") ) exit(putb(pargc, pargv));
    if ( ! strcmp(pargv[-1],"getboot") ) exit(getb(pargc, pargv));
    if ( ! strcmp(pargv[-1],"map") ) exit(map(pargc, pargv));
    if ( ! strcmp(pargv[-1],"label") ) exit(label(pargc, pargv));
    printusage();
}
