TARGET = UMDRescue

OBJS = main.o oe_malloc.o
#OBJS = main.o

BUILD_PRX = 1

EXTRA_TARGETS = EBOOT.PBP

PSP_EBOOT_TITLE = Universal UMD Dumper

PSP_FW_VERSION = 660

CFLAGS = -O2 -G0 -Wall 
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR = -L$(PSPSDK)/lib -L./lib
#LDFLAGS = -nostartfiles

#INCDIR = -I./inc

LIBS = -lpspumd #-lpspexploit -lpsprtc


PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

all: package


package: 
	@rm -rf PSP/
	@mkdir -p  PSP/GAME/$(TARGET)
	@mkdir -p PSP/GAME150/$(TARGET)
	@mkdir -p "PSP/GAME150/$(TARGET)%"
	@make -C pencrypt/ 
	@pencrypt/pencrypt $(TARGET).prx
	@pack-pbp EBOOT.PBP PARAM.SFO ICON0.PNG NULL NULL NULL NULL data.psp NULL
	@cp EBOOT.PBP PSP/GAME/$(TARGET)/EBOOT.PBP
	@pack-pbp EBOOT.PBP PARAM.SFO ICON0.PNG NULL NULL NULL NULL NULL NULL
	@cp EBOOT.PBP PSP/GAME150/$(TARGET)%/EBOOT.PBP
	@cp $(TARGET).prx PSP/GAME150/$(TARGET)/EBOOT.PBP
	
clean:
	rm -rf *.o data.psp *.PBP PSP/ *.elf *.prx
	make -C pencrypt clean

