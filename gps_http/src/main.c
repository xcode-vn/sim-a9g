/*
* author: lvdinh
* https://xcode.vn
* 2019/11/16
* v0.0.1
*/

#include "main.h"
#include "http.c"
#include "libs/utils.c"

HANDLE eventTaskHandle = NULL;

bool isSystemReady = false;
bool isSystemInitialized = false;

bool isGprsReady = false;

double lat, lng;

Network_PDP_Context_t context = {
	//mobifone
	.apn = "m-wap",
	.userName = "mms",
	.userPasswd = "mms"};

void Report_GPS()
{
	char retBuffer[300];
	int retBufferLen = sizeof(retBuffer);

	snprintf(retBuffer, retBufferLen, SERVER_API_URL_POSITION_ADD, lat, lng);

	//Trace(1, "url here1: %s", retBuffer);

	if (Http_Get(SERVER_IP, SERVER_PORT, retBuffer, &retBufferLen) < 0)
	{
		Trace(1, "Http_Get failed");
	}
	else
	{
		//Trace(1, "Http_Get ok");
	}

	OS_Free(retBuffer);
}
bool Start_GPS(GPS_Info_t *gpsInfo)
{
	uint8_t buffer[300];
	bool result = false;

	//open GPS hardware(UART2 open either)
	GPS_Init();
	GPS_Open(NULL);

	//wait for gps start up, or gps will not response command
	while (gpsInfo->rmc.latitude.value == 0)
	{
		Trace(1, "Starting GPS");
		OS_Sleep(1000);
	}
	// init GPS: try 5 times setting gps nmea output interval to 10s, break at first success
	for (uint8_t i = 0; i < 5; ++i)
	{
		if (GPS_SetOutputInterval(10000))
		{
			result = true;
			break;
		}
		else
		{
			Trace(1, "Setting nmea output interval failed");
			result = false;
		}
		OS_Sleep(1000);
	}
	if (!result)
		return result;

	if (!GPS_ClearInfoInFlash())
	{
		Trace(1, "erase gps fail");
		return false;
	}

	if (!GPS_SetQzssOutput(false))
	{
		Trace(1, "enable qzss nmea output fail");
		return false;
	}

	if (!GPS_SetSearchMode(true, false, true, false))
	{
		Trace(1, "set search mode fail");
		return false;
	}

	if (!GPS_SetSBASEnable(true))
	{
		Trace(1, "enable sbas fail");
		return false;
	}

	if (!GPS_SetFixMode(GPS_FIX_MODE_LOW_SPEED))
	{
		Trace(1, "set fix mode fail");
		return false;
	}

	if (!GPS_GetVersion(buffer, 150))
	{
		Trace(1, "Getting GPS firmware version failed");
		return false;
	}
	else
		Trace(1, "GPS firmware version:%s", buffer);

	// set gps nmea output interval to 1s
	if (!GPS_SetOutputInterval(1000))
	{
		Trace(1, "Setting nmea output interval failed");
		return false;
	}

	return true;
}
void Gps_Sending_Task(void *pData)
{
	GPS_Info_t *gpsInfo = Gps_GetInfo();

	//wait for gprs register complete
	//The process of GPRS registration network may cause the power supply voltage of GPS to drop,
	//which resulting in GPS restart.
	while (!isGprsReady)
	{
		Trace(1, "Registering GPRS");
		OS_Sleep(2000);
	}

	bool isGpsReady = Start_GPS(gpsInfo);

	Trace(1, "Start_GPS done: %s", isGpsReady ? "OK" : "FAILED");

	while (1)
	{
		if (isGpsReady && isGprsReady)
		{
			//convert unit ddmm.mmmm to degree(°)
			int temp = (int)(gpsInfo->rmc.latitude.value / gpsInfo->rmc.latitude.scale / 100);
			lat = temp + (double)(gpsInfo->rmc.latitude.value - temp * gpsInfo->rmc.latitude.scale * 100) / gpsInfo->rmc.latitude.scale / 60.0;
			temp = (int)(gpsInfo->rmc.longitude.value / gpsInfo->rmc.longitude.scale / 100);
			lng = temp + (double)(gpsInfo->rmc.longitude.value - temp * gpsInfo->rmc.longitude.scale * 100) / gpsInfo->rmc.longitude.scale / 60.0;

			Report_GPS();
		}
		OS_Sleep(REPORT_INTERVAL);
	}
}

void Event_Dispatch(API_Event_t *pEvent)
{
	switch (pEvent->id)
	{
	case API_EVENT_ID_SYSTEM_READY:
		Trace(1, "System initialized");
		isSystemReady = true;
		break;
	case API_EVENT_ID_NETWORK_REGISTERED_HOME:
	case API_EVENT_ID_NETWORK_REGISTERED_ROAMING:
	{
		uint8_t status;
		Trace(2, "Network registered");
		if (!Network_GetAttachStatus(&status))
		{
			Trace(1, "Getting Network AttachStasus failed");
		}
		Trace(1, "Network AttachStasus:%d", status);
		if (status == 0)
		{
			if (!Network_StartAttach())
			{
				Trace(1, "Getting Network AttachStasus failed");
			}
		}
		else
		{
			Network_StartActive(context);
			isGprsReady = true;
		}
		break;
	}
	case API_EVENT_ID_NETWORK_ATTACHED:
		Trace(2, "Network Attached");
		Network_StartActive(context);
		isGprsReady = true;
		break;

	case API_EVENT_ID_GPS_UART_RECEIVED:
		GPS_Update(pEvent->pParam1, pEvent->param1);
		break;
	default:
		break;
	}
}
void Event_Task(void *pData)
{
	API_Event_t *event = NULL;

	OS_CreateTask(Gps_Sending_Task, NULL, NULL, MAIN_TASK_STACK_SIZE, MAIN_TASK_PRIORITY, 0, 0, MAIN_TASK_NAME);

	//Wait event
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

void gps_http_Main(void)
{
	eventTaskHandle = OS_CreateTask(Event_Task, NULL, NULL, MAIN_TASK_STACK_SIZE, MAIN_TASK_PRIORITY, 0, 0, MAIN_TASK_NAME);
	OS_SetUserMainHandle(&eventTaskHandle);
}
