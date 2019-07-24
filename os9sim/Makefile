include ../config.mk
PLATFORM = 6809
CC = m6809-unknown-gcc
ASM = m6809-unknown-as
AR = m6809-unknown-ar
AS = $(ASM)
LINKER = m6809-unknown-ld
CFLAGS = -I$(FUZIX_DIR)/Library/include -I$(FUZIX_DIR)/Library/include/6809
LINKER_OPT = --oformat=raw -L$(FUZIX_DIR)/Library/libs -lc6809
LIBGCCDIR = $(dir $(shell $(CC) -print-libgcc-file-name))
LINKER_OPT += -L$(LIBGCCDIR) -lgcc
LINKER_OPT += --script=$(TARGET).link
ASM_OPT = -o
CRT0 = $(FUZIX_DIR)/Library/libs/crt0_6809.o

.SUFFIXES: .c .o

all: os9sim

tricks.o: tricks.s

os9sim.o: os9sim.c

os9sim: os9sim.o tricks.o
	$(LINKER) -Map=os9sim.map -o $@ $(LINKER_OPT) $(CRT0) $^

clean:
	rm -f *.o core *~ *.asm *.lst *.sym *.map *.noi *.lk *.ihx *.tmp *.bin size.report os9sim

rmbak:
	rm -f *~ core

install:
	$(UCP) $(FSIMG) < install.ucp
