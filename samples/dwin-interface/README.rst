.. zephyr:code-sample:: dwin_interface
   :name: DWIN DGUS II Display Interface
   :relevant-api: dgus2

   Interface with DWIN DGUS II (T5L) intelligent screens via UART in Zephyr RTOS.

Overview
********

This sample demonstrates bidirectional communication with a DWIN DGUS II (T5L ASIC)
smart display using the native ``zephyr-dgus`` driver.

Key capabilities demonstrated:

* Initializing display parameters (page selection, backlight brightness, acoustic buzzer).
* Transmitting string labels and numeric variables (16-bit and 32-bit VP registers).
* Streaming real-time trend curve points using Command ``0x84``.
* Receiving asynchronous touch events and button codes via callback registration.
* Querying synchronous data with timeout handling.
* Parsing error tracking and bus health diagnostics.

Hardware Requirements
*********************

* A DWIN DGUS II smart display (T5L1 or T5L2 platform, TTL UART interface).
* One of the supported development boards:
  - ``stm32f4_disco`` (STM32F407VG Discovery)
  - ``nucleo_f429zi`` (ST Nucleo-144 F429ZI)
  - ``nucleo_h753zi`` (ST Nucleo-144 H753ZI)
  - ``native_sim`` (POSIX simulation)
* 5V external power supply for the DWIN screen.
* Common ground connection between MCU and display.

Wiring and Pin Connections
**************************

Connect the DWIN display UART pins to the corresponding board pins as follows:

+-------------------+---------------+------------------+------------------+---------------+
| Board             | Peripheral    | MCU TX -> DWIN RX| MCU RX <- DWIN TX| GND / VCC     |
+===================+===============+==================+==================+===============+
| ``stm32f4_disco`` | USART2        | PA2              | PA3              | Common GND/5V |
+-------------------+---------------+------------------+------------------+---------------+
| ``nucleo_f429zi`` | USART3        | PD8 (CN11 p10)   | PD9 (CN11 p12)   | Common GND/5V |
+-------------------+---------------+------------------+------------------+---------------+
| ``nucleo_h753zi`` | USART3        | PD8 (CN11 p10)   | PD9 (CN11 p12)   | Common GND/5V |
+-------------------+---------------+------------------+------------------+---------------+
| ``native_sim``    | PTY UART (0)  | Simulated PTY    | Simulated PTY    | N/A           |
+-------------------+---------------+------------------+------------------+---------------+

.. note::
   Always ensure proper level matching (DWIN displays standardly expect 3.3V/5V TTL signals).

Variable Pointer (VP) Mapping
*****************************

+---------------+-----------------------+------------------------------------------+
| VP Address    | Type / Purpose        | Description                              |
+===============+=======================+==========================================+
| ``0x1000``    | ``int16_t``           | Simulated temperature (°C x 10)          |
+---------------+-----------------------+------------------------------------------+
| ``0x1002``    | ``uint16_t``          | Simulated atmospheric pressure (hPa)     |
+---------------+-----------------------+------------------------------------------+
| ``0x1004``    | ``uint32_t``          | System uptime counter (seconds)          |
+---------------+-----------------------+------------------------------------------+
| ``0x2000``    | String (Text Display) | Welcome message text display             |
+---------------+-----------------------+------------------------------------------+
| ``0x3000``    | Touch Key Return      | Asynchronous button touch feedback       |
+---------------+-----------------------+------------------------------------------+
| Channel 0     | Trend Curve (0x84)    | Dynamic multichannel waveform rendering  |
+---------------+-----------------------+------------------------------------------+

Building and Running
********************

Building for Native Simulator
=============================

.. code-block:: console

   west build -b native_sim samples/dwin-interface -p
   ./build/zephyr/zephyr.exe

Building and Flashing for STM32F4 Discovery
===========================================

.. code-block:: console

   west build -b stm32f4_disco samples/dwin-interface -p
   west flash

Building and Flashing for Nucleo F429ZI
======================================

.. code-block:: console

   west build -b nucleo_f429zi samples/dwin-interface -p
   west flash

Building and Flashing for Nucleo H753ZI
======================================

.. code-block:: console

   west build -b nucleo_h753zi samples/dwin-interface -p
   west flash

Sample Output
*************

Upon booting, the sample initializes the screen and outputs periodic logs:

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0 ***
   [00:00:00.005] <inf> dwin_sample: ==================================================
   [00:00:00.005] <inf> dwin_sample:   DWIN DGUS II (T5L) Intelligent Display Sample   
   [00:00:00.005] <inf> dwin_sample: ==================================================
   [00:00:00.005] <inf> dwin_sample: Display device 'dgus2' is ready
   [00:00:00.005] <inf> dwin_sample: Touch callback registered on VP 0x3000
   [00:00:00.005] <inf> dwin_sample: Starting display initialization sequence...
   [00:00:00.005] <inf> dwin_sample: Active page set to 0
   [00:00:00.005] <inf> dwin_sample: Backlight brightness set to 85%
   [00:00:00.005] <inf> dwin_sample: Buzzer triggered for 100 ms
   [00:00:00.005] <inf> dwin_sample: Welcome text written to VP 0x2000
   [00:00:00.506] <inf> dwin_sample: Sync page query timeout/unanswered (normal if no physical display attached): -116
   [00:00:00.506] <inf> dwin_sample: Entering periodic telemetry update loop...
   [00:00:01.506] <inf> dwin_sample: Telemetry: Uptime=1s, Temp=24.1 C, Press=1011 hPa | Parse Errors=0
   [00:00:02.506] <inf> dwin_sample: Telemetry: Uptime=2s, Temp=24.2 C, Press=1012 hPa | Parse Errors=0
