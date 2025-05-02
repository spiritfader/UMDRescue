/*
 UMDRescue -- create archive quality images of UMD optical discs
 
 Copyright (C) 2024, 2025 spiritfader
 Copyright (C) 2024, 2025 krazynez
 Copyright (C) 2025 NerdyKyogre

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// includes
#include <pspsdk.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspumd.h>
#include <psptypes.h>
#include <pspkernel.h>
#include <pspstdio.h>
#include <psppower.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// define preprocessor macro for version (makefile) 
#ifndef BUILD
	#define BUILD { 0 }
#endif

// macro pspDebugScreenPrintf to printf, 2048 to SECTOR_SIZE, and limit RGB params 
#define printf pspDebugScreenPrintf
#define screenSet pspDebugScreenSetXY
#define clearScreen pspDebugScreenClear()

#define titleBar screenSet(0, 1);printf("%66s", version);
#define statusBar screenSet(0, 29);printf("Status: %s", status);
#define footer screenSet(0, 31);printf("     L-R          <->     (SQUARE)    (X)      (O)        (TRIANGLE)");\
screenSet(0, 32);printf("    COLOR       DISPLAY   MOUNT UMD   DUMP  CANCEL DUMP      Exit   ");

char status_clear[128] = {0};
#define clearStatus screenSet(0, 29);printf("%s", status_clear); //pspdebugprintf does not appear to actually overwrite trailing spaces so this doesn't strictly work at the moment, need more research

#define RGB(r, g, b) ((r) | ((g) << 8) | ((b) << 16))
#define SECTOR_SIZE 0x800
SceCtrlData pad;

// call external funcs - not using oe_malloc anymore
//extern void* oe_malloc(size_t size);
//extern void oe_free(void*);

// define psp module as user module
PSP_MODULE_INFO("UMDRescue", PSP_MODULE_USER, 1, 0);

char* umdreadbuffer = NULL;

char umddatabin[96] = { 0 };
//char *discid, *guid, *region, *type;

// Update the discid buffer size to match the umddatabin size for maximum compatibility
char discid[96]; // Increase from 25 to 96 to match umddatabin buffer size
char guid[15]; // second field of umd_data.bin
char region[3]; // third field of umd_data.bin
char type[13]; // fourth field of umd_data.bin

char isopath[256] = { 0 };
char audioSfoTitle[128] = { 0 }; // PARAM.SFO Title for Audio Discs
char gameSfoTitle[128] = { 0 }; // PARAM.SFO Title for Game Discs
char videoSfoTitle[128] = { 0 }; // PARAM.SFO Title for Video Discs
char updaterSfoTitle[128] = { 0 }; // PARAM.SFO Title for Updater

char discTypeDisplay[128] = {0};
char discTypeDescription[64]  = {0};
char discTypeShort[5] = {0};

char status[128] = "Disc not mounted - safe to insert new UMD";
int umdDriveStatus = 0;

#ifdef PRX
	#define BUILDPRX BUILD " (PRX Mode)"
	char version[128] = BUILDPRX;
#endif

#ifndef PRX
	char version[128] = BUILD;
#endif

SceUID threadnumber, lbaread, umdlastlba, isosize, dumppercent, lbawritten, sec = -1;
SceUID umd, iso, fd, threadlist[66], st_thlist_first[66], st_thnum_first = -1;
static int color = 0;
static int display = 0;
float cpufreq = 0.0, busfreq = 0.0;

static int num_colors = 5;
static uint32_t COLORS[5] = {RGB(0, 255, 0),RGB(255, 0, 255),RGB(255, 191, 0),RGB(55, 255, 255),RGB(255, 255, 255)};

// sfo header
typedef struct {
    uint32_t magic;              // should be PSP_SFO_MAGIC
    uint32_t version;            // usually 0x101
    uint32_t key_table_offset;   // from start of file
    uint32_t data_table_offset;  // from start of file
    uint32_t num_entries;        // number of entries
} __attribute__((packed)) sfo_header_t;

// sfo entry
typedef struct {
    uint16_t key_offset;    // offset into key table (bytes)
    uint8_t  alignment;     // usually 4
    uint8_t  data_fmt;      // 0=binary, 2=utf8, ...
    uint32_t data_max_len;  // max size (in bytes)
    uint32_t data_len;      // actual size in file
    uint32_t data_offset;   // offset into data table
} __attribute__((packed)) sfo_entry_t;

// disc type info   
struct discType {
        unsigned int flag;        // bitmask flag for UMD type
        const char *label;        // human-readable media name
        char shorthand;           // single-letter code
        const char *sfoPath;      // PARAM.SFO path for partition
        char *titleOutput;        // buffer to store the title
    } discTypes[] = {
        { PSP_UMD_TYPE_GAME,  "Game",  'G', "disc0:/PSP_GAME/PARAM.SFO", gameSfoTitle },
        { PSP_UMD_TYPE_VIDEO, "Video", 'V', "disc0:/UMD_VIDEO/PARAM.SFO", videoSfoTitle },
        { PSP_UMD_TYPE_AUDIO, "Audio", 'A', "disc0:/UMD_AUDIO/PARAM.SFO", audioSfoTitle },
    };

// get entry from SFO (title)
int get_sfo_field(const char *path, const char *key, char *outBuf, size_t outBufSize) {
    int fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
    if (fd < 0) {
        return fd;
    }

    // read and validate header
    sfo_header_t hdr;
    if (sceIoRead(fd, &hdr, sizeof(hdr)) != sizeof(hdr)
     || hdr.magic != 0x46535000) { // “\0PSF” in little‐endian
        sceIoClose(fd);
        return -1;
    }

    // read the entry table
    size_t entry_table_offset = sizeof(sfo_header_t);
    size_t entries_size        = hdr.num_entries * sizeof(sfo_entry_t);
    sfo_entry_t *entries = malloc(entries_size);
    if (!entries) {
        sceIoClose(fd);
        return -1;
    }
    sceIoLseek(fd, entry_table_offset, PSP_SEEK_SET);
    if (sceIoRead(fd, entries, entries_size) != (int)entries_size) {
        free(entries);
        sceIoClose(fd);
        return -1;
    }

    // walk the entries looking for “key”
    for (uint32_t i = 0; i < hdr.num_entries; i++) {
        sfo_entry_t *e = &entries[i];

        // read the key into a small buffer
        char keybuf[32] = {0};
        sceIoLseek(fd,
                   hdr.key_table_offset + e->key_offset,
                   PSP_SEEK_SET);
        sceIoRead(fd, keybuf, sizeof(keybuf)-1);

        if (strcmp(keybuf, key) == 0) {
            // now actually read the key string
            uint32_t read_len = e->data_len;
            if (read_len > outBufSize - 1)
                read_len = outBufSize - 1;

            sceIoLseek(fd,
                      hdr.data_table_offset + e->data_offset,
                      PSP_SEEK_SET);
            sceIoRead(fd, outBuf, read_len);
            outBuf[read_len] = '\0';

            free(entries);
            sceIoClose(fd);
            return 0;
        }
    }

    free(entries);
    sceIoClose(fd);
    return -1;
}

// parse pspUmdInfo, UMD_DATA.bin and PARAM.SFO files to derive common disc info fields
int parse_umd_data(void) {
	
    // get disc type
    pspUmdInfo info;
    info.size = sizeof(info);
    int result = sceUmdGetDiscInfo(&info);
    unsigned int discTypeMask = (result >= 0) ? info.type : 0;
	
	// prepare description and code buffers
    char *writePtr = discTypeDescription;
    size_t remaining = sizeof(discTypeDescription);
    int codeIndex = 0;

    // build description, shorthand codes, and load titles for each disc type flag
    for (size_t i = 0; i < sizeof(discTypes)/sizeof(discTypes[0]); i++) {
        if (discTypeMask & discTypes[i].flag) {
            // delimiter between names
            const char *delimiter = (codeIndex > 0) ? " + " : "";
            int written = snprintf(writePtr, remaining, "%s%s", delimiter, discTypes[i].label);
            if (written < 0 || (size_t)written >= remaining) {
                break; // buffer full
            }
            writePtr  += written;
            remaining -= written;

            // add single-letter code
            discTypeShort[codeIndex++] = discTypes[i].shorthand;

            // retrieve SFO title
            if (get_sfo_field(discTypes[i].sfoPath,
                              "TITLE",
                              discTypes[i].titleOutput,
                              128) < 0) {
                discTypes[i].titleOutput[0] = '\0';
			}
        }
    }

    // fallback to "Unknown" if no types matched
    if (codeIndex == 0) {
        snprintf(discTypeDescription, sizeof(discTypeDescription), "Unknown"); 
		discTypeShort[codeIndex++] = 'U';
    }
    discTypeShort[codeIndex] = '\0';

    // compose final disc type display value
    snprintf(discTypeDisplay, sizeof(discTypeDisplay), "%s (0x%02X / %s)", discTypeDescription, discTypeMask, discTypeShort);

	//get disc id from UMD_DATA.BIN, based on delimiter "|"
	fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0777);
	if (fd >= 0) {
        // read the entire file
        sceIoLseek(fd, 0, SEEK_SET);
        sceIoRead(fd, umddatabin, sizeof(umddatabin));
        
        // ensure the buffer is null-terminated for string operations
        umddatabin[sizeof(umddatabin) - 1] = '\0';
        
        // now parse the buffer
        char* delimiterPos = strchr(umddatabin, '|');
        
        if (delimiterPos != NULL) {
            // calculate the length of the disc ID
            size_t idLength = delimiterPos - umddatabin;
            
            // copy the disc ID to the discid variable
            memcpy(discid, umddatabin, idLength);
            discid[idLength] = '\0'; // Ensure null termination
        } else {
            // no delimiter found, copy the entire buffer
            strcpy(discid, umddatabin);
        }
        
        sceIoClose(fd);
	}
	else {
		return -1;
	}

	// read size (last lba) of UMD disc to umdlastlba
	fd = sceIoOpen("umd0:", PSP_O_RDONLY, 0777);
	if (fd >= 0) {
		umdlastlba = sceIoLseek(fd, 0, SEEK_END);
		sceIoClose(fd);
	}

	return 0;
}

// reset titles for new dump/scan
static void clearTitles(void) {
	gameSfoTitle[0]  = '\0';
	videoSfoTitle[0] = '\0';
	audioSfoTitle[0] = '\0';
	updaterSfoTitle[0] = '\0';
}

// write contents of umdreadbuffer to iso, dumping disc to iso
int dumpSector(void) {
	SceUID written = sceIoWrite(iso, umdreadbuffer, lbaread * SECTOR_SIZE);

		// if memory stick runs out of space, quit
		if(written<0) {
			sceIoClose(iso);
			sceIoRemove(isopath);
			sprintf(status, "Out of free disk space");
			clearStatus;
			return 1;
		}

		// Print status of current dump through assigning lbaread to lbawritten
		lbawritten += lbaread;
		dumppercent = (lbawritten * 100) / umdlastlba;
		return 0;
}

// menu and display functionality
int dump(){
	pspDebugScreenClear();
	
	cpufreq = scePowerGetCpuClockFrequencyFloat();
	busfreq = scePowerGetBusClockFrequencyFloat();

	int dump_in_progress = 0, bytes=0;
	float megabytes=0.0, gigabytes=0.0;
	
	// malloc 1MB of memory for the umdread buffer 
	umdreadbuffer = (char*)malloc(512 * SECTOR_SIZE);
	
	// While loop to present disc information until Start is pressed (exiting) or X is pressed (exiting loop and following code logic)
	do {

		// determine battery life and output charging status
		//int batLifetimeMin = scePowerGetBatteryLifeTime();
		//int batTimeHours = (batLifetimeMin / 60);
		//int batTimeMinRemain = (batLifetimeMin % 60);
		//int batPercentLeft = scePowerGetBatteryLifePercent();

		if(sceUmdCheckMedium() == 0) {
			strcpy(status, "No UMD Inserted - please insert one now.");
			clearStatus;
		}
		// poll for input
		sceCtrlPeekBufferPositive(&pad, 1);

		// Change color of command output and define color based on control logic
		if((pad.Buttons & PSP_CTRL_LTRIGGER)==PSP_CTRL_LTRIGGER) {
			color++;
			sceKernelDelayThread(1000 * 100 * 2);
		}
		if((pad.Buttons & PSP_CTRL_RTRIGGER)==PSP_CTRL_RTRIGGER) {
			color--;
			sceKernelDelayThread(1000 * 100 * 2);
		}
		if(color>(num_colors - 1)) color = 0;
		if(color<0) color = (num_colors - 1);
		pspDebugScreenSetTextColor(COLORS[color]);

		// Navigate display output screens and define display based on control logic
		if((pad.Buttons & PSP_CTRL_LEFT)==PSP_CTRL_LEFT) {
			pspDebugScreenClear(); // blank screen
			display++;
			sceKernelDelayThread(1000 * 100 * 2);
		}
		if((pad.Buttons & PSP_CTRL_RIGHT)==PSP_CTRL_RIGHT) {
			pspDebugScreenClear(); // blank screen
			display--;
			sceKernelDelayThread(1000 * 100 * 2);
		}
		//display 2 should only be visible while dumping
		if (dump_in_progress) {
			if(display>2) display = 0;
			if(display<0) display = 2;
		}
		else {
			if(display>1) display = 0;
			if(display<0) display = 1;
		}

		// UMD Info vdisplay
		if(display == 0) {
			titleBar;
			if (umdDriveStatus) {
				int row = 4;
				screenSet(7, row);
				printf("%14s%s", "UMD Info: ", umddatabin);
				
				//Game Disc
				if (gameSfoTitle[0]) {
					screenSet(7, row += 2);
					printf("%14s%s", "Game Title: ", gameSfoTitle);
				}
				
				//Video Disc
				if (videoSfoTitle[0]) {
					screenSet(7, row += 2);
					printf("%14s%s", "Video Title: ", videoSfoTitle);
				}
				
				//Audio Disc
				if (audioSfoTitle[0]) {
					screenSet(7, row += 2);
					printf("%14s%s", "Audio Title: ", audioSfoTitle);
				}
				
				/*Updater (not here yet, looks bad because TM symbol
				if (updaterSfoTitle[0]) {
					screenSet(6, row += 2);
					printf("%14s%s", "Updater Title: ", updaterSfoTitle);
				}*/	
				
				screenSet(7, row += 2);
				printf("%14s%s", "Type: ", discTypeDisplay);
				screenSet(7, row += 2);
				printf("%14s%s", "Disc ID: ", discid);
				screenSet(7, row += 2);
				printf("%14s%d", "Total LBA: ", umdlastlba);
				screenSet(7, row += 2);
				printf("%14s%d", "Size (bytes): ", bytes);
				screenSet(7, row += 2);
				printf("%14s%.2f", "Size (MB): ", megabytes);
				screenSet(7, row += 2);
				printf("%14s%.2f", "Size (GB): ", gigabytes);
				//screenSet(7, 20);
				//printf("%14s%s", "ISO Path: ", isopath);
			}
			else {
				screenSet(2, 16);
				printf("To view UMD information, insert and mount UMD by pressing Square");
			}
			statusBar;
			footer;
		}
		// System information vdisplay
		else if(display == 1) {
			titleBar;
			screenSet(0, 4);
			printf("Sysinfo:");
			//screenSet(3, 6);
			//if (!scePowerIsBatteryExist()) {
			//	printf("%23s%s", "Battery: ", "No Battery Dectected");
			//}
			//else if (!scePowerIsBatteryCharging()) {
			//	printf("%23s%d%s%d%s%d%s", "Battery: ", batPercentLeft, "% (", (batTimeHours), ":", batTimeMinRemain, ") remaining");
			//}
			//else {
			//	printf("%23s%d%s%s", "Battery: ", batPercentLeft, "% ", "(charging)");
			//}
			screenSet(3, 8);
			printf("%23s%d", "Firmware Version: ", sceKernelDevkitVersion());
			screenSet(3, 10);
			printf("%23s%.2f%s%.2f%s", "CPU / Bus Clock Freq: ", cpufreq, " MHz / ", busfreq, " MHz");
			screenSet(3, 12);
			printf("%23s%.2f%s", "Kernel Total Free Mem: ", (sceKernelTotalFreeMemSize()/1024.0), " Kilobytes");
			screenSet(3, 14);
			printf("%23s%.2f%s", "User Total Free Mem: ", (pspSdkTotalFreeUserMemSize()/1024.0), " Kilobytes");
			statusBar;
			footer;
		}

		// stdout screen vdisplay
		//else if(display == 3) {
		//	titlebar;
		//	screenSet(0, 4);
		//	printf("STDOUT:");
		//	screenSet(7, 6);
		// 	statusBar;
		//	footer;
		//}

		// dump status vdisplay
		else if(display == 2) {
			titleBar;
			screenSet(0, 13);
			printf("Writing to %s", isopath);
			screenSet(0, 15);
			printf("Writing Sectors: %d/%d - %d%% ", lbawritten, umdlastlba, dumppercent);
			screenSet(0, 17);
			printf("Writing Bytes: %d/%d - %d%% ", lbawritten * SECTOR_SIZE, umdlastlba * SECTOR_SIZE, dumppercent);
			screenSet(0, 19);
			printf("Dump Status: %s", status);
			
			//if ((dump_in_progress == 0) && (((isosize) != (umdlastlba*SECTOR_SIZE)) && (isosize > 0))) {
			//	screenSet(0, 21);
			//	printf("WARNING: ISO size doesn't match UMD size\n\n");
			//	printf("ISO size: %d\n\nUMD size: %d\n", isosize, (umdlastlba*SECTOR_SIZE));
			//}
			statusBar;
			footer;
		}

		sceDisplayWaitVblankStart(); // Wait for vertical blank start

		if (pad.Buttons & PSP_CTRL_TRIANGLE) { // if triangle is pressed, free memory and quit program
			if (!dump_in_progress) {
				free(umdreadbuffer);
				return 0;
			}
			sceKernelDelayThread(1000 * 100 * 2);
		}

		// press circle to cancel in-progress dump
		if ((pad.Buttons & PSP_CTRL_CIRCLE) && (dump_in_progress)) {
			dump_in_progress = 0;
			sceIoClose(umd);
			sceIoClose(iso);
			sceIoRemove(isopath);
			sprintf(status, "Dump Cancelled");
			clearStatus;
			sceKernelDelayThread(1000 * 100 * 2);
		}

		// press square to mount UMD (control)
		if (pad.Buttons & PSP_CTRL_SQUARE) {
			clearTitles(); //clear titles that may be present from previous dump
			if (umdDriveStatus == 0){ //not yet activated
				if(sceUmdCheckMedium() == 0) {
					sceUmdWaitDriveStat(PSP_UMD_PRESENT);
					sceUmdWaitDriveStat(PSP_UMD_INITED);
				}
				sceUmdActivate(1, "disc0:"); // Mount UMD to disc0: file system
				sceUmdWaitDriveStat(PSP_UMD_READY);
				//sceUmdReplaceProhibit();
				
				sprintf(status, "Disc mounted: awaiting metadata");
				clearStatus;
				umdDriveStatus = 1;
				if (parse_umd_data() >= 0) {
					bytes = (umdlastlba * SECTOR_SIZE);
					megabytes = ((bytes / 1024.0) / 1024.0);
					gigabytes = (megabytes / 1024.0);
					sprintf(status, "Ready to Dump");
					clearStatus;
				}
				else {
					sprintf(status, "Failed to read UMD metadata: please make sure a disc is inserted");
					clearStatus;
					sceUmdDeactivate(1, "disc0:"); // unmount umd
					//sceUmdReplacePermit();
					umdDriveStatus = 0;  
				}
			}
			else if (umdDriveStatus == 1) { 
				sceUmdDeactivate(1, "disc0:"); // unmount umd
				//sceUmdReplacePermit();
				sprintf(status, "Disc not mounted: safe to insert new UMD");
				clearStatus;
				umdDriveStatus = 0;
				//break;
			}
			pspDebugScreenClear(); // blank screen
		
			sceKernelDelayThread(1000 * 100 * 2);
		}
		// dump start dump logic with X 
		if (pad.Buttons & PSP_CTRL_CROSS){
			//insert time function to measure elapsed time
			if (!dump_in_progress) {
				dump_in_progress = 1;
				sprintf(status, "Dumping...");
				clearStatus;
				umd = sceIoOpen("umd0:", PSP_O_RDONLY, 0777); // Open UMD disc in read-only mode and throw error if unsuccessful
				if (umd < 0) {
					sprintf(status, "Can't open UMD");
					clearStatus;
					dump_in_progress = 0;
					if (umdDriveStatus) {
						sceUmdDeactivate(1, "disc0:"); // unmount umd
						//sceUmdReplacePermit();
						umdDriveStatus = 0;
					}
					sceKernelDelayThread(1000 * 100 * 2);
					//continue;
				}
				else {
					// format/sanitize discid of erroneous chars to be used as the ISO name  
					char destName[sizeof(discid)] = "";
					strcpy(destName, discid);
					char invalidChar[3] = {' ',':',','};
					char *endDest = strchr(destName, '\0');
					for (char *midDest = destName; midDest < endDest; midDest++) {
						char *strTest=strchr(invalidChar, *midDest);
						if (strTest!=NULL)
						{
							*midDest = '_';
						}
					}

					// define and create UMD parent folder as location to store subdirectories
					SceUID checkUmdParDir = sceIoDopen("ms0:/UMD");
					if (checkUmdParDir < 0) {
						if (sceIoMkdir("ms0:/UMD", 0777) == -1) {
							sprintf(status, "Failed to create UMD rootlevel directory");
							clearStatus;
						}
					}
					
					// define and create absolute path directory variable to create subdirectory
					char absDir[151];
					sprintf(absDir, "ms0:/UMD/%s",  destName);
					SceUID checkUmdSubDir = sceIoDopen(absDir);
					if (checkUmdSubDir < 0) {
						if (sceIoMkdir(absDir, 0777) == -1) {
							strcpy(status, "Failed to create UMD subdirectory");
							clearStatus;
						}
					}
					// if UMD subdirectory already exists, append number to suffix of dir path
					else {
						//strcat(outputDir,discid);
						char append = 1; 
						char apDir[135];
						sprintf(apDir, "ms0:/UMD/%s",  destName);
						// while loop breaks to increment and find a unique name
						while(sceIoDopen(absDir) >= 0) {
							sprintf(absDir, "%s-(%d)", apDir, append);
							append++;
							if (append < 1) {
								break;
							}
						}
						if (sceIoMkdir(absDir, 0777) == -1) {
							strcpy(status, "Failed to create UMD subdirectory");
							clearStatus;
						}
					}

					sceIoDclose(checkUmdParDir);
					sceIoDclose(checkUmdSubDir);

					// set isopath to be the combination of outputDir/filename.iso

					sprintf(isopath, "%s/%s.iso",  absDir, destName);
					
					// open iso file descriptor from isopath for writing
					iso = sceIoOpen(isopath, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

					// if isopath contains invalid characters or ms0: is unable to be reached, this will result in ms0: error
					if (iso < 0) {
						int isoInvCheck = 0;
						for (int i = 0; i < 128; i++) {
							if(strchr(invalidChar, isopath[i])) {
								isoInvCheck++;
							}
							if (isoInvCheck) {
								strcpy(status, "Invalid character(s) detected in ISO path");
								clearStatus;
								break;
							}
						}
						if (!isoInvCheck) {
							strcpy(status, "Could not reach memory stick ms0");
							clearStatus;
						}
						dump_in_progress = 0;
					}
					lbawritten = 0;
				}
				// enable the umd dump status screen and delay
				sceKernelDelayThread(1000 * 100 * 2);
			}
			display = 3;
		}
		// dump the disc sector by sector
		if (dump_in_progress) {
			if((lbaread = sceIoRead(umd, umdreadbuffer, 512))>0) {
				//int sector_status = dumpSector();
				sprintf(status, "Writing Sectors: %d/%d - %d%% ", lbawritten, umdlastlba, dumppercent);
				clearStatus;
				if(dumpSector()) {
					dump_in_progress = 0;
					sprintf(status, "Something went wrong");
					clearStatus;
				}
			}
			//if we're finished, check image and clean up
			else {
				dump_in_progress = 0;
				//if (timerActive)
				//	stop the thing
				sprintf(status, "Successfully wrote ISO");
				clearStatus;
			}
			if (!dump_in_progress) {
				sceIoClose(iso);
				fd = sceIoOpen(isopath, PSP_O_RDONLY, 0777);
				if (fd >= 0) {
					isosize = sceIoLseek(fd, 0, SEEK_END);
					sceIoClose(fd);
					if ((isosize) != ((umdlastlba) * SECTOR_SIZE)) { // removed 1 because no off by one error
						sceIoRemove(isopath); //COMMENTED OUT FOR TESTING, UNCOMMENT FOR RELEASE
						sprintf(status, "Bad dump; ISO size: %d =/= UMD Size: %d.", isosize, ((umdlastlba) * SECTOR_SIZE));
						clearStatus;
					}
				}

				// close the umd file descriptor 
				// for future reference, DO NOT FREE MEMORY HERE
				// this results in a use-after-free/double-free on any dumps after the first one
				// instead, free memory in the input handler for triangle before exiting.
				sceIoClose(umd);
			}
		}
	} while (1);
	return 1;
}

// setup main function with psp threading logic
//int main(SceUID argc, char** argsv) { //superfluous arguments, unused 
int main(void) {
	pspDebugScreenInit(); // screenSet(X,Y) has a max of '68x34' character units (1 character = 8 pixels)
	pspDebugScreenSetBackColor(RGB(0, 0, 0)); // set background color
	pspDebugScreenSetTextColor(RGB(255, 0, 255)); // set text color

	// change program title/name header depending on kernel version/type
	if(sceKernelDevkitVersion() > 0x01050001) {
		if (!sceIoDevctl("emulator:", 0, NULL, 0, NULL, 0)) {
			strcat(version," (Emulator Mode)");
		}
		else{
			strcat(version, " (Compat Mode)");
		}
	}
	else {
		strcat(version," (1.5 Mode)");
	}

	// initialize program thread and exit if dump function is exited
	SceUID thid;
	thid = sceKernelCreateThread("dump_thread", dump, 0x18, 0x10000, 0, NULL);
	if(thid >= 0) {
		sceKernelStartThread(thid, 0, NULL);
		sceKernelWaitThreadEnd(thid, NULL);
	}
	sceKernelExitGame();

	return 0;
}
