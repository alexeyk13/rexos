#ifndef ESP8266_H
#define ESP8266_H

#include <stdint.h>
#include "object.h"
#include "ipc.h"
#include "ip.h"
#include "io.h"

#define ESP_CONNECT_COUNT           5

typedef enum {
    ESP_STATUS = IPC_USER,
    IPC_JOIN,
    IPC_REBOOT,
    IPC_ERROR,
    IPC_CREATE_TCB,
}ESP_IPCS;

typedef enum {
    ESP_JOIN_OK               =0,
    ESP_JOIN_TIMEOUT          = 1,
    ESP_JOIN_INVALID_PASSWORD = 2,
    ESP_JOIN_NOT_FOUND        = 3,
    ESP_JOIN_FAIL             = 4,
    ESP_NOT_FOUND             = 5,
    ESP_TIMEOUT               = 6,
    ESP_REFUSED               = 7,
    ESP_PROTOCOL_ERROR        = 8,
    ESP_SESSION_CLOSED        = 9,
    ESP_JOIN_DISCONNECT       = 10,
    ESP_JOIN_END
} ESP_JOIN_STATE;


typedef enum {
    ESP_CLOSE,      // uart close
    ESP_UNKNOWN_WAIT,
    ESP_UNKNOWN,    // ready nod received, state unknown
    ESP_IDLE,
    ESP_TRY_SEND,
    ESP_BUSY,
    ESP_ERROR,
} ESP_STATE;

typedef struct {
    IP ip;
    bool dhcp;
    char ssid[32];
    char password[64];
}ESP_STA;

//--------------------------------
const char* esp8266_get_result(ESP_JOIN_STATE res);

HANDLE esp8266_create(uint32_t process_size, uint32_t priority);

HANDLE esp8266_create_tcb(HANDLE tcpip, char* remote_host, uint16_t remote_port);

void esp8266_open(HANDLE esp, int uart_port);
void esp8266_join(HANDLE esp, ESP_STA* sta);
void esp8266_close(HANDLE esp);


#endif // ESP8266_H
