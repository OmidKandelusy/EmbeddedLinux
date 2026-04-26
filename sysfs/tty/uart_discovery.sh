#!/bin/bash

echo "UART/TTY discovery script"
echo ""

for tty in /sys/class/tty/ttyS*; do
    [ -e "$tty" ] || continue

    name=$(basename "$tty")

    echo "Device: $name"

    # sysfs info
    dev=$(cat "$tty/dev" 2>/dev/null)
    driver=$(readlink -f "$tty/device" 2>/dev/null | awk -F'/' '{print $NF}')

    echo "  sysfs dev:   $dev"
    echo "  driver:      $driver"

    # check if system console
    if grep -q "console=$name" /proc/cmdline 2>/dev/null; then
        echo "  role:        SYSTEM CONSOLE"
    else
        echo "  role:        normal UART"
    fi

    # check userspace usage
    if command -v fuser >/dev/null 2>&1; then
        if fuser "/dev/$name" >/dev/null 2>&1; then
            echo "  status:      IN USE (userspace)"
        else
            echo "  status:      free (no userspace lock)"
        fi
    else
        echo "  status:      fuser not available"
    fi

    echo ""
done

# USB gadget UART discovery
if [ -e /sys/class/tty/ttyGS0 ]; then
    echo "USB Gadget UART:"
    echo "  ttyGS0 present"
    echo ""
fi