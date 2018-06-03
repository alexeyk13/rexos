#include "sys_config.h"
#include "config.h"

#include "process.h"
#include "stdio.h"
#include "sys.h"
#include "io.h"
#include "tcp.h"
#include "uart.h"
#include "esp8266s_private.h"
#include "../params.h"

#include <string.h>

#define ESP_BAUD                115200
#define ESP_IO_READ_SIZE        200
#define ESP_IO_WRITE_SIZE       100

#define ESP_TO                  1000
#define ESP_TO_INIT             3000
#define ESP_TO_JOIN             20000
#define ESP_TO_SEND_DATA        6000
#define ESP_TO_CONNECT          6000
#define ESP_TO_LIST             5000

#define ESP_FLAGS_Pos           24
#define ESP_FLAGS_Pos           24

//---------- request
static const char str_reset[]      = "AT+RST";
static const char str_at[]         = "AT";
static const char str_echo_off[]   = "ATE0";
static const char str_set_mux[]    = "AT+CIPMUX=1";
static const char str_dhcp_off[]   = "AT+CWDHCP_CUR=1,0";
static const char str_dhcp_on[]   = "AT+CWDHCP_CUR=1,1";
static const char str_set_ip[]     = "AT+CIPSTA_CUR=";
static const char str_join[]       = "AT+CWJAP_CUR=\"";
static const char str_listen[]     = "AT+CIPSERVER=1,";
static const char str_listen_stop[]= "AT+CIPSERVER=0";
static const char str_send_data[]  = "AT+CIPSEND=";
static const char str_close_conn[] = "AT+CIPCLOSE=";
static const char str_open_conn[]  = "AT+CIPSTART=";
static const char str_get_ip[]  = "AT+CIPSTA_CUR?";
static const char str_auto_off[]  = "AT+CWAUTOCONN=0";


//--------- replay
static const char str_DATA[]     = "+IPD,";
//
static const char str_OK[]       = "OK\0";    // if compare all string - add \0
static const char str_SEND_OK[]  = "SEND OK\0";
static const char str_SEND_FAIL[]= "SEND FAIL\0"; //
static const char str_JOIN_FAIL[]= "FAIL\0";      //  fail connected to AP
static const char str_READY[]    = "ready\0";     //
static const char str_ERROR[]    = "ERROR\0";
//
static const char str_JOINED[]   = "WIFI CONNECTED\0";
static const char str_DISJOIN[]  = "WIFI DISCONNECT\0";
static const char str_GOT_IP[]   = "WIFI GOT IP";
static const char str_IP[]       = "+CIPSTA_CUR:ip:\"";
static const char str_AP_DATA[]  = "+CWJAP:";
static const char str_CONNECT[]  = ",CONNECT\0";
static const char str_CLOSED[]   = ",CLOSED\0";

#define LIST       \
    X(OK)          \
    X(ERROR)       \
    X(SEND_OK)     \
    X(SEND_FAIL)   \
    X(READY)       \
    X(JOIN_FAIL)   \
                    \
    X(JOINED)      \
    X(IP)          \
    X(GOT_IP)      \
    X(CONNECT)     \
    X(CLOSED)     \
    X(DISJOIN)   \
    X(AP_DATA)   \


typedef enum {
#define X(a) ESP_REPLAY_##a,       // ESP_REPLAY_DATA.....
    LIST
    ESP_REPLAY_NONE
#undef X
}ESP_REPLAY_NUM;


typedef struct {
    const char* strptr;
    int count;
}ESP_REPLAY;

static const  ESP_REPLAY esp_replay[] ={
#define X(a) {str_##a, sizeof(str_##a)-1 },
        LIST
#undef X
};

const char* const esp_replay_str[ESP_REPLAY_NONE+1] ={
#define X(a) #a ,
        LIST
        "UNKNOWN"
#undef X
};

//-----
static inline void esp_set_result(ESP8266* esp, ESP_JOIN_STATE res)
{
#if (ESP_DEBUG)
    printf("ESP: %s\n", esp8266_get_result(res));
#endif // ESP_DEBUG
    esp->join_state = res;
}

static inline void esp_join(ESP8266* esp);
static void esp_tcp_close(ESP8266* esp, int num);


