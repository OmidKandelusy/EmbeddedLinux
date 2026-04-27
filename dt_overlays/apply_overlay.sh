#!/bin/bash

set -e

NAME="i2c"
SRC="${NAME}.dts"
DTBO="${NAME}.dtbo"

echo "[1/3] Compiling overlay: $NAME"

dtc -O dtb -o ${DTBO} -b 0 -@ ${SRC}

echo "[2/3] Installing to /lib/firmware"

sudo cp ${DTBO} /lib/firmware/

echo "[3/3] Last step of applying overlay is on you!"
echo "Add the overlay.dtbo to the uEnv.txt in the boot path"
echo "once done, reboot"