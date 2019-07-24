all: booter

booter:
	lwasm -f obj -o crt0.o crt0.s
	lwasm -f obj -o dw_ll.o dw_ll.s
	m6809-unknown-gcc -I./ -fno-builtin -c -o booter.o booter.c
	m6809-unknown-gcc -I./ -fno-builtin -c -o dw.o dw.c
	lwlink -s BASIC.link -o boot.bin -fdecb \
		crt0.o booter.o dw_ll.o dw.o -mboot.map
	rm -f boot.dsk
	decb dskini boot.dsk
	decb copy -b -2 boot.bin boot.dsk,BOOT.BIN
	decb copy -a -0 -l AUTOEXEC.BAS boot.dsk,AUTOEXEC.BAS

clean:
	rm -f *~ *.o *.dsk boot.map *.bin