static inline void esp_rx_io_complete(ESP8266* esp, uint32_t conn)
{
    TCP_STACK* tcp_stack;
    ESP_SESSION* session = &esp->sessions[conn];
    if(session->rx)
    {
#if (ESP_DEBUG_FLOW)
    printf("ESP: io_complete %d len:%u\n", conn, session->rx->data_size);
#endif // ESP_DEBUG_FLOW
        if (session->rx->data_size)
        {
            tcp_stack = io_stack(session->rx);
            tcp_stack->flags |= TCP_PSH;
            io_complete(session->process, HAL_IO_CMD(HAL_TCP, IPC_READ), conn, session->rx);
        }
        else
        {
            io_complete_ex(session->process, HAL_IO_CMD(HAL_TCP, IPC_READ), conn, session->rx, ERROR_CONNECTION_CLOSED);
        }
        session->rx = NULL;
    }
    if(session->rx_buffer != NULL)
    {
        session->rx_buffer_len = 0;
        free(session->rx_buffer);
        session->rx_buffer = NULL;
    }

}

static inline void esp_tx_io_complete(ESP8266* esp, uint32_t conn)
{
    if(esp->sessions[conn].tx)
    {
        io_complete(esp->sessions[conn].tx_process, HAL_IO_CMD(HAL_TCP, IPC_WRITE), conn, esp->sessions[conn].tx);
        esp->sessions[conn].tx = NULL;
    }
}

static void esp_pkt_flush_internal(ESP8266* esp)
{
    int i;
    esp->req_head = esp->req_tail = 0;
    if(esp->is_receive)
        esp_rx_io_complete(esp, esp->rx_conn);
    esp->is_receive = false;
    for(i = 0; i < ESP_CONNECT_COUNT; i++)
    {
        esp_tcp_close(esp, i);
    }
    switch(esp->state)
    {

    case ESP_UNKNOWN_WAIT:
        esp->state = ESP_UNKNOWN;
        return;
    case ESP_TRY_SEND:
    case ESP_BUSY:
        break;
    default:
        return;
    }
    esp->state = ESP_IDLE;
}

static int esp_add_str(char* dst,char* src)
{
  int len = 0;
  char c;
  for(;;)
  {
      c = *src++;
      if(c == 0)
          break;
      len++;
      if( (c == ',') || (c == '"') || (c == '\\') )
      {
          *dst++ = '\\';
          len++;
      }
      *dst++ = c;
  }
  return len;
}

static bool esp_pkt_send_next(ESP8266* esp) // false - no packed
{
    char* dst = io_data(esp->uart.tx_io);
    ESP_SESSION* session;
    int len = 0;
    if(esp->req_head == esp->req_tail)
        return false;
    switch (esp->state)
    {
    case ESP_IDLE:
        esp->state = ESP_BUSY;
        break;
    case ESP_UNKNOWN:
        esp->state = ESP_UNKNOWN_WAIT;
        break;
    default:
        return true;
    }
    switch(esp->req[esp->req_tail].type)
    {
    case ESP_REQ_STRING:
        strcpy(dst,(char*)esp->req[esp->req_tail].param);
        break;
    case ESP_REQ_JOIN:
        len = strlen(str_join);
        memcpy(dst,  str_join, len);
        dst += len;
        dst += esp_add_str(dst, esp->sta.ssid);
        *dst++ = '"';
        *dst++ = ',';
        *dst++ = '"';
        dst += esp_add_str(dst, esp->sta.password);
        *dst++ = '"';
        *dst++ = 0;
        break;
    case ESP_REQ_SET_IP:
        len = strlen(str_set_ip);
        memcpy(dst, str_set_ip, strlen(str_set_ip));
        sprintf(&dst[len], "\"%u.%u.%u.%u\"\0", esp->sta.ip.u8[0], esp->sta.ip.u8[1], esp->sta.ip.u8[2], esp->sta.ip.u8[3]);
        break;
    case ESP_REQ_LISTEN:
        sprintf(dst, "%s%u\0", str_listen, esp->sessions[0].local_port);
        break;
    case ESP_REQ_CLOSE_CONN:
        sprintf(dst, "%s%u\0", str_close_conn, esp->req[esp->req_tail].param);
        break;
    case ESP_REQ_OPEN_CONN:
        len = strlen(str_open_conn);
        memcpy(dst, str_open_conn, strlen(str_open_conn));
        sprintf(&dst[len], "%u,\"TCP\",\"", esp->req[esp->req_tail].param);
        len +=9;
        session = &esp->sessions[esp->req[esp->req_tail].param];
        sprintf(&dst[len], "%s\",%u\0", session->remote_host,session->remote_port);

/*        sprintf(&dst[len], "%u.%u.%u.%u\",%u\0", session->remote_ip.u8[0],
                                                 session->remote_ip.u8[1],
                                                 session->remote_ip.u8[2],
                                                 session->remote_ip.u8[3],
                                                 session->remote_port);*/
        break;
    case ESP_REQ_SEND_DATA:
        esp->tx_conn = esp->req[esp->req_tail].param;
        sprintf(io_data(esp->uart.tx_io), "%s%u,%u", str_send_data, esp->tx_conn, esp->sessions[esp->tx_conn].tx->data_size );
        esp->state = ESP_TRY_SEND;
        break;
    default:
        break;
    }

    dst = io_data(esp->uart.tx_io);
    len = strlen(dst);
    dst[len] = '\r';
    dst[len+1] = '\n';
    esp->uart.tx_io->data_size = len+2;
    timer_start_ms(esp->timer, esp->req[esp->req_tail].timeout & ((1 << ESP_FLAGS_Pos)-1) );
    io_write_sync_exo(HAL_IO_REQ(HAL_UART, IPC_WRITE), esp->uart.port, esp->uart.tx_io);

    return true;
}

