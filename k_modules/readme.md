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

# If missing:
sudo apt install linux-headers-$(uname -r)
```

## Modules

| Directory | Description |
|-----------|-------------|
| hello_world | Minimal module demonstrating init/exit lifecycle and printk |