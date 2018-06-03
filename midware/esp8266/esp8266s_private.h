#ifndef ESP8266S_PRIVATE_H
#define ESP8266S_PRIVATE_H

#include "sys_config.h"
#include <stdint.h>
#include "object.h"
#include "esp8266.h"

#define ESP_MAX_STRING  80
#define ESP_MAX_REQ     8

typedef enum{
    ESP_REQ_STRING,
    ESP_REQ_SET_IP,
    ESP_REQ_JOIN,
    ESP_REQ_CONNECT,
    ESP_REQ_LISTEN,
    ESP_REQ_SEND_DATA,
    ESP_REQ_CLOSE_CONN,
    ESP_REQ_OPEN_CONN,
}ESP_REQ_TYPE;

typedef struct {
    IO* rx_io1;
    IO* rx_io2;
    IO* tx_io;
    int port;
}ESP_UART;

typedef enum{
    ESP_SESSION_FREE = 0,
    ESP_SESSION_LISTEN,
    ESP_SESSION_ASSIGN,
    ESP_SESSION_OPEN,
}ESP_SESSION_STATE;

typedef struct {
    ESP_SESSION_STATE state;
    HANDLE process, tx_process;
    IO* rx;
    uint32_t rx_size;
    IO* tx;
//    IP remote_ip;
    uint16_t remote_port;
    uint16_t local_port;
    char* rx_buffer;
    int rx_buffer_len;
    char* remote_host;
}ESP_SESSION;

typedef struct {
    ESP_REQ_TYPE type;
    uint32_t  param;
    uint32_t timeout;
}ESP_REQ_ENTRY;

typedef struct _ESP8266{
    HANDLE timer;
    HANDLE process;
    ESP_STATE state;
    ESP_JOIN_STATE join_state;
    ESP_UART uart;
    ESP_REQ_ENTRY req[ESP_MAX_REQ];
    uint32_t req_head;
    uint32_t req_tail;
    ESP_SESSION sessions[ESP_CONNECT_COUNT];
    uint32_t rx_conn;   // current rx session  +IPD
    bool is_receive;    //
    uint32_t tx_conn;   // current tx session
    uint32_t rx_size;   // rest to receive
    char str[ESP_MAX_STRING+1];
    int strlen;
    ESP_STA sta;
} ESP8266;


#endif //ESP8266S_PRIVATE_H