static void esp_pkt_add(ESP8266* esp, ESP_REQ_TYPE type, uint32_t param, uint32_t to)
{
    esp->req[esp->req_head].type = type;
    esp->req[esp->req_head].param = param;
    esp->req[esp->req_head].timeout = to;
    esp->req_head++;
    if(esp->req_head >= ESP_MAX_REQ)
        esp->req_head = 0;
    esp_pkt_send_next(esp);
}

static inline void esp_pkt_add_string(ESP8266* esp,const char* str, uint32_t to)
{
    esp_pkt_add(esp, ESP_REQ_STRING, (uint32_t)str, to);
}

static inline void esp_pkt_add_param(ESP8266* esp, ESP_REQ_TYPE type, uint32_t to)
{
    esp_pkt_add(esp, type, 0, to);
}

static inline  void esp_chip_open(ESP8266* esp)
{
#if(ESP_DEBUG == 0)
    esp_pkt_add_string(esp, str_echo_off, ESP_TO);
#endif // ESP_DEBUG
    esp_pkt_add_string(esp, str_listen_stop, ESP_TO);
    esp_pkt_add_string(esp, str_dhcp_on, ESP_TO);
    esp_pkt_add_string(esp, str_set_mux, ESP_TO);
    esp_pkt_add_string(esp, str_auto_off, ESP_TO);
}

static void init(ESP8266* esp)
{
    memset(esp, 0, sizeof(ESP8266));
    esp->timer = timer_create(0, HAL_WIFI);
    esp->uart.rx_io1 = io_create(ESP_IO_READ_SIZE);
    esp->uart.rx_io2 = io_create(ESP_IO_READ_SIZE);
    esp->uart.tx_io = io_create(ESP_IO_WRITE_SIZE);
    esp->state = ESP_CLOSE;
    esp->is_receive = false;
    esp->join_state = ESP_JOIN_DISCONNECT;
}

static void esp_open(ESP8266* esp, int uart_port, HANDLE process)
{
    BAUD baudrate;

    esp->uart.port = uart_port;
    uart_open(uart_port, UART_MODE_IO);
    baudrate.baud = ESP_BAUD;
    baudrate.data_bits = 8;
    baudrate.parity = 'N';
    baudrate.stop_bits = 1;
    uart_set_baudrate(uart_port, &baudrate);
    io_read_exo(HAL_IO_REQ(HAL_UART, IPC_READ), esp->uart.port, esp->uart.rx_io1, ESP_IO_READ_SIZE);
    io_read_exo(HAL_IO_REQ(HAL_UART, IPC_READ), esp->uart.port, esp->uart.rx_io2, ESP_IO_READ_SIZE);

    esp->state = ESP_UNKNOWN;
    esp->process = process;
    esp_pkt_add_string(esp, str_at, ESP_TO_INIT);
    esp_chip_open(esp);
#if(ESP_DEBUG)
    printf("ESP:try to connect to chip\n");
#endif //ESP_DEBUG

}

