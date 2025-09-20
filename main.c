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

#ifdef PRX
	#define BUILDPRX BUILD " (PRX Mode)"
	char version[128] = BUILDPRX;
#endif

#ifndef PRX
	char version[128] = BUILD;
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
#define clearStatus screenSet(0, 28);printf("\n\n"); //pspdebugprintf does not appear to actually overwrite trailing spaces so this doesn't strictly work at the moment, need more research

#define RGB(r, g, b) ((r) | ((g) << 8) | ((b) << 16))
#define SECTOR_SIZE 0x800
SceCtrlData pad;

// define psp module as user module
PSP_MODULE_INFO("UMDRescue", PSP_MODULE_USER, 1, 0);

char* umdreadbuffer = NULL; // buffer to hold the contents of the umd as its read
char umddatabin[49] = { 0 }; //umd_data.bin will always be 48 char, this is for strtok
char umd_databin[49]; // full container for umd_data.bin to output

char *discid; // first field of umd_data.bin, can be from 10 to 27 char (length of discid & guid is shared to a max of 27) will never be empty
char *guid; // second field of umd_data.bin,  can be from 0 to 16 char length CAN be empty
char *mode_attr; // third field of umd_data.bin, will always be either 0001 (user mode) or 0002 (kernel mode)
char *umdtype; // fourth field of umd_data.bin, will be any combination of GVA (ie; G, or GV, or V, or VA)

char empty[2048] = { 0 };
char sfoTitle[128] = { 0 }; // title from PARAM.SFO
char isopath[128] = { 0 };
char parsedSfoTitle[256] = { 0 }; // parsed title from PARAM.SFO to remove invalid characters 
char status[128] = "Disc not mounted - safe to insert new UMD";

int umdDriveStatus = 0, dirNumLinesParsed = 0, fix = 0;
float cpufreq = 0.0, busfreq = 0.0;

SceUID lbaread, umdlastlba, isosize, dumppercent, lbawritten, umd, iso, fd;

static int color = 0, display = 0, num_colors = 5;
static uint32_t COLORS[5] = {RGB(0, 255, 0),RGB(255, 0, 255),RGB(255, 191, 0),RGB(55, 255, 255),RGB(255, 255, 255)};

// parse UMD_DATA.bin to derive common disc info fields
int parse_umd_data(void) {
	fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0777);
	if (fd >= 0) {
		sceIoLseek(fd, 0, SEEK_SET);
		sceIoRead(fd, umddatabin, 96);
		strcpy(umd_databin, umddatabin);
		discid = strtok(umddatabin, "|");
		guid = strtok(NULL, "|");
		mode_attr = (strtok(NULL, "|") + 3);
		umdtype = strtok(NULL, "|");
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

	//determines content name from gtype variable and "PARAM.SFO" located in "/PSP_GAME" or "/UMD_VIDEO"
	if (umdtype[0] == 'G') {
		fd = sceIoOpen("disc0:/PSP_GAME/PARAM.SFO", PSP_O_RDONLY, 0777);
		if (fd >= 0) {
			sceIoLseek(fd, 0x158, SEEK_SET);
			sceIoRead(fd, sfoTitle, 17);
			sfoTitle[17] = 0;
			sceIoClose(fd);
		}
	}

	else if (umdtype[0] == 'V') {
		fd = sceIoOpen("disc0:/UMD_VIDEO/PARAM.SFO", PSP_O_RDONLY, 0777);
		if (fd >= 0) {
			sceIoLseek(fd, 0x74, SEEK_SET);
			sceIoRead(fd, sfoTitle, sizeof(sfoTitle));
			unsigned int i = 0, j = 0;
			for(;i<sizeof(sfoTitle); i++) {
				if(sfoTitle[i] == '\0') {
					sceIoLseek(fd, 0x50, SEEK_SET);
					sceIoRead(fd, sfoTitle, sizeof(sfoTitle));
					if(sfoTitle[i] == '\0') break;
					else {
						parsedSfoTitle[j] = sfoTitle[i];
						j++;
					}
				}
				else {
					parsedSfoTitle[j] = sfoTitle[i];
					j++;
				}
			}
			sceIoClose(fd);
		}
	}
	return 0;
}

