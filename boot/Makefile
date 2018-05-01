include ../config.mk

all:
	$(UCP) $(FSIMG) < install.ucp
	rm -f boot.dsk
	decb dskini boot.dsk
	decb copy -2 -b fuzix-real.bin boot.dsk,REAL.KRN
	decb copy -2 -b fuzix-emu.bin boot.dsk,EMU.KRN
	decb copy -2 -b $(FUZIX_DIR)/Kernel/platform-coco3/boot.bin \
	boot.dsk,BOOT.BIN
	decb copy -l -0 -a AUTOEXEC.BAS boot.dsk,AUTOEXEC.BAS
	# make boot disk 2
	rm -f boot2.dsk
	decb dskini boot2.dsk
	decb copy -2 -b fuzix-fpga.bin boot2.dsk,FPGA.KRN
	decb copy -2 -b fuzix-nano.bin boot2.dsk,NANO.KRN
	decb copy -2 -b $(FUZIX_DIR)/Kernel/platform-coco3/boot.bin \
	boot2.dsk,BOOT.BIN
	decb copy -l -0 -a AUTOEXEC2.BAS boot2.dsk,AUTOEXEC.BAS

clean:
	rm -f boot.dsk boot2.dsk fuzix.emu.bin fuzix-real.bin fuzix-fpga.bin \
	fuzix-nano.bin
