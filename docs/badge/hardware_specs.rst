Hardware Specs
==============

* ESP32-S3 SoC

   * 240MHz
   * 8MiB SPI RAM, 512KiB SRAM
   * 16MiB flash
   * WiFi (2.4GHz, b/g/n)
   * Bluetooth LE (no bluetooth audio)

* Round display, 240x240px, compatible with GC9A01
* Ten petals with captouch pads
* Two shoulder buttons
* Two speakers
* Two 3.5mm ports

   * Headphone / line out and microphone / line in
   * :ref:`Badge Link`

      * Can be configured in each 3.5mm port (3.3v UART)
      * MIDI compatible (31250 baud UART)
* Microphone (IM73D122)
* USB-C

   * Two modes:

      * USB Serial/JTAG (implemented in hardware)
      * TinyUSB (for serial, mass storage, USB MIDI)

   * SBU1/SBU2 (sideband use) pins have UART RX/TX for early debug
   * USB host mode supported in theory (currently not implemented in software,
     there's no 5V power source)

* 40 RGB LEDs (WS2812C)
* MicroSD card slot (512MB class 4 MicroSD included)
* Accelerometer/IMU: Bosch BMI270
* 2000mAh battery (505060), compatible with MCH2022 badge

