#!/bin/bash
set -e

# Build (kernel headers must be present on the board)
make

# Load the module
sudo insmod button_irq.ko

# Verify it loaded
lsmod | grep button_irq

# Check the kernel log
sudo dmesg | tail -5

# Unload
sudo rmmod button_irq

# Confirm the exit message
sudo dmesg | tail -5

make clean