static void esp_close(ESP8266* esp)
{
    uart_close(esp->uart.port);
    esp_pkt_flush_internal(esp);
}


static inline ESP_REPLAY_NUM esp_parse_replay(char* str)
{
    int i, j, char_cnt;
    const char* repl;
    for(i = 0; i < (sizeof(esp_replay)/sizeof(esp_replay[0])); i++)
    {
        repl = esp_replay[i].strptr;
        char_cnt = esp_replay[i].count;
        for(j = 0; j < char_cnt; j++)
            if(str[j] != repl[j])
                break;
        if(j != char_cnt)
            continue;
        if(repl[char_cnt - 1] == 0) // if exact string - leave last zero
            char_cnt--;
        for(j = 0; ;j++)
        {
            str[j] = str[j + char_cnt];
            if(str[j] == 0)
                return (ESP_REPLAY_NUM)i;
        }
    }
    return ESP_REPLAY_NONE;
}

static inline void esp_tcp_connect(ESP8266* esp, int num) // received num,CONNECT from ESP
{
    ESP_SESSION* session = &esp->sessions[num];
    if((num < 0) || (num > ESP_CONNECT_COUNT) )
            return;
    switch(session->state)
    {
    case ESP_SESSION_FREE:
        session->process = esp->sessions[0].process;
    case ESP_SESSION_ASSIGN:
        session->state = ESP_SESSION_OPEN;
    case ESP_SESSION_LISTEN:
        ipc_post_inline(session->process, HAL_CMD(HAL_TCP, IPC_OPEN), num, 0, 0);
        break;
    case ESP_SESSION_OPEN:
        break;
    }
    session->state = ESP_SESSION_OPEN;
}

static void esp_tcp_close(ESP8266* esp, int num)
{
    ESP_SESSION* session = &esp->sessions[num];
#if(ESP_DEBUG_FLOW)
    printf("ESP:close session %u process %x state:%u\n", num, session->process, esp->state);
#endif //ESP_DEBUG_FLOW
    if((num < 0) || (num > ESP_CONNECT_COUNT) )
            return;

    esp_rx_io_complete(esp, num);
    if(session->tx)
    {
        io_complete_ex(session->tx_process, HAL_IO_CMD(HAL_TCP, IPC_WRITE), num, session->tx, ERROR_CONNECTION_CLOSED);
        session->tx = NULL;
    }
    if((session->process != 0) && (session->state == ESP_SESSION_OPEN))
        ipc_post_inline(session->process, HAL_CMD(HAL_TCP, IPC_CLOSE), num, 0, 0);

    if((session->process != 0) && (session->state == ESP_SESSION_ASSIGN))
    {
        ipc_post_inline(session->process, HAL_CMD(HAL_TCP, IPC_OPEN), num, INVALID_HANDLE, ERROR_CONNECTION_REFUSED);
        session->state = ESP_SESSION_FREE;
    }

    if(num == 0)
        esp->sessions[num].state = ESP_SESSION_LISTEN;
    else
    {
        esp->sessions[num].state = ESP_SESSION_FREE;
        session->process = 0;
    }
}

static inline void esp_chip_rebooted(ESP8266* esp)
{
    esp_pkt_flush_internal(esp);
    esp->state = ESP_BUSY;
    esp_chip_open(esp);
    ipc_post_inline(esp->process, HAL_CMD(HAL_WIFI, IPC_REBOOT), 0, 0, 0);
}

static inline  void esp_join_complete(ESP8266* esp,ESP_JOIN_STATE new_state)
{
    if(new_state == esp->join_state)
        return;
    esp_set_result(esp, new_state);
    ipc_post_inline(esp->process, HAL_CMD(HAL_WIFI, IPC_JOIN), 0, 0, (uint32_t) new_state);
}
// received ERROR
static inline  void esp_error(ESP8266* esp)
{
    if(esp->req[esp->req_tail].type == ESP_REQ_JOIN)
        esp_join_complete(esp, ESP_JOIN_NOT_FOUND);
}

