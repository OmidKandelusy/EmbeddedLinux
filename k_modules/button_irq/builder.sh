#!/bin/bash
set -e

# Build
make

# Load the module
sudo insmod button_irq.ko

# Verify it loaded
lsmod | grep button_irq

# Check load message
sudo dmesg | tail -5

echo ""
echo "Module loaded. Press your button now..."
echo "Press ENTER when done testing to unload the module."
read  # waits here for you to press ENTER

# Show interrupt log
echo "Kernel log after button presses:"
sudo dmesg | tail -20

# Unload
sudo rmmod button_irq

# Confirm exit message
sudo dmesg | tail -5

make clean