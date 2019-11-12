#include "api_hal_gpio.h"
#include "stdint.h"
#include "stdbool.h"
#include "api_debug.h"
#include "api_os.h"
#include "api_hal_pm.h"
#include "api_os.h"
#include "api_event.h"
#include "sms_lib.c"

#define MAIN_TASK_STACK_SIZE (1024 * 2)
#define MAIN_TASK_PRIORITY 0
#define MAIN_TASK_NAME "xcode.vn|series_sim_a9g|sms_gpio"

#define LED_COUNT 3
#define MASTER_NUMBER_COUNT 2
#define LED_STATE_ON GPIO_LEVEL_LOW

const char *MASTER_NUMBERS[MASTER_NUMBER_COUNT] = {"+84123456789", "+84123456"};
const GPIO_PIN FLASH_LED = GPIO_PIN25;
const GPIO_PIN LEDS[LED_COUNT] = {GPIO_PIN25, GPIO_PIN30, GPIO_PIN26};

static HANDLE eventTaskHandle = NULL;
static HANDLE smsGpioTaskHandle = NULL;

bool Is_Master_Phone_Number(char *number)
{
    for (int i = 0; i < MASTER_NUMBER_COUNT; i++)
    {
        if (!strcmp(MASTER_NUMBERS[i], number))
            return true;
    }

    return false;
}

void Init_Pins()
{
    //wait for System to be ready
    GPIO_config_t gpioConfig = {
        .mode = GPIO_MODE_OUTPUT,
        .defaultLevel = GPIO_LEVEL_LOW};

    for (int i = 0; i < LED_COUNT; i++)
    {
        PM_PowerEnable(LEDS[i], true);
        gpioConfig.pin = LEDS[i];
        GPIO_Init(gpioConfig);
    }
}

//gửi report
void Send_SMS_Report(char *phoneNumber)
{
    GPIO_LEVEL level;
    char *status1, *status2, msg[100];

    //lấy trạng thái của led 1
    GPIO_Get(LEDS[1], &level);
    status1 = level == LED_STATE_ON ? "on" : "off";

    //lấy trạng thái của led 2
    GPIO_Get(LEDS[2], &level);
    status2 = level == LED_STATE_ON ? "on" : "off";

    //build thành nội dung tin nhắn
    snprintf(msg, sizeof(msg), "Leds status:\nLed1 %s\nLed2 %s", status1, status2);
    /*
        Leds status
        Led1 on
        Led off
    */

    //gửi tin nhắn
    SendUtf8Sms(phoneNumber, msg);
    Trace(1, "Report SMS: %s", msg);
}

//xử lý một lệnh dạng on/off
void Handle_Command(char *cmd)
{
    Trace(1, "Execute: %s", cmd);

    char *key, *value;

    //kiểm tra xem câu lệnh có hợp lệ không
    //câu lệnh hợp lệ là câu lệnh có dạng "blahblah <space> on" hoặc "blahblah <space> off"
    //A <space> B
    if(!(key = strsep(&cmd, " "))){
        Trace(1, "Invalid command");
        return;
    }
    if (key[0] == '\0')
    {
        Trace(1, "Invalid command");
        return;
    }
    
    if(!(value = strsep(&cmd, " "))){
        Trace(1, "Invalid command");
        return;
    }
    if (value[0] == '\0')
    {
        Trace(1, "Invalid command");
        return;
    }
    
    //A <space> on ->o
    //A <space> off ->o
    //A <space> xyz ->x
    if (strcmp(value, "on") && strcmp(value, "off"))
        return;

    //chuyẻn đổi on/off sang logic level
    GPIO_LEVEL level = strcmp(value, "on") ? !LED_STATE_ON : LED_STATE_ON;

    if (!strcmp(key, "led1"))
    {
        //set level cho led 1
        GPIO_Set(LEDS[1], level);
    }
    else if (!strcmp(key, "led2"))
    {
        //set level cho led 2
        GPIO_Set(LEDS[2], level);
    }
    Trace(1, "Executed");
}

//xử lý một tin nhắn SMS
void Handle_SMS(char *phoneNumber, char *smsContent)
{
    Trace(1, "Command: %s", smsContent);

    //chuyển tin nhắn về dạng chữ thường
    lowerStr(smsContent);
    bool shouldReport = false;
    char *line;

    //cắt từng dòng của tin nhắn, mỗi dòng là một lệnh
    //ví dụ một tin nhắn:
    /*
        led1 on
        led2 off
        report
    */
    while ((line = strsep(&smsContent, "\n")))
    {
        //nếu có lệnh "report", sau khi xử lý hết tất cả các lệnh sẽ phải gửi report
        if (!strcmp(line, "report"))
        {
            shouldReport = true;
        }
        else
        {
            //xử lý các lệnh dạng on/off
            Handle_Command(line);
        }
    }

    if (shouldReport)
    {
        //gửi report
        Send_SMS_Report(phoneNumber);
    }

    Trace(1, "Command handled");
}

//xử lý một event
void Event_Dispatch(API_Event_t *pEvent)
{
    switch (pEvent->id)
    {
    case API_EVENT_ID_SYSTEM_READY:
        //khởi tạo GPIO
        Init_Pins();

        //xóa bộ lưu tin nhắn
        ClearSmsStorage();

        Trace(1, "System initialized");
        break;
    case API_EVENT_ID_NETWORK_REGISTERED_HOME:
    case API_EVENT_ID_NETWORK_REGISTERED_ROAMING:
        //khởi tạo một tin nhắn SMS để tí nữa gửi sau
        InitSMS();

        Trace(1, "Network registered");
        break;
    case API_EVENT_ID_SMS_SENT:
        //ghi log báo gửi thành công
        Trace(1, "SMS sent");
        break;
    case API_EVENT_ID_SMS_RECEIVED:
        Trace(1, "Received message");
        char *header = pEvent->pParam1;
        char *content = pEvent->pParam2;

        Trace(1, "SMS length:%d,content:%s", strlen(content), content);

        char *phoneNumber[20];

        //trích số điện thoại từ header
        Get_PhoneNumer(header, phoneNumber);

        //kiểm tra xem có phải số của master không
        if (Is_Master_Phone_Number(phoneNumber))
        {
            Trace(1, "SMS from master");

            //đây là số của master, xử lý thôi
            Handle_SMS(phoneNumber, content);
        }
        else
        {
            //số lạ hoặc số QC của nhà mạng thì next
            Trace(1, "SMS from stranger or [QC], header: %s", header);
        }
        OS_Free(phoneNumber);
        break;
    default:
        //các event khác thì next
        break;
    }
}

void Blink_Task()
{
    while (1)
    {
        GPIO_Set(FLASH_LED, !LED_STATE_ON);
        OS_Sleep(1000);
        GPIO_Set(FLASH_LED, LED_STATE_ON);
        OS_Sleep(1000);
    }
}

void Event_Task(void *pData)
{
    API_Event_t *event = NULL;

    //khởi chạy thêm một luồng mới tên là Blink_Task để nhấp nháy led báo trạng thái module đang hoạt động
    smsGpioTaskHandle = OS_CreateTask(Blink_Task,
                                      NULL, NULL, MAIN_TASK_STACK_SIZE, MAIN_TASK_PRIORITY, 0, 0, MAIN_TASK_NAME);

    //vòng lặp vô tận, lắng nghe và xử lý các event.
    while (1)
    {
        if (OS_WaitEvent(eventTaskHandle, (void **)&event, OS_TIME_OUT_WAIT_FOREVER))
        {
            Event_Dispatch(event);
            OS_Free(event->pParam1);
            OS_Free(event->pParam2);
            OS_Free(event);
        }
    }
}

void sms_gpio_Main()
{
    eventTaskHandle = OS_CreateTask(Event_Task,
                                    NULL, NULL, MAIN_TASK_STACK_SIZE, MAIN_TASK_PRIORITY, 0, 0, MAIN_TASK_NAME);
    OS_SetUserMainHandle(&eventTaskHandle);
}