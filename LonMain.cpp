#include <stdio.h>
#include <stdlib.h>

#pragma fullpath_file  off 

/* Scheduler includes. */
#include "FreeRTOS.h"

#include "LonMain.hpp"

#include "CtrlProcess.hpp"

#if USE_HTTP == 1
#include "HTTP_ServerProcess.hpp"
#endif
#if USE_TELNET == 1
#include "TELNET_ServerProcess.hpp"
#endif
#if USE_FTP == 1
#include "FTP_ServerProcess.hpp"
#endif
#if CONF_USE_RUNTIME == 1
#include "RunTimeStats.hpp"
#endif
#if CONF_USE_ETHERNET == 1
#include "TcpProcess.hpp"
#endif
#if LON_USE_PWR_MGMT == 1
#include "PowerProcess.hpp"
#endif
#if CONF_USE_UART_TERMINAL == 1
#include "UartTerminal.hpp"
#endif

#include "LonInterfaceProcess.hpp"
#include "LonTrafficProcess.hpp"

#if LON_USE_SENSORS_DATABASE == 1
#include "LonSensorsDatabaseProcess.hpp"
#endif

#include "HeapManager.hpp"



#define configTOTAL_RAMD2_SIZE (30*1024)

uint8_t __attribute((section(RAM_SECTION_NAME))) ucHeap[ configTOTAL_HEAP_SIZE ] __attribute__ ((aligned (4)));
HeapRegion_t xHeapRegions[] =
  {
  { ucHeap ,			configTOTAL_HEAP_SIZE },
/*  { (uint8_t*) SDRAM_START ,	SDRAM_SIZE },  DRAM1 */
  { NULL, 0 }
  };

#if MEM_USE_DTCM == 1
uint8_t __attribute((section(DTCM_SECTION_NAME))) ucCCMHeap[ configTOTAL_CCM_HEAP_SIZE ] __attribute__ ((aligned (4)));
HeapRegion_t xCCMHeapRegions[] =
  {
  { ucCCMHeap ,			configTOTAL_CCM_HEAP_SIZE },
/*  { (uint8_t*) SDRAM_START ,	SDRAM_SIZE },  DRAM1 */
  { NULL, 0 }
  };
#endif

#if MEM_USE_RAM2 == 1
uint8_t __attribute((section(RAM2_SECTION_NAME))) ucD2Heap[ configTOTAL_RAMD2_SIZE ] __attribute__ ((aligned (4)));
HeapRegion_t xRAMD2HeapRegions[] =
  {
  { ucD2Heap ,			configTOTAL_RAMD2_SIZE },
/*  { (uint8_t*) SDRAM_START ,	SDRAM_SIZE },  DRAM1 */
  { NULL, 0 }
  };
#endif



#if MEM_USE_DTCM == 1

uint16_t baseManagerSizes[] = {32,64,128,340,540,1600,4096,16384};
HeapManager_c __attribute((section(DTCM_SECTION_NAME))) baseManager(8,baseManagerSizes,0);

#if MEM_USE_DTCM == 1
uint16_t ccmManagerSizes[] = {512,1024,2048,4096,8192,16384};
HeapManager_c __attribute((section(DTCM_SECTION_NAME))) ccmManager(6,ccmManagerSizes,1);
#endif

#if MEM_USE_RAM2 == 1
uint16_t ramD2ManagerSizes[] = {256,1600};
HeapManager_c __attribute((section(DTCM_SECTION_NAME))) ramD2Manager(2,ramD2ManagerSizes,2);
#endif

#else

uint16_t baseManagerSizes[] = {32,64,128,340,540,1600,4096,16384};
HeapManager_c __attribute((section(RAM_SECTION_NAME))) baseManager(8,baseManagerSizes,0);


#if MEM_USE_RAM2 == 1
uint16_t ramD2ManagerSizes[] = {256,1600};
HeapManager_c __attribute((section(RAM_SECTION_NAME))) ramD2Manager(2,ramD2ManagerSizes,2);
#endif

#endif


#ifdef __cplusplus
 extern "C" {
#endif

void *pvPortMalloc( size_t xWantedSize )
{
  return baseManager.Malloc(xWantedSize,1024);
}

void vPortFree( void *pv )
{
  baseManager.Free(pv);
}

#if MEM_USE_DTCM == 1
void * pvPortMallocStack( size_t xSize ) 
{
  return ccmManager.Malloc(xSize,1024);
}
void vPortFreeStack( void * pv )
{
  ccmManager.Free(pv);
}
#else
void * pvPortMallocStack( size_t xSize ) 
{
  return baseManager.Malloc(xSize,1024);
}
void vPortFreeStack( void * pv )
{
  baseManager.Free(pv);
}


#endif

#ifdef __cplusplus
}
#endif


void * operator new(size_t size) { 
  //return (pvPortMalloc(size));
  return baseManager.Malloc(size,0); 
}
void operator delete(void* ptr) {
  //vPortFree(wsk) ;

  uint8_t idx = HeapManager_c::GetManagerId(ptr);
  if(idx == 0)
  {
    baseManager.Free(ptr);
  }
  else if(idx == 1)
  {
    #if MEM_USE_DTCM == 1
    ccmManager.Free(ptr);
    #else
    baseManager.Free(ptr);
    #endif
  }
  else if(idx == 2)
  {
    #if MEM_USE_RAM2 == 1
    ramD2Manager.Free(ptr);
    #else
    baseManager.Free(ptr);
    #endif
  }

  //baseManager.Free(ptr);
}
 void* operator new[](size_t size) {
  //return (pvPortMalloc(size));
  return baseManager.Malloc(size,0);
}
void operator delete[](void* ptr) {
  //vPortFree(ptr);
  uint8_t idx = HeapManager_c::GetManagerId(ptr);
  if(idx == 0)
  {
    baseManager.Free(ptr);
  }
  else if(idx == 1)
  {
    #if MEM_USE_DTCM == 1
    ccmManager.Free(ptr);
    #else
    baseManager.Free(ptr);
    #endif
  }
  else if(idx == 2)
  {
    #if MEM_USE_RAM2 == 1
    ramD2Manager.Free(ptr);
    #else
    baseManager.Free(ptr);
    #endif
  }
  //baseManager.Free(ptr);
}

