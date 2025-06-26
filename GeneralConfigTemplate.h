#ifndef GENERAL_CONFIG_H
#define GENERAL_CONFIG_H

#include "main.h"


/* memory management definitions */

#define RUN_FROM_RAM 0
#define RAM_VECTORS_START 0x00000000

#ifdef STM32H725xx
#define MEM_USE_DTCM 1 /* used for processes, stacks and memory managers */
#define MEM_USE_RAM2 1 /* for ethernet */
#define RAM_SECTION_NAME ".AXI_RAM1" 
#define DTCM_SECTION_NAME ".DTCM_RAM1"
#define RAM2_SECTION_NAME ".RAM1"
#endif

#ifdef STM32F446xx
#define MEM_USE_DTCM 0 /* used for processes, stacks and memory managers */
#define MEM_USE_RAM2 0 /* for ethernet */
#define RAM_SECTION_NAME ".RAM1" 
#endif

/* configuration definitions */


#define  CONF_USE_OWN_FREERTOS 0

#define CONF_USE_SDCARD 0
#define CONF_USE_RNG 0
#define CONF_USE_BOOTUNIT 0


#define CONF_USE_UART_TERMINAL 0
#define CONF_USE_TIME 0
#define CONF_USE_RUNTIME 0
#define CONF_USE_ETHERNET 0
#define CONF_USE_COMMANDS 0
#define COMMAND_USE_TELNET 0
#define COMMAND_USE_UART 0
#define CONF_USE_WATCHDOG 1

#define DEBUG_PROCESS 0
#define DEBUG_SDCARD 0
#define FAT_DEBUG 0

/*#define LON_INTERFACE_FREQ 500 already defined in main.h */

#define LON_USE_VIRTUAL_CHANNELS 0
#define LON_USE_SENSORS 0
#define LON_USE_HTTP_INTERFACE 0
#define LON_USE_TIMERS 0
#define LON_USE_COMMAND_LINK 0
#define LON_USE_PWR_MGMT 0
#define LON_USE_SENSORS_DATABASE 0
#define LON_USE_IRRIGATION_UNIT 0
#define LON_USE_WEATHER_UNIT 0

#define IRRIG_NO_OF 4

#define NO_OF_BASIC_CHANNELS 8
#define NO_OF_VIRTUAL_CHANNELS 0

#define NO_OF_DEV_IN_CHANNEL 64

#define CHECK_DEV_INTERVAL 2
#define CHECK_ONCE_DEV_INTERVAL 30


#define CONFIG_FLASH_A  0x080A0000
#define CONFIG_FLASH_B  0x080C0000

#define CONFIG_FLASH_A_SIZE 0x00020000
#define CONFIG_FLASH_B_SIZE 0x00020000

#define CONFIG_FLASH_A_SECTOR FLASH_SECTOR_5
#define CONFIG_FLASH_B_SECTOR FLASH_SECTOR_6

#define CONFIG_FLASH_TIM 0x080E0000
#define CONFIG_FLASH_TIM_SIZE 0x00020000
#define CONFIG_FLASH_TIM_SECTOR FLASH_SECTOR_7
/*

for STM32F466
#define CONFIG_FLASH_A  0x08020000
#define CONFIG_FLASH_B  0x08040000

#define CONFIG_FLASH_A_SIZE 0x00010000
#define CONFIG_FLASH_B_SIZE 0x00010000

#define CONFIG_FLASH_A_SECTOR FLASH_SECTOR_5
#define CONFIG_FLASH_B_SECTOR FLASH_SECTOR_6

#define CONFIG_FLASH_TIM 0x08060000
#define CONFIG_FLASH_TIM_SIZE 0x00010000
#define CONFIG_FLASH_TIM_SECTOR FLASH_SECTOR_7
*/

/* 0-2 used by RTOS_FAT, 3-4 used by RUNTIMESTATS, 5 used by MyFAT */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS  6

#define configCHECK_FOR_STACK_OVERFLOW 1

/* RUNTIME config */
#if CONF_USE_RUNTIME == 1
#define configRUNTIME_THREAD_LOCAL_INDEX 3
#define configRUNTIME_MAX_NO_OF_TASKS 32
#ifdef __cplusplus
 extern "C" {
#endif
uint32_t GetRunTimeTimer(void);
void CreateTaskWrapper(void* task);
void DeleteTaskWrapper(void* task);
#ifdef __cplusplus
}
#endif

#define traceTASK_SWITCHED_IN() {pxCurrentTCB->pvThreadLocalStoragePointers[configRUNTIME_THREAD_LOCAL_INDEX + 0] = (void*)GetRunTimeTimer(); }
#define traceTASK_SWITCHED_OUT() {pxCurrentTCB->pvThreadLocalStoragePointers[configRUNTIME_THREAD_LOCAL_INDEX + 1] = (void*)((uint32_t)pxCurrentTCB->pvThreadLocalStoragePointers[configRUNTIME_THREAD_LOCAL_INDEX + 1] + GetRunTimeTimer() - (uint32_t)pxCurrentTCB->pvThreadLocalStoragePointers[configRUNTIME_THREAD_LOCAL_INDEX + 0]); }
#define traceTASK_CREATE(pxNewTCB) {CreateTaskWrapper((void*)pxNewTCB); pxNewTCB->pvThreadLocalStoragePointers[configRUNTIME_THREAD_LOCAL_INDEX + 0] = 0; pxNewTCB->pvThreadLocalStoragePointers[configRUNTIME_THREAD_LOCAL_INDEX + 1] = 0; }
#define traceTASK_DELETE(pxTCB) {DeleteTaskWrapper((void*)pxTCB); }
#endif

#if CONF_USE_BOOTUNIT == 0
#define VERSION_NAME "v2.0"
#endif

#endif