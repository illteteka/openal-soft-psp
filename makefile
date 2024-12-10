PSPPATH := $(shell psp-config --psp-prefix)
PSPSDK := $(shell psp-config --pspsdk-path)
ARCH = psp-

CC = $(ARCH)gcc -std=gnu99
AR = $(ARCH)ar
RANLIB = $(ARCH)ranlib
RM = rm -f
INCFLAGS = -I. -I./include -I./OpenAL32/Include -I$(PSPPATH)/include -I$(PSPSDK)/include
CFLAGS = -g -Wall -Wmissing-prototypes -O3 -G0 -fsingle-precision-constant -funit-at-a-time -DPSP -DAL_BUILD_LIBRARY $(INCFLAGS)
LFLAGS = -g -Wall -O3 -G0 -L$(PSPPATH)/lib

OPENAL_OBJS = 					\
	OpenAL32/alAuxEffectSlot.o	\
	OpenAL32/alBuffer.o			\
	OpenAL32/alEffect.o			\
	OpenAL32/alError.o			\
	OpenAL32/alExtension.o		\
	OpenAL32/alFilter.o			\
	OpenAL32/alListener.o		\
	OpenAL32/alSource.o			\
	OpenAL32/alState.o			\
	OpenAL32/alThunk.o

ALC_OBJS = 				\
	Alc/ALc.o			\
	Alc/ALu.o			\
	Alc/alcConfig.o		\
	Alc/alcReverb.o		\
	Alc/alcRing.o		\
	Alc/alcThread.o		\
	Alc/bs2b.o			\
	Alc/wave.o			\
	Alc/psp.o

libOpenAL32.a_OBJS = $(OPENAL_OBJS) $(ALC_OBJS)

all: $(libOpenAL32.a_OBJS) libOpenAL32.a

%.a: $(libOpenAL32.a_OBJS)
	$(RM) $@
	$(AR) cru $@ $($@_OBJS)
	$(RANLIB) $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -rf *.o *.a ./Alc/*.o ./OpenAL32/*.o

install: all
	mkdir -p $(PSPPATH)/include
	mkdir -p $(PSPPATH)/include/AL
	cp ./include/AL/*.h $(PSPPATH)/include/AL
	mkdir -p $(PSPPATH)/lib
	cp libOpenAL32.a $(PSPPATH)/lib
