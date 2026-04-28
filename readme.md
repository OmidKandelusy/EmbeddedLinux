# About

This repository serves as a laborartory for embedded linux in which I document my
hands-on developments on the BeagleBone Black hardware platform. The goal is to enable practical understanding of low-level Linux concept such hardware accessing and interfacing, the boot process, kernel configuration, and the build systems through small and incremental developments. Note: the initial bring up was carried out via ssh over usb connection as `ssh debian@192.168.7.2`. 

**recommended workflow :** it is recommended that to first clone the repository on the host and then copy the repository over onto the beagle bone black via the secure copy as 

> scp -r ./repo/path/on/host debian@192.168.6.2:~/

Next, for any subsequent updates or development, you can apply the changes to the repository seating on the beaglebone black via resyncing as

> rsync -av ./repo/path/on/host debian@192.168.6.2:~/
 
 ## Debugging and Recovery Notes
 During the devlepment it is very likely to brick the board. I did brick the beagleboard black myself early on when I was trying to add examples for using the device tree overlays. So, it is highly recommended that

- have a UART dongle nearby, that will be your safety net for U-Boot recovery.
- have a microSD card, it's the only recivery tool when eMMC is not accessible.
- have the imager tool and a debian image ready, from the [beaglebone page](https://www.beagleboard.org/distros).


### U-Boot Recovery (serial required)
Use your uart-dongle (USB-to-TTL) to connect to the board via the serial debug header where the label of the pins on the board are labeled one to six left to right starting from the label `j1` where the pin 1 is the ground, pin 4 is the rx and pin 5 is the tx [[*See page 73 on the manual*](/doc/beaglebone-black.pdf)]. Note: you do not need to connect the 3.3V pin of the dongle, as the board in not powered by the serial connection. A basic recovery manuver via the U-Boot shell interface for disabling the overlay culprit can be done as follows:

```bash
# Interrupt autoboot (spam key on power on), then:
setenv uboot_overlay_addr0
load mmc 1:3 ${loadaddr} /boot/vmlinuz-6.12.28-bone25
load mmc 1:3 ${fdtaddr} /boot/dtbs/6.12.28-bone25/am335x-boneblack.dtb
load mmc 1:3 ${rdaddr} /boot/initrd.img-6.12.28-bone25
setenv bootargs console=ttyS0,115200n8 root=/dev/mmcblk1p3 rw rootfstype=ext4 rootwait
bootz ${loadaddr} ${rdaddr}:${filesize} ${fdtaddr}
```

### microSD Recovery (when U-Boot can't be interrupted)
In cases when the eMMC drive is not accessible, for example an overlay messed up the derive's description by overwritting it, the only way to recover is through a secondary boot disck. Assuming an sdcard loaded with the debian image is ready, after loging in into the OS on the serial debug, the basic recovery manuver for diabling the culprit overlay can be done as 

```bash
# Mount the emmc and comment out the cuprit!
sudo mount -t ext4 /dev/mmcblk1p3 /mnt/emmc
sudo sed -i 's|uboot_overlay_addr0=|#uboot_overlay_addr0=|' /mnt/emmc/boot/uEnv.txt
sudo umount /mnt/emmc
sudo poweroff
```