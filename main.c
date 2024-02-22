#include <pspctrl.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspiofilemgr.h>
#include <pspkernel.h>
#include <psppower.h>
#include <pspsdk.h>
#include <pspthreadman.h>
#include <pspumd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define printf pspDebugScreenKprintf
#define RGB(r, g, b) ((r) | ((g) << 8) | ((b) << 16))
SceCtrlData pad;

extern void* oe_malloc(size_t size);
extern void oe_free(void*);
extern int oe_mallocinit(void);

PSP_MODULE_INFO("UMDRescue", PSP_MODULE_KERNEL, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

char* umdreadbuffer = NULL;
char discid[16] = { 0 };
char title[18] = { 0 };
char isopath[36] = { 0 };
char gtype[3] = { 0 };
char parsedTitle[17] = { 0 };
char parsedDiscId[17] = { 0 };
SceUID threadnumber, sectorsread, umdsize, isosize, dumppercent, sectorswritten, sec = -1;
SceUID umd, iso, fd, threadlist[66], st_thlist_first[66], st_thnum_first = -1;

void threadschanger(int stat, SceUID threadlist[], int threadnumber)
{
    int (*request_stat_func)(SceUID) = NULL;
    int i, j;
    SceUID selfid = sceKernelGetThreadId();

    if (stat == 1)
        request_stat_func = sceKernelResumeThread;

    else if (stat == 0)
        request_stat_func = sceKernelSuspendThread;

    SceKernelThreadInfo status;

    for (i = 0; i < threadnumber; i++) {
        int no_target = 0;
        for (j = 0; j < st_thnum_first; j++) {
            if (threadlist[i] == st_thlist_first[j] || selfid == threadlist[i]) {
                no_target = 1;
                break;
            }
        }

        sceKernelReferThreadStatus(threadlist[i], &status);
        if (!no_target)
            (*request_stat_func)(threadlist[i]);
    }
}

static int error(const char* err)
{
    int count = 11;
    pspDebugScreenClear(); // clear screen
    while (count > 0) {
        pspDebugScreenSetXY(0, 0);
        printf("%66s", "UMDRescue");
        pspDebugScreenSetXY(7, 12);
        printf("Error: %s", err);
        pspDebugScreenSetXY(7, 14);
        printf("Exiting in %d seconds...", count);
        count -= 1;
        sceKernelDelayThread(1000 * 1000);
    }
    threadschanger(1, threadlist, threadnumber);
    return 1;
}

static int start_dumper()
{
    pspDebugScreenInit(); // pspDebugScreenSetXY(X,Y) has a max of '68x34' character units (1 character = 8 pixels)
    pspDebugScreenSetBackColor(RGB(0, 0, 0)); // set background color
    pspDebugScreenSetTextColor(RGB(255, 0, 255)); // set text color
    sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, threadlist, 66, &threadnumber);
    threadschanger(0, threadlist, threadnumber);
    pspDebugScreenClear(); // clear screen

    // allocate 1MB to umdreadbuffer for read buffer - 512 (sectors) * 2048 (bytes per 1 sector) = 1048576 (1024KB, 1MB) bytes necessary to hold 512 sectors
    oe_mallocinit();
    if (sceUmdCheckMedium() == 0) // if UMD disc isn't present, quit
        if (error("UMD Disc not present"))
            return 0;

    sceUmdActivate(1, "disc0:");
    sceUmdWaitDriveStat(PSP_UMD_READY);

	
	// read offset 00000021 into gtype from "disc0:/UMD_DATA.BIN", determines whether or not the disc is a [G]ame or [V]ideo
    fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0777);
    if (fd >= 0) {
        sceIoLseek(fd, 0x21, SEEK_SET);
        sceIoRead(fd, gtype, 3);


    // read until offset 00000010 into discid variable from "disc0:/UMD_DATA.BIN"
        sceIoLseek(fd, 0, SEEK_SET);
		int i = 0;
		int j = 0;
		if(gtype[0] == 'G') {
			sceIoRead(fd, discid, 10);
			strncpy(parsedDiscId, discid, 10);
		}
		else {
			for(;i<sizeof(discid); i++) {
				if(discid[i] == '|' || discid[i] == ':') break;
				else {
					parsedDiscId[j] = discid[i];
					j++;
				}
			}
        	sceIoRead(fd, parsedDiscId, j-1);
		}

    }
	sceIoClose(fd);

    // read size of UMD disc to umdsize
    fd = sceIoOpen("umd0:", PSP_O_RDONLY, 0777);
    if (fd >= 0) {
        umdsize = sceIoLseek(fd, 0, SEEK_END);
        sceIoClose(fd);
    }

        // Fix this, determines content name from gtype variable and "PARAM.SFO" located in "/PSP_GAME" or "/UMD_VIDEO"
    if (gtype[0] == 'G') {
        fd = sceIoOpen("disc0:/PSP_GAME/PARAM.SFO", PSP_O_RDONLY, 0777);
        if (fd >= 0) {
            sceIoLseek(fd, 0x158, SEEK_SET);
            sceIoRead(fd, title, 17);
            title[17] = 0;
            sceIoClose(fd);
        }
    }

    else if (gtype[0] == 'V') {
        fd = sceIoOpen("disc0:/UMD_VIDEO/PARAM.SFO", PSP_O_RDONLY, 0777);
        if (fd >= 0) {
            sceIoLseek(fd, 0x74, SEEK_SET);
			int i = 0;
			int j = 0;
			for(;i<sizeof(title); i++) {
				if(title[i] == '|') break;
				else {
					parsedTitle[j] = title[i];
					j++;
				}
			}
            sceIoRead(fd, parsedTitle, j);
            sceIoClose(fd);
        }
    }

    pspDebugScreenClear(); // blank screen

    do { // While loop to present disc information until Start is pressed (exiting) or X is pressed (exiting loop and following code logic)
        sceCtrlReadBufferPositive(&pad, 1); // poll for input throughout entire function
        pspDebugScreenSetXY(0, 0);
        printf("%66s", "UMDRescue");
        pspDebugScreenSetXY(7, 10);
        printf("Title: %s", (gtype[0] == 'G') ? title : parsedTitle);
        pspDebugScreenSetXY(7, 12);
        printf("Type: %s", (gtype[0] == 'G') ? "Game" : "Video");
        pspDebugScreenSetXY(7, 14);
        printf("Disc ID: %s", (gtype[0] == 'G') ? discid : parsedDiscId);
        pspDebugScreenSetXY(7, 16);
        printf("Sectors: %d", umdsize + 1);
        pspDebugScreenSetXY(7, 18);
        printf("Size (bytes): %d", ((umdsize + 1) * 2048));
        pspDebugScreenSetXY(0, 32);
        printf("%66s", "PRESS X TO DUMP OR PRESS TRIANGLE TO EXIT");
        sceDisplayWaitVblankStart(); // Wait for vertical blank start
        if (pad.Buttons & PSP_CTRL_TRIANGLE) { // if triangle is pressed, quit program
            threadschanger(1, threadlist, threadnumber);
            return 0;
        }
    } while (!(pad.Buttons & PSP_CTRL_CROSS)); // if X is pressed, start dump logic

    umd = sceIoOpen("umd0:", PSP_O_RDONLY, 0777); // Open UMD disc in read-only mode and throw error if unsuccessful

    if (umd < 0)
        if (error("Can't open umd0:"))
            return 0;

    // declare and create "ms0:/ISO/VIDEO" directory if it doesn't already exist
    SceUID vdir = sceIoDopen("ms0:/ISO/VIDEO");
    if (vdir < 0) {
        sceIoMkdir("ms0:/ISO/VIDEO", 0777);
    }
    sceIoDclose(vdir);

    // determine location to write iso depending on gtype variable [V]ideo or [G]ame] (this logic sucks redo this and include better filename check)
    if (gtype[0] == 'G')
        sprintf(isopath, "ms0:/ISO/%s.iso", discid);
    else {
        int k = 0;
        for (; k < sizeof(parsedDiscId)-1; k++) {
            if (parsedDiscId[k] == ' ' || parsedDiscId[k] == ':')
                parsedDiscId[k] = '_';
        }
        sprintf(isopath, "ms0:/ISO/VIDEO/%s.iso", parsedDiscId);
    }

    // open iso fd from isopath for writing
    iso = sceIoOpen(isopath, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

    if (iso < 0) // if isopath contains invalid characters or ms0: is unable to be reached, this will result in ms0: error
        if (error("can't access ms0: (memory stick)"))
            return 0;

    pspDebugScreenClear(); // blank screen

    umdreadbuffer = (char*)oe_malloc(512 * 2048);

    sectorswritten = 0;

    while ((sectorsread = sceIoRead(umd, umdreadbuffer, 512))>0) {
		SceUID written = sceIoWrite(iso, umdreadbuffer, sectorsread * 0x800);
        // if memory stick runs out of space, quit
		if(written<0){
            sceIoClose(iso);
            sceIoRemove(isopath);
            if (error("Not enough free space ..."))
                return 0;
		}
        // Print status of current dump
        sectorswritten += sectorsread;
        dumppercent = (sectorswritten * 100) / umdsize;
        pspDebugScreenSetXY(0, 0);
        printf("%66s", "UMDRescue");
        pspDebugScreenSetXY(15, 11);
        printf("Writing to %s", isopath);
        pspDebugScreenSetXY(16, 15);
        printf("Writing Sectors: %d/%d - %d%% ", sectorswritten, umdsize, dumppercent);
        pspDebugScreenSetXY(16, 17);
        printf("Writing Bytes: %d/%d - %d%% ", sectorswritten * 2048, umdsize * 2048, dumppercent);
    }

    fd = sceIoOpen(isopath, PSP_O_RDONLY, 0777);
    if (fd >= 0) {
        isosize = sceIoLseek(fd, 0, SEEK_END);
        sceIoClose(fd);
    }    

    sceIoClose(iso);
    sceIoClose(umd);
    oe_free(umdreadbuffer);    
    pspDebugScreenClear(); // blank screen

    // Automatically exit after successful or failed dump and delete ISO if failed
    int count = 10;
    while (count > 0) {
        pspDebugScreenSetXY(0, 0);
        printf("%66s", "UMDRescue");
        if ((isosize) == ((umdsize+1)*2048)) {
            pspDebugScreenSetXY(7, 12);
            printf("Successfully wrote UMD:0 to %s", isopath);
        }
        else {
            pspDebugScreenSetXY(7, 12);
            printf("Failure: ISO size doesn't match UMD size");
            sceIoRemove(isopath);
        }
        pspDebugScreenSetXY(7, 14);
        printf("Exiting in %d seconds...", count);
        count -= 1;
        sceKernelDelayThread(1000 * 1000);
    }
    threadschanger(1, threadlist, threadnumber);
    return 0;
}

static int umdrescue_thread(SceSize args, void* argp)
{
    while (1) {
        sceKernelDelayThread(50000);
        sceCtrlPeekBufferPositive(&pad, 1);

        if ((pad.Buttons & PSP_CTRL_SQUARE) == PSP_CTRL_SQUARE)
            start_dumper();
    }
    return 0;
}

int module_start(SceSize args, void* argp)
{
    int thid;
    thid = sceKernelCreateThread("umdrescue_thread", umdrescue_thread, 30, 0x10000, 0, NULL);
    if (thid >= 0) {
        sceKernelStartThread(thid, args, argp);
    }
    return 0;
}

int module_stop(SceSize args, void* argp) { return 0; }

