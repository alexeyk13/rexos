/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.
*/

#define  SPI_DMA                                    0

#include "stm32_spi.h"
#include "stm32_exo_private.h"
#include "../kstdlib.h"
#include "../kerror.h"
#include "../../userspace/stdio.h"
#include "../kirq.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "../../userspace/stm32/stm32.h"

typedef SPI_TypeDef* SPI_TypeDef_P;
typedef DMA_Channel_TypeDef* DMA_Channel_TypeDef_P;
#if (SPI_COUNT > 1)
static const SPI_TypeDef_P         __SPI_REGS[] =                        {SPI1, SPI2};
static const DMA_Channel_TypeDef_P __SPI_DMA_TX_REGS[] =                 {DMA1_Channel3, DMA1_Channel5};
static const DMA_Channel_TypeDef_P __SPI_DMA_RX_REGS[] =                 {DMA1_Channel2, DMA1_Channel4};

static const uint8_t __SPI_DMA_RX_VECTORS[] =                           {12, 14};
static const uint8_t __SPI_VECTORS[] =                                  {35, 36};
static const uint8_t __SPI_POWER_PINS[] =                               {12, 14};// APB2ENR, APB1ENR
#else
#endif

void stm32_spi_init(EXO* exo)
{
    int i;
    for (i = 0; i < SPI_COUNT; ++i)
        exo->spi.spis[i] = NULL;
}

void stm32_spi_on_rxc_isr(int vector, void* param)
{
    EXO* exo = (EXO*)param;
    uint32_t port;
    SPI* spi;
#if(SPI_DMA)
    if(vector == __SPI_DMA_RX_VECTORS[0])
    {
        DMA1->IFCR = 0xff << (1 * 4);
        port = 0;
    }else{
        DMA1->IFCR = 0xff << (3 * 4);
        port = 1;
    }
    spi = exo->spi.spis[port];
    if(spi->io)
        iio_complete(spi->process, HAL_IO_CMD(HAL_SPI, IPC_READ), port, spi->io);
    spi->io = NULL;
#else
    if(vector == __SPI_VECTORS[0])
        port = 0;
    else
        port = 1;
    spi = exo->spi.spis[port];
    uint16_t prom =  __SPI_REGS[port]->DR;
    if(spi->io)
    {
        ((uint8_t*)io_data(spi->io))[spi->cnt] = prom;
        if(++spi->cnt == spi->io->data_size)
        {
            iio_complete(spi->process, HAL_IO_CMD(HAL_SPI, IPC_READ), port, spi->io);
            spi->io = NULL;
        }else
            __SPI_REGS[port]->DR = ((uint8_t*)io_data(spi->io))[spi->cnt];
    }
#endif //SPI_DMA
}

static inline void stm32_spi_io_open(EXO* exo, SPI_PORT port)
{
#if(SPI_DMA)

    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    __SPI_REGS[port]->CR2 |= SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN ;//| SPI_CR2_SSOE;
    __SPI_DMA_RX_REGS[port]->CPAR = (uint32_t)&__SPI_REGS[port]->DR;
    __SPI_DMA_TX_REGS[port]->CPAR = (uint32_t)&__SPI_REGS[port]->DR;

    kirq_register(KERNEL_HANDLE, __SPI_DMA_RX_VECTORS[port], stm32_spi_on_rxc_isr, (void*)exo);
    NVIC_EnableIRQ(__SPI_DMA_RX_VECTORS[port]);
    NVIC_SetPriority(__SPI_DMA_RX_VECTORS[port], 1);
#else
    kirq_register(KERNEL_HANDLE, __SPI_VECTORS[port], stm32_spi_on_rxc_isr, (void*)exo);
    NVIC_EnableIRQ(__SPI_VECTORS[port]);
    NVIC_SetPriority(__SPI_VECTORS[port], 14);
    __SPI_REGS[port]->CR2 |= SPI_CR2_RXNEIE;//| SPI_CR2_SSOE;
#endif  //SPI_DMA
}

void stm32_spi_open(EXO* exo, SPI_PORT port, uint32_t mode, uint32_t cs_pin)
{
    SPI* spi = exo->spi.spis[port];
    uint32_t cr1;
    if (spi)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }
    spi = kmalloc(sizeof(SPI));
    exo->spi.spis[port] = spi;
    if (spi == NULL)
    {
        kerror(ERROR_OUT_OF_MEMORY);
        return;
    }

    //enable clock
    if (port == SPI_1 )
        RCC->APB2ENR |= 1 << __SPI_POWER_PINS[port];
    else
        RCC->APB1ENR |= 1 << __SPI_POWER_PINS[port];
    spi->cs_pin = cs_pin;
    cr1 = 0;
    if (mode & SPI_LSBFIRST_MSK)
        cr1 |= SPI_CR1_LSBFIRST;
    if (mode & SPI_CPOL_MSK)
        cr1 |= SPI_CR1_CPOL;
    if (mode & SPI_CPHA_MSK)
        cr1 |= SPI_CR1_CPHA;

    __SPI_REGS[port]->CR1 |= ((mode & 0x07) << 3) | SPI_CR1_MSTR;
    cr1 |= SPI_CR1_SSM | SPI_CR1_SSI;
