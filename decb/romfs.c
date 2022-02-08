/*
  Take a color computer disk image and tries to convert to a ROMfs image
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <unistd.h>
#include <fcntl.h>


#define SECZ    256          /* RSDOS sector size */
#define FATZ    68           /* size of FAT table */
#define FATS    307L         /* sector of FAT */
#define DIRS    FATS+1       /* sector of dir */
#define FATST   307L*SECZ    /* start offset of FAT */
#define VERSION 1            /* version no */


uint8_t buf[SECZ];
uint8_t lsns[SECZ * 3];
uint8_t ones[SECZ];
uint8_t zero[SECZ];
int outpos = 5;

int in = 0;
int out = 0;
int banks = 1;

int load(char *buf, int sec) {
    int ret;
    ret = lseek(in, sec*SECZ, SEEK_SET);
    if (ret < 0) {
	perror("infile seek");
	exit(1);
    }
    return read(in, buf, SECZ);
    if (ret != 256) {
	perror("infile read");
	exit(1);
    }
}

void save(char *buf, int sec) {
    int ret;
    ret = lseek(out, sec*SECZ, SEEK_SET);
    if (ret < 0) {
	perror("outfile seek");
	exit(1);
    }
    ret =  write(out, buf, SECZ);
    if (ret != SECZ) {
	perror("outfile write");
	exit(1);
    }
}



void printusage(void) {
    fprintf(stderr, "usage: romfs -s# infile outfile\n");
}



int main(int argc, char *argv[]) {
    int ret;
    int x;

    memset(ones, 0xff, SECZ);
    memset(zero, 0, SECZ);
    memset(lsns, 0, SECZ*3);

    while ((x = getopt(argc, argv, "s:")) >= 0) {
	switch (x) {
	case 's':
	    banks = strtol(optarg, NULL, 0);
	    break;
	case '?':
	    printusage();
	    exit(0);
	default:
	    fprintf(stderr,"bad option %c\n", x);
	    exit(1);
	}
    }
    if (optind +1 >= argc) {
	printusage();
	exit(1);
    }

    in = open(argv[optind], O_RDONLY);
    if (in < 0) {
	perror("infile open");
	exit(1);
    }

    out = open(argv[optind+1], O_RDWR|O_CREAT|O_TRUNC, 0644);
    if (out <0) {
	perror("outfile open");
	exit(1);
    }


    if (banks < 1 || banks > 4) {
	fprintf(stderr,"Error: number of banks must be 1-4\n");
	exit(1);
    }

    outpos = 3;
    int i;
    for (i = 0; i < 630; i++) {
	load(buf, i);
	if (!memcmp(buf, ones, SECZ)) {
	    lsns[i] = 0;
	    continue;
	}
	else if (!memcmp(buf, zero, SECZ)) {
	    lsns[i] = 1;
	    continue;
	}
	else {
	    lsns[i] = outpos;
	    save(buf, outpos++);
	    if (outpos % 64 == 63)
		save(ones, outpos++);
	}
    }

    if (outpos > banks * 64) {
	fprintf(stderr,"Error: cannot fit into ROM\n");
	unlink(argv[optind+1]);
	exit(1);
    }

    save(lsns, 0);
    save(lsns + SECZ, 1);
    save(lsns + SECZ * 2, 2);
    exit(0);
}
