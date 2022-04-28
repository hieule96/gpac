include ../../../config.mak

vpath %.c $(SRC_PATH)/applications/testapps/SendRTP

CFLAGS= $(OPTFLAGS) -I"$(SRC_PATH)/include"

CFLAGS+=-g
LDFLAGS+=-g


#common obj
OBJS= RTP_packetizer.o RTP_send.o debug.o MPEG_hintTrackProcess.o

ifeq ($(CONFIG_WIN32),yes)
EXE=.exe
PROG=RTP_transmitVideo$(EXE)
else
EXT=
PROG=RTP_transmitVideo etmp4
endif

LINKFLAGS=-L ../../../bin/gcc
LINKFLAGS+=$(shell pkg-config ../../../gpac.pc --libs --static | sed 's/-lgpac //'  )
LINKFLAGS+=$(GPAC_SH_FLAGS)
LINKFLAGS+=$(EXTRALIBS)
LINKFLAGS+=-lgpac_static
SRCS := $(OBJS:.o=.c) 

all: $(PROG)

RTP_transmitVideo: RTP_packetizer.o RTP_send.o debug.o timing.o MPEG_hintTrackProcess.o RTP_transmitVideo.o
	$(CC) $^ -o $@ $(LINKFLAGS) $(LDFLAGS)

etmp4: writeMP4.o etmp4.o
	$(CC) $^ -o $@ $(LINKFLAGS) $(LDFLAGS)


clean: 
	rm -f $(OBJS) $(PROG)

dep: depend

depend:
	rm -f .depend	
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

distclean: clean
	rm -f Makefile.bak .depend

-include .depend