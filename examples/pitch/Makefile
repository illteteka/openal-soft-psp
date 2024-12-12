TARGET = openal_example
OBJS = main.o

INCDIR = 
CFLAGS = -O2 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS = -lopenal -lpspaudio -lpsphprm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = OpenAL Soft example

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak