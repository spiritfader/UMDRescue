#include <pspsdk.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspumd.h>
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>


#define printf pspDebugScreenPrintf
#define RGB(r, g, b) ((r) | ((g) << 8) | ((b) << 16))
#define SECTOR_SIZE 0x800
SceCtrlData pad;

extern void* oe_malloc(size_t size);
extern void oe_free(void*);

PSP_MODULE_INFO("UMDRescue", PSP_MODULE_USER, 1, 0);

char* umdreadbuffer = NULL;
char discid[16] = { 0 };
char title[128] = { 0 };
char isopath[30] = { 0 };
char gtype[3] = { 0 };
char parsedTitle[128] = { 0 };
char parsedDiscId[17] = { 0 };
SceUID threadnumber, lbaread, umdlastlba, isosize, dumppercent, lbawritten, sec = -1;
SceUID umd, iso, fd, threadlist[66], st_thlist_first[66], st_thnum_first = -1;
static int color = 0;


void error(char* err)
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
}

void dump() {
	pspDebugScreenClear();

	printf("Parsing UMD data ...");

	sceKernelDelayThread(800000);
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
			strncpy(parsedDiscId, "N/A", 4);
			/*for(;i<sizeof(discid); i++) {
				if(discid[i] == '|' || discid[i] == ':') break;
				else {
					parsedDiscId[j] = discid[i];
					j++;
				}
			}
        	sceIoRead(fd, parsedDiscId, j-1);
		*/
		}

	sceIoClose(fd);
    }


    // read size (last lba) of UMD disc to umdlastlba
    fd = sceIoOpen("umd0:", PSP_O_RDONLY, 0777);
    if (fd >= 0) {
        umdlastlba = sceIoLseek(fd, 0, SEEK_END);
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
            sceIoRead(fd, title, sizeof(title));
			int i = 0;
			int j = 0;
			for(;i<sizeof(title); i++) {
				if(title[i] == '\0') {
            		sceIoLseek(fd, 0x50, SEEK_SET);
            		sceIoRead(fd, title, sizeof(title));
					if(title[i] == '\0') break;
					else {
						parsedTitle[j] = title[i];
						j++;
					}
				}
				else {
					parsedTitle[j] = title[i];
					j++;
				}
			}
            sceIoClose(fd);
        }
    }
    pspDebugScreenClear(); // blank screen
    do { // While loop to present disc information until Start is pressed (exiting) or X is pressed (exiting loop and following code logic)
		sceCtrlPeekBufferPositive(&pad, 1);
		if((pad.Buttons & PSP_CTRL_LTRIGGER)==PSP_CTRL_LTRIGGER)
			color++;
		if((pad.Buttons & PSP_CTRL_RTRIGGER)==PSP_CTRL_RTRIGGER)
			color--;

		if(color>3) color = 0;
		if(color<0) color = 3;
		if(color == 0) 
    		pspDebugScreenSetTextColor(RGB(0, 255, 0)); // Default (Green) 
		else if(color==1)
    		pspDebugScreenSetTextColor(RGB(255, 0, 255)); // (Purple)
		else if(color==2)
    		pspDebugScreenSetTextColor(RGB(255, 191, 0)); // (Amber)
		else if(color==3)
    		pspDebugScreenSetTextColor(RGB(255, 255, 255)); // (White)

        sceCtrlReadBufferPositive(&pad, 1); // poll for input throughout entire function
        pspDebugScreenSetXY(0, 0);
        printf("%66s", "UMDRescue");
        pspDebugScreenSetXY(7, 6);
        printf("Title: %s", (gtype[0] == 'G') ? title : parsedTitle);
        pspDebugScreenSetXY(7, 8);
        printf("Type: %s", (gtype[0] == 'G') ? "Game" : "Video");
        pspDebugScreenSetXY(7, 10);
        printf("Disc ID: %s", (gtype[0] == 'G') ? discid : parsedDiscId);
        pspDebugScreenSetXY(7, 12);
        printf("Sectors (Total LBA): %d", umdlastlba);
        pspDebugScreenSetXY(7, 14);
        printf("Size (bytes): %d", (umdlastlba * SECTOR_SIZE));
        pspDebugScreenSetXY(0, 31);
        printf("%68s", "(X)           (O)        (TRIANGLE)");
        pspDebugScreenSetXY(0, 32);
        printf("%68s", "DUMP            LOGS              Exit  ");
        sceDisplayWaitVblankStart(); // Wait for vertical blank start
        if (pad.Buttons & PSP_CTRL_TRIANGLE) { // if triangle is pressed, quit program
            return 0;
        }
    } while (!(pad.Buttons & PSP_CTRL_CROSS)); // if X is pressed, start dump logic
											   //

    umd = sceIoOpen("umd0:", PSP_O_RDONLY, 0777); // Open UMD disc in read-only mode and throw error if unsuccessful
    if (umd < 0) {
        error("Can't open umd0:");
		return 1;
	}
    // declare and create "ms0:/ISO/VIDEO" directory if it doesn't already exist
    SceUID vdir = sceIoDopen("ms0:/ISO/VIDEO");
    if (vdir < 0) {
        sceIoMkdir("ms0:/ISO", 0777);
        sceIoMkdir("ms0:/ISO/VIDEO", 0777);
    }
    sceIoDclose(vdir);

    // determine location to write iso depending on gtype variable [V]ideo or [G]ame] (this logic sucks redo this and include better filename check)
    if (gtype[0] == 'G')
        sprintf(isopath, "ms0:/ISO/%s.iso", discid);
    else {
        int k = 0;
        for (; k < sizeof(parsedTitle); k++) {
            if (parsedTitle[k] == ' ' || parsedTitle[k] == ':' || parsedTitle[k] == ',')
				if(parsedTitle[k+1] != '\0' && ( parsedTitle[k] == ',' || parsedTitle[k] == ':' ) && parsedTitle[k+1] == ' ') {
					memmove(&parsedTitle[k], &parsedTitle[k+1], strlen(parsedTitle) - k);
					k--;
				}
				else
                	parsedTitle[k] = '_';
        }
        snprintf(isopath, strlen(parsedTitle)+20, "ms0:/ISO/VIDEO/%s.iso", parsedTitle);
    }
    // open iso fd from isopath for writing
    iso = sceIoOpen(isopath, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

    if (iso < 0) {// if isopath contains invalid characters or ms0: is unable to be reached, this will result in ms0: error
        error("can't access ms0: (memory stick)");
		return 1;
	}
	


	umdreadbuffer = (char*)oe_malloc(512 * SECTOR_SIZE);

    lbawritten = 0;


	pspDebugScreenClear();
    while ((lbaread = sceIoRead(umd, umdreadbuffer, 512))>0) {
		SceUID written = sceIoWrite(iso, umdreadbuffer, lbaread * SECTOR_SIZE);
        // if memory stick runs out of space, quit
		if(written<0){
            sceIoClose(iso);
            sceIoRemove(isopath);
            error("Not enough free space ...");
			return 1;
		}
        // Print status of current dump
        lbawritten += lbaread;
        dumppercent = (lbawritten * 100) / umdlastlba;
        pspDebugScreenSetXY(0, 0);
        printf("%66s", "UMDRescue");
        pspDebugScreenSetXY(0, 13);
        printf("Writing to %s", isopath);
        pspDebugScreenSetXY(0, 15);
        printf("Writing Sectors: %d/%d - %d%% ", lbawritten, umdlastlba, dumppercent);
        pspDebugScreenSetXY(0, 17);
        printf("Writing Bytes: %d/%d - %d%% ", lbawritten * SECTOR_SIZE, umdlastlba * SECTOR_SIZE, dumppercent);


        sceCtrlReadBufferPositive(&pad, 1);
		if((pad.Buttons & PSP_CTRL_LTRIGGER)==PSP_CTRL_LTRIGGER)
			color++;
		if((pad.Buttons & PSP_CTRL_RTRIGGER)==PSP_CTRL_RTRIGGER)
			color--;

		if(color>3) color = 0;
		if(color<0) color = 3;
		if(color == 0) 
    		pspDebugScreenSetTextColor(RGB(0, 255, 0)); // Default (Green) 
		else if(color==1)
    		pspDebugScreenSetTextColor(RGB(255, 0, 255)); // (Purple)
		else if(color==2)
    		pspDebugScreenSetTextColor(RGB(255, 191, 0)); // (Amber)
		else if(color==3)
    		pspDebugScreenSetTextColor(RGB(255, 255, 255)); // (White)
    }


    sceIoClose(iso);
    fd = sceIoOpen(isopath, PSP_O_RDONLY, 0777);
    if (fd >= 0) {
        isosize = sceIoLseek(fd, 0, SEEK_END);
        sceIoClose(fd);
    }    
    sceIoClose(umd);
    oe_free(umdreadbuffer);    
    pspDebugScreenClear(); // blank screen
    // Automatically exit after successful or failed dump and delete ISO if failed
    SceUID count = 10;
    while (count > 0) {
		sceCtrlReadBufferPositive(&pad, 1);	
        pspDebugScreenSetXY(0, 0);
        printf("%s", "UMDRescue");
        if ((isosize) == (umdlastlba*SECTOR_SIZE)) {
            pspDebugScreenSetXY(7, 12);
            printf("Successfully wrote iso to %s", isopath);
        }
        else {
            pspDebugScreenSetXY(7, 12);
			printf("WARNING: ISO size doesn't match UMD size\n\n");
            printf("ISO size: %d\n\nUMD size: %d\n", isosize, (umdlastlba*SECTOR_SIZE));
        }
		
        pspDebugScreenSetXY(7, 14);
        printf("Exiting in %d seconds...", count);
        count -= 1;
        sceKernelDelayThread(1000 * 1000);
    }

    return 0;
}

int main(SceUID argc, char** argsv)
{ 
	
	pspDebugScreenInit(); // pspDebugScreenSetXY(X,Y) has a max of '68x34' character units (1 character = 8 pixels)
    pspDebugScreenSetBackColor(RGB(0, 0, 0)); // set background color
	pspDebugScreenSetTextColor(RGB(255, 0, 255)); // set text color
	if(sceKernelDevkitVersion() > 0x01050001) {
		printf("Only GAMES can be dumped");
		sceKernelDelayThread(3000000);
	}
												  
	if(sceUmdCheckMedium() == 0) {
		pspDebugScreenClear();
		printf("No UMD Disc inserted please insert one now ...");
   		sceUmdWaitDriveStat(PSP_UMD_PRESENT);
   		sceUmdWaitDriveStat(PSP_UMD_INITED);
	}

	pspDebugScreenClear();
	printf("Waiting on UMD disc ...");
	sceUmdActivate(1, "disc0:"); // Mount UMD to disc0: file system
	sceUmdWaitDriveStat(PSP_UMD_READY);


	SceUID thid;
	thid = sceKernelCreateThread("dump_thread", dump, 0x18, 0x10000, 0, NULL);
	if(thid >= 0) {
		sceKernelStartThread(thid, 0, NULL);
		sceKernelWaitThreadEnd(thid, NULL);
	}
											  
	sceKernelExitGame();
	
    return 0;
}
