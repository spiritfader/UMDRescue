#include <stdio.h>
#include <stdlib.h>
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspumd.h>
#include <string.h>
#include <psploadexec_kernel.h> 

#define printf pspDebugScreenKprintf
#define RGB(r, g, b) ((r)|((g)<<8)|((b)<<16))


PSP_MODULE_INFO("UMDKiller", 0x1001, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

int start_dumper();
void threadschanger(int stat, SceUID threadlist[], int threadnumber);

SceCtrlData pad;

char *umdbuffer,discid[11],title[17],isopath[30],gtype[3];
int threadnumber,read,fd,umdsize,dir,dumppercent,written=0;
SceUID umd,iso,threadlist[66],st_thlist_first[66],st_thnum_first;

int st_thread( SceSize args, void *argp )
{
  while(1){
    sceKernelDelayThread( 50000 );
    sceCtrlPeekBufferPositive( &pad, 1 );

    if((pad.Buttons & PSP_CTRL_SELECT)) // Launch UMDKillerPRX when SELECT button is pressed
      start_dumper();
  }

  return 0;
}

// pspDebugScreenSetXY(X,Y) has a max of 60x34 character units (1 character = 8 pixels)

int start_dumper()
{ 
	pspDebugScreenInit();
	pspDebugScreenSetBackColor(RGB(0,0,0)); pspDebugScreenSetTextColor(RGB(0,255,0));
	pspDebugScreenClear();
	
	if(!(umdbuffer=malloc(1048576))){
		pspDebugScreenClear();
  	pspDebugScreenSetXY(0, 0); printf("CRITICAL ERROR : IMPOSSIBLE TO ALLOCATE MEMORY");
    pspDebugScreenSetXY(0, 4); printf("Auto-Exiting in 30 seconds...");
    sceDisplayWaitVblankStart(); sceKernelDelayThread(30*1000*1000);
  }
  
	if(sceUmdCheckMedium()==0){
   	return 0;
  }
  
  sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, threadlist, 66, &threadnumber ); threadschanger( 0, threadlist, threadnumber );
  sceUmdActivate(1, "disc0:"); sceUmdWaitDriveStat(PSP_UMD_READY);
	
    // read until offset 00000010 into discid variable from "disc0:/UMD_DATA.BIN"
	fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0777);
	if(fd >= 0){
		sceIoLseek(fd, 0, SEEK_SET);
		sceIoRead(fd, discid, 10);
		discid[10]=0;
   	sceIoClose(fd);
  }

    // read size of UMD disc to umdsize
  fd = sceIoOpen("umd0:", PSP_O_RDONLY, 0777);
	if(fd >= 0){
		umdsize=sceIoLseek(fd,0,SEEK_END);
   	sceIoClose(fd);
  }
  
    // read offset 00000021 into gtype from "disc0:/UMD_DATA.BIN", determines whether or not the disc is a [G]ame or [V]ideo
  fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0777); 
	if(fd >= 0){
		sceIoLseek(fd, 0x21, SEEK_SET); 
		sceIoRead(fd, gtype, 3);
		sceIoClose(fd);
 	}
	
    // Fix this, determines content name from gtype variable and "PARAM.SFO" located in "/PSP_GAME" or "/UMD_VIDEO"
	if(gtype[0]=='G'){ 
  	fd = sceIoOpen("disc0:/PSP_GAME/PARAM.SFO", PSP_O_RDONLY, 0777);
		if(fd >= 0) {
			sceIoLseek(fd, 0x158, SEEK_SET);
			sceIoRead(fd, title, 16);
			title[16]=0;
  		sceIoClose(fd);
		}
  }

  else if(gtype[0]=='V'){
  	fd = sceIoOpen("disc0:/UMD_VIDEO/PARAM.SFO", PSP_O_RDONLY, 0777);
		if(fd >= 0) {
			sceIoLseek(fd, 0x50, SEEK_SET);
			sceIoRead(fd, title, 16);
			title[16]=0;
  		sceIoClose(fd);
		}
  }
  
    // Clear debug screen
  pspDebugScreenClear();

    // While loop to present disc information until Start is pressed (exiting) or X is pressed (exiting loop and following code logic)
	do{
    sceCtrlPeekBufferPositive(&pad,1);
    pspDebugScreenSetXY(0, 0); printf("%66s", "UMDKillerPRX 2.0");     
	  pspDebugScreenSetXY(7, 10); printf("Title: %s",title);
	  pspDebugScreenSetXY(7, 12); printf("Type: %s",gtype);
   	pspDebugScreenSetXY(7, 14); printf("Disc ID: %s",discid);
   	pspDebugScreenSetXY(7, 16); printf("Size: %d",umdsize);
		pspDebugScreenSetXY(0, 32); printf("%66s", "PRESS X TO DUMP OR PRESS START TO EXIT");

      // Wait for vertical blank start
    sceDisplayWaitVblankStart();	

    // if start is pressed, quit program - Fix, this  removes ability to interact with xmb after exiting
    if(pad.Buttons & PSP_CTRL_START){
    	threadschanger( 1, threadlist, threadnumber );
   		return 0;
   	}
    // if X is pressed, start dump logic
  }while(!(pad.Buttons & PSP_CTRL_CROSS));
  
    // Open UMD disc in read-only mode and throw error if unsuccessful
  umd = sceIoOpen("umd0:", PSP_O_RDONLY, 0777); 
  if(umd<0){
  	pspDebugScreenClear();
  	pspDebugScreenSetXY(0, 0); printf("CRITICAL ERROR : IMPOSSIBLE TO OPEN UMD0");
    pspDebugScreenSetXY(0, 4); printf("Auto-Exiting in 30 seconds...");
    sceDisplayWaitVblankStart(); sceKernelDelayThread(30*1000*1000);
  }

    // Create "ms0:/ISO/" directory if it doesn't already exist
  dir = sceIoDopen("ms0:/ISO/");
 	if(dir < 0){ 
 		sceIoMkdir(	"ms0:/ISO",0777);	
 	} 
    
    // Create "ms0:/ISO/VIDEO" directory if it doesn't already exist
 	dir = sceIoDopen("ms0:/ISO/VIDEO");
 	if(dir < 0){ 
 		sceIoMkdir(	"ms0:/ISO/VIDEO",0777);	
 	} 

    // determine location to write iso depending on gtype variable [V]ideo or [G]ame]
  if(gtype[0]=='G')
  	sprintf(isopath, "ms0:/ISO/%s.iso",discid);	
  else{
  	int k;
  	for(k=0;k<11;k++)
  		if(discid[k]==' ' || discid[k]==':')
  			discid[k]='_';
  	sprintf(isopath, "ms0:/ISO/VIDEO/%s.iso",discid);		
  }

    // dump UMD to ISO
  iso=sceIoOpen(isopath, PSP_O_WRONLY| PSP_O_CREAT | PSP_O_TRUNC, 0777);

    // if isopath contains invalid characters or MS:0 is unable to be reached, this will result in MS0 error
  if(iso<0){
  	pspDebugScreenClear();
  	pspDebugScreenSetXY(0, 0); printf("CRITICAL ERROR : IMPOSSIBLE TO ACCESS MS0");
    pspDebugScreenSetXY(0, 4); printf("Auto-Exiting in 30 seconds...");
    sceDisplayWaitVblankStart(); sceKernelDelayThread(30*1000*1000);
  }
  
    // if iso will take up more space than available on memory stick, quit
  pspDebugScreenClear();
  read=sceIoRead(umd, umdbuffer,512);
  while(read>0){
		if(!(sceIoWrite(iso, umdbuffer, read * 2048))){
			sceIoClose(iso);
			sceIoRemove(isopath);
			pspDebugScreenClear();
  		pspDebugScreenSetXY(0, 0); printf("Impossible to write to ms. Not enough free space?");
    	pspDebugScreenSetXY(0, 4); printf("Auto-Exiting in 30 seconds...");
    	sceDisplayWaitVblankStart(); sceKernelDelayThread(30*1000*1000);
		}
		
      // if UMD disc throws read errors, quit
    if(!(read=sceIoRead(umd, umdbuffer,512))){
    	sceIoClose(iso);
    	sceIoRemove(isopath);
    	pspDebugScreenClear();
  		pspDebugScreenSetXY(0, 0); printf("UMD read error!");
    	pspDebugScreenSetXY(0, 4); printf("Auto-Exiting in 30 seconds...");
    	sceDisplayWaitVblankStart(); sceKernelDelayThread(30*1000*1000);
    }
  
      // Print status of current dump
    written+=read;
    dumppercent=(written*100)/umdsize;
    pspDebugScreenSetXY(0, 0); printf("%66s", "UMDKillerPRX 2.0");
    pspDebugScreenSetXY(15, 11); printf("Writing to %s",isopath);
		pspDebugScreenSetXY(16, 15); printf("Dumping...Sectors: %d/%d - %d%% ",written,umdsize,dumppercent);
    sceDisplayWaitVblankStart();	
  }
   
  sceIoClose(iso);
  
    // Prompt to reboot PSP after successful dump, and do so when START is pressed
  pspDebugScreenClear();
  pspDebugScreenSetXY(0, 0); printf("%66s", "UMDKillerPRX 2.0");
  pspDebugScreenSetXY(10, 11); printf("Successfully wrote to %s",isopath); 
  pspDebugScreenSetXY(25, 15); printf("PRESS START TO REBOOT");
 
  do{
  	sceCtrlPeekBufferPositive(&pad,1);

	}while(!(pad.Buttons & PSP_CTRL_START));	
    threadschanger( 1, threadlist, threadnumber );
    //sceKernelExitVSHVSH(NULL);
    return 0;
}

int module_start(SceSize args, void *argp)
{
	SceUID thid;

  thid = sceKernelCreateThread( "UMDKiller", st_thread, 30, 0x6000, PSP_THREAD_ATTR_NO_FILLSTACK, 0 );
  if(thid)
  	sceKernelStartThread( thid, args, argp );
  return 0;
}

int module_stop()
{
   return 0;
}

void threadschanger( int stat, SceUID threadlist[], int threadnumber )
{
  int ( *request_stat_func )( SceUID ) = NULL;
  int i, j;
  SceUID selfid = sceKernelGetThreadId();

  if(stat==1)
    request_stat_func = sceKernelResumeThread;

  else if(stat==0)
    request_stat_func = sceKernelSuspendThread;
  
  SceKernelThreadInfo status;

  for( i = 0; i < threadnumber; i++ ){
    int no_target = 0;
    for( j = 0; j < st_thnum_first; j++ ){
      if( threadlist[i] == st_thlist_first[j] || selfid == threadlist[i] ){
        no_target = 1;
        break;
      }
    }

    sceKernelReferThreadStatus(threadlist[i], &status);
    if( ! no_target ) ( *request_stat_func )( threadlist[i] );
  }
}