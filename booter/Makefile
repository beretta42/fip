all: booter


booter:
	lwasm -f obj -o crt0.o crt0.s
	m6809-unknown-gcc -fno-builtin -c -obooter.o booter.c
	lwlink -o boot.bin -fdecb crt0.o booter.o -mboot.map
	rm -f boot.dsk
	decb dskini boot.dsk
	decb copy -b -2 boot.bin boot.dsk,BOOT.BIN

clean:
	rm -f *~ *.o *.dsk *.map *.bin
