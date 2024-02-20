TARGET = UMDKiller

OBJS = oe_malloc.o main.o

BUILD_PRX = 1

PSP_FW_VERSION = 660

CFLAGS = -Os -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR = $(PSPSDK)/lib /usr/local/pspdev/psp/lib ./lib
LDFLAGS = -nostartfiles

LIBS = -lpspumd -lpsprtc -lpspdebug -lpspkernel

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
