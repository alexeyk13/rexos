/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis.h"
#include "../userspace/stdlib.h"
#include "../userspace/stdio.h"

typedef struct _SCSIS {
    void* storage;
    void* media;
} SCSIS;

void scsis_reset(SCSIS* scsis)
{

}

SCSIS* scsis_create()
{
    SCSIS* scsis = malloc(sizeof(SCSIS));
    if (scsis == NULL)
        return NULL;
    scsis->storage = NULL;
    scsis->media = NULL;
    scsis_reset(scsis);
    return scsis;
}

SCSIS_RESPONSE scsis_request(SCSIS* scsis, uint8_t* req, unsigned int size, IO* io)
{
    printf("scsis request\n\r");
    int i;
    for (i = 0; i < size; ++i)
        printf(" %02x", req[i]);
    printf(" (%d)\n\r", size);
    return SCSIS_RESPONSE_FAIL;
}

SCSIS_RESPONSE scsis_storage_response(SCSIS* scsis, IO* io)
{
    printd("scsis storage response\n\r");
    return SCSIS_RESPONSE_FAIL;
}


void scsis_destroy(SCSIS* scsis)
{
    free(scsis);
}
