
all:
	make -C cbe -f Makefile.6809 cbe install
	make -C decb -f Makefile.6809 decb install
	make -C bfc -f Makefile.6809 bfc install

clean:
	make -C cbe -f Makefile.6809 clean
	make -C decb -f Makefile.6809 clean
	make -C bfc -f Makefile.6809 clean
	rm *~



