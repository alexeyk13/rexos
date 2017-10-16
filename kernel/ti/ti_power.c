/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ti_power.h"
#include "ti_exo_private.h"
#include "ti_config.h"
#include "../../userspace/ti/ti.h"
#include "../kerror.h"
#include "../../userspace/process.h"

unsigned int ti_power_get_core_clock()
{
    //generally it's probably also possible to clock as 24MHz, but this mode is never used in reference.
    return 48000000;
}

unsigned int ti_power_get_clock(POWER_CLOCK_TYPE clock_type)
{
    switch (clock_type)
    {
    case POWER_CORE_CLOCK:
        return ti_power_get_core_clock();
    default:
        kerror(ERROR_NOT_SUPPORTED);
        return 0;
    }
}

void ti_power_domain_on(EXO* exo, POWER_DOMAIN domain)
{
    if (exo->power.power_domain_used[domain]++ == 0)
    {
        switch (domain)
        {
        case POWER_DOMAIN_PERIPH:
            PRCM->PDCTL0PERIPH = PRCM_PDCTL0PERIPH_ON;
            while ((PRCM->PDSTAT0PERIPH & PRCM_PDSTAT0PERIPH_ON) == 0) {}
            break;
        case POWER_DOMAIN_SERIAL:
            PRCM->PDCTL0SERIAL = PRCM_PDCTL0SERIAL_ON;
            while ((PRCM->PDSTAT0SERIAL & PRCM_PDSTAT0SERIAL_ON) == 0) {}
            break;
        default:
            break;
        }
    }
}

void ti_power_domain_off(EXO* exo, POWER_DOMAIN domain)
{
    if (--exo->power.power_domain_used[domain] == 0)
    {
        switch (domain)
        {
        case POWER_DOMAIN_PERIPH:
            PRCM->PDCTL0PERIPH = 0;
            while (PRCM->PDSTAT0PERIPH & PRCM_PDSTAT0PERIPH_ON) {}
            break;
        case POWER_DOMAIN_SERIAL:
            PRCM->PDCTL0SERIAL = 0;
            while (PRCM->PDSTAT0SERIAL & PRCM_PDSTAT0SERIAL_ON) {}
            break;
        default:
            break;
        }
    }
}

static inline void setup_trim_values()
{
    uint32_t rev;

    //power up AUX domain if disabled by previous power down
    AON_WUC->AUXCTL |= AON_WUC_AUXCTL_AUX_FORCE_ON;
    while ((AON_WUC->PWRSTAT & AON_WUC_PWRSTAT_AUX_PD_ON) == 0) {}
    //clock DDI0 OSC
    AUX_WUC->MODCLKEN0 = AUX_WUC_MODCLKEN0_AUX_DDI0_OSC | AUX_WUC_MODCLKEN0_SMPH | AUX_WUC_MODCLKEN0_AUX_ADI4;

    //some magic from TI sources. No CCFG is applied.
    rev = FCFG1->FCFG1_REVISION;
    if (rev == 0xffffffff)
        rev = 0;

    //just calibrate ADC, than turn off clocking
    AUX_ADI4->ADCREF1 = (FCFG1->SOC_ADC_REF_TRIM_AND_OFFSET_EXT & 0x1f);
    //default value
    AUX_ADI4->ADC0 = (3 << 3);
    AUX_WUC->MODCLKEN0 &= ~AUX_WUC_MODCLKEN0_AUX_ADI4;

    DDI0_OSC->ANABYPASSVAL1 = (((FCFG1->CONFIG_OSC_TOP >> 26) & 0xf) << 16) | (((FCFG1->CONFIG_OSC_TOP >> 10) & 0xffff) << 0);
    DDI0_OSC->CTL1 = ((FCFG1->OSC_CONF >> 19) & 3);
    DDI0_OSC->LFOSCCTL = (((FCFG1->CONFIG_OSC_TOP >> 2) & 0xff) << 0) | (((FCFG1->CONFIG_OSC_TOP >> 0) & 0x3) << 8);
    DDI0_OSC->ANABYPASSVAL2 = FCFG1->ANABYPASS_VALUE2 & 0x3fff;

    DDI0_OSC->AMPCOMPTH2 = FCFG1->AMPCOMP_TH2 & 0xfcfcfcfc;
    DDI0_OSC->AMPCOMPTH1 = FCFG1->AMPCOMP_TH1 & 0xfcffff;

    DDI0_OSC->ATESTCTL = 0;
    if (rev >= 0x00000022)
    {
        DDI0_OSC->AMPCOMPCTL = FCFG1->AMPCOMP_CTRL1 & 0x40ffffff;

        DDI0_OSC->ADCDOUBLERNANOAMPCTL = 0;
        if (FCFG1->OSC_CONF & (1 << 28))
            DDI0_OSC->ADCDOUBLERNANOAMPCTL |= DDI0_OSC_ADCDOUBLERNANOAMPCTL_ADC_SH_MODE_EN;
        if (FCFG1->OSC_CONF & (1 << 29))
            DDI0_OSC->ADCDOUBLERNANOAMPCTL |= DDI0_OSC_ADCDOUBLERNANOAMPCTL_ADC_SH_VBUF_EN;
        //In datasheet value is reserved, but present in TI code
        if (FCFG1->OSC_CONF & (1 << 27))
            DDI0_OSC->ATESTCTL |= (1 << 7);
        DDI0_OSC->LFOSCCTL |= (((FCFG1->OSC_CONF >> 25) & 0x3ul) << 22) | (((FCFG1->OSC_CONF >> 21) & 0xful) << 18);
    }
    else
    {
        DDI0_OSC->AMPCOMPCTL = FCFG1->AMPCOMP_CTRL1 & 0xffffff;
        DDI0_OSC->ADCDOUBLERNANOAMPCTL = DDI0_OSC_ADCDOUBLERNANOAMPCTL_ADC_SH_MODE_EN | DDI0_OSC_ADCDOUBLERNANOAMPCTL_ADC_SH_VBUF_EN;
    }

    if (rev >= 0x00000020)
    {
        DDI0_OSC->XOSCHFCTL = (((FCFG1->MISC_OTP_DATA_1 >> 27) & 3) << 8) | (((FCFG1->MISC_OTP_DATA_1 >> 24) & 7) << 2) |
                              (((FCFG1->MISC_OTP_DATA_1 >> 22) & 3) << 0);
        //In datasheet value is reserved, but present in TI code
        DDI0_OSC->ADCDOUBLERNANOAMPCTL |= (((FCFG1->MISC_OTP_DATA_1 >> 20) & 3) << 17);

        DDI0_OSC->RADCEXTCFG = (FCFG1->MISC_OTP_DATA_1 & 0xfffff) << 12;
    }
    else
    {
        DDI0_OSC->XOSCHFCTL = 0;
        DDI0_OSC->RADCEXTCFG = 0x403F8000;
    }

    ADI3->DCDCCTL1 = (ADI3->DCDCCTL1 & ~ADI3_DCDCCTL1_VDDR_TRIM_SLEEP) | (((FCFG1->LDO_TRIM >> 24) & 0x1f) << 0);
}

