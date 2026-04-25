# About

This repository serves as a laborartory for embedded linux in which I document my
hands-on developments on the BeagleBone Black hardware platform. The goal is to enable practical understanding of low-level Linux concept such hardware accessing and interfacing, the boot process, kernel configuration, and the build systems through small and incremental developments. Note: the initial bring up was carried out via ssh over usb connection as `ssh debian@192.168.7.2`. 

**recommended workflow :** it is recommended that to first clone the repository on the host and then copy the repository over onto the beagle bone black via the secure copy as 

> scp -r ./repo/path/on/host debian@192.168.6.2:~/

Next, for any subsequent updates or development, you can apply the changes to the repository seating on the beaglebone black via resyncing as

> rsync -av ./repo/path/on/host debian@192.168.6.2:~/
 