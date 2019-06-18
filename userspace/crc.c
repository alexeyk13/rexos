#include "crc.h"

#define CRC32_POLY          0x04C11DB7
#define CRC32_POLY_REV      0xedb88320
uint32_t crc32(const void* data, uint32_t len, uint32_t crc_init)
{
   int i;
   uint32_t crc = crc_init;
   while(len--)
   {
      crc = crc ^ *(uint8_t*)data++;
      for (i = 0; i < 8; i++)
         crc = (crc >> 1) ^ (CRC32_POLY_REV & (-(crc & 1)));
   }
   return crc;
}

uint16_t crc16 (const void* data, uint32_t len, uint16_t crc_init)
{
    uint16_t crc = crc_init;
    while(len--)
    {
        crc = (unsigned char)(crc >> 8) | (crc << 8);
        crc ^= *(uint8_t*)data++;
        crc ^= (unsigned char)(crc & 0xff) >> 4;
        crc ^= (crc << 8) << 4;
        crc ^= ((crc & 0xff) << 4) << 1;
    }
    return crc;
}

#define  CRC8_POLY          0x07
#define  CRC8_POLY_REV      0xe0
uint8_t crc8(const void* data, uint32_t len, uint8_t crc_init)
{
     int i;
     uint8_t crc = crc_init;
     while (len--)
     {
         crc ^= *(uint8_t*)data++;
         for (i = 0; i < 8; i++)
            crc = crc & 1 ? (crc >> 1) ^ CRC8_POLY_REV : crc >> 1;
     }
     return crc;
}
