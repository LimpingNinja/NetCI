# NetCI Root Makefile
# Run 'make' or 'make debug' from the root directory
#
# This Makefile automatically:
# - Generates src/autoconf.h if missing (required for compilation)
# - Builds the NetCI engine in src/
# - Moves the binary to the root directory
# - Creates std.db if needed

CC=cc
CCFLAGS=-ansi
DEFS=

all: netci

netci: debug

netci.ini:
	cp src/netci.ini .

release: src/release.bld src/netci netci.ini std.db version

debug: src/debug.bld src/netci netci.ini std.db version

src/autoconf.h: src/autoconf.c
	@echo "Generating autoconf.h..."
	$(CC) $(CCFLAGS) $(DEFS) src/autoconf.c -o src/autoconf
	cd src && ./autoconf
	@echo "autoconf.h generated successfully"

src/release.bld: src/autoconf.h
	@if [ -f src/debug.bld ] ; then rm -f src/debug.bld ; fi
	@if [ ! -f src/autoconf.h ] ; then $(MAKE) src/autoconf.h ; fi
	touch src/release.bld

src/debug.bld: src/autoconf.h
	@if [ -f src/release.bld ] ; then rm -f src/release.bld ; fi
	@if [ ! -f src/autoconf.h ] ; then $(MAKE) src/autoconf.h ; fi
	touch src/debug.bld

src/netci: src/autoconf.h
	cd src && $(MAKE) netci
	mv src/netci .

std.db:
	./netci -create -noisy -save=std.db

version:
	./netci -version

clean:
	@if [ -f netci ] ; then rm -f netci ; fi
	@if [ -f gmon.out ] ; then rm -f gmon.out ; fi
	@if [ -f src/autoconf.h ] ; then rm -f src/autoconf.h ; fi
	@if [ -f src/autoconf ] ; then rm -f src/autoconf ; fi
	@if [ -f src/release.bld ] ; then rm -f src/release.bld ; fi
	@if [ -f src/debug.bld ] ; then rm -f src/debug.bld ; fi
	@touch src/dummy.o
	@rm -f src/*.o

spotless: clean
	@if [ -f netci.ini ] ; then rm -f netci.ini ; fi
	@if [ -f syslog.txt ] ; then rm -f syslog.txt ; fi
	@if [ -f std.db ] ; then rm -f std.db ; fi
	@if [ -f libs/ci200fs/etc/pkg/packages.lst ] ; then rm -f libs/ci200fs/etc/pkg/packages.lst ; fi

.PHONY: all netci release debug version clean spotless
