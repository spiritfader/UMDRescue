TARGET = UMDRescue

OBJS = main.o oe_malloc.o
#OBJS = main.o
#BUILD_PRX = 1

EXTRA_TARGETS = EBOOT.PBP

PSP_EBOOT_ICON = ICON0.PNG

PSP_EBOOT_TITLE = Universal UMD Dumper

#PSP_FW_VERSION = 150

CFLAGS = -O2 -G0 -Wall 
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

VERSION = 01.00


#INCDIR = -I./inc

LIBS = -lpspumd #-lpspexploit -lpsprtc


PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

LIBDIR = -L$(PSPSDK)/lib

kxploit:
	@echo KXploit EBOOT
	@rm -rf PSP/
	@mkdir -p PSP/GAME150/$(TARGET)
	@mkdir -p "PSP/GAME150/$(TARGET)%"
	@rm -f EBOOT.PBP data.psp
	@pack-pbp EBOOT.PBP PARAM.SFO ICON0.PNG NULL NULL NULL NULL NULL NULL
	@cp EBOOT.PBP PSP/GAME150/$(TARGET)%/EBOOT.PBP
	@cp $(TARGET).elf PSP/GAME150/$(TARGET)/EBOOT.PBP

standalone:
	@echo Standalone EBOOT
	@mkdir -p  PSP/GAME/$(TARGET)
	$(CC) $(CFLAGS) $(OBJS) -specs=$(PSPSDK)/lib/prxspecs -Wl,-q,-T$(PSPSDK)/lib/linkfile.prx $(LDFLAGS)  $(LIBS) -o $(TARGET).elf
	@psp-fixup-imports UMDRescue.elf
	@psp-prxgen  $(TARGET).elf $(TARGET).prx
	@make -C pencrypt/ 
	@pencrypt/pencrypt $(TARGET).prx
	@pack-pbp EBOOT.PBP PARAM.SFO ICON0.PNG NULL NULL NULL NULL data.psp NULL
	@cp EBOOT.PBP PSP/GAME/$(TARGET)/EBOOT.PBP


all: kxploit standalone

ifeq ($(MAKECMDGOALS),standalone)
BUILD_PRX = 1
endif

clean:
	rm -rf *.o data.psp *.PBP PSP/ *.elf *.prx
	make -C pencrypt clean

