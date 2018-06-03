#include "sys_config.h"
#include "object.h"
#include "esp8266.h"
#include "io.h"
#include <string.h>

const char* const str_esp_result[ESP_JOIN_END] =
                                         {"OK",
                                          "join timeout",
                                          "invalid password",
                                          "AP not found",
                                          "join fail",
                                          "ESP not found",
                                          "ESP timeout",
                                          "conn refused",
                                          "proto error",
                                          "session closed by host",
                                          "ESP unknown",
                                         };

extern void esp8266s_main();

const char* esp8266_get_result(ESP_JOIN_STATE res)
{
    if(res < ESP_JOIN_END)
        return str_esp_result[res];
    else
        return str_esp_result[ESP_JOIN_END - 1];
}

HANDLE esp8266_create(uint32_t process_size, uint32_t priority)
{
    char name[] = "ESP8266 Server";
    REX rex;
    rex.name = name;
    rex.size = process_size;
    rex.priority = priority;
    rex.flags = PROCESS_FLAGS_ACTIVE;
    rex.fn = esp8266s_main;
    return process_create(&rex);
}

void esp8266_open(HANDLE esp, int uart_port)
{
    ack(esp, HAL_REQ(HAL_WIFI, IPC_OPEN), uart_port, 0, 0);
}

void esp8266_close(HANDLE esp)
{
    ack(esp, HAL_REQ(HAL_WIFI, IPC_CLOSE), 0, 0, 0);
}

void esp8266_join(HANDLE esp, ESP_STA* sta)
{
    IO* io;
    io = io_create(sizeof(ESP_STA));
    memcpy(io_data(io), sta, sizeof(ESP_STA));
    ack(esp, HAL_REQ(HAL_WIFI, IPC_JOIN), 0, (uint32_t)io, 0);
    io_destroy(io);
}

HANDLE esp8266_create_tcb(HANDLE esp, char* remote_host, uint16_t remote_port)
{
    IO* io;
    HANDLE res;
    io = io_create(strlen(remote_host)+1);
    strcpy((char*)io_data(io), remote_host);
    ack(esp, HAL_REQ(HAL_WIFI, IPC_CREATE_TCB), 0, (uint32_t)io, remote_port);
    res = *(HANDLE*)io_data(io);
    io_destroy(io);
    return res;

}

