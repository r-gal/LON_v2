# Overview

This program is firmware to Home Automation Main module.
Code for peripferal modules is stored in https://github.com/r-gal/LON_devices repository.


# Compilation

General information: Project is prepared on IDE that uses GCC compiller, files from System directory may need be adapted to other compillers.
Program is prepared to use on dedicated Hardware, Schematic and PCB layout in KiCad format is available in PCB directory.
Program uses 3rd party software: CMSIS, HAL libraries and FreeRTOS, in Core directory files generated in STM32CubeMX tool are stored.

1. Clone this repository with submodules

```
git clone https://github.com/r-gal/LON_v2 --recursive
```

2. Create project in your IDE and attach all files from repository

3. Attach FreeRTOS Kernel files
```
git submodule add https://github.com/FreeRTOS/FreeRTOS-Kernel
``` 
Use source files from main directory and from correct portable.
Choose correct port.c and portmacro.h files from portable directory - for STM32H725 and GCC compilator use portable\GCC\ARM_CM7\r0p1
Do not add any memory manager from portable/MemMang - Application offers own memory manager.

4. Add preprocesor definitions:
```
USE_PWR_LDO_SUPPLY
USE_HAL_DRIVER
STM32H725xx
STM32_THREAD_SAFE_STRATEGY=5
```
5. Add user include directories
```
stm32h7xx-hal-driver\Inc
cmsis-device-h7\Include
FreeRTOS-Kernel\include
FreeRTOS-Kernel\portable\GCC\ARM_CM7\r0p1
Core\Inc
Appl\Inc
Lib_Fat
Lib_Common
Lib_Ethernet
```

6. Configure Memory map to use DTCM memory - see files in System directory

## Method 1 - using STM32CubeMX 

1. Open myCnc2_v2.ioc in STM32CubeMX
2. Run Generate code
3. Add to project generated files: Core, CMSIS, HAL drivers

## Method 2 - use generated files
1. Attach CMSIS files
```
git submodule add https://github.com/STMicroelectronics/cmsis-device-h7
```
Add to project file startup_stm32h725xx.s
2. Attach Hal drivers
```
git submodule add https://github.com/STMicroelectronics/stm32h7xx-hal-driver
```

# Usage

1. Load generated software into module


