#ifndef CRC_H
#define CRC_H
#include <stdint.h>

#define  CRC8_INIT     0xff
#define  CRC16_INIT    0xffff
#define  CRC32_INIT    0xffffffff

// CCITT polinom 0x04C11DB7, no XorOut variant  CRC-32/JAMCRC 
uint32_t crc32(const void* data, uint32_t len, uint32_t crc_init);

// CCITT polinom 0x1021, no XorOut variant  CRC-16/CCITT-FALSE 
uint16_t crc16(const void* data, uint32_t len, uint16_t crc_init);

// CCITT polinom 0x07, no XorOut variant  CRC-8/ROHC  
uint8_t crc8(const void* data, uint32_t len, uint8_t crc_init);

#endif // CRC_H
