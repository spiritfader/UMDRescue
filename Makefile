TARGET = UMDRescue

VERSION = v2.1.2

UNIXTIME := $(shell date +%s)

OBJS = main.o oe_malloc.o

EXTRA_TARGETS = EBOOT.PBP

PSP_EBOOT_ICON = res/ICON0.PNG

PSP_EBOOT_TITLE = Universal UMD Dumper

#PSP_FW_VERSION = 150

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = -lpspumd #-lpspexploit -lpsprtc

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

kxploit:
	@echo KXploit EBOOT
	@rm -rf PSP/
	@mkdir -p PSP/GAME150/__SCE__$(TARGET)
	@mkdir -p "PSP/GAME150/%__SCE__$(TARGET)"
	@rm -f EBOOT.PBP data.psp
	@pack-pbp EBOOT.PBP PARAM.SFO res/ICON0.PNG NULL NULL NULL res/sound.at3 NULL NULL
	@cp EBOOT.PBP PSP/GAME150/%__SCE__$(TARGET)/EBOOT.PBP
	@cp $(TARGET).elf PSP/GAME150/__SCE__$(TARGET)/EBOOT.PBP

standalone:
	@echo Standalone EBOOT
	@mkdir -p PSP/GAME/$(TARGET)
	$(CC) $(CFLAGS) $(OBJS) -specs=$(PSPSDK)/lib/prxspecs -Wl,-q,-T$(PSPSDK)/lib/linkfile.prx $(LDFLAGS) $(LIBS) -o $(TARGET).elf
	@psp-fixup-imports UMDRescue.elf
	@psp-prxgen $(TARGET).elf $(TARGET).prx
	@pack-pbp EBOOT.PBP PARAM.SFO res/ICON0.PNG NULL NULL NULL res/sound.at3 $(TARGET).prx NULL
	@./psptools/pack_ms_game.py --vanity UMDRescue EBOOT.PBP EBOOT_ENC.PBP && mv EBOOT_ENC.PBP EBOOT.PBP
	@cp EBOOT.PBP PSP/GAME/$(TARGET)/EBOOT.PBP

ifeq ($(MAKECMDGOALS),standalone)
BUILD_PRX = 1
endif

all: kxploit standalone
	if command -v 7z &> /dev/null;then 7z a release/$(TARGET)-$(VERSION)-"($(UNIXTIME))".7z ./PSP/*;fi

clean:
	rm -rf *.o data.psp *.PBP PSP/ *.elf *.prx PARAM* release psptools/psptool/__pycache__/ psptools/psptool/prxtypes/__pycache__/
