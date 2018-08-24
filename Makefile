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
	./build-filesystem-ng -X -y -f fuzixfs.dsk -g 256 65535
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

pack:
	# make dist directory
	mkdir -p fuzix-0.1
	cp boot/boot.dsk fuzix-0.1
	cp boot/boot2.dsk fuzix-0.1
	cp $(FUZIX_DIR)/Standalone/filesystem-src/fuzixfs.dsk fuzix-0.1
	cp README.dist fuzix-0.1
	# make a zip
	zip -jr fuzix-0.1.zip fuzix-0.1
	# make a tar
	tar -czf fuzix-0.1.tar.gz fuzix-0.1

clean:
	make -C cbe -f Makefile.6809 clean
	make -C decb -f Makefile.6809 clean
	make -C bfc -f Makefile.6809 clean
	make -C tcl -f Makefile.6809 clean
	rm -f *~ fuzix.zip fuzix-0.1