static inline void esp_parse_string(ESP8266* esp)
{
    ESP_REPLAY_NUM replay;
    int conn_num = -1;
    char* str = esp->str;
    if( (str[0] >= '0') && (str[0] < (ESP_CONNECT_COUNT + '0')) )
    {
        conn_num = str[0] - '0';
        str++;
    }
    replay = esp_parse_replay(str);
#if(ESP_DEBUG_REPLAY)
    if(replay == ESP_REPLAY_NONE)
    {
        puts(esp->str);
        putc('\n');
    }else{
        if(conn_num <0)
            printf("ESP:replay %s\n", esp_replay_str[replay]);
        else
            printf("ESP:replay %s:%d\n", esp_replay_str[replay], conn_num);
    }
#endif // ESP_DEBUG_REPLAY
    switch(replay)
    {
    case ESP_REPLAY_OK:
        switch(esp->state)
        {
        case ESP_TRY_SEND:
            return;
        case ESP_UNKNOWN_WAIT:
            ipc_post_inline(esp->process, HAL_CMD(HAL_WIFI, IPC_OPEN), 0, 0, 0);// chip works when open
            break;
        default:
            break;
        }
        break;
    case ESP_REPLAY_ERROR:
        esp_error(esp);
        break;
    case ESP_REPLAY_SEND_OK:
        esp_tx_io_complete(esp, esp->tx_conn);
        break;
    case ESP_REPLAY_SEND_FAIL:
        esp_tx_io_complete(esp, esp->tx_conn);// TODO: send error
        break;
    case ESP_REPLAY_READY:

        esp_chip_rebooted(esp);
        break;
    case ESP_REPLAY_JOIN_FAIL:
        break;
// continue receive, wait another answer
    case ESP_REPLAY_IP:            // WIFI DISCONNECT
        param_str_to_ip(str, &esp->sta.ip, '\"');
#if (ESP_DEBUG) && (!ESP_DEBUG_REPLAY)
        printf("ESP:assign IP:%d.%d.%d.%d\n", esp->sta.ip.u8[0], esp->sta.ip.u8[1], esp->sta.ip.u8[2], esp->sta.ip.u8[3]);
#endif
        return;
    case ESP_REPLAY_DISJOIN:            // WIFI DISCONNECT
        return;
    case ESP_REPLAY_AP_DATA:
        if (str[1] == 0)
            esp_join_complete(esp, (ESP_JOIN_STATE)str[0]-'0');
        return;
    case ESP_REPLAY_JOINED:
        esp_join_complete(esp, ESP_JOIN_OK);
        return;
    case ESP_REPLAY_CONNECT:
            esp_tcp_connect(esp, conn_num);
        return;
    case ESP_REPLAY_CLOSED:
            esp_tcp_close(esp, conn_num);
        return;
    default:
        return;
    }
// remove last command
    esp->req_tail++;
    if(esp->req_tail >= ESP_MAX_REQ)
        esp->req_tail = 0;

    timer_stop(esp->timer, 0, HAL_WIFI);
    esp->state = ESP_IDLE;
// and try to send next
    esp_pkt_send_next(esp);
}

static inline void esp_received(ESP8266* esp, char c)
{
    if( (c < ' ') && (c != '\n') && (c != '\r'))
    {
        esp->strlen = 0;
        return;
    }

    if(esp->strlen >= ESP_MAX_STRING-1)
    {
        for(int i = 0; i < (ESP_MAX_STRING-10); i++)
            esp->str[i] = esp->str[i+10];
        esp->strlen-=10;
    }

    if(c == '\n')
    {
        if( (esp->strlen <=2) || (esp->str[esp->strlen-1] != '\r') )
        {
            esp->strlen = 0;
            return;
        }
// received valid string
        esp->str[esp->strlen-1] = 0;
        esp_parse_string(esp);
        esp->strlen = 0;
    }else
        esp->str[esp->strlen++] = c;
}

static inline bool esp_test_rx(ESP8266* esp)
{
    int i;
    ESP_SESSION* session;
    uint32_t conn_num;
    for(i = 0; i< (sizeof(str_DATA)-1); i++)
        if(str_DATA[i] != esp->str[i])
            return false;

    conn_num = esp->str[i++] - '0';
#if (ESP_DEBUG_FLOW)
    printf("ESP: test rx session %d\n", conn_num);
#endif // ESP_DEBUG_FLOW

    if(conn_num > ESP_CONNECT_COUNT)
        return false;
    if(esp->str[i++] != ',')
        return false;
    session = &esp->sessions[conn_num];
    esp->rx_size = atou(&esp->str[i], esp->strlen - i);
    esp->rx_conn = conn_num;
    esp->is_receive = true;
    if(session->rx ==0)
    {
        if(session->rx_buffer)
            session->rx_buffer = realloc(session->rx_buffer, esp->rx_size);
        else
            session->rx_buffer = malloc(esp->rx_size);
        session->rx_buffer_len = 0;
    }
    return true;
}

