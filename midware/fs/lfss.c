/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: zurabob (zurabob@list.ru)
*/

#include "lfs.h"
#include "lfss.h"
#include "storage.h"
#include "storage.h"
#include "stdlib.h"
#include "stdio.h"
#include "sys.h"
#include "crc.h"
#include <string.h>

//-------- raw storage proc-----------------------
static bool storage_raw_read_sync(HAL hal, HANDLE process, HANDLE user, IO* io, uint32_t offset, uint32_t size)
{
    int res;
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = offset;
    stack->flags = STORAGE_OFFSET_INSTED_SECTOR;
    res = io_read_sync(process, HAL_IO_REQ(hal, IPC_READ), user, io, size);
    if (res == size)
    {
        return true;
    }else{
#if (LFS_DEBUG_ERRORS)
    printf("LFS: can't read data, offset:%x size:%d error:%d\n",offset, size, res);
#endif //LFS_DEBUG_ERRORS
    return false;
    }
}

static bool storage_raw_write_sync(HAL hal, HANDLE process, HANDLE user, IO* io, uint32_t offset, uint32_t size)
{
    int i;
    STORAGE_STACK* stack = io_push(io, sizeof(STORAGE_STACK));
    stack->sector = offset;
    stack->flags = STORAGE_OFFSET_INSTED_SECTOR | STORAGE_FLAG_WRITE | STORAGE_FLAG_VERIFY;
    for (i = 0; i < 3; i++)
    {
        uint32_t res = io_read_sync(process, HAL_IO_REQ(hal, IPC_WRITE), user, io, size);
        if (res == size)
            return true;
    }
#if (LFS_DEBUG_ERRORS)
    printf("LFS: can't write data offset:%d\n", offset);
#endif //LFS_DEBUG_ERRORS
    return false;

}
//-------- internal proc-----------------------
static inline uint32_t lfss_get_size_c(uint32_t size_b)
{
    return  (size_b + RBS_CHUNK_SIZE-1)/ RBS_CHUNK_SIZE;;
}

static inline uint32_t lfss_chunk_to_offset(LFSS* lfss, uint32_t offset_c)
{
    return  (lfss->curr_file_offset_c + offset_c) * RBS_CHUNK_SIZE;
}

static inline uint32_t lfss_get_first_c(LFSS_FILE* file, uint32_t header_c, uint32_t len_c)
{
    int res =  header_c - len_c;
    if(res < 0)
        res += file->len_c;
    return (uint32_t)res;
}
static inline uint32_t lfss_get_prev_c(LFSS_FILE* file, uint32_t header_c)
{
    return lfss_get_first_c(file, header_c, 1);
}

static inline uint32_t lfss_get_next_c(LFSS_FILE* file, uint32_t chunk)
{
    chunk++;
    if(chunk >= file->len_c)
        return 0;
    return chunk;
}

static bool lfss_read_chunk(LFSS* lfss, uint32_t offset_c)
{
    return storage_raw_read_sync(lfss->hal, KERNEL_HANDLE, 0, lfss->io, lfss_chunk_to_offset(lfss, offset_c), RBS_IO_SIZE);
}

static bool lfss_write_chunk(LFSS* lfss, uint32_t offset_c)
{
    return storage_raw_write_sync(lfss->hal, KERNEL_HANDLE, 0, lfss->io, lfss_chunk_to_offset(lfss, offset_c), RBS_IO_SIZE);
}

static void lfss_erase_sector(LFSS* lfss, uint32_t sector)
{
    uint32_t fat_sector = ((lfss->curr_file_offset_c/RBS_CHUNKS_PER_SECTOR)+sector) * LFS_SECTOR_SIZE / FAT_SECTOR_SIZE;
    for(int i = 0; i < 3; i++)
    {
        if (storage_erase_sync(lfss->hal, KERNEL_HANDLE, 0, lfss->io, fat_sector,  LFS_SECTOR_SIZE / FAT_SECTOR_SIZE))
            return;
     }
    return;
}

static bool lfss_is_valid_header(LFSS* lfss, LFSS_FILE* file, uint32_t header_c)
{
    uint32_t first_c, len_c;
    uint16_t crc;
    LFS_HEADER* header = io_data(lfss->io);
    lfss_read_chunk(lfss, header_c);
    if(header->signat_lo != RBS_SIGN_LO || header->signat_hi != RBS_SIGN_HI ||
                           header->len >RBS_MAX_DATA_SIZE || header->len == 0)
        return false;
    //  calc crc
    len_c = lfss_get_size_c(header->len);
    first_c = lfss_get_first_c(file, header_c, len_c);
    crc = CRC16_INIT;
    do{
        lfss_read_chunk(lfss, first_c);
        crc = crc16(io_data(lfss->io), RBS_CHUNK_SIZE, crc);
        first_c = lfss_get_next_c(file, first_c);
    }while(--len_c);

    lfss_read_chunk(lfss, first_c); // read header again
    crc = crc16(io_data(lfss->io), 6, crc);
#if (LFS_DEBUG_TEST)
    printf("LFS: found header calc crc:%d stored:%d\n", crc, header->crc);
#endif //LFS_DEBUG_TEST

    if(crc == header->crc)
        return true;
    return false;
}

