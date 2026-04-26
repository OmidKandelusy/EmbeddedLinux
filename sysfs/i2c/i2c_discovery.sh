#!/bin/bash

echo "I2C Discovery Tool"
echo ""

# Listing I2C buses from sysfs
echo "I2C Buses (sysfs)"
for bus in /sys/class/i2c-dev/i2c-*; do
    [ -e "$bus" ] || continue

    name=$(basename "$bus")

    echo "$name"

    # device node mapping
    if [ -e "/dev/$name" ]; then
        echo "  device: /dev/$name"
    fi

    # driver / adapter name
    if [ -f "$bus/name" ]; then
        echo "  name: $(cat $bus/name)"
    fi

    # kernel device mapping
    if [ -L "$bus/device" ]; then
        echo "  device path: $(readlink -f $bus/device)"
    fi

    echo ""
done

# scaning the I2C ports:
echo "scanning I2C ports"

if command -v i2cdetect >/dev/null 2>&1; then
    echo "i2cdetect found - scanning buses..."

    for i in /dev/i2c-*; do
        [ -e "$i" ] || continue
        bus=$(basename "$i" | cut -d'-' -f2)

        echo ""
        echo "Scanning /dev/i2c-$bus:"
        i2cdetect -y "$bus"
    done

else
    echo "[WARNING]: i2cdetect not installed"
    echo "Install with: 'sudo apt install i2c-tools'"
    echo ""
    echo "Skipping runtime scan - showing sysfs-only view"
    echo ""
fi