static inline int esp_rx_receive(ESP8266* esp, IO* io, int pos)
{
    ESP_SESSION* session = &esp->sessions[esp->rx_conn];
    IO* rx = session->rx;
    char* ptr;
    int chunk = io->data_size - pos;
    if(chunk > esp->rx_size)
        chunk = esp->rx_size;
    esp->rx_size -= chunk;
#if(ESP_DEBUG_FLOW)
    printf("chunk:%u io:%u\n", chunk, rx);
#endif // ESP_DEBUG_FLOW
    if(esp->rx_size == 0)
        esp->is_receive = false;

    if(rx == NULL)
    {
        if(session->rx_buffer == NULL)
        {
            session->rx_buffer = malloc(chunk);
            session->rx_buffer_len = 0;
        }
        ptr = session->rx_buffer;
        memcpy(&ptr[session->rx_buffer_len], io_data(io)+pos, chunk);
        session->rx_buffer_len += chunk;
    }else{
        ptr = io_data(rx);
        memcpy(&ptr[rx->data_size], io_data(io)+pos, chunk);
        rx->data_size += chunk;
        if(esp->rx_size == 0)
            esp_rx_io_complete(esp, esp->rx_conn);
    }
    return chunk;
}

static inline void uart_rx(ESP8266* esp, IO* io)
{
    char* ptr = (char*)io_data(io);
    char c;

    for(int i = 0; i < io->data_size; i++)
    {
        c = ptr[i];
        if(esp->is_receive)
        {
            i+=esp_rx_receive(esp, io, i) -1;
            esp->strlen = 0;
        }else{
            if(c == ':') if (esp_test_rx(esp))
                    continue;
            if( (esp->state == ESP_TRY_SEND) && (c == ' ') && (esp->str[0] == '>') )
            {
                io_write_exo(HAL_IO_REQ(HAL_UART, IPC_WRITE), esp->uart.port, esp->sessions[esp->tx_conn].tx);
                esp->strlen = 0;
                esp->state = ESP_BUSY;
                continue;

            }
            esp_received(esp, ptr[i]);
        }
    }
}

static inline void esp_uart_tx_complete(ESP8266* esp)
{

}

static inline void uart_request(ESP8266* esp, IPC* ipc)
{
    IO* io;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        if((int)ipc->param3 == ERROR_IO_CANCELLED)
            break;
        io = (IO*)ipc->param2;
        if((int)ipc->param3 >0)
            uart_rx(esp, io);
        io_read_exo(HAL_IO_REQ(HAL_UART, IPC_READ), esp->uart.port, io, ESP_IO_READ_SIZE);

#if(ESP_DEBUG_FLOW)
        {
        int16_t err = get_exo(HAL_REQ(HAL_UART, IPC_UART_GET_LAST_ERROR), esp->uart.port, 0, 0);
        if(err != 0 )
                printf("ESP:uart err:%d \n", err);
        }
#endif // ESP_DEBUG_FLOW
        break;
    case IPC_WRITE:
        esp_uart_tx_complete(esp);
    default:
        error (ERROR_NOT_SUPPORTED);
        break;
    }
}

static inline void esp_join(ESP8266* esp)
{
    esp->join_state = ESP_JOIN_DISCONNECT;
    esp_pkt_add_param(esp, ESP_REQ_JOIN , ESP_TO_JOIN);
    esp_pkt_add_string(esp, str_get_ip, ESP_TO_JOIN);
#if (ESP_DEBUG) && (!ESP_DEBUG_REPLAY)
    printf("ESP: joining to SSID:\"%s\" pass: \"%s\"\n", esp->sta.ssid, esp->sta.password);
#endif

}

