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


## Creating a new device tree overlay
The best practice when creating a new overlay for a peripheral or hardware on the beaglebone black is to first check what is already configured on the device tree of the running linux. This guide documents that investigation process step by step, using the PWM peripheral as a concrete worked example.

**Platform**: BeagleBone Black  
**Kernel**: 6.12.28-bone25  
**Worked Example**: EHRPWM1A PWM output on P9_14

## Step 1 — Check Userspace Visibility

The first check is simple, does the kernel already expose anyhting for this peripheral to userspace?
```bash
ls /sys/class/pwm/
```
Each peripheral class has a standard location under `/sys/class/`. If the directory is empty or the subdirectory is absent, the kernel has not yet brought this hardware up. If it lists devices (e.g. `pwmchip0`), the hardware is already active and you may not need an overlay at all.

**What we found**: empty — no PWM exposed.

## Step 2 — Check if the Kernel Driver is Loaded

An empty `/sys/class/` entry does not mean the driver is missing. The driver may be loaded and waiting for a device tree node to bind to.

```bash
ls /sys/bus/platform/drivers/ehrpwm/
```

A driver directory containing only `bind`, `uevent`, and `unbind` means the driver is present in the kernel but has nothing bound to it yet. This is the ideal situation — the overlay just needs to introduce the hardware node and the driver will bind automatically.

**What we found**: driver loaded, nothing bound.

## Step 3 — Examine the Base Device Tree Structure

The next question is whether the peripheral nodes exist in the base device tree at all. On modern mainline kernels for the BBB, many peripherals are absent from the base DTB and must be introduced entirely by an overlay.

```bash
sudo ls /proc/device-tree/ocp/ | grep epwm
```

If this returns nothing, the nodes are absent. Also check the actual DTB file on disk by decompiling it:

```bash
sudo dtc -I dtb -O dts /boot/dtbs/$(uname -r)/am335x-boneblack.dtb 2>/dev/null \
    | grep -A5 "epwmss\|ehrpwm"
```

The decompiled DTB will reveal two important things. First, whether the nodes exist (possibly under a different path than expected). Second, the `__symbols__` section at the bottom will show the label names assigned to each node — these are the `&label` references used in overlay source files.

**What we found**: the nodes exist in the base DTB but are set to `status = "disabled"`, and are nested under `interconnect@48000000/segment@300000/target-module@2000/` rather than directly under `ocp/`. The symbols section confirmed the labels `epwmss1` and `ehrpwm1` are valid.

## Step 4 — Understand the Hardware Address Map

The AM335x SoC assigns fixed base addresses to each peripheral block. These addresses come from the Texas Instruments AM335x Technical Reference Manual and never change regardless of software configuration. They are used as node identifiers in the device tree.

| Peripheral Block | Base Address | Contains |
|---|---|---|
| EPWMSS0 | `0x48300000` | EHRPWM0, ECAP0, EQEP0 |
| EPWMSS1 | `0x48302000` | EHRPWM1, ECAP1, EQEP1 |
| EPWMSS2 | `0x48304000` | EHRPWM2, ECAP2, EQEP2 |

Each EPWMSS block occupies `0x2000` bytes (8KB) of address space, which explains the `0x2000` increment between modules. The EHRPWM peripheral inside each subsystem sits at offset `+0x200` from its parent, giving the child address `0x48302200` for EHRPWM1.

## Step 5 — Find the Pinmux Controller Path

The pinmux controller manages which function each physical pin serves. Before writing the pin configuration in the overlay, you need to know the actual path and driver name the kernel uses for this controller, since it changed between kernel versions.

```bash
sudo find /sys/kernel/debug/pinctrl/ -name "pins" 2>/dev/null
```

**What we found on kernel 6.12**: the controller is at  
`/sys/kernel/debug/pinctrl/44e10800.pinmux-pinctrl-single/pins`  
(older kernels used `44e10800.pinmux`)

## Step 6 — Read the Current Pin State

With the correct path known, read the current state of the target pin:

```bash
sudo cat /sys/kernel/debug/pinctrl/44e10800.pinmux-pinctrl-single/pins | grep "pin 18"
```

Pin numbers are derived from the hardware register offset:

```
pin_number = (register_address - 0x44e10800) / 4

P9_14 register = 0x44e10848
pin number     = (0x44e10848 - 0x44e10800) / 4 = 0x48 / 4 = 18
```

The output shows the current register value. The low nibble of that value is the mux mode:

| Mode | Function |
|---|---|
| 7 | GPIO (default) |
| 6 | EHRPWM1A output |
| 0–5 | Other peripherals |

**What we found**: `00000027` — mode 7, GPIO. Needs to change to mode 6.

## Step 7 — Discover the Correct Pinmux Entry Format

This is the step most overlay guides omit. The `pinctrl-single` driver on kernel 6.12 requires **three values** per pin entry, not two:

```
<offset   input_config   mux_mode>
```

The three-value format was confirmed by decompiling the base DTB and examining existing pin groups:

```bash
sudo dtc -I dtb -O dts /boot/dtbs/$(uname -r)/am335x-boneblack.dtb 2>/dev/null \
    | grep -A3 "pinmux-single\|pinctrl-single" | head -40
```

Example from the base DTB:
```
uart0-pins {
    pinctrl-single,pins = <0x170 0x30 0x00   0x174 0x00 0x00>;
```

