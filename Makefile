#
# Makefile
#

# Configuration
DEST	?= .
#CFLAGS	:= -O2
CFLAGS	:= -O0 -ggdb3
CFLAGS	+= -DCONFIG_FAST_UNWIND
CFLAGS	+= -DCONFIG_PRINTVARS
#GLIB	:= -DCONFIG_GLIB $(shell pkg-config --cflags --libs glib-2.0)

# Variables
CFLAGS	+= -Ideps -Ldeps/$(ARCH)
ARFLIBS	:= -l:libdw_pic.a -l:libelf_pic.a -ldl
THREADS	:= -D_THREAD_SAFE -lpthread

# Fucking gcc in scratchbox/x86 generates code for the i386,
# which doesn't have cmpxchg8.
ifeq ($(shell uname -m), i486)
EROFLAGS := -march=i486
CFLAGS	 += $(EROFLAGS)
endif

FUCK_MAKE := $(shell mkdir -p $(DEST))

# Rules
all:	barf libero
barf:	$(DEST)/libarf.so
libero:	$(DEST)/libero.so $(DEST)/libero_mt.so

# Scripts
ifneq ($(DEST),.)
$(DEST)/arf: arf
	cp -f $< $@;
$(DEST)/ero: $(DEST)/arf
	ln -s arf $@;
$(DEST)/mtero: $(DEST)/ero
	ln -s mtero $@;
endif

# Libraries
$(DEST)/libarf.so: libarf.c arf.h
	cc -shared -Wall $(CFLAGS) -fPIC $< $(ARFLIBS) $(GLIB) -o $@;
	chmod -x $@;
$(DEST)/libero.so: libero.c libarf.c arf.h
	cc -shared -Wall $(CFLAGS) -fPIC $< $(ARFLIBS) -o $@;
	chmod -x $@;
$(DEST)/libero_mt.so: libero.c libarf.c arf.h
	cc -shared -Wall $(CFLAGS) -fPIC $< $(ARFLIBS) $(THREADS) -o $@;
	chmod -x $@;

# Test programs
# For libarf
$(DEST)/dso.so: test_dso.c test.h arf.h
	cc -shared -Wall -g -fPIC $< -o $@;
	objcopy --only-keep-debug $@ $(basename $@).dbg;
	objcopy --strip-debug --add-gnu-debuglink=$(basename $@).dbg $@;

$(DEST)/liblib-ctlink.so: test_lib.c test.h arf.h $(DEST)/libarf.so
	cc -shared -Wall -g -fPIC -L$(DEST) -larf $< -o $@;
$(DEST)/liblib-rtlink.so: test_lib.c test.h arf.h
	cc -shared -Wall -g -fPIC                 $< -o $@;

$(DEST)/prg-ctlink: test_prg.c test_obj.c test.h arf.h
$(DEST)/prg-ctlink: $(DEST)/liblib-ctlink.so $(DEST)/dso.so
	cc -Wall -Wno-unused -g -L$(DEST) -Wl,-rpath,. -export-dynamic \
		-llib-ctlink -ldl test_prg.c test_obj.c -o $@;
$(DEST)/prg-rtlink: test_prg.c test_obj.c test.h arf.h
$(DEST)/prg-rtlink: $(DEST)/liblib-rtlink.so $(DEST)/dso.so
	cc -Wall -Wno-unused -g -L$(DEST) -Wl,-rpath,. -export-dynamic \
		-llib-rtlink -ldl test_prg.c test_obj.c -o $@;

ctlink: $(DEST)/prg-ctlink
	cd $(DEST) && ./prg-ctlink
rtlink: $(DEST)/prg-rtlink $(DEST)/libarf.so $(DEST)/arf
	cd $(DEST) && ./arf -printvars ./prg-rtlink
arftest: ctlink rtlink

# For libero
$(DEST)/testero: testero.c
	cc -Wall -g testero.c -o $@;
$(DEST)/testero_mt: testero.c
	cc -Wall -g testero.c $(THREADS) -o $@;
erotest: $(DEST)/libero.so $(DEST)/testero $(DEST)/ero
	cd $(DEST) && LIBERO_START=1 ./ero ./testero;
erotest_mt: $(DEST)/libero_mt.so testero_mt $(DEST)/mtero
	cd $(DEST) && LIBERO_START=1 ./mtero ./testero_mt;

clean:
	rm -f	$(DEST)/dso.so $(DEST)/dso.dbg				\
		$(DEST)/prg-ctlink $(DEST)/prg-rtlink			\
		$(DEST)/liblib-ctlink.so $(DEST)/liblib-rtlink.so	\
		$(DEST)/testero $(wildcard $(DEST)/testero.*.leaks)	\
		$(DEST)/testero_mt $(wildcard testero_mt.*.leaks);
xclean: clean
	rm -f	$(DEST)/libarf.so $(DEST)/libero.so $(DEST)/libero_mt.so;
	[ $(DEST)/mtero -ef mtero ] || rm -f $(DEST)/mtero;
	[ $(DEST)/ero   -ef ero   ] || rm -f $(DEST)/ero;
	[ $(DEST)/arf   -ef arf   ] || rm -f $(DEST)/arf;
	-rmdir $(DEST);

.PHONY: all barf libero ctlink rtlink arftest erotest erotest_mt clean xclean

# End of Makefile