static inline void esp_listen(ESP8266* esp, IPC* ipc)
{
    ESP_SESSION* session = &esp->sessions[0];
    if (session->process != 0)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    session->state = ESP_SESSION_LISTEN;
    session->local_port = (uint16_t)ipc->param1;
    session->process = ipc->process;
    ipc->param2 = 0;   // session handler
    esp_pkt_add_param(esp, ESP_REQ_LISTEN, ESP_TO);
}
//--------------- TCP requests ----------
static void esp_close_session(ESP8266* esp, HANDLE conn)
{
    if(conn > ESP_CONNECT_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if(esp->sessions[conn].state != ESP_SESSION_OPEN)
        return;
#if (ESP_DEBUG_FLOW)
    printf("ESP: close tcp session %d rx:%x\n", conn, esp->sessions[conn].rx);
#endif // ESP_DEBUG_FLOW
    esp_pkt_add(esp, ESP_REQ_CLOSE_CONN, conn, ESP_TO);
}

static void esp_open_session(ESP8266* esp, HANDLE conn)
{
    ESP_SESSION* session = &esp->sessions[conn];

#if (ESP_DEBUG_FLOW)
    printf("ESP: try to open session %d rx:%x\n", conn, esp->sessions[conn].rx);
#endif // ESP_DEBUG_FLOW
    if((conn > ESP_CONNECT_COUNT) && (conn !=0) )  // session 0 - only server, passive listen
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    if(session->state != ESP_SESSION_ASSIGN)
    {
        error(ERROR_INVALID_STATE);
        return;
    }
    esp_pkt_add(esp, ESP_REQ_OPEN_CONN, conn, ESP_TO_CONNECT);
    error(ERROR_SYNC);
}

static inline void esp_close_listen(ESP8266* esp)
{
    switch(esp->sessions[0].state)
    {
    case ESP_SESSION_OPEN:
        esp_pkt_add(esp, ESP_REQ_CLOSE_CONN, 0, ESP_TO);
    case ESP_SESSION_LISTEN:
        esp_pkt_add_string(esp, str_listen_stop, ESP_TO);
        break;
    default:
        break;
    }
    esp->sessions[0].state = ESP_SESSION_FREE;
    esp->sessions[0].process = 0;
}

static inline void esp_tcp_read(ESP8266* esp, HANDLE handle, IO* io, HANDLE process)
{
    TCP_STACK* tcp_stack;
    ESP_SESSION* session;
    int chunk;
    if(handle > ESP_CONNECT_COUNT)
        return;
    session = &esp->sessions[handle];
    io->data_size = io->stack_size = 0;
#if (ESP_DEBUG_FLOW)
    printf("ESP: TCP read session %d rx:%x\n", handle, session->rx);
#endif // ESP_DEBUG_FLOW
    if (session->state != ESP_SESSION_OPEN)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }


    if (session->rx != NULL)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    tcp_stack = io_push(io, sizeof(TCP_STACK));
    tcp_stack->flags = 0;
    tcp_stack->urg_len = 0;
    if(session->rx_buffer_len)
    {
        if(session->rx_buffer_len > io_get_free(io))
        {
            chunk = io_get_free(io);
            memcpy(io_data(io), session->rx_buffer,chunk);
            io->data_size = chunk;
            memmove(session->rx_buffer, &session->rx_buffer[chunk], session->rx_buffer_len - chunk);
            session->rx_buffer_len -= chunk;
        }else{
            memcpy(io_data(io), session->rx_buffer, session->rx_buffer_len);
            free(session->rx_buffer);
            io->data_size = session->rx_buffer_len;
            session->rx_buffer_len = 0;
            session->rx_buffer = NULL;
        }
        session->rx = NULL;
        error(ERROR_OK);
#if (ESP_DEBUG_FLOW)
    printf("ESP: TCP read complete len:%d\n", io->data_size);
#endif // ESP_DEBUG_FLOW
        return;
    }
    session->rx = io;
    session->process = process;
    session->rx_size = io_get_free(io);

#if (ESP_DEBUG_FLOW)
    printf("ESP: TCP read len:%d\n", io->data_size);
#endif // ESP_DEBUG_FLOW

    error(ERROR_SYNC);
}

static inline void esp_tcp_write(ESP8266* esp, uint32_t conn, IO* io, HANDLE process)
{
    ESP_SESSION* session;
    if(conn > ESP_CONNECT_COUNT)
        return;
#if (ESP_DEBUG)
    printf("ESP: tx req %d size:%u\n", conn, io->data_size);
#endif // ESP_DEBUG

    session = &esp->sessions[conn];
    if (session->tx != NULL)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    session->tx = io;
    session->tx_process = process;
    esp_pkt_add(esp, ESP_REQ_SEND_DATA, conn, ESP_TO_SEND_DATA);
    error(ERROR_SYNC);
}

