#!/bin/bash
set -e

# Build
make

# Load the module
sudo insmod gpio_char_dev.ko

# Verify it loaded
lsmod | grep gpio_char_dev

# Check load message — should show /dev/gpio_event ready
sudo dmesg | tail -5

echo ""
echo "Module loaded. /dev/gpio_event is ready."
echo "In another terminal, run:  cat /dev/gpio_event | xxd"
echo "Then press your button to see events appear."
echo ""
echo "Press ENTER when done testing to unload the module."
read

# Show full interaction log
echo "Kernel log after session:"
sudo dmesg | tail -20

# Unload
sudo rmmod gpio_char_dev

# Confirm exit message
sudo dmesg | tail -5

make clean