UMDKillerPRX is a kernel level UMD disc dumper allowing you to backup your UMD discs for personal use using a PSP.


***Disclaimer***: Only dump UMDs that you legally own and do not share them with anyone. <br/>
This tool is not to be used for piracy under any circumstances.


***Installation***
- Create a `seplugins` folder in the root of your memory stick, denoted as `ms:0` for the rest of these instructions.
- Copy the `UMDKiller.prx` to `ms0:/seplugins`.
- If you do not already have a `VSH.TXT` file in the `ms0:/seplugins` folder, create it or copy over the provided `VSH.TXT`.
  - If you do already have a `VSH.TXT` file, add the line `ms0:/seplugins/UMDKiller.prx 1` to the bottom of the file.
- Power on the PSP **WITH** the memory stick already inserted, then insert the UMD you wish to backup. Wait until it loads, then press the *NOTE* button to launch UMDKillerPRX.
- Press **X** to begin dumping your UMD.
  - Game images will be saved to `ms0:/IS0` 
  - Video images will be saved to `ms0:/ISO/VIDEO`


***Compiling from source***
If you wish to compile from source, you need `PSPSDK (0.11.2r3)`


***Credits***
Original source code and all credit for idea to Avanaboy <br/>
https://web.archive.org/web/20121215031736/http://avanaboy-console.com/