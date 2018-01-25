include config.mk

all:
# apply special patches to Fuzix
	cp diffs/* $(FUZIX_DIR)
	- cd $(FUZIX_DIR); git apply levee.dif
# Make fuzix root filesystem
	make -C $(FUZIX_DIR) TARGET=coco3 clean
	make -C $(FUZIX_DIR) TARGET=coco3
	cd $(FUZIX_DIR)/Standalone/filesystem-src; \
	./build-filesystem -X fuzixfs.dsk 256 65535
# Make different flavors of the kernel
	make -C $(FUZIX_DIR)/Kernel TARGET=coco3 clean
	make -C $(FUZIX_DIR) TARGET=coco3 SUBTARGET=emu kernel
	cp $(FUZIX_DIR)/Kernel/fuzix.bin boot/fuzix-emu.bin
	make -C $(FUZIX_DIR)/Kernel TARGET=coco3 clean
	make -C $(FUZIX_DIR) TARGET=coco3 SUBTARGET=real kernel
	cp $(FUZIX_DIR)/Kernel/fuzix.bin boot/fuzix-real.bin
# Make and Install fip stuff
	make -C cbe -f Makefile.6809 cbe install
	make -C decb -f Makefile.6809 decb install
	make -C bfc -f Makefile.6809 bfc install
	make -C etc
	make -C tcl -f Makefile.6809 picol install
	make -C extra install
	make -C dev
	make -C usr
	make -C boot
	make -C useradd install
# Make smarter DECB boot disk

clean:
	make -C cbe -f Makefile.6809 clean
	make -C decb -f Makefile.6809 clean
	make -C bfc -f Makefile.6809 clean
	make -C tcl -f Makefile.6809 clean
	rm -f *~



