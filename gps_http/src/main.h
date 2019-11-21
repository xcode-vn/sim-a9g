/*
* author: lvdinh
* https://xcode.vn
* 2019/11/16
* v0.0.1
*/

#ifndef _MAIN_H_
#define _MAIN_H_

#include <string.h>
#include <stdio.h>
#include <api_os.h>
#include <api_gps.h>
#include <api_event.h>
#include <api_hal_uart.h>
#include <api_debug.h>
#include <api_socket.h>
#include <api_network.h>
#include "buffer.h"
#include "gps_parse.h"
#include "gps.h"
#include "math.h"

#define MAIN_TASK_STACK_SIZE (2048 * 2)
#define MAIN_TASK_PRIORITY 0
#define MAIN_TASK_NAME "xcode.vn|gps_http"

#define SERVER_IP "demo.traccar.org"
#define SERVER_PORT 5055
#define SERVER_API_URL_POSITION_ADD "/?id={YOUR_DEVICE_ID_HERE}&lat=%f&lon=%f"

#define REPORT_INTERVAL 2000

#endif