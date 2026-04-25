#!/bin/bash
#
# The AM335x GPIO pins are exposed through the kernel's GPIO subsystem
# at /sys/class/gpio/. Exporting a pin makes it accessible from userspace.
# The kernel uses an offset scheme — pin numbers here are global GPIO numbers,
# not header pin numbers (e.g. P9_12 = gpio-540).
#
# Usage: ./pin_control.sh <gpio_number> <on|off|toggle|status>
# Example: ./pin_control.sh 540 on

set -euo pipefail


if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <gpio_number> <on|off|toggle|status>"
    exit 1
fi

GPIO_NUM="$1"
ACTION="$2"
GPIO_PATH="/sys/class/gpio/gpio${GPIO_NUM}"
EXPORT_PATH="/sys/class/gpio/export"
UNEXPORT_PATH="/sys/class/gpio/unexport"


if [ ! -d "$GPIO_PATH" ]; then
    echo "$GPIO_NUM" > "$EXPORT_PATH"
    sleep 0.1  # give kernel time to create the sysfs entry
fi

DIRECTION=$(cat "$GPIO_PATH/direction")
if [ "$DIRECTION" != "out" ] && [ "$ACTION" != "status" ]; then
    echo out > "$GPIO_PATH/direction"
fi


disp_status() {
    echo "GPIO:       gpio${GPIO_NUM}"
    echo "Direction:  $(cat "$GPIO_PATH/direction")"
    echo "Value:      $(cat "$GPIO_PATH/value")"
}

cleanup() {
    echo "$GPIO_NUM" > "$UNEXPORT_PATH"
}
trap cleanup EXIT

# --- Main ---
case "$ACTION" in
    on)
        echo 1 > "$GPIO_PATH/value"
        echo "gpio${GPIO_NUM} set HIGH"
        ;;
    off)
        echo 0 > "$GPIO_PATH/value"
        echo "gpio${GPIO_NUM} set LOW"
        ;;
    toggle)
        CURRENT=$(cat "$GPIO_PATH/value")
        if [ "$CURRENT" = "1" ]; then
            echo 0 > "$GPIO_PATH/value"
            echo "gpio${GPIO_NUM} toggled LOW"
        else
            echo 1 > "$GPIO_PATH/value"
            echo "gpio${GPIO_NUM} toggled HIGH"
        fi
        ;;
    status)
        disp_status
        ;;
    *)
        echo "Usage: $0 <gpio_number> <on|off|toggle|status>"
        exit 1
        ;;
esac