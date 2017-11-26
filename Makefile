CC=cc
CCFLAGS= -ansi
DEFS=

all: netci

netci: debug

netci.ini:
	cp src/netci.ini .

release: src/release.bld src/netci netci.ini std.db version

debug: src/debug.bld src/netci netci.ini std.db version

src/release.bld: src/autoconf.c
	( if [ -f src/debug.bld ] ; then rm -f src/debug.bld ; fi )
	$(CC) $(CCFLAGS) $(DEFS) src/autoconf.c -o src/autoconf
	( cd src ; ./autoconf )
	touch src/release.bld

src/debug.bld: src/autoconf.c
	( if [ -f src/release.bld ] ; then rm -f src/release.bld ; fi )
	$(CC) $(CCFLAGS) $(DEFS) -DDEBUG src/autoconf.c -o src/autoconf
	( cd src ; ./autoconf )
	touch src/debug.bld

src/netci:
	( cd src ; make netci )
	mv src/netci .

std.db:
	./netci -create -noisy -save=std.db

version:
	./netci -version

clean:
	( if [ -f netci ] ; then rm -f netci ; fi )
	( if [ -f gmon.out ] ; then rm -f gmon.out ; fi )
	( if [ -f src/autoconf.h ] ; then rm -f src/autoconf.h ; fi )
	( if [ -f src/autoconf ] ; then rm -f src/autoconf ; fi )
	( if [ -f src/Makefile ] ; then rm -f src/Makefile ; fi )
	( if [ -f src/release.bld ] ; then rm -f src/release.bld ; fi )
	( if [ -f src/debug.bld ] ; then rm -f src/debug.bld ; fi )
	touch src/dummy.o
	rm -f src/*.o

spotless: clean
	( if [ -f netci.ini ] ; then rm -f netci.ini ; fi )
	( if [ -f syslog.txt ] ; then rm -f syslog.txt ; fi )
	( if [ -f std.db ] ; then rm -f std.db ; fi )
	( if [ -f ci200fs/etc/pkg/packages.lst ] ; \
		then rm -f ci200fs/etc/pkg/packages.lst ; fi )
