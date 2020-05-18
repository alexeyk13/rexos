/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_RADIO_H_
#define _NRF_RADIO_H_

#include "nrf_driver.h"
//#include "nrf_radio_config.h"
#include "../ipc.h"
#include "../io.h"

#define BLE_ADV_DATA_SIZE                    31
#define BLE_ADV_ADDR_SIZE                    6
#define BLE_ADV_HEADER_SIZE                  2
#define BLE_ADV_PKT_SIZE                     (BLE_ADV_DATA_SIZE + BLE_ADV_ADDR_SIZE + BLE_ADV_HEADER_SIZE)


#define RADIO_FLAG_EMPTY                                                (0 << 0)
#define RADIO_FLAG_TIMEOUT                                              (1 << 0)

#define RADIO_MIN_POWER_DBM                                             -20
#define RADIO_MAX_POWER_DBM                                             4

typedef enum {
    RADIO_SET_CHANNEL= IPC_USER,
    RADIO_SET_TXPOWER,
    RADIO_SET_ADV_ADDRESS,
    RADIO_START_RX,
    RADIO_STOP_RX,
} RADIO_IPCS;

#define RADIO_FLAG_PUBLC_ADDR                   (0 << 1)
#define RADIO_FLAG_RANDOM_ADDR                  (1 << 1)


#define BLE_CHSEL_Msk                           (1 << 5)
#define BLE_TXADD_Msk                           (1 << 6) // 1 - random address
#define BLE_RXADD_Msk                           (1 << 7)

// Radio BLE Packet Type
#define BLE_TYPE_Msk                            0x0F
#define BLE_TYPE(b)                            ((b) & BLE_TYPE_Msk)

#define BLE_TYPE_ADV_IND                        0x00
#define BLE_TYPE_ADV_DIRECT_IND                 0x01
#define BLE_TYPE_ADV_NONCONN_IND                0x02
#define BLE_TYPE_SCAN_REQ                       0x03
#define BLE_TYPE_SCAN_RSP                       0x04
#define BLE_TYPE_CONNECT_REQ                    0x05
#define BLE_TYPE_ADV_SCAN_IND                   0x06
/* Reserved bits 0111-1111 */

// Advertisement segment types
#define BLE_ADV_TYPE_FLAGS                      0x1
#define BLE_ADV_TYPE_UUID16_INC                 0x2  //Incomplete List of 16-bit Service Class UUIDs
#define BLE_ADV_TYPE_UUID16                     0x3  //Complete List of 16-bit Service Class UUIDs
#define BLE_ADV_TYPE_UUID32_INC                 0x4
#define BLE_ADV_TYPE_UUID32                     0x5
#define BLE_ADV_TYPE_UUID128_INC                0x6
#define BLE_ADV_TYPE_UUID128                    0x7
#define BLE_ADV_TYPE_NAME_SHORT                 0x8    //
#define BLE_ADV_TYPE_NAME                       0x9
#define BLE_ADV_TYPE_TRANSMITPOWER              0xA
#define BLE_ADV_TYPE_CONNINTERVAL               0x12  // The Slave Connection Interval Range See [Vol 3] Part C, Section 12.3.
#define BLE_ADV_TYPE_SERVICE_SOLICITATION16     0x14
#define BLE_ADV_TYPE_SERVICE_SOLICITATION128    0x15
#define BLE_ADV_TYPE_SERVICEDATA16              0x16 // The first 2 octets contain the 16 bit Service UUID followed by additional service data
#define BLE_ADV_TYPE_APPEARANCE                 0x19 // This value shall be the same as the Appearance characteristic, see [Vol 3] Part C, Section 12.2.
#define BLE_ADV_TYPE_SERVICEDATA32              0x20
#define BLE_ADV_TYPE_SERVICEDATA128             0x21
#define BLE_ADV_TYPE_MANUFACTURER_SPECIFIC      0xFF // The first two data octets shall contain a company identifier from Assigned Numbers

#define BLE_ADV_FLAG_DISC_LTD_Msk               (1 << 0)
#define BLE_ADV_FLAG_DISC_GEN_Msk               (1 << 1)
#define BLE_ADV_FLAG_BREDR_UNSUP_Msk            (1 << 2)
#define BLE_ADV_FLAG_LEBR_CAP_CONTR_Msk         (1 << 3) // Simultaneous LE and BR/EDR to Same Device Capable (Controller)
#define BLE_ADV_FLAG_LEBR_CAP_HOST_Msk          (1 << 4) // Simultaneous LE and BR/EDR to Same Device Capable (Host)


typedef  uint16_t UUID16;
typedef  uint32_t UUID32;
typedef  struct {
#pragma pack(push,1)
    union {
        uint32_t u32[4];
        uint8_t u8[16];
    };
#pragma pack(pop)
} UUID128;

#pragma pack(push,1)
typedef struct {
    union{
        uint8_t addr[6];
        struct{
            uint32_t lo;
            uint16_t hi;
        };
    };
}BLE_MAC;

typedef struct {
    uint8_t header;
    uint8_t len;                     // length (mac+data)
    BLE_MAC mac;
    uint8_t data[BLE_ADV_DATA_SIZE];
} BLE_ADV_PKT;
#pragma pack(pop)

typedef struct {
    uint32_t flags;
    BLE_MAC mac;
    uint8_t header;
    uint8_t ch;
    uint32_t id;
    int8_t rssi;
} BLE_ADV_RX_STACK;


__STATIC_INLINE void addrcpy(BLE_MAC* dst, const BLE_MAC* src)
{
    dst->hi = src->hi;
    dst->lo = src->lo;
}



bool radio_open(void* param);
void radio_close();

int radio_tx_sync(IO* io, uint8_t channel, uint32_t id, uint32_t flags);
void radio_rx(IO* io);
//int radio_rx_sync(IO* io);

void radio_cancel_rx();
void radio_set_channel(uint8_t channel);
void radio_set_tx_power(int8_t tx_power_dBm);
void radio_set_mac_addr(const BLE_MAC* addr);

// weak callback from interrupt. Called on receive packet with valid CRC.
bool  nrf_rf_is_valid_pkt(BLE_ADV_PKT* pkt, void* param);


int ble_adv_add_flags(IO* io, uint8_t flags);
int ble_adv_add_data(IO* io, uint8_t type, const uint8_t* src, uint32_t data_size);
int ble_adv_add_str(IO* io, uint8_t type, const char* src);

#endif /* _NRF_RADIO_H_ */
