# POD-Firmware

This project is based on the crazyflie-firmware. It uses STM32CUBEMX to generate the orignal structure and uses HAL library instead of STM32 Standard Peripheral Libraries (SPL), which is deprecated.

## Modifications to Auto-generated Files

### STM32F405RGTx_FLASH.ld

Change the flash origin address to 0x8004000:

```FLASH (rx)      : ORIGIN = 0x8004000, LENGTH = 1024K```