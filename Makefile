TARGET = UMDKiller

BUILD_PRX = 1

#CLASSG_LIBS = libs

INCDIR = $(CLASSG_LIBS)
CFLAGS = -Os -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =

LIBS = -lpspreg -lpspumd -lpsprtc 
#LIBS = -lz -lpspsdk -lpspctrl -lpspumd -lpsprtc  -lpspgu  -lm -lpsphprm

OBJS =  main.o

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