void dirLoop(SceUID dirLoopfd, SceIoDirent dir, char *path, int li) {
	if (dirLoopfd >= 0) {
		while (sceIoDread(fd, &dir) > 0) {
			SceMode mode = dir.d_stat.st_mode;
			int fileType = (mode & 0x0F000) >> 12;
			int systemExecute = (mode & 0x01);
			int systemWrite = (mode & 0x02) >> 1;
			int systemRead = (mode & 0x04) >> 2;
			int groupExecute = (mode & 0x010) >> 4;
			int groupWrite = (mode & 0x020) >> 5;
			int groupRead = (mode & 0x040) >> 6;
			int userExecute = (mode & 0x0100) >> 8;
			int userWrite = (mode & 0x0200) >> 9;
			int userRead = (mode & 0x0400) >> 10;

			int currentY = pspDebugScreenGetY();
			if (currentY > 27) {
				break;
			}

			char fileTypeLetter = '-';
			if (fileType == 4) {
				fileTypeLetter = 'l';
			}
			else if (fileType == 1){
				fileTypeLetter = 'D';
			}
			int allPerms[9] = {userRead, userWrite, userExecute, groupRead, groupWrite, groupExecute, systemRead, systemWrite, systemExecute};
			char allPermsStr[10] = "rwxrwxrwx";
			for (int j = 0; j < 9; j++) {
				if (!(allPerms[j])) {
					allPermsStr[j] = '-';
				}
			}
			if (fileTypeLetter == 'D') {
				if ((strcmp(dir.d_name,".") != 0) && (strcmp(dir.d_name,"..") != 0)) {
					if(dirNumLinesParsed >= li) {
						printf("%c%-9s %7lld %s\n", fileTypeLetter, allPermsStr, (dir.d_stat.st_size), dir.d_name);
					}
					dirNumLinesParsed++;
					char newPath[256] = { 0 };
					strcpy(newPath, path);
					strcat(newPath, dir.d_name);
					strcat(newPath, "/");
					SceUID newFD = sceIoDopen(newPath);
					SceIoDirent newDir;
					dirLoop(newFD, newDir, newPath, li);
				}
			}
			// print all files
			else {
				if(dirNumLinesParsed >= li) {
					printf("%c%-9s %7lld %s\n", fileTypeLetter, allPermsStr, (dir.d_stat.st_size), dir.d_name);
				}
				dirNumLinesParsed++;
			}
		}
		sceIoDclose(dirLoopfd);
	}
	return;
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
	dumppercent = (lbawritten * 100) / (umdlastlba + 1);
	return 0;
}

