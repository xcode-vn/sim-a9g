/*
* author: lvdinh
* https://xcode.vn
* 2019/11/16
* v0.0.1
*/

#ifndef _HTTP_C_
#define _HTTP_C_

#include <string.h>
#include <stdio.h>
#include <api_os.h>
#include <api_event.h>
#include <api_debug.h>
#include "buffer.h"
#include "time.h"
#include "api_info.h"
#include "api_socket.h"
#include "api_network.h"
#include "main.h"
#include "libs/utils.c"

#define GET_URL_FIRST "GET %s "
#define GET_URL_SECOND "HTTP/1.1\r\nHost: %s\r\n\r\n"

int fd = -1;
bool keepConn = false;
uint8_t ip[16];
int httpBufferLen;
char *servInetAddr;
int connStatus;
char *url;

int Http_Get(const char *domain, int port, char *retBuffer, int *bufferLen)
{
    httpBufferLen = *bufferLen;
    if (!keepConn)
    {
        //connect server
        memset(ip, 0, sizeof(ip));
        if (DNS_GetHostByName2(domain, ip) != 0)
        {
            Trace(1, "get ip error");
            keepConn = false;
            if (fd >= 0)
            {
                close(fd);
            }
            return -1;
        }
        Trace(1, "get ip ok:%s -> %s", domain, ip);
        servInetAddr = ip;
    }
    char first[200];
    snprintf(first, sizeof(first), GET_URL_FIRST, retBuffer);

    char second[50];
    snprintf(second, sizeof(second), GET_URL_SECOND, domain);

    url = concatStrs(first, second);
	//Trace(1, "url here2: %s", url);

    OS_Free(first);
    OS_Free(second);

    if (!keepConn)
    {
        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd < 0)
        {
            Trace(1, "socket fail");
            keepConn = false;
            return -1;
        }
        Trace(1, "fd=%d", fd);
    }

    if (!keepConn)
    {
        struct sockaddr_in sockaddr;
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(port);
        inet_pton(AF_INET, servInetAddr, &sockaddr.sin_addr);

        connStatus = connect(fd, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in));
        if (connStatus < 0)
        {
            Trace(1, "socket connect fail");
            keepConn = false;
            close(fd);
            return -1;
        }
        Trace(1, "socket connect ok");
    }
    connStatus = send(fd, url, strlen(url), 0);
    if (connStatus < 0)
    {
        Trace(1, "Socket sending failed,fd:%d,:%s",fd, url);
        keepConn = false;
        close(fd);
        return -1;
    }
    Trace(1, "Socket sending OK! connId=%d, URL=%s",fd, url);
    keepConn = true;
    //close(fd);
    return 1;
}

void Http_Close_Conn()
{
    keepConn = false;
    if (fd >= 0)
    {
        close(fd);
    }
}

#endif