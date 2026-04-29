#!/bin/bash
set -e

# Build (kernel headers must be present on the board)
make

# Load the module
sudo insmod hello.ko

# Verify it loaded
lsmod | grep hello

# Check the kernel log
sudo dmesg | tail -5

# Unload
sudo rmmod hello

# Confirm the exit message
sudo dmesg | tail -5

make clean