static bool lfss_seek_last(LFSS* lfss, LFSS_FILE* file)
{
    file->curr_pos = 0;
    file->curr_id = 0;
    for(int i = 0; i < file->len_c;i++)
    {
        if(lfss_is_valid_header(lfss,file, i))
        {
            LFS_HEADER* header = io_data(lfss->io);
#if (LFS_DEBUG_TEST)
        printf("LFS open: found header id:%d offset_c:%d\n", header->id, i);
#endif // LFS_DEBUG_TEST
            if(header->id > file->curr_id)
            {
                file->curr_pos = i;
                file->curr_id = header->id;
            }
        }
    }
    return true;
}
//---------------- process requests -------------
static inline void lfss_get_stat(LFSS* lfss, HANDLE id, IO* io)
{

}

static inline uint32_t lfss_get_prev(LFSS* lfss, HANDLE record)
{
    LFSS_FILE* file = lfss->curr_file;
    LFS_HEADER* header = io_data(lfss->io);
    uint32_t len, first_c, id;
    if(file->curr_id == 0)
        return INVALID_HANDLE;
    if(record == INVALID_HANDLE)
        return file->curr_pos;
    lfss_read_chunk(lfss, record);
    id = header->id;
    len = header->len;
    first_c = lfss_get_first_c(file, record, lfss_get_size_c(len)+1);
#if (LFS_DEBUG_TEST)
        printf("LFS prev: header id:%d offset_c:%d id:%d\n", header->id, first_c, id);
#endif // LFS_DEBUG_TEST
    if(lfss_is_valid_header(lfss, file, first_c))
    {
        if(header->id < id)
            return first_c;
        else
            return INVALID_HANDLE;
    }
    while(first_c != record)
    {
        first_c = lfss_get_prev_c(file, first_c);
        if(!lfss_is_valid_header(lfss, file, first_c))
            continue;
#if (LFS_DEBUG_TEST)
        printf("LFS prev: found header id:%d offset_c:%d id:%d\n", header->id, first_c, id);
#endif // LFS_DEBUG_TEST
        if(header->id < id)
            return first_c;
        else
            return INVALID_HANDLE;
    }
    return INVALID_HANDLE;
}

static inline uint32_t lfss_write(LFSS* lfss, IO* io, uint32_t size)
{
    LFSS_FILE* file = lfss->curr_file;
    uint32_t pos_c, chunk;
    uint16_t crc;
    bool res;
    if(size ==0 || size > RBS_MAX_DATA_SIZE)
        return ERROR_INVALID_PARAMS;
    pos_c  = file->curr_pos;
    if(file->curr_id == 0)
        pos_c =  (uint32_t)-1l;
    res = false;
    for(int i = 0; i < file->len_c; i++)
    {
        if(!res)
        {
            io_unhide(io, size);
            file->curr_id++;
            crc = CRC16_INIT;
        }
        pos_c = lfss_get_next_c(file, pos_c);
        if((pos_c & RBS_SECTOR_MASK) == 0)
            lfss_erase_sector(lfss, pos_c / RBS_CHUNKS_PER_SECTOR);

        if(io->data_size)
        {
            chunk = RBS_CHUNK_SIZE;
            if(io->data_size < chunk)
                chunk = io->data_size;
            memcpy(io_data(lfss->io), io_data(io), chunk);
            io_hide(io, chunk);
            crc = crc16(io_data(lfss->io), RBS_CHUNK_SIZE, crc);
            res = lfss_write_chunk(lfss, pos_c);
        }else{
            LFS_HEADER* header = io_data(lfss->io);
            header->id = file->curr_id;
            header->len = size;
            header->signat_hi = RBS_SIGN_HI;
            header->signat_lo = RBS_SIGN_LO;
            header->crc = crc16(io_data(lfss->io), 6, crc);
            file->curr_pos = pos_c;
            io_unhide(io, size);
            res = lfss_write_chunk(lfss, pos_c);
#if (LFS_DEBUG_TEST)
    printf("LFS: write complete id:%d pos:%d size:%d\n", file->curr_id, pos_c, size);
#endif //LFS_DEBUG_TEST
            if(res)
                return size;
        }
    };
#if (LFS_DEBUG_ERRORS)
    printf("LFS: flash corrupted!\n");
#endif //LFS_DEBUG_ERRORS
    return ERROR_CORRUPTED;
}

