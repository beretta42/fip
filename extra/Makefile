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


install:
	cp $(FUZIX_DIR)/Applications/util/bogomips ./
	$(UCP) $(FSIMG) < install.ucp
