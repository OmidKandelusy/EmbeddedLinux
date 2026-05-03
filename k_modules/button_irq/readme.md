# GPIO Interrupt Kernel Module 

## What We Built

A Linux kernel module that registers a GPIO interrupt on the BeagleBone Black.
When a pin transitions from LOW to HIGH (rising edge), the kernel fires an
interrupt handler that prints a message to the kernel log.

## Key Lessons

### 1. GPIO Numbering Has Changed in Newer Kernels

The old formula `(bank × 32) + pin` no longer gives you the correct GPIO number.
Newer kernels assign a dynamic base offset. The only reliable source of truth is:

```bash
sudo cat /sys/kernel/debug/gpio
```

This shows every GPIO with its header pin name (e.g. `P8_16`) and its current
kernel GPIO number. On this board, the base starts at 512, so:

- GPIO1_14 (old number 46) → gpio-526
- GPIO1_15 (old number 47) → gpio-527

**Always read the GPIO number from this file, never calculate it manually.**

### 2. Finding a Free Pin

From the `/sys/kernel/debug/gpio` output, free pins have no function label in
brackets. Pins showing `[hdmi]`, `[mmc1]`, `[spi]` etc. are claimed by
peripherals and must not be used.

We chose **P8_16 = gpio-526** because it had no function label.

### 3. Physical Pin Counting on the Header

The P8 and P9 headers each have 46 pins arranged in 2 columns of 23 rows.

- **Pin 1** is marked by a **square solder pad** (all others are round)
- On P8, pin 1 is on the **inner side** (closer to board center)
- **Odd pins** (1,3,5...) are on the inner column
- **Even pins** (2,4,6...) are on the outer edge column
- Pins increment down the rows: row 8 = pins 15 (inner) and 16 (outer)

**Counting errors are easy — always verify with a multimeter or by probing
with a known GPIO output before trusting your pin location.**

### 4. GPIO Ownership — Module vs Sysfs

Only one owner can hold a GPIO at a time:

- When the **module is loaded**, it calls `gpio_request()` and owns the pin.
  The sysfs file `/sys/class/gpio/gpioN/value` disappears.
- When using **sysfs** (`echo 526 > /sys/class/gpio/export`), the module
  cannot load — `gpio_request()` will fail.

Always unexport before loading the module:

```bash
echo 526 > /sys/class/gpio/unexport
sudo insmod button_irq.ko
```

### 5. Pull-up / Pull-down Resistors Are Essential

A GPIO input pin with nothing connected **floats** — its value is undefined
and unpredictable. For an interrupt to fire reliably you need a defined
resting voltage:

- **Pull-up resistor (10kΩ) to 3.3V** → pin rests HIGH → use `IRQF_TRIGGER_FALLING`
- **Pull-down resistor (10kΩ) to GND** → pin rests LOW → use `IRQF_TRIGGER_RISING`

The internal pull-up on P8_16 was configured in the pinmux register (`0x27`)
but was not reliably overcoming the floating state. An **external resistor
is always the correct solution** in real hardware.

Without a resistor, we worked around it by:
- Setting trigger to `IRQF_TRIGGER_RISING`
- Using another GPIO (gpio524 / P8_12) driven HIGH as the signal source
- Touching P8_12 to P8_16 to simulate a button press

### 6. Verifying the Interrupt

Three ways to confirm the interrupt is working:

```bash
# 1. Watch kernel log live
sudo dmesg -w | grep gpio_irq

# 2. Check interrupt counter (increments each time IRQ fires)
cat /proc/interrupts | grep gpio_irq

# 3. Check GPIO ownership and state
sudo cat /sys/kernel/debug/gpio | grep 526
```

The `/proc/interrupts` counter is especially useful — if it increments but
`dmesg` shows nothing, the IRQ is firing but printk may be rate-limited.

### 7. Switch Bounce Is Normal

When the interrupt fired, it triggered multiple times per touch:

```
[gpio_irq] Button interrupt triggered!
[gpio_irq] Button interrupt triggered!
[gpio_irq] Button interrupt triggered!
```

This is **contact bounce** — a single physical touch creates multiple rapid
electrical transitions. It happens with both wires and real buttons.
The fix is software debouncing using a kernel timer to ignore subsequent
triggers within a short window (e.g. 200ms).

## Correct Wiring (with resistor)

```
P9_3 (3.3V) ──[10kΩ]──┬── P8_16 (gpio-526)
                        │
                      [button]
                        │
                       GND
```

With this wiring, change trigger back to `IRQF_TRIGGER_FALLING`.

## Useful Commands Reference

```bash
# Find GPIO numbers from header pin names
sudo cat /sys/kernel/debug/gpio

# Check pinmux register values
sudo cat /sys/kernel/debug/pinctrl/44e10800.pinmux-pinctrl-single/pins

# Test a pin without loading the module
echo 526 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio526/direction
cat /sys/class/gpio/gpio526/value
echo 526 > /sys/class/gpio/unexport

# Drive a pin HIGH (useful for testing input pins)
echo 524 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio524/direction
echo 1 > /sys/class/gpio/gpio524/value

# Watch interrupt counter
watch -n 0.5 'cat /proc/interrupts | grep gpio_irq'

# Fix clock skew warnings (set correct date)
sudo date -s "2026-05-03 12:00:00"
```

## Module Quick Reference

```c
#define BUTTON_GPIO  526        // P8_16 on this kernel

// Rising edge (pin rests LOW, signal goes HIGH to trigger)
IRQF_TRIGGER_RISING

// Falling edge (pin rests HIGH via pull-up, goes LOW to trigger)
IRQF_TRIGGER_FALLING
```


## Next Steps

- Add a 10kΩ pull-down resistor and wire a real button
- Implement debouncing with a kernel `hrtimer`
- Use the interrupt to toggle one of the onboard USR LEDs:
  `/sys/class/leds/beaglebone:green:usr0/`
- Explore the descriptor-based GPIO API (`gpiod_get()`) used in kernels 4.8+