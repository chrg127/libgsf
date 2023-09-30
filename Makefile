CC=gcc
CPP=g++
LD=$(CPP)

CFLAGS=-DLINUX -I./VBA -DVERSION_STR=\"0.07\" -DHA_VERSION_STR=\"0.11\" -I./libresample-0.1.3/include -O3 -DC_CORE
CXXFLAGS=-g -O2
LDFLAGS=-lz -lresample -L./libresample-0.1.3 -lao

OBJS=gsf.o VBA/GBA.o VBA/Globals.o VBA/Sound.o VBA/Util.o VBA/bios.o VBA/memgzio.o VBA/snd_interp.o VBA/unzip.o linuxmain.o VBA/psftag.o

all: libresample-0.1.3/libresample.a $(OBJS) 
	$(LD) $(OBJS) -lresample -o playgsf $(LDFLAGS)

libresample-0.1.3/libresample.a: libresample-0.1.3/Makefile
	$(MAKE) -C libresample-0.1.3

libresample-0.1.3/Makefile:
	cd libresample-0.1.3 ; ./configure ; cd ..

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CPP) $(CFLAGS) -c $< -o $@

%.o: %.cpp %.h
	$(CPP) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o VBA/*.o playgsf autom4te.cache libresample-0.1.3/Makefile libresample-0.1.3/config.log libresample-0.1.3/config.status libresample-0.1.3/src/*.o

distclean: 
	rm -f *.o VBA/*.o playgsf config.cache config.status Makefile config.h config.log libresample-0.1.3/src/*.o
