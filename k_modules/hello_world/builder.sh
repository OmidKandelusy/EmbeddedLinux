#!/bin/bash
set -e

# Build (kernel headers must be present on the board)
make

# Load the module
sudo insmod hello_world.ko

# Verify it loaded
lsmod | grep hello_world

# Check the kernel log
sudo dmesg | tail -5

# Unload
sudo rmmod hello_world

# Confirm the exit message
sudo dmesg | tail -5

make clean