static inline HANDLE esp_create_tcb_internal(ESP8266* esp, char* remote_host, uint16_t remote_port, HANDLE process)
{
    int i;
    for(i = ESP_CONNECT_COUNT-1; i >=0 ; i--)
        if(esp->sessions[i].state == ESP_SESSION_FREE)
            break;
    if(i == ESP_CONNECT_COUNT)
        return INVALID_HANDLE;
    esp->sessions[i].state = ESP_SESSION_ASSIGN;
    esp->sessions[i].remote_port = remote_port;
    esp->sessions[i].process = process;
    if(esp->sessions[i].remote_host != NULL)
        free(esp->sessions[i].remote_host);
    esp->sessions[i].remote_host = malloc(strlen(remote_host)+1);
    strcpy(esp->sessions[i].remote_host, remote_host);
    return i;
}

static HANDLE esp_create_tcb(ESP8266* esp, uint16_t remote_port, uint32_t remote_ip, HANDLE process)
{
    char str[4*3+5];
    IP ip;
    ip.u32.ip = remote_ip;
    sprintf(str, "%u.%u.%u.%u", ip.u8[0], ip.u8[1], ip.u8[2], ip.u8[3]);
    return  esp_create_tcb_internal(esp, str, remote_port, process);
}

static void esp_create_text_tcb(ESP8266* esp, IO* io, uint32_t remote_port, HANDLE process)
{
    *(HANDLE*)io_data(io) = esp_create_tcb_internal(esp, (char*)io_data(io), remote_port, process);
}


static inline void esp_timeout(ESP8266* esp)
{
    if(esp->state == ESP_UNKNOWN_WAIT)
        esp_set_result(esp, ESP_NOT_FOUND);
    else
        esp_set_result(esp, ESP_TIMEOUT);
    esp_pkt_flush_internal(esp);
    ipc_post_inline(esp->process, HAL_CMD(HAL_WIFI, IPC_TIMEOUT), 0, 0, esp->join_state);
}

static void wifi_request(ESP8266* esp, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        esp_open(esp, ipc->param1, ipc->process);
        break;
    case IPC_CLOSE:
        esp_close(esp);
        break;
    case IPC_TIMEOUT:
        esp_timeout(esp);
        break;
    case IPC_CREATE_TCB:
        esp_create_text_tcb(esp, (IO*)ipc->param2, ipc->param3, ipc->process);
        break;
    case IPC_JOIN:
        memcpy(&esp->sta, io_data((IO*)ipc->param2), sizeof(ESP_STA));
        esp_join(esp);
        break;
    default:
        error (ERROR_NOT_SUPPORTED);
        break;
    }
}

static void tcp_request(ESP8266* esp, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case TCP_LISTEN:
        esp_listen(esp, ipc);
        break;
    case TCP_CLOSE_LISTEN:
        esp_close_listen(esp);
        break;
    case TCP_CREATE_TCB:
        ipc->param2 = esp_create_tcb(esp, ipc->param1, ipc->param2, ipc->process);
        break;
    case IPC_OPEN:  // req for open session
        esp_open_session(esp, (HANDLE)ipc->param1);
        break;
    case IPC_CLOSE:  // req for close session
        esp_close_session(esp, (HANDLE)ipc->param1);
        break;
    case IPC_WRITE:  // req for write data
        esp_tcp_write(esp, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->process);
        break;
    case IPC_READ:  // req for read data from remote
        esp_tcp_read(esp, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->process);
        break;
    default:
        error (ERROR_NOT_SUPPORTED);
        break;
    }
}

void esp8266s_main()
{
    IPC ipc;
    ESP8266 esp;
#if (ESP_DEBUG)
    open_stdout();
#endif //ESP_DEBUG
    init(&esp);

    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_UART:
            uart_request(&esp, &ipc);
            break;
        case HAL_WIFI:
            wifi_request(&esp, &ipc);
            break;
        case HAL_TCP:
            tcp_request(&esp, &ipc);
            break;
        default:
            error (ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}