static inline int lfss_read(LFSS* lfss,  IO* io, HANDLE record)
{
    LFSS_FILE* file = lfss->curr_file;
    LFS_HEADER* header = io_data(lfss->io);
    uint32_t first_c, len, chunk;
    if(file->curr_id == 0)
        return ERROR_IO_FAIL;
    if(record == INVALID_HANDLE)
        record = file->curr_pos;
    if(record > file->len_c)
        return ERROR_INVALID_PARAMS;

    lfss_read_chunk(lfss, record);
    if(header->signat_hi != RBS_SIGN_HI  || header->signat_lo != RBS_SIGN_LO)
        return ERROR_INVALID_PARAMS;

    len = header->len;
    io_reset(io);
    if(io_get_free(io) < len)
        return ERROR_IO_BUFFER_TOO_SMALL;
    first_c = lfss_get_first_c(file, record, lfss_get_size_c(len));
    while(len)
    {
        chunk = len;
        if(chunk > RBS_CHUNK_SIZE)
            chunk = RBS_CHUNK_SIZE;
        lfss_read_chunk(lfss, first_c);
        io_data_append(io, io_data(lfss->io), chunk);
        first_c = lfss_get_next_c(file, first_c);
        len -= chunk;
    }
    return io->data_size;
}

static inline void lfss_format(LFSS* lfss)
{
    for(int i = 0; i < (lfss->curr_file->len_c/RBS_CHUNKS_PER_SECTOR); i++)
        lfss_erase_sector(lfss, i);
}

static inline void lfss_open(LFSS* lfss, IO* io)
{
    LFS_OPEN_TYPE* type;
    LFSS_FILE* file;
    uint32_t curr_offset_s;
    type = io_data(io);
    if(type->files >= LFS_MAX_FILES)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    lfss->hal = type->hal;
    if (!storage_open(lfss->hal, KERNEL_HANDLE, 0))
        return;

    lfss->io = io_create(RBS_IO_SIZE + sizeof(STORAGE_STACK));
    curr_offset_s = type->offset;
    lfss->files_cnt = type->files;
    lfss->files = malloc(lfss->files_cnt * sizeof(LFSS_FILE));
    for(int i = 0; i < lfss->files_cnt; i++)
    {
        file = &lfss->files[i];
        file->offset_c = lfss->curr_file_offset_c = curr_offset_s * RBS_CHUNKS_PER_SECTOR;
        file->len_c =  type->sizes[i] * RBS_CHUNKS_PER_SECTOR;
        curr_offset_s += type->sizes[i];
        file->curr_id = 0;
        if(lfss_seek_last(lfss, file))
            continue;
        error(ERROR_INTERNAL);
        return;
    }
    lfss->active = true;
}

static inline void lfss_close(LFSS* lfss)
{
    for(int i = 0; i < lfss->files_cnt; i++)
        free(&lfss->files[i]);
    io_destroy(lfss->io);
    storage_close(lfss->hal, KERNEL_HANDLE, 0);
    lfss->active = false;
}

static inline void lfss_init(LFSS* lfss)
{
    lfss->active = false;
    lfss->hal = lfss->files_cnt = 0;
    lfss->io = NULL;
    lfss->files = NULL;
}

static inline void lfss_request(LFSS* lfss, IPC* ipc)
{
    if(HAL_ITEM(ipc->cmd) == IPC_OPEN)
    {
        if(lfss->active)
            error(ERROR_ALREADY_CONFIGURED);
        else
            lfss_open(lfss, (IO*)ipc->param2);
        return;
    }
    if(!lfss->active)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    if(ipc->param1 >= lfss->files_cnt)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    lfss->curr_file = &lfss->files[ipc->param1];
    lfss->curr_file_offset_c = lfss->curr_file->offset_c;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_FORMAT:
        lfss_format(lfss);
        break;
    case IPC_CLOSE:
        lfss_close(lfss);
        break;
    case IPC_READ:
        ipc->param3 = lfss_read(lfss, (IO*)ipc->param2, ipc->param3);
        break;
    case IPC_WRITE:
        ipc->param3 = lfss_write(lfss, (IO*)ipc->param2, ipc->param3);
        break;
    case IPC_SEEK:
        ipc->param2 = lfss_get_prev(lfss, ipc->param2);
        break;
    case IPC_GET_STAT:
        lfss_get_stat(lfss, ipc->param1, (IO*)ipc->param2);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void lfss()
{
    IPC ipc;
    LFSS lfss;
#if (LFS_DEBUG || LFS_DEBUG_ERRORS)
    open_stdout();
#endif //(RBS_DEBUG)
    lfss_init(&lfss);
    for (;;)
    {
        ipc_read(&ipc);
        lfss_request(&lfss, &ipc);
        ipc_write(&ipc);
    }
}