static inline void sclk_hf_setup()
{
    //TI: CC26_V1_BUG00261)
    DDI0_OSC->CTL0 |= DDI0_OSC_CTL0_FORCE_KICKSTART_EN;

#if (HSE_VALUE) == 24000000
    DDI0_OSC->CTL0 |= DDI0_OSC_CTL0_XTAL_IS_24M;
#elif (HSE_VALUE) == 48000000
//default value
#else
#error Setup correct HSE value!
#endif //HSE_VALUE

    DDI0_OSC->CTL0 &= ~DDI0_OSC_CTL0_CLK_LOSS_EN;

    //this will start HSE
    DDI0_OSC->CTL0 |= DDI0_OSC_CTL0_SCLK_HF_SRC_SEL_XOSC | DDI0_OSC_CTL0_SCLK_MF_SRC_SEL;
    while ((DDI0_OSC->STAT0 & DDI0_OSC_STAT0_XOSC_HF_EN) == 0) {}

    //switch to sclk_hf, sclk_mf sources (must be called from SRAM/ROM)
    while ((DDI0_OSC->STAT0 & DDI0_OSC_STAT0_SCLK_HF_SRC) == 0)
    {
        while ((DDI0_OSC->STAT0 & DDI0_OSC_STAT0_PENDINGSCLKHFSWITCHING) == 0) {}
        //called while IRQs are still disabled, so no manual disabling is required
        HARD_API->HFSourceSafeSwitch();
    }
}

static inline void sclk_lf_setup()
{
#if (LSE_VALUE)
    DDI0_OSC->CTL0 |= DDI0_OSC_CTL0_XOSC_LF_DIG_BYPASS;
    DDI0_OSC->CTL0 = (DDI0_OSC->CTL0 & ~DDI0_OSC_CTL0_SCLK_LF_SRC_SEL) | DDI0_OSC_CTL0_SCLK_LF_SRC_SEL_XOSC_LF;
#else
    DDI0_OSC->CTL0 = (DDI0_OSC->CTL0 & ~DDI0_OSC_CTL0_SCLK_LF_SRC_SEL) | DDI0_OSC_CTL0_SCLK_LF_SRC_SEL_RCOSC_LF;
#endif

    //wait will next SCLK_LF
    AON_RTC->SYNC = AON_RTC_SYNC_WBUSY;
    __REG_RC32(AON_RTC->SYNC);
}

void ti_power_init(EXO* exo)
{
    setup_trim_values();

    //switch to DC/DC
    AON_BATMON->FLASHPUMPP0 &= ~(1 << 5);
    AON_SYSCTL->PWRCTL |= AON_SYSCTL_PWRCTL_DCDC_ACTIVE | AON_SYSCTL_PWRCTL_DCDC_EN;

    //some power saving
    PRCM->PDCTL1VIMS = 0;
    AON_WUC->JTAGCFG = 0;

    //enable caching
    VIMS->CTL = VIMS_CTL_DYN_CG_EN | VIMS_CTL_PREF_EN | VIMS_CTL_MODE_CACHE;
    while (VIMS->STAT & VIMS_STAT_MODE_CHANGING) {}

    sclk_hf_setup();
    sclk_lf_setup();

    //tune flash latency? Some magic from TI code
    FLASH->CFG |= (1 << 5);
    FLASH->FPAC1 = (FLASH->FPAC1 & ~(0xffful << 16)) | (0x139ul << 16);

    //yet another one more TI magic
    if (((AON_SYSCTL->RESETCTL >> 12) & 3) == 1)
        AON_SYSCTL->RESETCTL |= AON_SYSCTL_RESETCTL_BOOT_DET_1_SET;

    exo->power.power_domain_used[POWER_DOMAIN_PERIPH] = exo->power.power_domain_used[POWER_DOMAIN_SERIAL] = 0;
}

void ti_power_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case POWER_GET_CLOCK:
        ipc->param2 = ti_power_get_clock(ipc->param1);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}

