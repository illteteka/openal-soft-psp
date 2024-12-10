TARGET_LIB = libopenal.a
TARGET := OpenAL
PSP_FW_VERSION = 371
SOURCE_DIR := src

ALCOBJS :=		$(SOURCE_DIR)/Alc/ALc.o \
				$(SOURCE_DIR)/Alc/ALu.o \
				$(SOURCE_DIR)/Alc/alcConfig.o \
				$(SOURCE_DIR)/Alc/alcReverb.o \
				$(SOURCE_DIR)/Alc/alcRing.o \
				$(SOURCE_DIR)/Alc/alcThread.o \
				$(SOURCE_DIR)/Alc/bs2b.o \
				$(SOURCE_DIR)/Alc/wave.o \
				$(SOURCE_DIR)/Alc/psp.o

LIBOBJS :=		$(ALCOBJS) \
				$(SOURCE_DIR)/OpenAL32/alAuxEffectSlot.o \
				$(SOURCE_DIR)/OpenAL32/alBuffer.o \
				$(SOURCE_DIR)/OpenAL32/alEffect.o \
				$(SOURCE_DIR)/OpenAL32/alError.o \
				$(SOURCE_DIR)/OpenAL32/alExtension.o \
				$(SOURCE_DIR)/OpenAL32/alFilter.o \
				$(SOURCE_DIR)/OpenAL32/alListener.o \
				$(SOURCE_DIR)/OpenAL32/alSource.o \
				$(SOURCE_DIR)/OpenAL32/alState.o \
				$(SOURCE_DIR)/OpenAL32/alThunk.o

OBJS :=			$(LIBOBJS)

INCDIR :=		$(INCDIR) \
				$(SOURCE_DIR) \
				$(SOURCE_DIR)/include/ \
				$(SOURCE_DIR)/OpenAL32/include/

DEFINES :=		-DPSP

CFLAGS :=		$(DEFINES) -O2 -G0 -ggdb -Wall -DHAVE_AV_CONFIG_H -fno-strict-aliasing -fverbose-asm
CXXFLAGS :=		$(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS :=		$(CFLAGS)

PSPSDK :=		$(PSPDEV)/psp/sdk
PSPDIR :=		$(PSPDEV)/psp

include			$(PSPSDK)/lib/build.mak

lib: $(STATICLIB)

$(STATICLIB): $(LIBOBJS)
	$(AR) rcs $@ $(LIBOBJS)
	$(RANLIB) $@

install: lib
	install -d $(DESTDIR)$(PSPDIR)/lib
	install -m644 $(TARGET_LIB) $(DESTDIR)$(PSPDIR)/lib

	install -d $(DESTDIR)$(PSPDIR)/include/AL/
	install -m644 \
		$(SOURCE_DIR)/include/AL/al.h \
		$(SOURCE_DIR)/include/AL/alc.h \
		$(SOURCE_DIR)/include/AL/alext.h \
		$(DESTDIR)$(PSPDIR)/include/AL/

	install -d $(DESTDIR)$(PSPDIR)/include/OpenAL
	install -m644 \
		$(SOURCE_DIR)/OpenAL32/include/alAuxEffectSlot.h \
		$(SOURCE_DIR)/OpenAL32/include/alBuffer.h \
		$(SOURCE_DIR)/OpenAL32/include/alEffect.h \
		$(SOURCE_DIR)/OpenAL32/include/alError.h \
		$(SOURCE_DIR)/OpenAL32/include/alExtension.h \
		$(SOURCE_DIR)/OpenAL32/include/alFilter.h \
		$(SOURCE_DIR)/OpenAL32/include/alListener.h \
		$(SOURCE_DIR)/OpenAL32/include/alMain.h \
		$(SOURCE_DIR)/OpenAL32/include/alReverb.h \
		$(SOURCE_DIR)/OpenAL32/include/alSource.h \
		$(SOURCE_DIR)/OpenAL32/include/alState.h \
		$(SOURCE_DIR)/OpenAL32/include/alThunk.h \
		$(SOURCE_DIR)/OpenAL32/include/alu.h \
		$(SOURCE_DIR)/OpenAL32/include/bs2b.h \
		$(DESTDIR)$(PSPDIR)/include/OpenAL/