The three fields are:
- **offset**: register address minus pinmux base (`0x44e10848 - 0x44e10800 = 0x48`)
- **input_config**: `0x00` = output, no pull | `0x10` = input | `0x30` = input + pullup
- **mux_mode**: `0x06` for EHRPWM1A on P9_14

A working overlay must also be compiled with the `-@` flag to preserve symbol references:

```bash
dtc -O dtb -o pwm.dtbo -b 0 -@ pwm.dts
```

## Step 8 — Check Existing Overlays

Before writing the new overlay, confirm what is already loaded to avoid conflicts:

```bash
cat /boot/uEnv.txt | grep overlay
```

On this system, overlays are loaded by U-Boot at addresses `uboot_overlay_addr0` through `addr7`. The `addr0` slot was occupied by the I2C overlay, so the PWM overlay was placed at `addr1`.


## Verification After Reboot

```bash
# Confirm PWM chip appeared
ls /sys/class/pwm/

# Check kernel log for any errors
sudo dmesg | grep -i "pwm\|ehrpwm\|pinmux"

# Test the output manually
echo 20000000 | sudo tee /sys/class/pwm/pwmchip0/pwm0/period
echo 10000000 | sudo tee /sys/class/pwm/pwmchip0/pwm0/duty_cycle
echo 1        | sudo tee /sys/class/pwm/pwmchip0/pwm0/enable
```

At 50% duty cycle on a 3.3V signal, a multimeter on P9_14 to P9_1 (GND) should read approximately 1.65V.

## Another case study

This example shows that when we employ this investigation methodology, we sometimes
end up with a much simpler case — just enabling a peripheral that the base DTB already knows
about. This is what happened with the I2C2 bus on P9_19 and P9_20.

Starting with the same first question — is the hardware already visible to userspace:
```bash
ls /sys/class/i2c-dev/
```
This returned `i2c-0` only. I2C0 is the BBB's internal bus used for the onboard power
management chip and cape EEPROM detection — it is always active and not meant for general use.
I2C2 is the user-accessible bus and it was absent, so the investigation continues.

Rather than going straight to the pinmux registers, the next step is to check whether the
node even exists in the base DTB:

```bash
sudo dtc -I dtb -O dts /boot/dtbs/$(uname -r)/am335x-boneblack.dtb 2>/dev/null \
    | grep -A5 "i2c2-pins"
```

This immediately revealed that unlike the PWM case, the base DTB already contains a fully
defined `i2c2-pins` group with the correct pinmux configuration for both pins. The symbols
section of the DTB confirmed the labels are valid:

```
i2c2      = "/ocp/interconnect@48000000/segment@100000/target-module@9c000/i2c@0";
i2c2_pins = "/.../pinmux@800/i2c2-pins";
```

This means the hardware node exists, the pin group is defined, and the kernel driver is
present — the peripheral is simply not activated. No register offset calculation, no
three-value pinmux entry, no hardware address research needed. The overlay just needs to
reference the existing pin group and flip the status to `okay`.

```dts
/dts-v1/;
/plugin/;

/*
 * Device Tree Overlay for I2C2
 *
 * Hardware target : BeagleBone Black
 * Kernel tested   : 6.12.28-bone25
 * Pins            : P9_19 (SCL), P9_20 (SDA)
 *
 * Investigation summary:
 *   - /sys/class/i2c-dev/ showed i2c-0 only, not i2c-2
 *   - Base DTB confirmed i2c2 node exists but status = disabled
 *   - i2c2-pins group already fully defined in the pinmux controller
 *   - No new pinmux configuration needed, only node activation required
 *
 * After loading:
 *   /sys/class/i2c-dev/i2c-2 appears
 *   Bus is accessible via /dev/i2c-2
 */

&i2c2 {
    pinctrl-names = "default";
    pinctrl-0 = <&i2c2_pins>;
    status = "okay";
};
```

After compiling and deploying this overlay and rebooting, the verification steps confirmed
a clean result:

```bash
ls /sys/class/i2c-dev/
# i2c-0  i2c-2

sudo dmesg | grep -i "i2c"
# [    2.021018] i2c_dev: i2c /dev entries driver
# [    3.202747] omap_i2c 4819c000.i2c: bus 2 rev0.11 at 100 kHz
# [    3.531236] omap_i2c 44e0b000.i2c: bus 0 rev0.11 at 400 kHz
```

I2C2 bound cleanly at 100 kHz standard mode with no errors. The key takeaway is that the investigation methodology is the same regardless of the peripheral — it is the findings that determine how much work the overlay needs to do. In this case the answer was: very little.

## Investigation Checklist Summary

| Step | Command | What you are determining |
|---|---|---|
| 1 | `ls /sys/class/<peripheral>/` | Is the hardware already exposed to userspace? |
| 2 | `ls /sys/bus/platform/drivers/<driver>/` | Is the kernel driver loaded? |
| 3 | `ls /proc/device-tree/ocp/` | Do the device tree nodes exist? |
| 4 | TI TRM / base DTB decompile | What are the hardware base addresses and node labels? |
| 5 | `find /sys/kernel/debug/pinctrl/ -name pins` | What is the pinmux controller path on this kernel? |
| 6 | `cat <pinctrl_path>/pins \| grep "pin N"` | What mode is the target pin currently in? |
| 7 | Decompile base DTB, examine existing pin groups | What is the exact pinctrl-single entry format? |
| 8 | `cat /boot/uEnv.txt \| grep overlay` | Which overlay slots are already occupied? |