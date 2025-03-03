TARGET := UMDRescue
VERSION := v2.1.2
UNIXTIME := "$(shell date +%s)"
RELEASENAME := "$(TARGET)-$(VERSION)-($(UNIXTIME))"

EXTRA_TARGETS := EBOOT.PBP
PSP_EBOOT_ICON := res/ICON0.PNG
PSP_EBOOT_SND := res/sound.at3
PSP_EBOOT_TITLE := Universal UMD Dumper

#_PSP_FW_VERSION := 150

PSPSDK := $(shell psp-config --pspsdk-path)
PSPDEV := $(shell psp-config --psp-prefix)

override OBJS := main.o oe_malloc.o
override INCLUDE := -I. -I$(PSPDEV)/include -I$(PSPSDK)/include
override CC := psp-gcc
override CFLAGS := $(INCLUDE) -std=c99 -Os -G0 -O2 -Wall -fdiagnostics-color=always
override CXXFLAGS := $(CFLAGS) -fno-exceptions -fno-rtti
override ASFLAGS := $(CFLAGS)
override LIBS = -lpspumd -lpspdebug -lpspdisplay -lpspge -lpspctrl -lpspnet -lpspnet_apctl
override LDFLAGS = -L. -L$(PSPDEV)/lib -L$(PSPSDK)/lib -Wl,-zmax-page-size=128

include $(PSPSDK)/lib/build.mak

kxploit:
	@echo KXploit EBOOT
	@rm -rf PSP/
	@mkdir -p PSP/GAME150/__SCE__$(TARGET)
	@mkdir -p "PSP/GAME150/%__SCE__$(TARGET)"
	@rm -f EBOOT.PBP data.psp
	@pack-pbp EBOOT.PBP PARAM.SFO $(PSP_EBOOT_ICON) NULL NULL NULL $(PSP_EBOOT_SND) NULL NULL
	@cp EBOOT.PBP PSP/GAME150/%__SCE__$(TARGET)/EBOOT.PBP
	@cp $(TARGET).elf PSP/GAME150/__SCE__$(TARGET)/EBOOT.PBP

standalone:
	@echo Standalone EBOOT
	@mkdir -p PSP/GAME/$(TARGET)
	$(CC) $(CFLAGS) $(OBJS) -specs=$(PSPSDK)/lib/prxspecs -Wl,-q,-T $(PSPSDK)/lib/linkfile.prx $(LDFLAGS) $(LIBS) -o $(TARGET).elf
	@psp-fixup-imports UMDRescue.elf
	@psp-prxgen $(TARGET).elf $(TARGET).prx
	@pack-pbp EBOOT.PBP PARAM.SFO $(PSP_EBOOT_ICON) NULL NULL NULL $(PSP_EBOOT_SND) $(TARGET).prx NULL
	@./psptools/pack_ms_game.py --vanity UMDRescue EBOOT.PBP EBOOT_ENC.PBP && mv EBOOT_ENC.PBP EBOOT.PBP
	@cp EBOOT.PBP PSP/GAME/$(TARGET)/EBOOT.PBP

ifeq ($(MAKECMDGOALS),standalone)
BUILD_PRX = 1
endif

all: kxploit standalone
	if command -v 7z &> /dev/null;then 7z a release/$(RELEASENAME).7z ./PSP/*;fi

clean:
	rm -rf *.o data.psp *.PBP PSP/ *.elf *.prx PARAM* release psptools/psptool/__pycache__/ psptools/psptool/prxtypes/__pycache__/

rebuild: clean all