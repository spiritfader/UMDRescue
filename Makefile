TARGET = UMDRescue

OBJS = main.o oe_malloc.o

EXTRA_TARGETS = EBOOT.PBP

PSP_EBOOT_ICON = ICON0.PNG

PSP_EBOOT_TITLE = Universal UMD Dumper

#PSP_FW_VERSION = 150

CFLAGS = -O2 -G0 -Wall 
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = -lpspumd #-lpspexploit -lpsprtc

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

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
	@pack-pbp EBOOT.PBP PARAM.SFO ICON0.PNG NULL NULL NULL NULL $(TARGET).prx NULL
	@./psptools/pack_ms_game.py --vanity UMDRescue EBOOT.PBP EBOOT_ENC.PBP && mv EBOOT_ENC.PBP EBOOT.PBP
	@cp EBOOT.PBP PSP/GAME/$(TARGET)/EBOOT.PBP

ifeq ($(MAKECMDGOALS),standalone)
BUILD_PRX = 1
endif

all: kxploit standalone

clean:
	rm -rf *.o data.psp *.PBP PSP/ *.elf *.prx
