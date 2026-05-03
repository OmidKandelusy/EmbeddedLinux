# Kernel Modules

Out-of-tree kernel modules built and tested on a BeagleBone Black (AM335x, Linux 5.x).
Each subdirectory is a self-contained module with its own source, Makefile, and load script.


## How Kernel Modules Work

A kernel module is an object file (`.ko`) that is dynamically linked into the running kernel at load time. Unlike userspace programs, modules run in kernel space — no libc, no user memory protection, and a bug can panic the entire system.

Every module has two entry points defined:

```c
static int __init hello_init(void)  { ... }   // runs on insmod
static void __exit hello_exit(void) { ... }   // runs on rmmod

module_init(hello_init);
module_exit(hello_exit);
```
`__init` and `__exit` are macros that place these functions in special ELF sections so the kernel can discard them from memory after use.


## Kbuild

Modules are not compiled with a plain gcc call. They are built using the kernel's own build system (Kbuild), which supplies the correct flags, include paths, and kernel symbol table. The minimal Makefile for an out-of-tree module requires the path to the kernel build system and the object declaration like
```makefile
obj-m += hello.o

KDIR ?= /lib/modules/$(shell uname -r)/build
```
where `obj-m` tells Kbuild to build `hello.c` as a loadable module. `KDIR` points to the kernel headers for the running kernel. The build output lands in the module's own directory.


## General Workflow

```bash
make                  # produces hello.ko
sudo insmod hello.ko  # loads module into running kernel
lsmod | grep hello    # confirm it is loaded
sudo dmesg | tail -5  # read kernel log output
sudo rmmod hello      # unload module
make clean            # remove build artifacts
```

## Prerequisites

Kernel headers must be present on the board:

```bash
ls /lib/modules/$(uname -r)/build
```

By default, the linux hedear files will not be probably present on the beagle bone black. So, you would need to install the headers. To do so, we first check the version of the current kernel running on the board:

```bash
uname -r
```

Next, we query the repo metadata to find the exact `.deb` filename matching the kernel version:

```bash
curl -L "http://repos.rcn-ee.com/debian/dists/bookworm/main/binary-armhf/Packages.gz" -o Packages.gz && gunzip Packages.gz && grep -A10 "6.12.28-bone25" Packages
```
Look for the `Filename:` field under the `linux-headers` entry and then download the Headers Package on your host
machine because the beagle bone connect (the base mode) does not have the Wi-Fi:
```bash
curl -L "http://repos.rcn-ee.com/debian/pool/main/l/linux-upstream/linux-headers-6.12.28-bone25_1bookworm_armhf.deb" -o linux-headers-6.12.28-bone25.deb
```
Note that the file is saved in whichever directory you run this command from. Once downloaded, we transfer the file to the
beagle bone black via secure copy and then install it:
```bash
scp linux-headers-6.12.28-bone25.deb debian@<BBB_IP>:/home/debian/

sudo dpkg -i linux-headers-6.12.28-bone25.deb
```
You can verify that the header were installed correctly by looking at the build folder:
```bash
ls /lib/modules/$(uname -r)/build
```
You should see: `Makefile  Module.symvers  arch  include  scripts`

Furthermore, there might be some warning regarind the time and data mismatch. The beagle bone black clock is often
out of sync which causes harmless but noisy build warnings. You can fix it with:
```bash
sudo date -s "$(date '+%Y-%m-%d %H:%M:%S')"
```

# Current Modules

| Directory | Description |
|-----------|-------------|
| hello_world | Minimal module demonstrating init/exit lifecycle and printk |
| button_irq  | it shows how to add interrupt to a gpio pin and get a callback |