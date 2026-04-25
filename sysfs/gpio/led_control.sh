#!/bin/bash
# led_control.sh — Control BeagleBone Black USR LEDs via sysfs
#
# The BeagleBone Black exposes 4 user LEDs (usr0–usr3) through the
# kernel's LED subsystem at /sys/class/leds/. Writing to 'trigger'
# selects the LED driver mode; 'none' enables manual brightness control.
#
# Usage: ./led_control.sh <0-3> <on|off|blink|status>

set -euo pipefail

# input check
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <led: 0-3> <on|off|blink|status>"
    exit 1
fi

LED_INDEX="$1"
ACTION="$2"
LED="/sys/class/leds/beaglebone:green:usr${LED_INDEX}"

# checking if the led path exists
if [ ! -d "$LED" ]; then
    echo "[Error] LED path not found: $LED"
    exit 1
fi

set_manual_mode() {
    echo none > "$LED/trigger"
}

disp_status() {
    echo "LED:        usr${LED_INDEX}"
    echo "Trigger:    $(cat "$LED/trigger")"
    echo "Brightness: $(cat "$LED/brightness")"
}

case "$ACTION" in
    on)
        set_manual_mode
        echo 1 > "$LED/brightness"
        ;;
    off)
        set_manual_mode
        echo 0 > "$LED/brightness"
        ;;
    blink)
        # kernel's built-in timer trigger for blinking
        echo timer > "$LED/trigger"
        ;;
    status)
        disp_status
        ;;
    *)
        echo "Usage: $0 <led: 0-3> <on|off|blink|status>"
        exit 1
        ;;
esac