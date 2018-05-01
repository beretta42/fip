/*
lwwire.h

*/
#ifndef lwwire_h_seen___
#define lwwire_h_seen___ 

void lwwire_protoerror(void);
int lwwire_readdata(void *, int, int);
int lwwire_writedata(void *, int);
void lwwire_write(void *, int);
int lwwire_read(void *, int);
int lwwire_read2(void *, int, int);
void lwwire_reset(void);
int lwwire_fetch_sector(int dn, int lsn, void *);
int lwwire_save_sector(int dn, int lsn, void *);
int nonblock(int);
int lwwire_drive_readononly(int dn);
int lwwire_register_extension(int, int (*handler)(int), int (*enable)(void), int (*disable)(void), void (*reset)(void));
void lwwire_proto_read(void);
void lwwire_proto_write(void);
void lwwire_proto_readex(void);
void lwwire_proto_requestextension(void);
void lwwire_proto_disableextension(void);
void lwwire_proto_extensionop(void);

#endif // lwwire_h_seen___
