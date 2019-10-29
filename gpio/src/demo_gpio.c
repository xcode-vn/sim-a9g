#include "api_hal_gpio.h"
#include "stdint.h"
#include "stdbool.h"
#include "api_debug.h"
#include "api_os.h"
#include "api_hal_pm.h"
#include "api_os.h"
#include "api_event.h"
 
#define MAIN_TASK_STACK_SIZE    (1024 * 2)
#define MAIN_TASK_PRIORITY      0 
#define MAIN_TASK_NAME         "MAIN Test Task"
 
#define TEST_TASK_STACK_SIZE    (1024 * 2)
#define TEST_TASK_PRIORITY      1
#define TEST_TASK_NAME         "GPIO Test Task"
 
static HANDLE mainTaskHandle = NULL;
static HANDLE secondTaskHandle = NULL;
 
void GPIO_TestTask()
{
    static GPIO_LEVEL ledBlueLevel = GPIO_LEVEL_HIGH;
 
    GPIO_config_t gpioLedBlue = {
        .mode         = GPIO_MODE_OUTPUT,
        .defaultLevel = GPIO_LEVEL_LOW
    };
 
    for(uint8_t i=0;i<POWER_TYPE_MAX;++i) PM_PowerEnable(i,true);
 
    gpioLedBlue.pin = GPIO_PIN27;
    GPIO_Init(gpioLedBlue);
 
    gpioLedBlue.pin = GPIO_PIN28;
    GPIO_Init(gpioLedBlue);
 
    while(1)
    {
        GPIO_Set(GPIO_PIN28, !ledBlueLevel);
        GPIO_Set(GPIO_PIN27, ledBlueLevel);
        Trace(1,"Hello");
        OS_Sleep(1000);
 
        GPIO_Set(GPIO_PIN28, ledBlueLevel);
        GPIO_Set(GPIO_PIN27, !ledBlueLevel);
        Trace(1,"World!");
        OS_Sleep(1000);
    }
}
 
void EventDispatch(API_Event_t* pEvent)
{
    switch(pEvent->id)
    {
        default:
            break;
    }
}
 
void MainTask(void *pData)
{
    API_Event_t* event=NULL;
 
    secondTaskHandle = OS_CreateTask(GPIO_TestTask,
        NULL, NULL, TEST_TASK_STACK_SIZE, TEST_TASK_PRIORITY, 0, 0, TEST_TASK_NAME);
 
    while(1)
    {
        if(OS_WaitEvent(mainTaskHandle, (void**)&event, OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatch(event);
            OS_Free(event->pParam1);
            OS_Free(event->pParam2);
            OS_Free(event);
        }
    }
}
 
void gpio_Main()
{
    mainTaskHandle = OS_CreateTask(MainTask ,
        NULL, NULL, MAIN_TASK_STACK_SIZE, MAIN_TASK_PRIORITY, 0, 0, MAIN_TASK_NAME);
    OS_SetUserMainHandle(&mainTaskHandle);
}