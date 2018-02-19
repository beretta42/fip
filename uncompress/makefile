include ../config.mk
PLATFORM = 6809
CC = m6809-unknown-gcc
# These are wrappers for lwasm and lwar
ASM = m6809-unknown-as
AR = m6809-unknown-ar
LINKER = lwlink
CFLAGS = -I$(FUZIX_DIR)/Library/include -I$(FUZIX_DIR)/Library/include/6502
LINKER_OPT = --format=raw -L$(FUZIX_DIR)/Library/libs -lc6809
LIBGCCDIR = $(dir $(shell $(CC) -print-libgcc-file-name))
LINKER_OPT += -L$(LIBGCCDIR) -lgcc
LINKER_OPT += --script=$(FUZIX_DIR)/Applications/util/$(TARGET).link
ASM_OPT = -o
CRT0 = $(FUZIX_DIR)/Library/libs/crt0_6809.o

.SUFFIXES: .c .o


SRCS  = uncompress.c compress.c

OBJS = $(SRCS:.c=.o)

APPS = $(OBJS:.o=)

all: $(APPS)

$(OBJS): $(SRCS)

$(APPS): $(CRT0)

%: %.o
	$(LINKER) -o $@ $(LINKER_OPT) $(CRT0) $<

clean:
	rm -f $(OBJS) $(APPS) $(SRCS:.c=) core *~ *.asm *.lst *.sym *.map *.noi *.lk *.ihx *.tmp *.bin size.report

rmbak:
	rm -f *~ core

install:
	$(UCP) $(FSIMG) < install.ucp