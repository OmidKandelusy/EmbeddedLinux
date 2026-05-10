# GPIO Character Device Driver

A kernel module that exposes a GPIO interrupt to user space via a misc
character device (`/dev/gpio_event`). A companion user space program
blocks on `read()` and prints each button press event as it arrives.

This demo closes the loop between kernel interrupt handling and user
space I/O — the same pattern used in real Linux device drivers.

## Directory Structure

```
gpio_char_device/
├── gpio_char_dev.c       # Kernel module — IRQ handler + misc char device
├── Makefile              # Kernel build system (out-of-tree module)
├── build_and_deploy.sh   # Build, load, test, and unload in one script
└── userspace/
    ├── gpio_reader.c     # User space reader — blocks on read()
    └── Makefile          # Compiles gpio_reader with gcc
```

## Hardware

- Board: BeagleBone Black
- Pin: P9_12 (GPIO 526)
- Wiring: button between P9_12 and 3.3V, triggered on rising edge

## How It Works

```
Button press
    │
    ▼
Rising-edge IRQ (GPIO 526)
    │
    ▼
button_isr()
    ├── atomic_inc(&press_count)
    └── wake_up_interruptible(&event_wq)
                │
                ▼
        gpio_event_read()        ← user space blocked here on read()
            ├── copies count byte to user buffer
            └── returns to user space
                        │
                        ▼
                gpio_reader.c
                    └── prints "Button press detected — cumulative count: N"
```

## Build and Run

### 1. Kernel module

```bash
chmod +x build_and_deploy.sh
sudo ./build_and_deploy.sh
```

This will build the module, load it, and wait for you to finish testing
before unloading and cleaning up.

### 2. User space reader

In a second terminal, before pressing the button:

```bash
cd userspace/
make
sudo ./gpio_reader
```

## Expected Output

**Kernel log** (`dmesg`):
```
gpio_event: loaded — /dev/gpio_event ready (GPIO 526, IRQ 174)
```

**gpio_reader terminal**:
```
Listening on /dev/gpio_event — press the button (Ctrl+C to quit)

Button press detected — cumulative count: 1
Button press detected — cumulative count: 2
Button press detected — cumulative count: 3
```

## Key Kernel Concepts

| Concept | Where used |
|---|---|
| `misc_register()` | Registers `/dev/gpio_event` without allocating a major number |
| `wait_queue` | Puts the reader to sleep until an interrupt fires |
| `atomic_t` | Safe counter shared between IRQ and process context |
| `wake_up_interruptible()` | Wakes the blocked reader from IRQ context |
| `copy_to_user()` | Safely transfers data from kernel to user space buffer |
| `IRQF_TRIGGER_RISING` | Interrupt fires on rising edge only |