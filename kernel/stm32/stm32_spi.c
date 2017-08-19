/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_spi.h"
#include "stm32_exo_private.h"
#include "../kstdlib.h"
#include "../../userspace/stdio.h"
#include "../kirq.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "../../userspace/stm32/stm32.h"

typedef SPI_TypeDef* SPI_TypeDef_P;
#if (SPI_COUNT > 1)
static const SPI_TypeDef_P __SPI_REGS[] =                               {SPI1, SPI2};
static const uint8_t __SPI_VECTORS[] =                                  {25, 26};
static const uint8_t __SPI_POWER_PINS[] =                               {12, 14};// APB2ENR, APB1ENR
#else
#endif

void stm32_spi_init(EXO* exo)
{
    int i;
    for (i = 0; i < I2C_COUNT; ++i)
        exo->spi.spis[i] = NULL;
}

void stm32_spi_on_isr(int vector, void* param)
{

}

void stm32_spi_open(EXO* exo, SPI_PORT port, uint32_t mode, uint32_t cs_pin)
{
    SPI* spi = exo->spi.spis[port];
    uint32_t cr1;
    if (spi)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    spi = kmalloc(sizeof(SPI));
    exo->spi.spis[port] = spi;
    if (spi == NULL)
    {
        error(ERROR_OUT_OF_MEMORY);
        return;
    }

    //enable clock
    if (port == SPI_1 )
        RCC->APB2ENR |= 1 << __SPI_POWER_PINS[port];
    else
        RCC->APB1ENR |= 1 << __SPI_POWER_PINS[port];

    spi->cs_pin = cs_pin;
    cr1=0;// | SPI_CR1_MSTR;// | SPI_CR1_SPE;
    if(mode & SPI_LSBFIRST_MSK) cr1 |= SPI_CR1_LSBFIRST;
    if(mode & SPI_CPOL_MSK)     cr1 |= SPI_CR1_CPOL;
    if(mode & SPI_CPHA_MSK)     cr1 |= SPI_CR1_CPHA;

/*    SPI1->CR1 =SPI_CR1_MSTR |((mode & 0x07) << 3);
    __SPI_REGS[port]->CR1 |= cr1 ;

    SPI1->CR2 =SPI_CR2_SSOE |SPI_CR2_RXNEIE |SPI_CR2_FRXTH
            |SPI_CR2_DS_3 |SPI_CR2_DS_2 |SPI_CR2_DS_1 |SPI_CR2_DS_0;
    SPI1->CR1 |=SPI_CR1_SPE;
*/

    __SPI_REGS[port]->CR1 |= ((mode & 0x07) << 3) | SPI_CR1_MSTR;
    __SPI_REGS[port]->CR1 |= cr1 ;
    __SPI_REGS[port]->CR2 = (mode & 0x0F00)| SPI_CR2_SSOE ;
    __SPI_REGS[port]->CR1 |=  SPI_CR1_SPE;

    //enable interrupt
//    kirq_register(KERNEL_HANDLE, __SPI_VECTORS[port], stm32_spi_on_isr, (void*)exo);
//    NVIC_EnableIRQ(__I2C_VECTORS[port]);
//    NVIC_SetPriority(__I2C_VECTORS[port], 13);
}

void stm32_spi_close(EXO* exo, SPI_PORT port)
{
    SPI* spi = exo->spi.spis[port];
    if (spi == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    //disable interrupt
//    NVIC_DisableIRQ(__SPI_VECTORS[port]);
//    kirq_unregister(KERNEL_HANDLE, __SPI_VECTORS[port]);

    __SPI_REGS[port]->CR1 &= ~SPI_CR1_SPE;
    if (port == SPI_1)
        RCC->APB2ENR &= ~(1 << __SPI_POWER_PINS[port]);
    else
        RCC->APB1ENR &= ~(1 << __SPI_POWER_PINS[port]);

    kfree(spi);
    exo->spi.spis[port] = NULL;
}

static uint16_t spi_write(SPI_PORT port, uint16_t data)
{
    __SPI_REGS[port]->DR = data;
    while((__SPI_REGS[port]->SR & SPI_SR_RXNE) == 0);
    return __SPI_REGS[port]->DR;
}

void stm32_spi_write(EXO* exo, IPC* ipc, SPI_PORT port)
{
    uint32_t size = (ipc->param1 >> 8) & 0xFF;
    SPI* spi = exo->spi.spis[port];
    if (spi == NULL)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    ipc->param3 = spi_write(port, ipc->param3);
    if(size < 2) return;
    ipc->param2 = spi_write(port, ipc->param2);
    if(size < 3) return;
    ipc->param1 = spi_write(port, ipc->param1 >> 16);

}

void stm32_spi_request(EXO* exo, IPC* ipc)
{
    SPI_PORT port = (SPI_PORT)(ipc->param1 & 0xFF);
    if (port >= SPI_COUNT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_spi_open(exo, port, ipc->param2, ipc->param3);
        break;
    case IPC_CLOSE:
        stm32_spi_close(exo, port);
    case IPC_WRITE:
        stm32_spi_write(exo, ipc, port);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}
