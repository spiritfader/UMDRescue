TARGET = UMDKiller

OBJS = stubkk.o main.o
#OBJS = scePower.o oe_malloc.o main.o

BUILD_PRX = 1

PSP_FW_VERSION = 660

PSP_EXPORTS = exports.exp

CFLAGS = -Os -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR = $(PSPSDK)/lib /usr/local/pspdev/psp/lib ./lib
LDFLAGS = -nostartfiles

LIBS = -lpspsystemctrl_kernel -lpspumd -lpsprtc -lpspreg -lpspdebug -lpspkernel

all:
	psp-packer $(TARGET).prx

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
