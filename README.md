# UMDRescue
UMDRescue is a UMD disc dumper allowing you to backup your UMD discs for personal use using a PSP. The kxploit version will create a 1:1 backup unlike the higher Firmware version. This is due to permissions it seems that Sony
decided to bump security up on runlevel permissions later on in higher Firmwares. The None kxploit version will still dump Games fine. Just nothing else on the UMD.


## 1.50 kxploit version

**Note: this only works for 1K's ARK-4 4.20.69 r150 and DCARK with 150Addon, or older 1K's that can run 1.50 Kernel addon, and 2K with older TM that supports 3.40/1.50 mixed mode**

- Extract `GAME150` folder to your memory stick in `ms0:/PSP` 

- Run UMDRescue


## Other version

- Extract `GAME` folder to your memory stick in `ms0:/PSP` 

- Run UMDRescue


## Contributing

If you want to compile it yourself, this project is built and compiled against this custom [SDK docker image](https://hub.docker.com/r/spiritfader/pspdev-plus). 

``podman pull spiritfader/pspdev-plus:latest``

Instructions on how to use this SDK docker image to compile can be found in the [dockerfile repo for this image](https://github.com/spiritfader/pspdev-plus). 

**Use this SDK** as compiling with other SDKs (either containzerized or manually) run the risk of creating compatibility issues as well as making it harder to reproduce said bugs/issues. 

Also I don't use windows cause that shit sucks so especially use this SDK if you run windows.

Basically you can **expect most issues to be ignored unless you're using this SDK** to compile with.
