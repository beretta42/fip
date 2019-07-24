#include <stdint.h>


uint16_t dw_ll(char *s, int sz, char *r, int rz);
extern uint16_t dw_cksum;

static char b1[2] = { 0x5a, 0x00 };
static char b2[5] = { 0xd2, 0x00, 0x00, 0x00, 0x00 };
static uint8_t type;


uint16_t dw_init(uint8_t drive_no) {
    b2[1] = drive_no;
    return dw_trans(b1, 2, &type, 1);
}

static uint16_t read(char *b, uint32_t blk) {
    int ret;
    b2[4] = blk & 0xff;
    blk >>= 8;
    b2[3] = blk & 0xff;
    blk >>= 8;
    b2[2] = blk;
    ret = dw_trans(b2, 5, b, 256);
    ret |= dw_trans(&dw_cksum, 2, &b2[4], 1);
    ret |= b2[4];
    return ret;
}

dw_read(char *b, uint32_t blk) {
    if (blk & 0xff800000)
	return -1;
    blk *= 2;
    if (read(b, blk))
	return -1;
    b += 256;
    blk++;
    return read(b, blk);
}
		
