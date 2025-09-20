TARGET := UMDRescue
VERSION := 2.21
UNIXTIME := "$(shell date +%s)"
RELEASENAME := "$(TARGET)-$(VERSION)-($(UNIXTIME))"
BUILD := \"$(TARGET)\ $(VERSION)\"

PSP_EBOOT_TITLE := $(TARGET)
PSP_EBOOT_SFO := PARAM.SFO
PSP_EBOOT_ICON := assets/icon0.png
PSP_EBOOT_ICON1 := NULL
PSP_EBOOT_UNKPNG := NULL
PSP_EBOOT_PIC1 := assets/pic1.png
PSP_EBOOT_SND0 := assets/sound.at3
PSP_EBOOT_PSAR := NULL
PSP_EBOOT := EBOOT.PBP

_PSP_FW_VERSION := 661
PSP_LARGE_MEMORY = 0

override SFOFLAGS := -d MEMSIZE=0 -s PARENTAL_CONTROL=0 -s DISC_ID=UMDRESCUE -s VER=$(VERSION) -s DISC_TOTAL=1 -s DISC_NUMBER=1 \
-s DISC_VERSION=$(VERSION) -s REGION=32768 -s BOOTABLE=1 -s DRIVER_PATH=NULL -s PSP_SYSTEM_VER=1.00 -s CATEGORY=MG

PSPSDK := $(shell psp-config --pspsdk-path)
PSPDEV := $(shell psp-config --psp-prefix)

DEFINE := -DBUILD=$(BUILD) -D_PSP_FW_VERSION=$(_PSP_FW_VERSION)
WARNINGS := -Wall -Wextra -Wno-incompatible-pointer-types
TESTING := -Werror -Wshadow -Wpedantic

override OBJS := main.o
override INCLUDE := -I. -I$(PSPDEV)/include -I$(PSPSDK)/include
override CC := psp-gcc
override CFLAGS := $(INCLUDE) -march=allegrex -mtune=allegrex -std=c23 -g -G0 -O2 -Os -fdiagnostics-color=always -fno-strict-aliasing \
$(DEFINE) $(WARNINGS) $(TESTING)
override CXXFLAGS := $(CFLAGS) -fno-exceptions -fno-rtti
override ASFLAGS := $(CFLAGS)
override LIBS = -lpspumd -lpspdebug -lpspdisplay -lpspge -lpspctrl -lpspnet -lpspnet_apctl -lpsppower
override LDFLAGS = -L. -L$(PSPDEV)/lib -L$(PSPSDK)/lib -Wl,-zmax-page-size=128

include $(PSPSDK)/lib/build.mak

SFOFLAGS := -d MEMSIZE=$(EXPAND_MEMORY) $(SFOFLAGS)

kxploit:
	@echo; echo KXploit EBOOT
	@rm -rf PSP/
	@mkdir -p release/GAME150/__SCE__$(TARGET)
	@mkdir -p "release/GAME150/%__SCE__$(TARGET)"
	@rm -f $(PSP_EBOOT) data.psp
	@pack-pbp $(PSP_EBOOT) $(PSP_EBOOT_SFO) $(PSP_EBOOT_ICON) $(PSP_EBOOT_ICON1) $(PSP_EBOOT_UNKPNG) $(PSP_EBOOT_PIC1) $(PSP_EBOOT_SND0) NULL $(PSP_EBOOT_PSAR)
	@cp $(PSP_EBOOT) release/GAME150/%__SCE__$(TARGET)/$(PSP_EBOOT)
	@cp $(TARGET).elf release/GAME150/__SCE__$(TARGET)/$(PSP_EBOOT)

standalone:
	@echo; echo Standalone EBOOT
	@mkdir -p release/GAME/$(TARGET)
	$(CC) $(CFLAGS) $(OBJS) -specs=$(PSPSDK)/lib/prxspecs -Wl,-q,-T $(PSPSDK)/lib/linkfile.prx $(LDFLAGS) $(LIBS) -o $(TARGET).elf
	@psp-fixup-imports $(TARGET).elf
	@psp-prxgen $(TARGET).elf $(TARGET).prx
	@pack-pbp $(PSP_EBOOT) $(PSP_EBOOT_SFO) $(PSP_EBOOT_ICON) $(PSP_EBOOT_ICON1) $(PSP_EBOOT_UNKPNG) \
	$(PSP_EBOOT_PIC1) $(PSP_EBOOT_SND0) $(TARGET).prx $(PSP_EBOOT_PSAR)
	@./psptools/pack_ms_game.py --vanity $(TARGET) $(PSP_EBOOT) EBOOT_ENC.PBP && mv EBOOT_ENC.PBP $(PSP_EBOOT)
	@cp $(PSP_EBOOT) release/GAME/$(TARGET)/$(PSP_EBOOT)


ifeq ($(MAKECMDGOALS),standalone)
BUILD_PRX = 1
endif

all: kxploit standalone
	@echo
	if command -v 7z > /dev/null;then 7z a release/$(RELEASENAME).zip release/GAME release/GAME150 > /dev/null;fi

clean:
	@echo
	rm -rf *.o data.psp *.PBP PSP/ *.elf *.prx PARAM* release psptools/psptool/__pycache__/ psptools/psptool/prxtypes/__pycache__/

rebuild: clean all