#if defined(STM32F1)
    switch (mode & 0x0F00)
    {
    case (7 << 8):
            break;
    case (15 << 8):
            cr1 |= SPI_CR1_DFF;
            break;
    default:
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    __SPI_REGS[port]->CR1 |= cr1;
    if(mode & SPI_IO_MODE)
    {
        stm32_spi_io_open(exo, port);
        spi->io_mode = true;
    }else{
        __SPI_REGS[port]->CR2 = SPI_CR2_SSOE;
        spi->io_mode = false;
    }
#else
    __SPI_REGS[port]->CR2 = (mode & 0x0F00) | SPI_CR2_SSOE | SPI_CR2_FRXTH;
#endif // STM32F1
    __SPI_REGS[port]->CR1 |= SPI_CR1_SPE;
}

void stm32_spi_close(EXO* exo, SPI_PORT port)
{
    SPI* spi = exo->spi.spis[port];
    if (spi == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    __SPI_REGS[port]->CR1 &= ~SPI_CR1_SPE;
    if (port == SPI_1)
        RCC->APB2ENR &= ~(1 << __SPI_POWER_PINS[port]);
    else
        RCC->APB1ENR &= ~(1 << __SPI_POWER_PINS[port]);

    kfree(spi);
    exo->spi.spis[port] = NULL;
}

static uint16_t stm32_spi_write_word(SPI_PORT port, uint16_t data)
{
//    *(uint8_t *)&__SPI_REGS[port]->DR = (uint8_t)data;
    __SPI_REGS[port]->DR = data;
    while ((__SPI_REGS[port]->SR & SPI_SR_RXNE) == 0);
    return __SPI_REGS[port]->DR;
}

static inline void stm32_spi_read(EXO* exo, IPC* ipc, SPI_PORT port)
{
    SPI* spi = exo->spi.spis[port];
    IO* io= (IO*)ipc->param2;
    if ((spi == NULL) || !spi->io_mode || (io->data_size == 0))
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    spi->process = ipc->process;
    spi->io = io;
#if(SPI_DMA)
    __SPI_DMA_RX_REGS[port]->CCR =  DMA_CCR1_MINC | DMA_CCR1_PL_0 | DMA_CCR1_PL_1 | DMA_CCR1_TCIE;
    __SPI_DMA_TX_REGS[port]->CCR = DMA_CCR1_DIR | DMA_CCR1_MINC;// | DMA_CCR1_PSIZE_1 ;//| DMA_CCR1_PL_0;
    __SPI_DMA_RX_REGS[port]->CMAR =  __SPI_DMA_TX_REGS[port]->CMAR = (uint32_t)io_data(io);
    __SPI_DMA_RX_REGS[port]->CNDTR = __SPI_DMA_TX_REGS[port]->CNDTR = io->data_size;
    __SPI_DMA_RX_REGS[port]->CCR |= DMA_CCR1_EN;
    __SPI_DMA_TX_REGS[port]->CCR |= DMA_CCR1_EN;
#else
    spi->cnt = 0;
    __SPI_REGS[port]->DR = ((uint8_t*)io_data(spi->io))[0];
#endif  //SPI_DMA
    kerror(ERROR_SYNC);
}

void stm32_spi_write(EXO* exo, IPC* ipc, SPI_PORT port)
{
// TODO: 8bit access if data len  <=8 bit
    uint32_t size = (ipc->param1 >> 8) & 0xFF;
    SPI* spi = exo->spi.spis[port];
    if (spi == NULL)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    ipc->param3 = stm32_spi_write_word(port, ipc->param3);
    if (size < 2)
        return;
    ipc->param2 = stm32_spi_write_word(port, ipc->param2);
    if (size < 3)
        return;
    ipc->param1 = stm32_spi_write_word(port, ipc->param1 >> 16);

}

void stm32_spi_request(EXO* exo, IPC* ipc)
{
    SPI_PORT port = (SPI_PORT)(ipc->param1 & 0xFF);
    if (port >= SPI_COUNT)
    {
        kerror(ERROR_INVALID_PARAMS);
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
    case IPC_READ:
        stm32_spi_read(exo, ipc, port);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}