static void prvSetupHardware( void );
static void prvSetupHeap( void );


extern void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
void MPU_Config(void);
void MX_IWDG1_Init(void);






/*********************************************************************
*
*       applMain()
*
*  Function description
*   Application entry point.
*/
int applMain(void)
{
  #ifdef STM32H725xx
    #if RUN_FROM_RAM == 1
    SCB->VTOR = RAM_VECTORS_START;
    #endif


   if(FLASH->OPTSR2_CUR != OB_TCM_AXI_SHARED_ITCM64KB)
  {
    HAL_FLASH_OB_Unlock();
    FLASH->OPTSR2_PRG = OB_TCM_AXI_SHARED_ITCM64KB; /* 192kB ITCM / 192 kB AXI */
    HAL_FLASH_OB_Launch();
    HAL_FLASH_OB_Lock();
  }
  #endif
/*
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  #if CONF_USE_WATCHDOG == 1
  MX_IWDG1_Init();
  #endif

  HAL_Init();

  SystemClock_Config();
  PeriphCommonClock_Config();

  */
  prvSetupHardware();
  prvSetupHeap();
/*
    MX_GPIO_Init();
  MX_DMA_Init();
  MX_BDMA_Init();
  MX_ADC1_Init();
  MX_ADC3_Init();
  MX_FDCAN1_Init();
  MX_SPI2_Init();
  MX_SPI4_Init();
    MX_USART1_UART_Init();
  MX_RNG_Init();
  MX_RTC_Init();
  MX_USB_OTG_HS_PCD_Init();
  MX_SDMMC1_SD_Init();
  MX_IWDG1_Init();
  MX_ETH_Init();
  MX_DTS_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();*/


  #if CONF_USE_RUNTIME == 1
  new RunTime_c ;
  #endif

  new CtrlProcess_c(2048,tskIDLE_PRIORITY+4,64,SignalLayer_c::HANDLE_CTRL);
  #if CONF_USE_ETHERNET == 1
  new TcpProcess_c(2048,tskIDLE_PRIORITY+5,64,SignalLayer_c::HANDLE_TCP);
  #endif
  #if USE_HTTP == 1
  new HttpProcess_c(512,tskIDLE_PRIORITY+3,64,SignalLayer_c::HANDLE_HTTP);
  #endif
  #if USE_TELNET == 1
  new TelnetProcess_c(512,tskIDLE_PRIORITY+3,64,SignalLayer_c::HANDLE_TELNET);
  #endif
  #if USE_FTP == 1
  new FtpProcess_c(1024,tskIDLE_PRIORITY+3,64,SignalLayer_c::HANDLE_FTP);
  #endif

  new LonInterfaceProcess_c(512,tskIDLE_PRIORITY+5,64,SignalLayer_c::HANDLE_LON_INTERFACE);
  new LonTrafficProcess_c(2048,tskIDLE_PRIORITY+4,64,SignalLayer_c::HANDLE_LON_TRAFFIC);

  #if LON_USE_SENSORS_DATABASE == 1
  new LonSensorsDatabaseProcess_c(1024,tskIDLE_PRIORITY+3,64,SignalLayer_c::HANDLE_LON_SENSORS_DATABASE);
  #endif
  #if LON_USE_PWR_MGMT == 1
  new PwrProcess_c(1024,tskIDLE_PRIORITY+2,64,SignalLayer_c::HANDLE_POWER);
  #endif
  #if CONF_USE_UART_TERMINAL == 1
  new UartTerminalProcess_c(1024,tskIDLE_PRIORITY+2,64,SignalLayer_c::HANDLE_UART);
  #endif

  #if CONF_USE_OWN_FREERTOS == 1
  vTaskStartScheduler();
  #endif

  return 0;
}


static void prvSetupHeap( void )
{

  
  baseManager.DefineHeapRegions(xHeapRegions);
  #if MEM_USE_DTCM == 1
  ccmManager.DefineHeapRegions(xCCMHeapRegions);
  #endif
  #if MEM_USE_RAM2 == 1
  ramD2Manager.DefineHeapRegions(xRAMD2HeapRegions);
  ramD2Manager.allowNullResult = true;
  #endif

}

static void prvSetupHardware( void )
{

}


#ifdef __cplusplus
 extern "C" {
#endif




void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    char *pcTaskName )
{
  printf("stack overflow in %s\n",pcTaskName);
  while(1) {}

}

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t __attribute((section(".DTCM"))) xIdleTaskTCB;
static StackType_t __attribute((section(".DTCM"))) uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t __attribute((section(".DTCM"))) xTimerTaskTCB;
static StackType_t __attribute((section(".DTCM"))) uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void vApplicationTickHook(void)
{
 /*  static bool state = false;

   if(state)
   {
    
     state = false;
     HAL_GPIO_WritePin(Led1_GPIO_Port,Led1_Pin,GPIO_PIN_RESET);
   }
   else
   {
     HAL_GPIO_WritePin(Led1_GPIO_Port,Led1_Pin,GPIO_PIN_SET);
     state = true;
   }
*/


}



#ifdef __cplusplus
}
#endif

