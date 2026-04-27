# Device Tree Overlays

Device Tree Overlays are used in embedded Linux to configure or modify hardware behavior **at boot time or runtime**, without changing the Linux kernel itself. They allow us to describe how hardware is connected (GPIOs, LEDs, buttons, buses, etc.) so the kernel can correctly initialize and manage it.

## Why Device Tree Exists

The Linux kernel is designed to be **generic across many boards** and unlike the real time operating systems that bake in
hardware description in the compile time, it does not hardcode hardware details like:

- which pin is used for GPIO
- which pins are UART/I2C/SPI
- how LEDs or buttons are wired

Instead, this information is provided externally using a **Device Tree** which allows:

- one kernel to support many boards
- hardware configuration without recompiling the kernel
- flexible experimentation with peripherals

## What is a Device Tree Overlay?

A Device Tree Overlay is a **modular modification** of the base hardware description. Instead of redefining the entire system, an overlay basically patches some specific parts of the existing hardware configuration like

- enable or disable peripherals
- change pin multiplexing
- add new hardware definitions


In order to use the device tree in Linux, we need to compile the device tree files into something that is recognizable to the kernel. This is done using the Linux device tree compiler. I have made a simple shell script to automate the process of working with Device Tree overlays for the following steps:

- Compiling Device Tree source files via `dtc`
- Copying the compiled overlay into `/lib/firmware/` path


