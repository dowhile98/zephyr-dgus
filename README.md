# zephyr-dgus

[![CI](https://github.com/tecna-smart-lab/zephyr-dgus/actions/workflows/ci.yml/badge.svg)](https://github.com/tecna-smart-lab/zephyr-dgus/actions/workflows/ci.yml)
[![Zephyr RTOS](https://img.shields.io/badge/Zephyr%20RTOS-v4.0%2B-blue.svg)](https://zephyrproject.org/)
[![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](LICENSE)
[![Documentation](https://img.shields.io/badge/docs-Sphinx%20%7C%20Doxygen-brightgreen.svg)](doc/)

Out-of-tree **Zephyr RTOS** device driver module for **DWIN DGUS II** intelligent displays based on the **T5L ASIC family** (T5L1, T5L2) communicating over UART.

---

## Table of Contents

- [Key Features](#key-features)
- [Architecture](#architecture)
- [Devicetree Model](#devicetree-model)
- [Hardware Wiring & Pinout](#hardware-wiring--pinout)
- [Quick Start](#quick-start)
- [API Overview](#api-overview)
- [Testing & Quality Assurance](#testing--quality-assurance)
- [Documentation Build](#documentation-build)
- [CI/CD Automation](#cicd-automation)
- [Contributing & License](#contributing--license)

---

## Key Features

- **Multi-Instance Devicetree Driver**: Instantiate multiple displays simultaneously (`dwin,dgus2` compatible) attached to separate UART controllers.
- **Hardware-Agnostic Protocol Core**: Pure C99 framing, CRC-16 Modbus computation, and byte-by-byte Finite State Machine (FSM) parser testable on host (`native_sim`) without physical hardware.
- **Robust Concurrency & Thread Safety**:
  - Lockless ring buffers between UART Interrupt Service Routines (ISRs) and the RX worker thread.
  - Non-blocking ISRs: zero mutex locks or delays in interrupt context.
  - Thread-safe TX transactions protected by kernel mutexes (`k_mutex`).
  - Synchronous request/response synchronization with semaphores (`k_sem`) and configurable timeouts.
- **High-Level VP Helpers**:
  - Variable Pointer (VP) read/write operations for `uint16_t`, `uint32_t`, `int16_t`, `int32_t`, strings (with automatic `0xFFFF` termination), and raw byte buffers.
- **Advanced Display Functions**:
  - Active page switching with `0x5A01` magic handshake and synchronous page query.
  - Backlight brightness control (0%–100%) and acoustic buzzer activation (8 ms timebase).
  - Real-time trend curve plotting via DGUS II Command `0x84`.
  - Asynchronous touch button and slider event notification callbacks.
- **Telemetry & Diagnostics**: Framing and checksum error counter for serial bus link health monitoring.

---

## Architecture

The driver strictly adheres to clean layered architecture and separation of concerns:

```
+-----------------------------------------------------------------------+
|                    Application / Sample Code                          |
+-----------------------------------------------------------------------+
                                   |
                                   v  Public API (include/dgus2/dgus2.h)
+-----------------------------------------------------------------------+
|                    Core Driver (src/dgus2_core.c)                     |
|  - Thread synchronization (k_mutex, k_sem)                            |
|  - Synchronous request/response timeout handling                      |
|  - Asynchronous event dispatcher & subscriber table                   |
|  - High-level VP encoding/decoding helpers                            |
+-----------------------------------------------------------------------+
               |                                           |
               v                                           v
+-----------------------------------+     +-----------------------------+
| Protocol Layer (dgus2_protocol.c) |     |  HAL Adapter (dgus2_hal.c)  |
| - Pure C99 FSM parser             |     |  - Zephyr UART Device API   |
| - Packet builders (0x80 - 0x84)   |     |  - Ring buffer & RX worker  |
| - CRC-16 Modbus engine            |     |  - Interrupt-driven UART    |
+-----------------------------------+     +-----------------------------+
                                                         |
                                                         v
                                          +-----------------------------+
                                          | Hardware UART Controller    |
                                          +-----------------------------+
```

### Layer Breakdown

1. **`dgus2_protocol`**: Operates purely in memory. No RTOS or hardware includes. Completely deterministic and unit-testable.
2. **`dgus2_core`**: Manages driver instances, thread safety, synchronization, and user callback dispatches.
3. **`dgus2_hal`**: The only point of coupling with the hardware; uses Zephyr's `CONFIG_UART_INTERRUPT_DRIVEN` API and lockless ring buffers.

---

## Devicetree Model

Displays are configured in Devicetree using the `dwin,dgus2` compatible binding (`dts/bindings/display/dwin,dgus2.yaml`).

### DTS Node Example

```dts
/ {
    chosen {
        zephyr,dgus = &dwin_screen;
    };

    dwin_screen: dgus2 {
        compatible = "dwin,dgus2";
        uart = <&usart2>;
        status = "okay";
        current-speed = <115200>;
        reset-gpios = <&gpioc 13 GPIO_ACTIVE_LOW>; /* Optional */
    };
};

&usart2 {
    status = "okay";
    current-speed = <115200>;
    pinctrl-0 = <&usart2_tx_pa2 &usart2_rx_pa3>;
    pinctrl-names = "default";
};
```

### Supported Devicetree Properties

| Property | Type | Required | Description |
|---|---|---|---|
| `compatible` | string | **Yes** | Must be `"dwin,dgus2"` |
| `uart` | phandle | **Yes** | Phandle pointing to UART device controller |
| `current-speed` | int | No | Initial baud rate (defaults to `115200`) |
| `reset-gpios` | phandle-array | No | Optional hardware reset GPIO pin |

---

## Hardware Wiring & Pinout

DWIN displays require a 5V power supply and communicate via 3.3V/5V TTL UART.

```
       +-----------------------+              +------------------------+
       |   STM32 Board (MCU)   |              |   DWIN DGUS II Screen  |
       |                       |              |                        |
       |              MCU TX   |------------->| RXD (TTL)              |
       |              MCU RX   |<-------------| TXD (TTL)              |
       |                 GND   |--------------| GND                    |
       +-----------------------+              |                        |
                                              | 5V Power Supply        |
                                              +------------------------+
```

### Reference Board Configurations

| Board | Zephyr Target | UART | MCU TX $\to$ Screen RX | MCU RX $\leftarrow$ Screen TX | Notes |
|---|---|---|---|---|---|
| **STM32F4-Discovery** | `stm32f4_disco` | `USART2` | `PA2` | `PA3` | Connect common GND |
| **ST Nucleo-144 F429ZI** | `nucleo_f429zi` | `USART3` | `PD8` (CN11 pin 10) | `PD9` (CN11 pin 12) | Connect common GND |
| **ST Nucleo-144 H753ZI** | `nucleo_h753zi` | `USART3` | `PD8` (CN11 pin 10) | `PD9` (CN11 pin 12) | Connect common GND |
| **Native POSIX Simulator** | `native_sim` | PTY UART | Simulated loopback | Simulated loopback | Software testing |

---

## Quick Start

### 1. Include in Your Zephyr Workspace

Add the module to your `west.yml` manifest:

```yaml
manifest:
  projects:
    - name: zephyr-dgus
      url: https://github.com/tecna-smart-lab/zephyr-dgus
      revision: main
      path: modules/lib/zephyr-dgus
```

Run `west update` to synchronize the repository.

### 2. Enable Kconfig Options

Enable the driver in your `prj.conf`:

```ini
CONFIG_DGUS2_DRIVER=y
CONFIG_SERIAL=y
CONFIG_UART_INTERRUPT_DRIVEN=y
CONFIG_DGUS2_MAX_SUBSCRIBERS=16
```

### 3. Basic Application Example

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <dgus2/dgus2.h>

/* Callback for touch events */
static void on_touch_button(uint16_t vp_addr, const uint8_t *data, uint16_t len, void *user_data)
{
    uint16_t button_val = (data[0] << 8) | data[1];
    printk("Button pressed on VP 0x%04X: value %u\n", vp_addr, button_val);
}

int main(void)
{
    const struct device *dgus = DEVICE_DT_GET(DT_CHOSEN(zephyr_dgus));

    if (!device_is_ready(dgus)) {
        printk("Display device not ready!\n");
        return -EIO;
    }

    /* Initialize display */
    dgus2_init(dgus);

    /* Set active screen to Page 1 */
    dgus2_set_page(dgus, 1);

    /* Set brightness to 80% */
    dgus2_set_brightness(dgus, 80);

    /* Write 16-bit sensor reading to VP 0x1000 */
    dgus2_write_vp_u16(dgus, 0x1000, 245); /* 24.5 °C */

    /* Write label to VP 0x2000 */
    dgus2_write_vp_string(dgus, 0x2000, "System OK");

    /* Register touch listener on VP 0x3000 */
    dgus2_register_vp_callback(dgus, 0x3000, on_touch_button, NULL);

    while (1) {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
```

---

## API Overview

Full API documentation is available in `include/dgus2/dgus2.h` and the generated HTML docs.

### System & Control Functions
- `dgus2_init(dev)`: Initialize driver instance and start RX thread.
- `dgus2_set_page(dev, page_id)`: Switch active display page.
- `dgus2_get_page_sync(dev, &page_id, timeout)`: Query active page synchronously.
- `dgus2_set_brightness(dev, pct)`: Set backlight level (0–100%).
- `dgus2_trigger_buzzer(dev, duration_ms)`: Beep internal buzzer.

### Variable Pointer (VP) Operations
- `dgus2_write_vp_u16(dev, vp, val)`: Write 16-bit unsigned integer.
- `dgus2_write_vp_u32(dev, vp, val)`: Write 32-bit unsigned integer.
- `dgus2_write_vp_i16(dev, vp, val)`: Write 16-bit signed integer.
- `dgus2_write_vp_i32(dev, vp, val)`: Write 32-bit signed integer.
- `dgus2_write_vp_string(dev, vp, str)`: Write null-terminated text.
- `dgus2_write_vp_bytes(dev, vp, buf, len)`: Write raw byte buffer.
- `dgus2_read_vp_sync(dev, vp, buf, max, &len, timeout)`: Synchronously read raw bytes.
- `dgus2_read_vp_u16_sync(dev, vp, &val, timeout)`: Synchronously read 16-bit integer.
- `dgus2_read_vp_u32_sync(dev, vp, &val, timeout)`: Synchronously read 32-bit integer.

### Advanced Features
- `dgus2_write_curve(dev, channel, points, count)`: Stream trend curve waveform (Command 0x84).
- `dgus2_register_vp_callback(dev, vp, cb, user_data)`: Subscribe to touch/VP asynchronous events.
- `dgus2_unregister_vp_callback(dev, vp, cb)`: Unsubscribe callback.
- `dgus2_get_parse_error_count(dev)`: Read total framing/parse error counter.

---

## Testing & Quality Assurance

Unit tests are written using Zephyr's native **Ztest** framework and run locally on `native_sim` without target hardware:

```bash
# Source Zephyr environment
source /home/tecna/zephyrproject/zephyr/zephyr-env.sh

# Run all unit tests via Twister
west twister -T tests/ -p native_sim --verbose
```

### Test Coverage Highlights
- FSM parser validation for all valid commands (`0x80`, `0x81`, `0x82`, `0x83`, `0x84`).
- Corrupted frame detection, sync byte recovery, payload truncation, and length overflow.
- Modbus CRC-16 computation accuracy against standard test vectors.
- Multi-instance concurrent callbacks and subscriber unregistration safety.

---

## Documentation Build

Documentation is generated using **Doxygen**, **Sphinx**, and **Breathe**:

```bash
# Generate documentation locally
./scripts/build-docs.sh
```

Or manually:

```bash
cd doc
doxygen Doxyfile
sphinx-build -b html . _build_sphinx/html
```

Generated HTML output will be placed in `doc/_build_sphinx/html/index.html`.

---

## CI/CD Automation

This project uses GitHub Actions conforming to official Zephyr RTOS project conventions:

- **`twister-tests`**: Runs unit test suites on `native_sim` with Twister and generates XML test reports.
- **`sample-builds`**: Matrix build compiling `samples/dwin-interface` across multiple architectures:
  - `native_sim`
  - `stm32f4_disco`
  - `nucleo_f429zi`
  - `nucleo_h753zi`
- **`documentation`**: Generates Doxygen XML and Sphinx HTML, validating zero warnings (`-W`), and uploads artifacts.
- **`compliance`**: Enforces coding standards using `clang-format`.

Run local CI checks in one command:

```bash
./scripts/ci.sh
```

---

## Contributing & License

Contributions are welcome! Please follow Zephyr RTOS coding style and ensure all tests and format checks pass.

Distributed under the **Apache-2.0 License**. See [LICENSE](LICENSE) for details.