// menu and display functionality
int dump(){
	pspDebugScreenClear();

	cpufreq = scePowerGetCpuClockFrequencyFloat();
	busfreq = scePowerGetBusClockFrequencyFloat();

	int dump_in_progress = 0, bytes = 0, li= 0;
	float megabytes = 0.0, gigabytes = 0.0;

	// malloc 1MB of memory for the umdread buffer 
	umdreadbuffer = (char*)malloc(512 * SECTOR_SIZE);

	// While loop to display multi-screens of info ie; main menu
	do {
		sceDisplayWaitVblankStart();

		// determine battery life and output charging status
		int batLifetimeMin = scePowerGetBatteryLifeTime();
		int batTimeHours = (batLifetimeMin / 60);
		int batTimeMinRemain = (batLifetimeMin % 60);
		int batPercentLeft = scePowerGetBatteryLifePercent();

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

		// change display screens horizontally and define display based on control logic
		if((pad.Buttons & PSP_CTRL_LEFT)==PSP_CTRL_LEFT) {
			clearScreen; // blank screen
			li = 0;
			display++;
			sceKernelDelayThread(1000 * 100 * 2);
		}
		if((pad.Buttons & PSP_CTRL_RIGHT)==PSP_CTRL_RIGHT) {
			clearScreen;; // blank screen
			li = 0;
			display--;
			sceKernelDelayThread(1000 * 100 * 2);
		}
		//display 2 should only be visible while dumping
		if (dump_in_progress) {
			if(display>3) display = 0;
			if(display<0) display = 3;
		}
		else {
			if(display>2) display = 0;
			if(display<0) display = 2;
		}
		// change display screens vertically (scroll) and define display based on control logic
		if((pad.Buttons & PSP_CTRL_DOWN)==PSP_CTRL_DOWN) {
			//clearScreen; // blank screen
			li++;
			sceKernelDelayThread(1000 * 10 * .273);
		}
		if((pad.Buttons & PSP_CTRL_UP)==PSP_CTRL_UP) {
			//clearScreen;; // blank screen
			li--;
			sceKernelDelayThread(1000 * 10 * .273);
		}

		// umd info vdisplay
		if(display == 0) {
			titleBar;
			if (umdDriveStatus) { 
				screenSet(0, 4);
				printf("UMD_DATA.BIN: %s", umd_databin);
				screenSet(7, 6);
				//printf("%14s%s", "SFO Title: ", (umdtype[0] == 'G') ? sfoTitle : parsedSfoTitle);
				screenSet(7, 8);
				printf("%14s%s", "Type: ", umdtype);
				screenSet(7, 10);
				printf("%14s%s", "Disc ID: ", discid);
				screenSet(7, 12);
				printf("%14s%d", "Total LBA: ", (umdlastlba + 1));
				screenSet(7, 14);
				printf("%14s%d", "Size (bytes): ", bytes);
				screenSet(7, 16);
				printf("%14s%.2f", "Size (MB): ", megabytes);
				screenSet(7, 18);
				printf("%14s%.2f", "Size (GB): ", gigabytes);
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
			screenSet(3, 6);
			if (!scePowerIsBatteryExist()) {
				printf("%23s%s", "Battery: ", "No Battery Dectected");
			}
			else if (!scePowerIsBatteryCharging()) {
				printf("%23s%d%s%d%s%d%s", "Battery: ", batPercentLeft, "% (", (batTimeHours), ":", batTimeMinRemain, ") remaining");
			}
			else {
				printf("%23s%d%s%s", "Battery: ", batPercentLeft, "% ", "(charging)");
			}
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

		// disc contents vdisplay
		else if(display == 2) {
			titleBar;
			screenSet(0, 4);
			printf("Disc Contents:");
			screenSet(0, 5);printf("\n");
			SceIoDirent dir;
			SceUID dirLoopfd;
			char path[] = "disc0:/";
			dirLoopfd = sceIoDopen(path);
			if(li < 0) {
				li = 0;
			}
			dirNumLinesParsed = 0;
			dirLoop(dirLoopfd, dir, path, li);
			if ((dirNumLinesParsed < (li+21)) && (li > 0)) {
				li--;
			}
			statusBar;
			footer;
		}

		// dump status vdisplay
		else if(display == 3) {
			titleBar;
			screenSet(0, 13);
			printf("Writing to %s", isopath);
			screenSet(0, 15);
			printf("Writing Sectors: %d/%d - %d%% ", lbawritten, (umdlastlba + 1), dumppercent);
			screenSet(0, 17);
			printf("Writing Bytes: %d/%d - %d%% ", lbawritten * SECTOR_SIZE, (umdlastlba + 1) * SECTOR_SIZE, dumppercent);
			screenSet(0, 19);
			printf("Dump Status: %s", status);
			// check legitimacy of dump
			//if ((dump_in_progress == 0) && (((isosize) != (umdlastlba*SECTOR_SIZE)) && (isosize > 0))) {
			//	screenSet(0, 21);
			//	printf("WARNING: ISO size doesn't match UMD size\n\n");
			//	printf("ISO size: %d\n\nUMD size: %d\n", isosize, (umdlastlba*SECTOR_SIZE));
			//}
			statusBar;
			footer;
		}

		sceDisplayWaitVblankStart(); // Wait for vertical blank start

		// press triangle to free mem and exit program
		if (pad.Buttons & PSP_CTRL_TRIANGLE) {
			if (!dump_in_progress) {
				free(umdreadbuffer);
				return 0;
			}
			sceKernelDelayThread(1000 * 100 * 2);
		}

		// press circle to cancel in-progress dump
		if ((pad.Buttons & PSP_CTRL_CIRCLE) && (dump_in_progress)) {
			clearScreen;
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
			clearScreen;
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
					bytes = ((umdlastlba + 1) * SECTOR_SIZE);
					megabytes = ((bytes / 1024.0) / 1024.0);
					gigabytes = (megabytes / 1024.0);
					sprintf(status, "Ready to Dump");
					clearStatus;
				}
				else {
					sprintf(status, "Failed to read UMD metadata: please make sure a disc is inserted");
					sceUmdDeactivate(1, "disc0:"); // unmount umd
					//sceUmdReplacePermit();
					umdDriveStatus = 0;  
				}
			}
			else if (umdDriveStatus == 1) { 
				sceUmdDeactivate(1, "disc0:"); // unmount umd
				//sceUmdReplacePermit();
				sprintf(status, "Disc not mounted: safe to insert new UMD");
				umdDriveStatus = 0;
				//break;
			}
			sceKernelDelayThread(1000 * 100 * 2);
		}

		// press X to start dump logic
		if (pad.Buttons & PSP_CTRL_CROSS){
			clearScreen;
			//insert time function to measure elapsed time
			if (!dump_in_progress) {
				dump_in_progress = 1;
				sprintf(status, "Dumping...");
				clearStatus;
				umd = sceIoOpen("umd0:", PSP_O_RDONLY, 7777); // Open UMD disc in read-only mode and throw error if unsuccessful
				if (umd < 0) {
					sprintf(status, "Can't open UMD");
					clearStatus;
					dump_in_progress = 0;
					if (umdDriveStatus) {
						sceUmdDeactivate(1, "disc0:"); // unmount umd
						//sceUmdReplacePermit();
						umdDriveStatus = 0;
					}
					sceKernelDelayThread(1000 * 100 * .273);
				}

				else {
					// format/sanitize discid of erroneous chars to be used as the ISO name  
					char destName[28] = "";
					strcpy(destName, discid);
					char invalidChar[16] = {' ',':','/','(',')','!','&','?','\'','<','>','"','\\','|','*','^'};
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
					char absDir[80];
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
						char append = 1; 
						char apDir[64];
						sprintf(apDir, "ms0:/UMD/%s", destName);

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

					// set isopath to be the combination of absoluteDir/filename.iso
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
			display = 4;
		}
		// dump the disc sector by sector
		if (dump_in_progress) {
			if((lbaread = sceIoRead(umd, umdreadbuffer, 512))>0) {
				//int sector_status = dumpSector();
				sprintf(status, "Writing Sectors: %d/%d - %d%% ", lbawritten, (umdlastlba + 1), dumppercent);
				clearStatus;
				if(dumpSector()) {
					dump_in_progress = 0;
					sprintf(status, "Something went wrong");
					clearStatus;
				}
			}
			// if we're finished, check image and clean up
			else {
				dump_in_progress = 0;
				sprintf(status, "Successfully wrote ISO");
				clearStatus;
			}
			if (!dump_in_progress) {
				if (fix) {
					sceIoWrite(iso, empty, SECTOR_SIZE);
				}
				sceIoClose(iso);
				fd = sceIoOpen(isopath, PSP_O_RDONLY, 0777);
				if (fd >= 0) {
					isosize = sceIoLseek(fd, 0, SEEK_END);
					sceIoClose(fd);
					if ((isosize) != ((umdlastlba + 1) * SECTOR_SIZE)) {
						//sceIoRemove(isopath); //COMMENTED OUT FOR TESTING, UNCOMMENT FOR RELEASE
						sprintf(status, "Bad dump; ISO size: %d =/= UMD Size: %d.", isosize, (umdlastlba + 1));
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
			strcat(version, " (CFW Mode)");
		}
	}
	else {
		strcat(version," (1.5 Mode)");
		fix = 1;
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