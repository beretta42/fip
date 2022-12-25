include config.mk

all:
# checkout known working kernel
#	cd $(FUZIX_DIR); git checkout 9f1e60c85bd2e51e236ddcdd40e6b0f2f9bd5a1b
# apply special patches to Fuzix
	cp diffs/* $(FUZIX_DIR)
	- cd $(FUZIX_DIR); git apply levee.dif
	- cd $(FUZIX_DIR); git apply dw.dif
	- cd $(FUZIX_DIR); git apply sd.dif
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
	make -C $(FUZIX_DIR)/Kernel TARGET=coco3 clean
	make -C $(FUZIX_DIR) TARGET=coco3 SUBTARGET=fpga kernel
	cp $(FUZIX_DIR)/Kernel/fuzix.bin boot/fuzix-fpga.bin
	make -C $(FUZIX_DIR)/Kernel TARGET=coco3 clean
	make -C $(FUZIX_DIR) TARGET=coco3 SUBTARGET=nano kernel
	cp $(FUZIX_DIR)/Kernel/fuzix.bin boot/fuzix-nano.bin
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
	make -C util -f Makefile.6809 
	make -C util -f Makefile.6809 install
	make -C slz -f Makefile.6809
	make -C slz -f Makefile.6809 install
	make -C firc -f Makefile.6809 
	make -C firc -f Makefile.6809 install
	make -C booter all install
	make -C os9sim all install
# Make smarter DECB boot disk


kernel:
	make -C $(FUZIX_DIR)/Kernel TARGET=coco3 clean
	make -C $(FUZIX_DIR) TARGET=coco3 SUBTARGET=emu kernel
	cp $(FUZIX_DIR)/Kernel/fuzix.bin boot/fuzix-emu.bin
	make -C $(FUZIX_DIR)/Kernel TARGET=coco3 clean
	make -C $(FUZIX_DIR) TARGET=coco3 SUBTARGET=real kernel
	cp $(FUZIX_DIR)/Kernel/fuzix.bin boot/fuzix-real.bin
	make -C $(FUZIX_DIR)/Kernel TARGET=coco3 clean
	make -C $(FUZIX_DIR) TARGET=coco3 SUBTARGET=fpga kernel
	cp $(FUZIX_DIR)/Kernel/fuzix.bin boot/fuzix-fpga.bin
	make -C $(FUZIX_DIR)/Kernel TARGET=coco3 clean
	make -C $(FUZIX_DIR) TARGET=coco3 SUBTARGET=nano kernel
	cp $(FUZIX_DIR)/Kernel/fuzix.bin boot/fuzix-nano.bin
	make -C boot all

REL = 0.3pre1

pack:
	# make dist directory
	mkdir -p fuzix-$(REL)
	cp boot/boot.dsk fuzix-$(REL)
	cp boot/boot2.dsk fuzix-$(REL)
	cp $(FUZIX_DIR)/Standalone/filesystem-src/fuzixfs.dsk fuzix-$(REL)
	cat boot/idehdr.img $(FUZIX_DIR)/Standalone/filesystem-src/fuzixfs.dsk > fuzix-$(REL)/fuzixfs_ide.img
	cp README.dist fuzix-$(REL)
	# make a zip
	zip -jr fuzix-$(REL).zip fuzix-$(REL)
	# make a tar
	tar -czf fuzix-$(REL).tar.gz fuzix-$(REL)

clean:
	make -C cbe -f Makefile.6809 clean
	make -C decb -f Makefile.6809 clean
	make -C bfc -f Makefile.6809 clean
	make -C tcl -f Makefile.6809 clean
	rm -f *~ fuzix.zip fuzix-0.1



