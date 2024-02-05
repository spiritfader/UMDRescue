/*
UMDKiller V1.5 PRX EDITION source code
This is the source code of UMDKiller V1.5 PRX EDITION
You can compile this source using PSPSDK.
You can download it from my website.
If you have some questions or problems feel free to send me
and email , or contact me on twitter.

WWW.AVANABOY-CONSOLE.COM
avanaboy@avanaboy-console.com
*/
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

char *umdbuffer,gamesr[11],gamename[17],isopath[25],gtype[3];
int threadnumber,read,fd,umdsize,dir,dumppercent,written=0;
SceUID umd,iso,threadlist[66],st_thlist_first[66],st_thnum_first;


int st_thread( SceSize args, void *argp )
{
  while(1){
    sceKernelDelayThread( 50000 );
    sceCtrlPeekBufferPositive( &pad, 1 );
    
    if((pad.Buttons & PSP_CTRL_NOTE))
      start_dumper();
  }

  return 0;
}




int start_dumper()
{
	
		
	pspDebugScreenInit();
	pspDebugScreenSetBackColor(RGB(0,0,0));
	pspDebugScreenSetTextColor(RGB(0,255,0));
	pspDebugScreenClear();
	
	if(!(umdbuffer=malloc(1048576))){
		pspDebugScreenClear();
  	pspDebugScreenSetXY(0, 0);
    printf("CRITICAL ERROR : IMPOSSIBLE TO ALLOCATE MEMORY");
    pspDebugScreenSetXY(0, 4);
    printf("Auto-Exiting in 30 seconds...");
    sceDisplayWaitVblankStart();
    sceKernelDelayThread(30*1000*1000);
  }
  
	if(sceUmdCheckMedium()==0){
		
   	return 0;
   	
  }
  
  sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, threadlist, 66, &threadnumber );
  threadschanger( 0, threadlist, threadnumber );
  
  sceUmdActivate(1, "disc0:");
	sceUmdWaitDriveStat(PSP_UMD_READY);
	
	fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0777);
	if(fd >= 0){
		sceIoLseek(fd, 0, SEEK_SET);
		sceIoRead(fd, gamesr, 10);
		gamesr[10]=0;
   	sceIoClose(fd);
  }
  
  fd = sceIoOpen("umd0:", PSP_O_RDONLY, 0777);
	if(fd >= 0){
		umdsize=sceIoLseek(fd,0,SEEK_END);
   	sceIoClose(fd);
  }
  
  fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0777); 
	if(fd >= 0){
		sceIoLseek(fd, 0x21, SEEK_SET); 
		sceIoRead(fd, gtype, 3);
		sceIoClose(fd);
 	}
	
	if(gtype[0]=='G'){
  	fd = sceIoOpen("disc0:/PSP_GAME/PARAM.SFO", PSP_O_RDONLY, 0777);
		if(fd >= 0) {
			sceIoLseek(fd, 0x158, SEEK_SET);
			sceIoRead(fd, gamename, 16);
			gamename[16]=0;
  		sceIoClose(fd);
		}
  }
  
  else if(gtype[0]=='V'){
  	fd = sceIoOpen("disc0:/UMD_VIDEO/PARAM.SFO", PSP_O_RDONLY, 0777);
		if(fd >= 0) {
			sceIoLseek(fd, 0x50, SEEK_SET);
			sceIoRead(fd, gamename, 16);
			gamename[16]=0;
  		sceIoClose(fd);
		}
  }
  
   
  pspDebugScreenClear();
	
	do{
		sceCtrlPeekBufferPositive(&pad,1);
		pspDebugScreenSetXY(15, 0);
    printf("UMDKiller V1.5 PRX EDITION BY AVANABOY");
    pspDebugScreenSetXY(21, 2);
    printf("WWW.AVANABOY-CONSOLE.COM");
	  pspDebugScreenSetXY(7, 10);
	  printf("NAME: %s",gamename);
	  pspDebugScreenSetXY(7, 14);
   	printf("TYPE: %s",gtype);
   	pspDebugScreenSetXY(7, 18);
   	printf("SR: %s",gamesr);
   	pspDebugScreenSetXY(7,22);
   	printf("SIZE: %d",umdsize);
		
		pspDebugScreenSetXY(12, 27);
    printf("PRESS X TO DUMP OR PRESS START TO EXIT");
   	
   
   	
    sceDisplayWaitVblankStart();	
    
    if(pad.Buttons & PSP_CTRL_START){
    	threadschanger( 1, threadlist, threadnumber );
 
   		return 0;
   	}
   	
   	
  }while(!(pad.Buttons & PSP_CTRL_CROSS));
  
  
  umd = sceIoOpen("umd0:", PSP_O_RDONLY, 0777); 
  if(umd<0){
  	pspDebugScreenClear();
  	pspDebugScreenSetXY(0, 0);
    printf("CRITICAL ERROR : IMPOSSIBLE TO OPEN UMD0");
    pspDebugScreenSetXY(0, 4);
    printf("Auto-Exiting in 30 seconds...");
    sceDisplayWaitVblankStart();
    sceKernelDelayThread(30*1000*1000);
  }
  
  
  dir = sceIoDopen("ms0:/ISO/");
 	if(dir < 0){ 
 		sceIoMkdir(	"ms0:/ISO",0777);	
 	} 
 	dir = sceIoDopen("ms0:/ISO/VIDEO");
 	if(dir < 0){ 
 		sceIoMkdir(	"ms0:/ISO/VIDEO",0777);	
 	} 
  
  if(gtype[0]=='G')
  	sprintf(isopath, "ms0:/ISO/%s.iso",gamesr);	
  else{
  	int k;
  	for(k=0;k<11;k++)
  		if(gamesr[k]==' ')
  			gamesr[k]='.';
  	sprintf(isopath, "ms0:/ISO/VIDEO/%s.iso",gamesr);		
  }
  iso= sceIoOpen(isopath, PSP_O_WRONLY| PSP_O_CREAT | PSP_O_TRUNC, 0777);
  if(iso<0){
  	pspDebugScreenClear();
  	pspDebugScreenSetXY(0, 0);
    printf("CRITICAL ERROR : IMPOSSIBLE TO ACCESS MS0");
    pspDebugScreenSetXY(0, 4);
    printf("Auto-Exiting in 30 seconds...");
    sceDisplayWaitVblankStart();
    sceKernelDelayThread(30*1000*1000);
  }
  
  
  
  pspDebugScreenClear();
  read=sceIoRead(umd, umdbuffer,512);
  while(read>0){
  		

		if(!(sceIoWrite(iso, umdbuffer, read * 2048))){
			sceIoClose(iso);
			sceIoRemove(isopath);
			pspDebugScreenClear();
  		pspDebugScreenSetXY(0, 0);
    	printf("Impossible to write to ms. Not enough free space?");
    	pspDebugScreenSetXY(0, 4);
    	printf("Auto-Exiting in 30 seconds...");
    	sceDisplayWaitVblankStart();
    	sceKernelDelayThread(30*1000*1000);
		}
		
    if(!(read=sceIoRead(umd, umdbuffer,512))){
    	sceIoClose(iso);
    	sceIoRemove(isopath);
    	pspDebugScreenClear();
  		pspDebugScreenSetXY(0, 0);
    	printf("UMD read error!");
    	pspDebugScreenSetXY(0, 4);
    	printf("Auto-Exiting in 30 seconds...");
    	sceDisplayWaitVblankStart();
    	sceKernelDelayThread(30*1000*1000);
    }
    		 
    written+=read;
    dumppercent=(written*100)/umdsize;
    pspDebugScreenSetXY(15, 0);
    printf("UMDKiller V1.5 PRX EDITION BY AVANABOY");
    pspDebugScreenSetXY(21, 2);
    printf("WWW.AVANABOY-CONSOLE.COM");
		pspDebugScreenSetXY(16, 15);
		printf("Dumping...Sectors: %d/%d - %d%% ",written,umdsize,dumppercent);
    sceDisplayWaitVblankStart();	
   	
  }
  
  sceIoClose(iso);
  
  pspDebugScreenClear();
  pspDebugScreenSetXY(15, 0);
  printf("UMDKiller V1.5 PRX EDITION BY AVANABOY");
  pspDebugScreenSetXY(21, 2);
  printf("WWW.AVANABOY-CONSOLE.COM");
  pspDebugScreenSetXY(10, 15);
  printf("Dumping Completed... PRESS START TO REBOOT");
 
     	
  do{
  	sceCtrlPeekBufferPositive(&pad,1);

	}while(!(pad.Buttons & PSP_CTRL_START));	
   	
   threadschanger( 1, threadlist, threadnumber );
   sceKernelExitVSHVSH(NULL);
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