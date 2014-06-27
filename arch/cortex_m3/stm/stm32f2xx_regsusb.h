/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32F2XX_REGSUSB_H
#define STM32F2XX_REGSUSB_H

#include "arch.h"

typedef struct {
	__IO uint32_t OTGCTL;     //0x00 Control and status register
	__IO uint32_t OTGINT;     //0x04 Interrupt register
	__IO uint32_t AHBCFG;     //0x08 AHB configuration register
	__IO uint32_t USBCFG;     //0x0c USB configuration register
	__IO uint32_t RSTCTL;     //0x10 Reset register
	__IO uint32_t INTSTS;     //0x14 Core interrupt register
	__IO uint32_t INTMSK;     //0x18 Interrupt mask register
	__I  uint32_t RXSTSR;     //0x1c Receive status debug read/ORG status read register
	__IO uint32_t RXSTSP;     //0x20 Receive status debug pop/ORG status pop register
	__IO uint32_t RXSFSIZ;    //0x24 Receive FIFO size register
	__IO uint32_t TX0FSIZ;    //0x28 Host: Nonperiodic transmit FIFO size, device: EP0 transmit FIFO size register
	__I  uint32_t NPTXSTS;    //0x2c Host only: Nonperiodic transmit FIFO/queue status register
	__IO uint32_t I2CCTL;	  //0x30 I2C access register
	__I  uint32_t RESERVED;	  //0x34
	__IO uint32_t CCFG;		  //0x38 Core configuration register
	__IO uint32_t CID;		  //0x3c Core ID register
	__I  uint32_t RESERVED1[48];//0x40..0xfc
	__IO uint32_t HPTXFSIZ;	 //0x100 Host periodic transmit FIFO size register
	__IO uint32_t DIEPTXF[7];//0x104 + (n *4) device IN endpoint transmit FIFO size register
} USB_HS_GENERAL_TypeDef;

typedef struct {
	__IO uint32_t CHAR;			//+0x00 characteristics register
	__IO uint32_t SPLT;			//+0x04 split control register
	__IO uint32_t INT;			//+0x08 interrupt register
	__IO uint32_t INTMSK;		//+0x0c interrupt mask register
	__IO uint32_t HCTSIZ;		//+0x10 transfer size register
	__IO uint32_t DMA;			//+0x14 DMA address register
	__I  uint32_t RESERVED[2];	//+0x18..0x1c
} USB_HS_HOST_CHANNEL_TypeDef;

typedef struct {
	__IO uint32_t CFG;			//0x400 configuration register
	__IO uint32_t FIR;			//0x404 frame interval register
	__IO uint32_t FNUM;			//0x408 frame number/frame time remaining register
	__I  uint32_t RESERVED;	   //0x40c
	__IO uint32_t PTXSTS;		//0x410 periodic transmit FIFO/queue status register
	__IO uint32_t AINT;			//0x414 all channels interrupt register
	__IO uint32_t AINTMSK;		//0x418 all channels interrupt mask register
	__I  uint32_t RESERVED1[9];//0x41c..0x43c
	__IO uint32_t PRT;			//0x440 port control and status register
	__IO uint32_t RESERVED2[47];//0x444..0x4fc
	USB_HS_HOST_CHANNEL_TypeDef CHANNEL[12];//0x500 0x20 * 12 channel control
} USB_HS_HOST_TypeDef;

typedef struct {
	__IO uint32_t CTL;			//+0x00 control register
	__I  uint32_t RESERVED;	   //+0x04
	__IO uint32_t INT;			//+0x08 interrupt register
	__I  uint32_t RESERVED1;   //+0x0c
	__IO uint32_t TSIZ;			//+0x10 transfer size register
	__IO uint32_t DMAADDR;		//+0x14 DMA address
	__IO uint32_t FSTS;			//+0x18 IN endpoint TxFIFO status register
	__I  uint32_t RESERVED2;   //+0x1c
} USB_HS_DEVICE_ENDPOINT_TypeDef;

typedef struct {
	__IO uint32_t CFG;			//0x800 device configuration register
	__IO uint32_t CTL;			//0x804 device control register
	__IO uint32_t STS;			//0x808 device status register
	__I  uint32_t RESERVED;	   //0x80c
	__IO uint32_t IEPMSK;		//0x810 device IN endpoint common interrupt mask register
	__IO uint32_t OEPMSK;		//0x814 device OUT endpoint common interrupt mask register
	__IO uint32_t AINT;			//0x818 all endpoints interrupt register
	__IO uint32_t AINTMSK;		//0x81c all endpoints interrupt mask register
	__I  uint32_t RESERVED1[2];//0x820..0x824
	__IO uint32_t VBUSDIS;		//0x828 VBUS discharge time register
	__IO uint32_t VBUSPULSE;	//0x82c VBUS pulsing time register
	__IO uint32_t THRCTL;		//0x830 threshold control register
	__IO uint32_t EIPEMPMSK;	//0x834 IN endpoint FIFO empty interrupt mask register
	__IO uint32_t EACHINT;		//0x838 each endpoint interrupt register
	__IO uint32_t EACHINTMSK;	//0x83c each endpoint interrupt mask register
	__IO uint32_t IEPEACHMSK1;	//0x840 each IN endpoint-1 interrupt register
	__I  uint32_t RESERVED2[15];//0x844..0x87c
	__IO uint32_t OEPEACHMSK1;	//0x880 each OUT endpoint-1 interrupt register
	__I  uint32_t RESERVED3[31];//0x884..0x8fc
	USB_HS_DEVICE_ENDPOINT_TypeDef INEP[16];//0x900.. 0xafc IN EP configs
	USB_HS_DEVICE_ENDPOINT_TypeDef OUTEP[16];//0xB00.. 0xcfc OUT EP configs
} USB_HS_DEVICE_TypeDef;

typedef struct {
	__IO uint32_t GCCTL;			//0xE00 power and clock gating control register
} USB_HS_PC_TypeDef;

#define USB_HS_BASE				(AHB1PERIPH_BASE + 0x00020000)
#define USB_HS_GENERAL_BASE	(USB_HS_BASE + 0x0000)
#define USB_HS_HOST_BASE		(USB_HS_BASE + 0x0400)
#define USB_HS_DEVICE_BASE		(USB_HS_BASE + 0x0800)
#define USB_HS_PC_BASE			(USB_HS_BASE + 0x0e00)

#define USB_HS_GENERAL        ((USB_HS_GENERAL_TypeDef *) USB_HS_GENERAL_BASE)
#define USB_HS_HOST	         ((USB_HS_HOST_TypeDef *) USB_HS_HOST_BASE)
#define USB_HS_DEVICE	      ((USB_HS_DEVICE_TypeDef *) USB_HS_DEVICE_BASE)
#define USB_HS_PC			      ((USB_HS_PC_TypeDef *) USB_HS_PC_BASE)

/******************************************************************************/
/*                         Peripheral Registers_Bits_Definition               */
/******************************************************************************/

/******************************************************************************/
/*                                                                            */
/*                            USB High Speed GENERAL                          */
/*                                                                            */
/******************************************************************************/
/**********  Bit definition for USB_HS_GENERAL_OTGCTL register  ***************/
#define  USB_HS_GENERAL_OTGCTL_BSVLD						(uint32_t)(1ul << 19ul) //B-session valid
#define  USB_HS_GENERAL_OTGCTL_ASVLD						(uint32_t)(1ul << 18ul) //A-session valid
#define  USB_HS_GENERAL_OTGCTL_DBCT							(uint32_t)(1ul << 17ul) //Long/short debounce time
#define  USB_HS_GENERAL_OTGCTL_CITST						(uint32_t)(1ul << 16ul) //Connector ID status
#define  USB_HS_GENERAL_OTGCTL_DHNPEN						(uint32_t)(1ul << 11ul) //Device HNP enabled
#define  USB_HS_GENERAL_OTGCTL_HSHNPEN						(uint32_t)(1ul << 10ul) //Host set HNP enabled
#define  USB_HS_GENERAL_OTGCTL_HNPRQ						(uint32_t)(1ul << 9ul)  //HNP request
#define  USB_HS_GENERAL_OTGCTL_HNGSCS						(uint32_t)(1ul << 8ul)  //Host negotiations success
#define  USB_HS_GENERAL_OTGCTL_SRQ							(uint32_t)(1ul << 1ul)  //Session request
#define  USB_HS_GENERAL_OTGCTL_SRQSCS						(uint32_t)(1ul << 0ul)  //Session request success

/**********  Bit definition for USB_HS_GENERAL_OTGINT register  ***************/
#define  USB_HS_GENERAL_OTGINT_DBCDNE						(uint32_t)(1ul << 19ul) //Debounce done
#define  USB_HS_GENERAL_OTGINT_ADTOCHG						(uint32_t)(1ul << 18ul) //A-device timeout change
#define  USB_HS_GENERAL_OTGINT_HNGDET						(uint32_t)(1ul << 17ul) //Host negotiation detected
#define  USB_HS_GENERAL_OTGINT_HNSSCHG						(uint32_t)(1ul << 9ul)  //Host negotiation success status change
#define  USB_HS_GENERAL_OTGINT_SRSSCHG						(uint32_t)(1ul << 8ul)  //Session request success status change
#define  USB_HS_GENERAL_OTGINT_SEDET						(uint32_t)(1ul << 2ul)  //Session end detected

/**********  Bit definition for USB_HS_GENERAL_AHBCFG register  ***************/
#define  USB_HS_GENERAL_AHBCFG_PTXFELVL					(uint32_t)(1ul << 8ul)  //Periodic TxFIFO empty level
#define  USB_HS_GENERAL_AHBCFG_TXFELVL						(uint32_t)(1ul << 7ul)  //TxFIFO empty level
#define  USB_HS_GENERAL_AHBCFG_DMAEN						(uint32_t)(1ul << 5ul)  //DMA enable
#define  USB_HS_GENERAL_AHBCFG_HBSTLEN						(uint32_t)(0xful << 1ul)//Burst length/type
#define  USB_HS_GENERAL_AHBCFG_GINT							(uint32_t)(1ul << 0ul)  //Global interrupt mask

#define  USB_HS_GENERAL_AHBCFG_HBSTLEN_SINGLE			(uint32_t)(0x0ul << 1ul)
#define  USB_HS_GENERAL_AHBCFG_HBSTLEN_INCR				(uint32_t)(0x1ul << 1ul)
#define  USB_HS_GENERAL_AHBCFG_HBSTLEN_INCR4				(uint32_t)(0x3ul << 1ul)
#define  USB_HS_GENERAL_AHBCFG_HBSTLEN_INCR8				(uint32_t)(0x5ul << 1ul)
#define  USB_HS_GENERAL_AHBCFG_HBSTLEN_INCR16			(uint32_t)(0x7ul << 1ul)

/**********  Bit definition for USB_HS_GENERAL_USBCFG register  ***************/
#define  USB_HS_GENERAL_USBCFG_CTXPKT						(uint32_t)(1ul << 31ul) //Corrupt Tx packet
#define  USB_HS_GENERAL_USBCFG_FDMOD						(uint32_t)(1ul << 30ul) //Forced peripheral mode
#define  USB_HS_GENERAL_USBCFG_FHMOD						(uint32_t)(1ul << 29ul) //Forced host mode
#define  USB_HS_GENERAL_USBCFG_ULPIPD						(uint32_t)(1ul << 25ul) //ULPI interface protect disable
#define  USB_HS_GENERAL_USBCFG_PTCI							(uint32_t)(1ul << 24ul) //Indicator passthru
#define  USB_HS_GENERAL_USBCFG_PCCI							(uint32_t)(1ul << 23ul) //Indicator complement
#define  USB_HS_GENERAL_USBCFG_TSDPS						(uint32_t)(1ul << 22ul) //TermSel DLine pulsing selection
#define  USB_HS_GENERAL_USBCFG_ULPIEVBUSI					(uint32_t)(1ul << 21ul) //ULPI external VBus indicator
#define  USB_HS_GENERAL_USBCFG_ULPIEVBUSD					(uint32_t)(1ul << 20ul) //ULPI external VBus drive
#define  USB_HS_GENERAL_USBCFG_ULPICSM						(uint32_t)(1ul << 19ul) //ULPI Clock SuspendM
#define  USB_HS_GENERAL_USBCFG_ULPIAR						(uint32_t)(1ul << 18ul) //ULPI Auto-resume
#define  USB_HS_GENERAL_USBCFG_ULPIFSLS					(uint32_t)(1ul << 17ul) //ULPI FS/LS select
#define  USB_HS_GENERAL_USBCFG_PHYLPCS						(uint32_t)(1ul << 15ul) //PHY low power clock select
#define  USB_HS_GENERAL_USBCFG_TRDT							(uint32_t)(0xful << 10ul)//USB turn around time
#define  USB_HS_GENERAL_USBCFG_HNPCAP						(uint32_t)(1ul << 9ul)  //HNP-capable
#define  USB_HS_GENERAL_USBCFG_SRPCAP						(uint32_t)(1ul << 8ul)  //SRP-capable
#define  USB_HS_GENERAL_USBCFG_PHYSEL						(uint32_t)(1ul << 6ul)  //USB 2.0 high-speed ULPI PHY or USB 1.1 full speed serial tranceiver
#define  USB_HS_GENERAL_USBCFG_TOCAL						(uint32_t)(7ul << 0ul)  //FS timeout calibration

#define  USB_HS_GENERAL_USBCFG_TRDT_POS					(uint32_t)(10ul)
#define  USB_HS_GENERAL_USBCFG_TOCAL_POS		\			(uint32_t)(0ul)

/**********  Bit definition for USB_HS_GENERAL_RSTCTL register  ***************/
#define  USB_HS_GENERAL_RSTCTL_AHBIDL						(uint32_t)(1ul << 31ul)  //AHB master idle
#define  USB_HS_GENERAL_RSTCTL_DMAREQ						(uint32_t)(1ul << 30ul)  //DMA request
#define  USB_HS_GENERAL_RSTCTL_TXFNUM						(uint32_t)(0x1ful << 6ul)//TxFIFO number
#define  USB_HS_GENERAL_RSTCTL_TXFFLSH						(uint32_t)(1ul << 5ul)   //TxFIFO flush
#define  USB_HS_GENERAL_RSTCTL_RXFFLSH						(uint32_t)(1ul << 4ul)   //RxFIFO flush
#define  USB_HS_GENERAL_RSTCTL_FCRST						(uint32_t)(1ul << 2ul)   //Host frame counter reset
#define  USB_HS_GENERAL_RSTCTL_HSRST						(uint32_t)(1ul << 1ul)   //HCLK soft reset
#define  USB_HS_GENERAL_RSTCTL_CSRST						(uint32_t)(1ul << 0ul)   //CORE soft reset

#define  USB_HS_GENERAL_RSTCTL_TXFNUM_POS					(uint32_t)(6ul)

/**********  Bit definition for USB_HS_GENERAL_INTSTS register  ***************/
#define  USB_HS_GENERAL_INTSTS_WKUPINT						(uint32_t)(1ul << 31ul)  //Resume/remote wakeup detected interrupt
#define  USB_HS_GENERAL_INTSTS_SRQINT						(uint32_t)(1ul << 30ul)  //Session request/new session detected interrupt
#define  USB_HS_GENERAL_INTSTS_DISCINT						(uint32_t)(1ul << 29ul)  //Disconnect detected interrupt
#define  USB_HS_GENERAL_INTSTS_CIDSCHG						(uint32_t)(1ul << 28ul)  //Connector ID status change
#define  USB_HS_GENERAL_INTSTS_PTXFE						(uint32_t)(1ul << 26ul)  //Periodic TxFIFO empty
#define  USB_HS_GENERAL_INTSTS_HCINT						(uint32_t)(1ul << 25ul)  //Host channel interrupt
#define  USB_HS_GENERAL_INTSTS_HPRTINT						(uint32_t)(1ul << 24ul)  //Host port interrupt
#define  USB_HS_GENERAL_INTSTS_DATAFSUSP					(uint32_t)(1ul << 22ul)  //Data fetch suspended
#define  USB_HS_GENERAL_INTSTS_IPXFR						(uint32_t)(1ul << 21ul)  //Host: incomplete periodic transfer
#define  USB_HS_GENERAL_INTSTS_INCOMPISOOUT				(uint32_t)(1ul << 21ul)  //Device: incomplete isochronous OUT transfer
#define  USB_HS_GENERAL_INTSTS_ISOIXFR						(uint32_t)(1ul << 20ul)  //Incomplete isochronous transfer
#define  USB_HS_GENERAL_INTSTS_OEPINT						(uint32_t)(1ul << 19ul)  //OUT endpoint interrupt
#define  USB_HS_GENERAL_INTSTS_IEPINT						(uint32_t)(1ul << 18ul)  //IN endpoint interrupt
#define  USB_HS_GENERAL_INTSTS_EOPF							(uint32_t)(1ul << 15ul)  //End of periodic frame interrupt
#define  USB_HS_GENERAL_INTSTS_ISOODRP						(uint32_t)(1ul << 14ul)  //Isochronous OUT packet dropped interrupt
#define  USB_HS_GENERAL_INTSTS_ENUMDNE						(uint32_t)(1ul << 13ul)  //Enumeration done interrupt
#define  USB_HS_GENERAL_INTSTS_USBRST						(uint32_t)(1ul << 12ul)  //USB reset
#define  USB_HS_GENERAL_INTSTS_USBSUSP						(uint32_t)(1ul << 11ul)  //USB suspend
#define  USB_HS_GENERAL_INTSTS_ESUSP						(uint32_t)(1ul << 10ul)  //Early suspend
#define  USB_HS_GENERAL_INTSTS_GONAKEFF					(uint32_t)(1ul << 7ul)   //Global OUT NAK effective
#define  USB_HS_GENERAL_INTSTS_GINAKEFF					(uint32_t)(1ul << 6ul)   //Global OUT nonperiodic NAK effective
#define  USB_HS_GENERAL_INTSTS_NPTXFE						(uint32_t)(1ul << 5ul)   //Nonperiodic TxFIFO empty
#define  USB_HS_GENERAL_INTSTS_RXFLVL						(uint32_t)(1ul << 4ul)   //RxFIFO nonempty
#define  USB_HS_GENERAL_INTSTS_SOF							(uint32_t)(1ul << 3ul)   //Start of frame
#define  USB_HS_GENERAL_INTSTS_OTGINT						(uint32_t)(1ul << 2ul)   //OTG interrupt
#define  USB_HS_GENERAL_INTSTS_MMIS							(uint32_t)(1ul << 1ul)   //Mode mismatch interrupt
#define  USB_HS_GENERAL_INTSTS_CMOD							(uint32_t)(1ul << 0ul)   //Current mode operation

/**********  Bit definition for USB_HS_GENERAL_INTMSK register  ***************/
#define  USB_HS_GENERAL_INTMSK_WKUPM						(uint32_t)(1ul << 31ul)  //Resume/remote wakeup detected interrupt mask
#define  USB_HS_GENERAL_INTMSK_SRQM							(uint32_t)(1ul << 30ul)  //Session request/new session detected interrupt mask
#define  USB_HS_GENERAL_INTMSK_DISCM						(uint32_t)(1ul << 29ul)  //Disconnect detected interrupt mask
#define  USB_HS_GENERAL_INTMSK_CIDSCHGM					(uint32_t)(1ul << 28ul)  //Connector ID status change mask
#define  USB_HS_GENERAL_INTMSK_PTXFEM						(uint32_t)(1ul << 26ul)  //Periodic TxFIFO empty mask
#define  USB_HS_GENERAL_INTMSK_HCM							(uint32_t)(1ul << 25ul)  //Host channel interrupt mask
#define  USB_HS_GENERAL_INTMSK_HPRTM						(uint32_t)(1ul << 24ul)  //Host port interrupt mask
#define  USB_HS_GENERAL_INTMSK_DATAFSUSPM					(uint32_t)(1ul << 22ul)  //Data fetch suspended mask
#define  USB_HS_GENERAL_INTMSK_IPXFRM						(uint32_t)(1ul << 21ul)  //Host: incomplete periodic transfer mask
#define  USB_HS_GENERAL_INTMSK_INCOMPISOOUTM				(uint32_t)(1ul << 21ul)  //Device: incomplete isochronous OUT transfer mask
#define  USB_HS_GENERAL_INTMSK_ISOIXFRM					(uint32_t)(1ul << 20ul)  //Incomplete isochronous transfer mask
#define  USB_HS_GENERAL_INTMSK_OEPM							(uint32_t)(1ul << 19ul)  //OUT endpoint interrupt mask
#define  USB_HS_GENERAL_INTMSK_IEPM							(uint32_t)(1ul << 18ul)  //IN endpoint interrupt mask
#define  USB_HS_GENERAL_INTMSK_EPMISM						(uint32_t)(1ul << 17ul)  //Endpoint mismatch interrupt mask
#define  USB_HS_GENERAL_INTMSK_EOPFM						(uint32_t)(1ul << 15ul)  //End of periodic frame interrupt mask
#define  USB_HS_GENERAL_INTMSK_ISOODRPM					(uint32_t)(1ul << 14ul)  //Isochronous OUT packet dropped interrupt mask
#define  USB_HS_GENERAL_INTMSK_ENUMDNEM					(uint32_t)(1ul << 13ul)  //Enumeration done interrupt mask
#define  USB_HS_GENERAL_INTMSK_USBRSTM						(uint32_t)(1ul << 12ul)  //USB reset mask
#define  USB_HS_GENERAL_INTMSK_USBSUSPM					(uint32_t)(1ul << 11ul)  //USB suspend mask
#define  USB_HS_GENERAL_INTMSK_ESUSPM						(uint32_t)(1ul << 10ul)  //Early suspend mask
#define  USB_HS_GENERAL_INTMSK_GONAKEFFM					(uint32_t)(1ul << 7ul)   //Global OUT NAK effective mask
#define  USB_HS_GENERAL_INTMSK_GINAKEFFM					(uint32_t)(1ul << 6ul)   //Global OUT nonperiodic NAK effective mask
#define  USB_HS_GENERAL_INTMSK_NPTXFEM						(uint32_t)(1ul << 5ul)   //Nonperiodic TxFIFO empty mask
#define  USB_HS_GENERAL_INTMSK_RXFLVLM						(uint32_t)(1ul << 4ul)   //RxFIFO nonempty mask
#define  USB_HS_GENERAL_INTMSK_SOFM							(uint32_t)(1ul << 3ul)   //Start of frame mask
#define  USB_HS_GENERAL_INTMSK_OTGM							(uint32_t)(1ul << 2ul)   //OTG interrupt mask
#define  USB_HS_GENERAL_INTMSK_MMISM						(uint32_t)(1ul << 1ul)   //Mode mismatch interrupt mask

/**********  Bit definition for USB_HS_GENERAL_RXSTSR register  ***************/
#define  USB_HS_GENERAL_RXSTSR_FRMNUM						(uint32_t)(0xful << 21ul)  //Device: frame number
#define  USB_HS_GENERAL_RXSTSR_PKTSTS						(uint32_t)(0xful << 17ul)  //Packet status
#define  USB_HS_GENERAL_RXSTSR_DPID							(uint32_t)(0x3ul << 15ul)  //Data PID
#define  USB_HS_GENERAL_RXSTSR_BCNT							(uint32_t)(0x7fful << 4ul) //Bytes count
#define  USB_HS_GENERAL_RXSTSR_CHNUM						(uint32_t)(0xful << 0ul)   //Host: Channel number
#define  USB_HS_GENERAL_RXSTSR_EPNUM						(uint32_t)(0xful << 0ul)   //Device: EP number

#define  USB_HS_GENERAL_RXSTSR_FRMNUM_POS					(uint32_t)(21ul)

#define  USB_HS_GENERAL_RXSTSR_PKTSTS_OUT_NAK			(uint32_t)(1ul << 17ul)    //Device: global OUT NAK
#define  USB_HS_GENERAL_RXSTSR_PKTSTS_IN_RX				(uint32_t)(2ul << 17ul)    //Host: IN data packet received
#define  USB_HS_GENERAL_RXSTSR_PKTSTS_OUT_RX				(uint32_t)(2ul << 17ul)    //Device: OUT data packet received
#define  USB_HS_GENERAL_RXSTSR_PKTSTS_IN_DONE			(uint32_t)(3ul << 17ul)    //Host: IN transfer complete
#define  USB_HS_GENERAL_RXSTSR_PKTSTS_OUT_DONE			(uint32_t)(3ul << 17ul)    //Device: OUT transfer complete
#define  USB_HS_GENERAL_RXSTSR_PKTSTS_DATA_ERROR		(uint32_t)(5ul << 17ul)    //Host: Data toggle error
#define  USB_HS_GENERAL_RXSTSR_PKTSTS_SETUP_DONE		(uint32_t)(5ul << 17ul)    //Device: SETUP transfer complete
#define  USB_HS_GENERAL_RXSTSR_PKTSTS_SETUP_RX			(uint32_t)(6ul << 17ul)    //Device: SETUP received
#define  USB_HS_GENERAL_RXSTSR_PKTSTS_HALTED				(uint32_t)(7ul << 17ul)    //Host: Channel halted

#define  USB_HS_GENERAL_RXSTSR_DPID_DATA0					(uint32_t)(0ul << 15ul)    //DATA0
#define  USB_HS_GENERAL_RXSTSR_DPID_DATA1					(uint32_t)(2ul << 15ul)    //DATA1
#define  USB_HS_GENERAL_RXSTSR_DPID_DATA2					(uint32_t)(1ul << 15ul)    //DATA2
#define  USB_HS_GENERAL_RXSTSR_DPID_MDATA					(uint32_t)(3ul << 15ul)    //MDATA

#define  USB_HS_GENERAL_RXSTSR_BCNT_POS					(uint32_t)(4ul)
#define  USB_HS_GENERAL_RXSTSR_CHNUM_POS					(uint32_t)(0ul)
#define  USB_HS_GENERAL_RXSTSR_EPNUM_POS					(uint32_t)(0ul)

/**********  Bit definition for USB_HS_GENERAL_RXSTSP register  ***************/
#define  USB_HS_GENERAL_RXSTSP_PKTSTS						(uint32_t)(0xful << 17ul)  //Packet status
#define  USB_HS_GENERAL_RXSTSP_DPID							(uint32_t)(0x3ul << 15ul)  //Data PID
#define  USB_HS_GENERAL_RXSTSP_BCNT							(uint32_t)(0x7fful << 4ul) //Bytes count
#define  USB_HS_GENERAL_RXSTSP_CHNUM						(uint32_t)(0xful << 0ul)   //Host: Channel number
#define  USB_HS_GENERAL_RXSTSP_EPNUM						(uint32_t)(0xful << 0ul)   //Device: EP number

#define  USB_HS_GENERAL_RXSTSP_PKTSTS_OUT_NAK			(uint32_t)(1ul << 17ul)    //Device: global OUT NAK
#define  USB_HS_GENERAL_RXSTSP_PKTSTS_IN_RX				(uint32_t)(2ul << 17ul)    //Host: IN data packet received
#define  USB_HS_GENERAL_RXSTSP_PKTSTS_OUT_RX				(uint32_t)(2ul << 17ul)    //Device: OUT data packet received
#define  USB_HS_GENERAL_RXSTSP_PKTSTS_IN_DONE			(uint32_t)(3ul << 17ul)    //Host: IN transfer complete
#define  USB_HS_GENERAL_RXSTSP_PKTSTS_OUT_DONE			(uint32_t)(3ul << 17ul)    //Device: OUT transfer complete
#define  USB_HS_GENERAL_RXSTSP_PKTSTS_DATA_ERROR		(uint32_t)(5ul << 17ul)    //Host: Data toggle error
#define  USB_HS_GENERAL_RXSTSP_PKTSTS_SETUP_DONE		(uint32_t)(5ul << 17ul)    //Device: SETUP transfer complete
#define  USB_HS_GENERAL_RXSTSP_PKTSTS_SETUP_RX			(uint32_t)(6ul << 17ul)    //Device: SETUP received
#define  USB_HS_GENERAL_RXSTSP_PKTSTS_HALTED				(uint32_t)(7ul << 17ul)    //Host: Channel halted


#define  USB_HS_GENERAL_RXSTSP_DPID_DATA0					(uint32_t)(0ul << 15ul)    //DATA0
#define  USB_HS_GENERAL_RXSTSP_DPID_DATA1					(uint32_t)(2ul << 15ul)    //DATA1
#define  USB_HS_GENERAL_RXSTSP_DPID_DATA2					(uint32_t)(1ul << 15ul)    //DATA2
#define  USB_HS_GENERAL_RXSTSP_DPID_MDATA					(uint32_t)(3ul << 15ul)    //MDATA

#define  USB_HS_GENERAL_RXSTSP_BCNT_POS					(uint32_t)(4ul)
#define  USB_HS_GENERAL_RXSTSP_CHNUM_POS					(uint32_t)(0ul)
#define  USB_HS_GENERAL_RXSTSP_EPNUM_POS					(uint32_t)(0ul)

/**********  Bit definition for USB_HS_GENERAL_RXFSIZ register  ***************/
#define  USB_HS_GENERAL_RXFSIZ_RXFD							(uint32_t)(0xfffful << 0ul)  //RxFIFO depth

#define  USB_HS_GENERAL_RXFSIZ_POS							(uint32_t)(0ul)

/**********  Bit definition for USB_HS_GENERAL_NPTXFSIZ register  ***************/
#define  USB_HS_GENERAL_NPTXFSIZ_NPTXFD					(uint32_t)(0xfffful << 16ul)  //Nonperiodic TxFIFO depth
#define  USB_HS_GENERAL_NPTXFSIZ_NPTXFSA					(uint32_t)(0xfffful << 0ul)   //Nonperiodic trasmit RAM start address

#define  USB_HS_GENERAL_NPTXFSIZ_NPTXFD_POS				(uint32_t)(16ul)
#define  USB_HS_GENERAL_NPTXFSIZ_NPTXFSA_POS				(uint32_t)(0ul)

/**********  Bit definition for USB_HS_GENERAL_TX0FSIZ register  ***************/
#define  USB_HS_GENERAL_TX0FSIZ_TX0FD						(uint32_t)(0xfffful << 16ul)  //Endpoint 0 TxFIFO depth
#define  USB_HS_GENERAL_TX0FSIZ_TX0FSA						(uint32_t)(0xfffful << 0ul)   //Endpoint 0 RAM start address

#define  USB_HS_GENERAL_TX0FSIZ_TX0FD_POS					(uint32_t)(16ul)
#define  USB_HS_GENERAL_TX0FSIZ_TX0FSA_POS				(uint32_t)(0ul)

/**********  Bit definition for USB_HS_GENERAL_NPTXSTS register  ***************/
#define  USB_HS_GENERAL_NPTXSTS_NPTXQTOP					(uint32_t)(0x7ful << 24ul)   //Top of nonperiodic transmit request queue
#define  USB_HS_GENERAL_NPTXSTS_NPTQXSAV					(uint32_t)(0xfful << 16ul)   //Nonperiodic transmit request queue space available
#define  USB_HS_GENERAL_NPTXSTS_NPTXFSAV					(uint32_t)(0xfffful << 0ul)  //Nonperiodic TxFIFO space available

#define  USB_HS_GENERAL_NPTXSTS_NPTXQTOP_POS				(uint32_t)(24ul)
#define  USB_HS_GENERAL_NPTXSTS_NPTQXSAV_POS				(uint32_t)(16ul)
#define  USB_HS_GENERAL_NPTXSTS_NPTXFSAV_POS				(uint32_t)(0ul)

/**********  Bit definition for USB_HS_GENERAL_I2CCTL register  ***************/
#define  USB_HS_GENERAL_I2CCTL_BSYDNE						(uint32_t)(1ul << 31ul)     //I2C busy/done
#define  USB_HS_GENERAL_I2CCTL_RW							(uint32_t)(1ul << 30ul)     //Read/write indicator
#define  USB_HS_GENERAL_I2CCTL_I2CDATSE0					(uint32_t)(1ul << 28ul)     //I2C DatSe0 USB mode
#define  USB_HS_GENERAL_I2CCTL_I2CDEVADR					(uint32_t)(3ul << 26ul)     //I2C device address
#define  USB_HS_GENERAL_I2CCTL_ACK							(uint32_t)(1ul << 24ul)     //I2C ACK
#define  USB_HS_GENERAL_I2CCTL_I2CEN						(uint32_t)(1ul << 23ul)     //I2C enable
#define  USB_HS_GENERAL_I2CCTL_ADDR							(uint32_t)(0x7ful << 16ul)  //I2C address
#define  USB_HS_GENERAL_I2CCTL_REGADDR						(uint32_t)(0xfffful << 8ul) //I2C register address
#define  USB_HS_GENERAL_I2CCTL_RWDATA						(uint32_t)(0xfffful << 0ul) //I2C Read/Write data

#define  USB_HS_GENERAL_I2CCTL_I2CDEVADR_POS				(uint32_t)(26ul)
#define  USB_HS_GENERAL_I2CCTL_ADDR_POS					(uint32_t)(16ul)
#define  USB_HS_GENERAL_I2CCTL_REGADDR_POS				(uint32_t)(8ul)
#define  USB_HS_GENERAL_I2CCTL_RWDATA_POS					(uint32_t)(0ul)

/**********  Bit definition for USB_HS_GENERAL_CCFG register  ***************/
#define  USB_HS_GENERAL_CCFG_NOVBUSSENS					(uint32_t)(1ul << 21ul)     //VBus sensing disable option
#define  USB_HS_GENERAL_CCFG_SOFOUTEN						(uint32_t)(1ul << 20ul)     //SOF output enable
#define  USB_HS_GENERAL_CCFG_VBUSBSEN						(uint32_t)(1ul << 19ul)     //Enable VBus sensing "B" device
#define  USB_HS_GENERAL_CCFG_VBUSASEN						(uint32_t)(1ul << 18ul)     //Enable VBus sensing "A" device
#define  USB_HS_GENERAL_CCFG_I2CPADEN						(uint32_t)(1ul << 17ul)     //Enable I2C bus connection for the external I2C PHY interface
#define  USB_HS_GENERAL_CCFG_PWRDWN							(uint32_t)(1ul << 16ul)     //Power down

/*************  Bit definition for USB_HS_GENERAL_CID register  ***************/
#define  USB_HS_GENERAL_CID_PRODUCT_ID						(uint32_t)(0xfffffffful << 0ul)//Product ID

#define  USB_HS_GENERAL_CID_PRODUCT_ID_POS				(uint32_t)(0ul)

/**********  Bit definition for USB_HS_GENERAL_HPTXFSIZ register  ************/
#define  USB_HS_GENERAL_HPTXFSIZ_PTXFD						(uint32_t)(0xfffful << 16ul)//Host periodic TxFIFO depth
#define  USB_HS_GENERAL_HPTXFSIZ_PTXFSA					(uint32_t)(0xfffful << 0ul) //Host periodic TxFIFO start address

#define  USB_HS_GENERAL_HPTXFSIZ_PTXFD_POS				(uint32_t)(16ul)
#define  USB_HS_GENERAL_HPTXFSIZ_PTXFSA_POS				(uint32_t)(0ul)

/**********  Bit definition for USB_HS_GENERAL_DIEPTXF register  ************/
#define  USB_HS_GENERAL_DIEPTXF_INEPTXFD					(uint32_t)(0xfffful << 16ul)//IN endpoint TxFIFO depth
#define  USB_HS_GENERAL_DIEPTXF_INEPTXFSA					(uint32_t)(0xfffful << 0ul) //IN endpoint TxFIFO start address

#define  USB_HS_GENERAL_DIEPTXF_INEPTXFD_POS				(uint32_t)(16ul)
#define  USB_HS_GENERAL_DIEPTXF_INEPTXFSA_POS			(uint32_t)(0ul)

/******************************************************************************/
/*                                                                            */
/*                            USB High Speed HOST                             */
/*                                                                            */
/******************************************************************************/
/**************  Bit definition for USB_HS_HOST_CFG register  *****************/
#define  USB_HS_HOST_CFG_FSLSS								(uint32_t)(1ul << 2ul)	    //FS- and LS-only support
#define  USB_HS_HOST_CFG_FSLSSPCS							(uint32_t)(3ul << 0ul)	    //FS/LS PHY clock select

#define  USB_HS_HOST_CFG_FSLSSPCS_48MHZ					(uint32_t)(1ul << 0ul)	    //PHY clock is running at 48 MHz
#define  USB_HS_HOST_CFG_FSLSSPCS_6MHZ						(uint32_t)(2ul << 0ul)	    //LS 6 MHz PHY clock frequency

/**************  Bit definition for USB_HS_HOST_FIR register  *****************/
#define  USB_HS_HOST_FIR_FRIVL								(uint32_t)(0xfffful << 0ul) //Frame interval

#define  USB_HS_HOST_FIR_FRIVL_POS							(uint32_t)(0ul)

/**************  Bit definition for USB_HS_HOST_FNUM register  *****************/
#define  USB_HS_HOST_FNUM_FTREM								(uint32_t)(0xfffful << 16ul)//Frame time remaining
#define  USB_HS_HOST_FNUM_FRNUM								(uint32_t)(0xfffful << 0ul) //Frame number

#define  USB_HS_HOST_FNUM_FTREM_POS							(uint32_t)(16ul)
#define  USB_HS_HOST_FNUM_FRNUM_POS							(uint32_t)(0ul)

/**************  Bit definition for USB_HS_HOST_PTXSTS register  ***************/
#define  USB_HS_HOST_PTXSTS_PTXQTOP							(uint32_t)(0xfful << 24ul) //Top of the periodic transmit request queue
#define  USB_HS_HOST_PTXSTS_PTXQSAV							(uint32_t)(0xfful << 16ul) //Periodic transmit request queue space available
#define  USB_HS_HOST_PTXSTS_PTXFSAVL						(uint32_t)(0xfffful << 0ul)//Periodic transmit data FIFO space available

#define  USB_HS_HOST_PTXSTS_PTXQTOP_ODD					(uint32_t)(0x1ul << 31ul)   //Odd/Even frame
#define  USB_HS_HOST_PTXSTS_PTXQTOP_CHANNEL				(uint32_t)(0xful << 27ul)   //Channel number
#define  USB_HS_HOST_PTXSTS_PTXQTOP_TYPE					(uint32_t)(0x3ul << 25ul)   //Type
#define  USB_HS_HOST_PTXSTS_PTXQTOP_TERMINATE			(uint32_t)(0x1ul << 24ul)   //Terminate (last entry for the selected channel/endpoint)

#define  USB_HS_HOST_PTXSTS_PTXQTOP_CHANNEL_POS			(uint32_t)(27ul)

#define  USB_HS_HOST_PTXSTS_PTXQTOP_TYPE_IN_OUT			(uint32_t)(0x0ul << 25ul)   //IN/OUT
#define  USB_HS_HOST_PTXSTS_PTXQTOP_TYPE_IN_ZERO_LENGTH(uint32_t)(0x1ul << 25ul)   //Zero-length packet
#define  USB_HS_HOST_PTXSTS_PTXQTOP_TYPE_DISABLE		(uint32_t)(0x3ul << 25ul)   //Disable channel command

#define  USB_HS_HOST_PTXSTS_PTXQSAV_POS					(uint32_t)(16ul)
#define  USB_HS_HOST_PTXSTS_PTXFSAVL_POS					(uint32_t)(0ul)

/**************  Bit definition for USB_HS_HOST_AINT register  ****************/
#define  USB_HS_HOST_AINT_HAINT								(uint32_t)(0xfffful << 0ul) //Channel interrupts

#define  USB_HS_HOST_AINT_HAINT_0							(uint32_t)(0x1ul << 0ul)
#define  USB_HS_HOST_AINT_HAINT_1							(uint32_t)(0x1ul << 1ul)
#define  USB_HS_HOST_AINT_HAINT_2							(uint32_t)(0x1ul << 2ul)
#define  USB_HS_HOST_AINT_HAINT_3							(uint32_t)(0x1ul << 3ul)
#define  USB_HS_HOST_AINT_HAINT_4							(uint32_t)(0x1ul << 4ul)
#define  USB_HS_HOST_AINT_HAINT_5							(uint32_t)(0x1ul << 5ul)
#define  USB_HS_HOST_AINT_HAINT_6							(uint32_t)(0x1ul << 6ul)
#define  USB_HS_HOST_AINT_HAINT_7							(uint32_t)(0x1ul << 7ul)
#define  USB_HS_HOST_AINT_HAINT_8							(uint32_t)(0x1ul << 8ul)
#define  USB_HS_HOST_AINT_HAINT_9							(uint32_t)(0x1ul << 9ul)
#define  USB_HS_HOST_AINT_HAINT_10							(uint32_t)(0x1ul << 10ul)
#define  USB_HS_HOST_AINT_HAINT_11							(uint32_t)(0x1ul << 11ul)
#define  USB_HS_HOST_AINT_HAINT_12							(uint32_t)(0x1ul << 12ul)
#define  USB_HS_HOST_AINT_HAINT_13							(uint32_t)(0x1ul << 13ul)
#define  USB_HS_HOST_AINT_HAINT_14							(uint32_t)(0x1ul << 14ul)
#define  USB_HS_HOST_AINT_HAINT_15							(uint32_t)(0x1ul << 15ul)

/************  Bit definition for USB_HS_HOST_AINTMSK register  ***************/
#define  USB_HS_HOST_AINTMSK_HAINTM							(uint32_t)(0xfffful << 0ul) //Channel interrupts mask

#define  USB_HS_HOST_AINTMSK_HAINTM_0						(uint32_t)(0x1ul << 0ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_1			\			(uint32_t)(0x1ul << 1ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_2						(uint32_t)(0x1ul << 2ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_3						(uint32_t)(0x1ul << 3ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_4						(uint32_t)(0x1ul << 4ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_5						(uint32_t)(0x1ul << 5ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_6						(uint32_t)(0x1ul << 6ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_7						(uint32_t)(0x1ul << 7ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_8						(uint32_t)(0x1ul << 8ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_9						(uint32_t)(0x1ul << 9ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_10						(uint32_t)(0x1ul << 10ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_11 					(uint32_t)(0x1ul << 11ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_12						(uint32_t)(0x1ul << 12ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_13						(uint32_t)(0x1ul << 13ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_14						(uint32_t)(0x1ul << 14ul)
#define  USB_HS_HOST_AINTMSK_HAINTM_15						(uint32_t)(0x1ul << 15ul)

/**************  Bit definition for USB_HS_HOST_PRT register  ***************/
#define  USB_HS_HOST_PRT_PSPD									(uint32_t)(0x3ul << 17ul) //Port speed
#define  USB_HS_HOST_PRT_PTCTL								(uint32_t)(0xful << 13ul) //Port test control
#define  USB_HS_HOST_PRT_PPWR									(uint32_t)(0x1ul << 12ul) //Port power
#define  USB_HS_HOST_PRT_PLSTS								(uint32_t)(0x3ul << 10ul) //Port line status
#define  USB_HS_HOST_PRT_PRST									(uint32_t)(0x1ul << 8ul)  //Port reset
#define  USB_HS_HOST_PRT_PSUSP								(uint32_t)(0x1ul << 7ul)  //Port suspend
#define  USB_HS_HOST_PRT_PRES									(uint32_t)(0x1ul << 6ul)  //Port resume
#define  USB_HS_HOST_PRT_POCCHNG								(uint32_t)(0x1ul << 5ul)  //Port overcurrent change
#define  USB_HS_HOST_PRT_POCA									(uint32_t)(0x1ul << 4ul)  //Port overcurrent active
#define  USB_HS_HOST_PRT_PENCHNG								(uint32_t)(0x1ul << 3ul)  //Port enable/disable change
#define  USB_HS_HOST_PRT_PENA									(uint32_t)(0x1ul << 2ul)  //Port enable
#define  USB_HS_HOST_PRT_PCDET								(uint32_t)(0x1ul << 1ul)  //Port connect detected
#define  USB_HS_HOST_PRT_PCSTS								(uint32_t)(0x1ul << 0ul)  //Port connect status

#define  USB_HS_HOST_PRT_PSPD_HS								(uint32_t)(0x0ul << 17ul) //High speed
#define  USB_HS_HOST_PRT_PSPD_FS								(uint32_t)(0x1ul << 17ul) //Fast speed
#define  USB_HS_HOST_PRT_PSPD_LS								(uint32_t)(0x2ul << 17ul) //Low speed

#define  USB_HS_HOST_PRT_PTCTL_DISABLED					(uint32_t)(0x0ul << 13ul) //Test mode disabled
#define  USB_HS_HOST_PRT_PTCTL_J								(uint32_t)(0x1ul << 13ul) //J mode
#define  USB_HS_HOST_PRT_PTCTL_K								(uint32_t)(0x2ul << 13ul) //K mode
#define  USB_HS_HOST_PRT_PTCTL_SE0_NAK						(uint32_t)(0x3ul << 13ul) //Se0 NAK mode
#define  USB_HS_HOST_PRT_PTCTL_PACKET						(uint32_t)(0x4ul << 13ul) //Packet mode
#define  USB_HS_HOST_PRT_PTCTL_FORCE_ENABLE				(uint32_t)(0x5ul << 13ul) //Force enable

#define  USB_HS_HOST_PRT_PLSTS_DP							(uint32_t)(0x1ul << 10ul) //Logic level of OTF_HS_FS_DP
#define  USB_HS_HOST_PRT_PLSTS_DM							(uint32_t)(0x1ul << 11ul) //Logic level of OTF_HS_FS_DM

/**********  Bit definition for USB_HS_HOST_CHANNEL_CHAR register  ***************/
#define  USB_HS_HOST_CHANNEL_CHAR_CHENA					(uint32_t)(0x1ul << 31ul) //Channel enable
#define  USB_HS_HOST_CHANNEL_CHAR_CHDIS					(uint32_t)(0x1ul << 30ul) //Channel disable
#define  USB_HS_HOST_CHANNEL_CHAR_ODDFRM					(uint32_t)(0x1ul << 29ul) //ODD frame
#define  USB_HS_HOST_CHANNEL_CHAR_DAD						(uint32_t)(0x7ful << 22ul)//Device address
#define  USB_HS_HOST_CHANNEL_CHAR_MC						(uint32_t)(0x3ul << 20ul) //Multi Count (MC) / Error Count (EC)
#define  USB_HS_HOST_CHANNEL_CHAR_EPTYP					(uint32_t)(0x2ul << 18ul) //Endpoint type
#define  USB_HS_HOST_CHANNEL_CHAR_LSDEV					(uint32_t)(0x1ul << 17ul) //Low speed device
#define  USB_HS_HOST_CHANNEL_CHAR_EPDIR					(uint32_t)(0x1ul << 15ul) //Endpoint direction
#define  USB_HS_HOST_CHANNEL_CHAR_EPNUM					(uint32_t)(0xful << 11ul) //Endpoint number
#define  USB_HS_HOST_CHANNEL_CHAR_MPSIZ					(uint32_t)(0x7fful << 0ul)//Maximum packet size

#define  USB_HS_HOST_CHANNEL_CHAR_DAD_POS					(uint32_t)(22ul)
#define  USB_HS_HOST_CHANNEL_CHAR_MC_POS					(uint32_t)(20ul)

#define  USB_HS_HOST_CHANNEL_CHAR_EPTYP_CONTROL			(uint32_t)(0x0ul << 18ul) //Control endpoint type
#define  USB_HS_HOST_CHANNEL_CHAR_EPTYP_ISOCHRONOUS	(uint32_t)(0x1ul << 18ul) //Isochronous endpoint type
#define  USB_HS_HOST_CHANNEL_CHAR_EPTYP_BULK				(uint32_t)(0x2ul << 18ul) //Bulk endpoint type
#define  USB_HS_HOST_CHANNEL_CHAR_EPTYP_INTERRUPT		(uint32_t)(0x3ul << 18ul) //Interrupt endpoint type

#define  USB_HS_HOST_CHANNEL_CHAR_EPNUM_POS				(uint32_t)(11ul)
#define  USB_HS_HOST_CHANNEL_CHAR_MPSIZ_POS				(uint32_t)(0ul)

/**********  Bit definition for USB_HS_HOST_CHANNEL_SPLT register  ***************/
#define  USB_HS_HOST_CHANNEL_SPLT_SPLTEN					(uint32_t)(0x1ul << 31ul) //Split enable
#define  USB_HS_HOST_CHANNEL_SPLT_COMPLSPLT				(uint32_t)(0x1ul << 16ul) //Do complete split
#define  USB_HS_HOST_CHANNEL_SPLT_XACTPOS					(uint32_t)(0x3ul << 14ul) //Transaction position
#define  USB_HS_HOST_CHANNEL_SPLT_HUBADDR					(uint32_t)(0x7ful << 7ul) //Hub address
#define  USB_HS_HOST_CHANNEL_SPLT_PORTADDR				(uint32_t)(0x7ful << 0ul) //Port address

#define  USB_HS_HOST_CHANNEL_SPLT_XACTPOS_ALL			(uint32_t)(0x3ul << 14ul) //All. This is the entire data payload of this transaction (which is less than or equal to 188 bytes)
#define  USB_HS_HOST_CHANNEL_SPLT_XACTPOS_BEGIN			(uint32_t)(0x2ul << 14ul) //Begin. This is the first data payload of this transaction (which is larger than 188 bytes)
#define  USB_HS_HOST_CHANNEL_SPLT_XACTPOS_MID			(uint32_t)(0x0ul << 14ul) //Mid. This is the middle payload of this transaction (which is larger than 188 bytes)
#define  USB_HS_HOST_CHANNEL_SPLT_XACTPOS_END			(uint32_t)(0x1ul << 14ul) //End. This is the last payload of this transaction (which is larger than 188 bytes)

#define  USB_HS_HOST_CHANNEL_SPLT_HUBADDR_POS			(uint32_t)(7ul)
#define  USB_HS_HOST_CHANNEL_SPLT_PORTADDR_POS			(uint32_t)(0ul)

/**********  Bit definition for USB_HS_HOST_CHANNEL_INT register  ****************/
#define  USB_HS_HOST_CHANNEL_INT_DTERR						(uint32_t)(0x1ul << 10ul) //Data toggle error
#define  USB_HS_HOST_CHANNEL_INT_FRMOR						(uint32_t)(0x1ul << 9ul)  //Frame overrun
#define  USB_HS_HOST_CHANNEL_INT_BBERR						(uint32_t)(0x1ul << 8ul)  //Babble error
#define  USB_HS_HOST_CHANNEL_INT_TXERR						(uint32_t)(0x1ul << 7ul)  //Transaction error
#define  USB_HS_HOST_CHANNEL_INT_NYET						(uint32_t)(0x1ul << 6ul)  //Responce received interrupt
#define  USB_HS_HOST_CHANNEL_INT_ACK						(uint32_t)(0x1ul << 5ul)  //ACK response received/transmitted interrupt
#define  USB_HS_HOST_CHANNEL_INT_NAK						(uint32_t)(0x1ul << 4ul)  //NAK response received interrupt
#define  USB_HS_HOST_CHANNEL_INT_STALL						(uint32_t)(0x1ul << 3ul)  //STALL response received interrupt
#define  USB_HS_HOST_CHANNEL_INT_AHBERR					(uint32_t)(0x1ul << 2ul)  //AHB error
#define  USB_HS_HOST_CHANNEL_INT_CHH						(uint32_t)(0x1ul << 1ul)  //Channel halted
#define  USB_HS_HOST_CHANNEL_INT_XFRC						(uint32_t)(0x1ul << 0ul)  //Transfer completed

/**********  Bit definition for USB_HS_HOST_CHANNEL_INTMSK register  *************/
#define  USB_HS_HOST_CHANNEL_INTMSK_DTERRM				(uint32_t)(0x1ul << 10ul) //Data toggle error interrupt mask
#define  USB_HS_HOST_CHANNEL_INTMSK_FRMORM				(uint32_t)(0x1ul << 9ul)  //Frame overrun interrupt mask
#define  USB_HS_HOST_CHANNEL_INTMSK_BBERR					(uint32_t)(0x1ul << 8ul)  //Babble error interrupt mask
#define  USB_HS_HOST_CHANNEL_INTMSK_TXERRM				(uint32_t)(0x1ul << 7ul)  //Transaction error interrupt mask
#define  USB_HS_HOST_CHANNEL_INTMSK_NYETM					(uint32_t)(0x1ul << 6ul)  //Responce received interrupt mask
#define  USB_HS_HOST_CHANNEL_INTMSK_ACKM					(uint32_t)(0x1ul << 5ul)  //ACK response received/transmitted interrupt mask
#define  USB_HS_HOST_CHANNEL_INTMSK_NAKM					(uint32_t)(0x1ul << 4ul)  //NAK response received interrupt mask
#define  USB_HS_HOST_CHANNEL_INTMSK_STALLM				(uint32_t)(0x1ul << 3ul)  //STALL response received interrupt mask
#define  USB_HS_HOST_CHANNEL_INTMSK_AHBERRM				(uint32_t)(0x1ul << 2ul)  //AHB error interrupt mask
#define  USB_HS_HOST_CHANNEL_INTMSK_CHHM					(uint32_t)(0x1ul << 1ul)  //Channel halted interrupt mask
#define  USB_HS_HOST_CHANNEL_INTMSK_XFRCM					(uint32_t)(0x1ul << 0ul)  //Transfer completed interrupt mask

/**********  Bit definition for USB_HS_HOST_CHANNEL_TSIZ register  ***************/
#define  USB_HS_HOST_CHANNEL_TSIZ_DOPING					(uint32_t)(0x1ul << 31ul) //Do ping
#define  USB_HS_HOST_CHANNEL_TSIZ_DPID						(uint32_t)(0x3ul << 29ul) //Data PID
#define  USB_HS_HOST_CHANNEL_TSIZ_PKTCNT					(uint32_t)(0x3fful << 19ul)//Packet count
#define  USB_HS_HOST_CHANNEL_TSIZ_XFRSIZ					(uint32_t)(0x7fffful << 0ul)//Transfer size

#define  USB_HS_HOST_CHANNEL_TSIZ_DPID_DATA0				(uint32_t)(0x0ul << 29ul) //DATA0
#define  USB_HS_HOST_CHANNEL_TSIZ_DPID_DATA2				(uint32_t)(0x1ul << 29ul) //DATA2
#define  USB_HS_HOST_CHANNEL_TSIZ_DPID_DATA1				(uint32_t)(0x2ul << 29ul) //DATA1
#define  USB_HS_HOST_CHANNEL_TSIZ_DPID_MDATA_SETUP		(uint32_t)(0x3ul << 29ul) //MDATA (noncontrol)/SETUP (control)

#define  USB_HS_HOST_CHANNEL_TSIZ_PKTCNT_POS				(uint32_t)(19ul)
#define  USB_HS_HOST_CHANNEL_TSIZ_XFRSIZ_POS				(uint32_t)(0ul)

/**********  Bit definition for USB_HS_HOST_CHANNEL_DMA register  ***************/
#define  USB_HS_HOST_CHANNEL_DMA_DMAADDR					(uint32_t)(0xfffffffful << 0ul)//DMA address

#define  USB_HS_HOST_CHANNEL_DMA_DMAADDR_POS				(uint32_t)(0ul)

/******************************************************************************/
/*                                                                            */
/*                            USB High Speed DEVICE                           */
/*                                                                            */
/******************************************************************************/
/**************  Bit definition for USB_HS_DEVICE_CFG register  ***************/
#define  USB_HS_DEVICE_CFG_PERSCHIVL						(uint32_t)(3ul << 25ul)	  //Periodic scheduling interval
#define  USB_HS_DEVICE_CFG_PFIVL								(uint32_t)(3ul << 11ul)	  //Periodic frame interval
#define  USB_HS_DEVICE_CFG_DAD								(uint32_t)(0x7ful << 4ul)	  //Device address
#define  USB_HS_DEVICE_CFG_NZLSOHSK							(uint32_t)(1ul << 1ul)	  //Non-zero length status OUT handshake
#define  USB_HS_DEVICE_CFG_DSPD								(uint32_t)(3ul << 0ul)	  //Device speed

#define  USB_HS_DEVICE_CFG_PERSCHIVL_25					(uint32_t)(0ul << 25ul)	  //25% of micro(frame)
#define  USB_HS_DEVICE_CFG_PERSCHIVL_50					(uint32_t)(1ul << 25ul)	  //50% of micro(frame)
#define  USB_HS_DEVICE_CFG_PERSCHIVL_75					(uint32_t)(2ul << 25ul)	  //75% of micro(frame)

#define  USB_HS_DEVICE_CFG_PFIVL_80							(uint32_t)(0ul << 11ul)	  //80% of micro(frame)
#define  USB_HS_DEVICE_CFG_PFIVL_85							(uint32_t)(1ul << 11ul)	  //85% of micro(frame)
#define  USB_HS_DEVICE_CFG_PFIVL_90							(uint32_t)(2ul << 11ul)	  //90% of micro(frame)
#define  USB_HS_DEVICE_CFG_PFIVL_95							(uint32_t)(3ul << 11ul)	  //95% of micro(frame)

#define  USB_HS_DEVICE_CFG_DAD_POS							(uint32_t)(4ul)

#define  USB_HS_DEVICE_CFG_DSPD_HS							(uint32_t)(0ul << 0ul)	  //High speed
#define  USB_HS_DEVICE_CFG_DSPD_FS							(uint32_t)(3ul << 0ul)	  //Full speed (USB tranceiver clock is 48MHz)

/**************  Bit definition for USB_HS_DEVICE_CTL register  ***************/
#define  USB_HS_DEVICE_CTL_POPRGDNE							(uint32_t)(1ul << 11ul)	  //Power-on programming done
#define  USB_HS_DEVICE_CTL_CGONAK							(uint32_t)(1ul << 10ul)	  //Clear global OUT NAK
#define  USB_HS_DEVICE_CTL_SGONAK							(uint32_t)(1ul << 9ul)	  //Set global OUT NAK
#define  USB_HS_DEVICE_CTL_CGINAK							(uint32_t)(1ul << 8ul)	  //Clear global IN NAK
#define  USB_HS_DEVICE_CTL_SGINAK							(uint32_t)(1ul << 7ul)	  //Set global IN NAK
#define  USB_HS_DEVICE_CTL_TCTL								(uint32_t)(7ul << 4ul)	  //Test control
#define  USB_HS_DEVICE_CTL_GONSTS							(uint32_t)(1ul << 3ul)	  //Global OUT NAK status
#define  USB_HS_DEVICE_CTL_GINSTS							(uint32_t)(1ul << 2ul)	  //Global IN NAK status
#define  USB_HS_DEVICE_CTL_SDIS								(uint32_t)(1ul << 1ul)	  //Soft disconnect
#define  USB_HS_DEVICE_CTL_RWUSIG							(uint32_t)(1ul << 0ul)	  //Remote wakeup signaling

#define  USB_HS_DEVICE_CTL_TCTL_DISABLED					(uint32_t)(0x0ul << 4ul)  //Test mode disabled
#define  USB_HS_DEVICE_CTL_TCTL_J							(uint32_t)(0x1ul << 4ul)  //J mode
#define  USB_HS_DEVICE_CTL_TCTL_K							(uint32_t)(0x2ul << 4ul)  //K mode
#define  USB_HS_DEVICE_CTL_TCTL_SE0_NAK					(uint32_t)(0x3ul << 4ul)  //Se0 NAK mode
#define  USB_HS_DEVICE_CTL_TCTL_PACKET						(uint32_t)(0x4ul << 4ul)  //Packet mode
#define  USB_HS_DEVICE_CTL_TCTL_FORCE_ENABLE				(uint32_t)(0x5ul << 4ul)  //Force enable

/**************  Bit definition for USB_HS_DEVICE_STS register  ***************/
#define  USB_HS_DEVICE_STS_FNSOF								(uint32_t)(0x3ffful << 8ul)//Frame number of received SOF
#define  USB_HS_DEVICE_STS_EERR								(uint32_t)(1ul << 3ul)		//Erratic error
#define  USB_HS_DEVICE_STS_ENUMSPD							(uint32_t)(3ul << 1ul)		//Enumerated speed
#define  USB_HS_DEVICE_STS_SUSPSTS							(uint32_t)(1ul << 0ul)		//Suspend status

#define  USB_HS_DEVICE_STS_FNSOF_POS						(uint32_t)(8ul)

#define  USB_HS_DEVICE_STS_ENUMSPD_HS						(uint32_t)(0ul << 1ul)		//High speed
#define  USB_HS_DEVICE_STS_ENUMSPD_FS						(uint32_t)(3ul << 1ul)		//Full speed (PHY clock is running on 48MHz)

/************  Bit definition for USB_HS_DEVICE_IEPMSK register  ***************/
#define  USB_HS_DEVICE_IEPMSK_BIM							(uint32_t)(1ul << 9ul)		//BNA interrupt mask
#define  USB_HS_DEVICE_IEPMSK_TXFURM						(uint32_t)(1ul << 8ul)		//Tx FIFO underrun mask
#define  USB_HS_DEVICE_IEPMSK_INEPNEM						(uint32_t)(1ul << 6ul)		//IN endpoint NAK effective mask
#define  USB_HS_DEVICE_IEPMSK_INEPNMM						(uint32_t)(1ul << 5ul)		//IN token receive with EP mismatch mask
#define  USB_HS_DEVICE_IEPMSK_ITTXFEMSK					(uint32_t)(1ul << 4ul)		//IN token receive when TxFIFO empty mask
#define  USB_HS_DEVICE_IEPMSK_TOCM							(uint32_t)(1ul << 3ul)		//Timeout condition mask (nonisochronous endpoint)
#define  USB_HS_DEVICE_IEPMSK_EPDM							(uint32_t)(1ul << 1ul)		//Endpoint disabled interrupt mask
#define  USB_HS_DEVICE_IEPMSK_XFRCM							(uint32_t)(1ul << 0ul)		//Transfer completed interrupt mask

/************  Bit definition for USB_HS_DEVICE_OEPMSK register  ***************/
#define  USB_HS_DEVICE_OEPMSK_BOIM							(uint32_t)(1ul << 9ul)		//BNA interrupt mask
#define  USB_HS_DEVICE_OEPMSK_OPEM							(uint32_t)(1ul << 8ul)		//OUT packet error mask
#define  USB_HS_DEVICE_OEPMSK_B2BSTUP						(uint32_t)(1ul << 6ul)		//Back-to-back SETUP packets received mask
#define  USB_HS_DEVICE_OEPMSK_OTEPDM						(uint32_t)(1ul << 4ul)		//OUT token received when endpoint disabled mask
#define  USB_HS_DEVICE_OEPMSK_STUPM							(uint32_t)(1ul << 3ul)		//SETUP phase done mask
#define  USB_HS_DEVICE_OEPMSK_EPDM							(uint32_t)(1ul << 1ul)		//Endpoint disabled interrupt mask
#define  USB_HS_DEVICE_OEPMSK_XFRCM							(uint32_t)(1ul << 0ul)		//Transfer completed interrupt mask

/************  Bit definition for USB_HS_DEVICE_AINT register  *****************/
#define  USB_HS_DEVICE_AINT_OEPINT							(uint32_t)(0xfffful << 16ul)//OUT endpoint interrupt bits
#define  USB_HS_DEVICE_AINT_IEPINT							(uint32_t)(0xfffful << 0ul) //IN endpoint interrupt bits

#define  USB_HS_DEVICE_AINT_OEPINT_POS						(uint32_t)(16ul)
#define  USB_HS_DEVICE_AINT_IEPINT_POS						(uint32_t)(0ul)

#define  USB_HS_DEVICE_AINT_OEPINT_0						(uint32_t)(1ul << 16ul)
#define  USB_HS_DEVICE_AINT_OEPINT_1						(uint32_t)(1ul << 17ul)
#define  USB_HS_DEVICE_AINT_OEPINT_2						(uint32_t)(1ul << 18ul)
#define  USB_HS_DEVICE_AINT_OEPINT_3						(uint32_t)(1ul << 19ul)
#define  USB_HS_DEVICE_AINT_OEPINT_4						(uint32_t)(1ul << 20ul)
#define  USB_HS_DEVICE_AINT_OEPINT_5						(uint32_t)(1ul << 21ul)
#define  USB_HS_DEVICE_AINT_OEPINT_6						(uint32_t)(1ul << 22ul)
#define  USB_HS_DEVICE_AINT_OEPINT_7						(uint32_t)(1ul << 23ul)
#define  USB_HS_DEVICE_AINT_OEPINT_8						(uint32_t)(1ul << 24ul)
#define  USB_HS_DEVICE_AINT_OEPINT_9						(uint32_t)(1ul << 25ul)
#define  USB_HS_DEVICE_AINT_OEPINT_10						(uint32_t)(1ul << 26ul)
#define  USB_HS_DEVICE_AINT_OEPINT_11						(uint32_t)(1ul << 27ul)
#define  USB_HS_DEVICE_AINT_OEPINT_12						(uint32_t)(1ul << 28ul)
#define  USB_HS_DEVICE_AINT_OEPINT_13						(uint32_t)(1ul << 29ul)
#define  USB_HS_DEVICE_AINT_OEPINT_14						(uint32_t)(1ul << 30ul)
#define  USB_HS_DEVICE_AINT_OEPINT_15						(uint32_t)(1ul << 31ul)

#define  USB_HS_DEVICE_AINT_IEPINT_0						(uint32_t)(1ul << 0ul)
#define  USB_HS_DEVICE_AINT_IEPINT_1						(uint32_t)(1ul << 1ul)
#define  USB_HS_DEVICE_AINT_IEPINT_2						(uint32_t)(1ul << 2ul)
#define  USB_HS_DEVICE_AINT_IEPINT_3						(uint32_t)(1ul << 3ul)
#define  USB_HS_DEVICE_AINT_IEPINT_4						(uint32_t)(1ul << 4ul)
#define  USB_HS_DEVICE_AINT_IEPINT_5						(uint32_t)(1ul << 5ul)
#define  USB_HS_DEVICE_AINT_IEPINT_6						(uint32_t)(1ul << 6ul)
#define  USB_HS_DEVICE_AINT_IEPINT_7						(uint32_t)(1ul << 7ul)
#define  USB_HS_DEVICE_AINT_IEPINT_8						(uint32_t)(1ul << 8ul)
#define  USB_HS_DEVICE_AINT_IEPINT_9						(uint32_t)(1ul << 9ul)
#define  USB_HS_DEVICE_AINT_IEPINT_10						(uint32_t)(1ul << 10ul)
#define  USB_HS_DEVICE_AINT_IEPINT_11						(uint32_t)(1ul << 11ul)
#define  USB_HS_DEVICE_AINT_IEPINT_12						(uint32_t)(1ul << 12ul)
#define  USB_HS_DEVICE_AINT_IEPINT_13						(uint32_t)(1ul << 13ul)
#define  USB_HS_DEVICE_AINT_IEPINT_14						(uint32_t)(1ul << 14ul)
#define  USB_HS_DEVICE_AINT_IEPINT_15						(uint32_t)(1ul << 15ul)

/************  Bit definition for USB_HS_DEVICE_AINTMSK register  *****************/
#define  USB_HS_DEVICE_AINTMSK_OEPINTM						(uint32_t)(0xfffful << 16ul)//OUT endpoint interrupt mask bits
#define  USB_HS_DEVICE_AINTMSK_IEPINTM						(uint32_t)(0xfffful << 0ul) //IN endpoint interrupt mask bits

#define  USB_HS_DEVICE_AINTMSK_OEPINTM_POS				(uint32_t)(16ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_POS				(uint32_t)(0ul)

#define  USB_HS_DEVICE_AINTMSK_OEPINTM_0					(uint32_t)(1ul << 16ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_1					(uint32_t)(1ul << 17ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_2					(uint32_t)(1ul << 18ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_3					(uint32_t)(1ul << 19ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_4					(uint32_t)(1ul << 20ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_5					(uint32_t)(1ul << 21ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_6					(uint32_t)(1ul << 22ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_7					(uint32_t)(1ul << 23ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_8					(uint32_t)(1ul << 24ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_9					(uint32_t)(1ul << 25ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_10					(uint32_t)(1ul << 26ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_11					(uint32_t)(1ul << 27ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_12					(uint32_t)(1ul << 28ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_13					(uint32_t)(1ul << 29ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_14					(uint32_t)(1ul << 30ul)
#define  USB_HS_DEVICE_AINTMSK_OEPINTM_15					(uint32_t)(1ul << 31ul)

#define  USB_HS_DEVICE_AINTMSK_IEPINTM_0					(uint32_t)(1ul << 0ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_1					(uint32_t)(1ul << 1ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_2					(uint32_t)(1ul << 2ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_3					(uint32_t)(1ul << 3ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_4					(uint32_t)(1ul << 4ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_5					(uint32_t)(1ul << 5ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_6					(uint32_t)(1ul << 6ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_7					(uint32_t)(1ul << 7ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_8					(uint32_t)(1ul << 8ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_9					(uint32_t)(1ul << 9ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_10					(uint32_t)(1ul << 10ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_11					(uint32_t)(1ul << 11ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_12					(uint32_t)(1ul << 12ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_13					(uint32_t)(1ul << 13ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_14					(uint32_t)(1ul << 14ul)
#define  USB_HS_DEVICE_AINTMSK_IEPINTM_15					(uint32_t)(1ul << 15ul)

/************  Bit definition for USB_HS_DEVICE_VBUSDIS register  ***************/
#define  USB_HS_DEVICE_VBUSDIS_VBUSDT						(uint32_t)(0xfffful << 0ul) //VBus discharge time

#define  USB_HS_DEVICE_VBUSDIS_VBUSDT_POS					(uint32_t)(0ul)

/************  Bit definition for USB_HS_DEVICE_VBUSPULSE register  ***************/
#define  USB_HS_DEVICE_VBUSPULSE_VBUSP						(uint32_t)(0xffful << 0ul) //VBus pulse time

#define  USB_HS_DEVICE_VBUSPULSE_VBUSP_POS				(uint32_t)(0ul)

/************  Bit definition for USB_HS_DEVICE_THRCTL register  ******************/
#define  USB_HS_DEVICE_THRCTL_ARPEN							(uint32_t)(1ul << 27ul)		//Arbiter parking enable
#define  USB_HS_DEVICE_THRCTL_RXTHRLEN						(uint32_t)(0x1fful << 17ul)//Receive threshold length
#define  USB_HS_DEVICE_THRCTL_RXTHREN						(uint32_t)(1ul << 16ul)		//Receive threshold enable
#define  USB_HS_DEVICE_THRCTL_TXTHRLEN						(uint32_t)(0x1fful << 2ul)	//Transmit threshold length
#define  USB_HS_DEVICE_THRCTL_ISOTHREN						(uint32_t)(1ul << 1ul)		//Isochronous IN threshold enable
#define  USB_HS_DEVICE_THRCTL_NONISOTHREN					(uint32_t)(1ul << 0ul)		//Non-isochronous threshold enable

#define  USB_HS_DEVICE_THRCTL_RXTHRLEN_POS				(uint32_t)(17ul)
#define  USB_HS_DEVICE_THRCTL_TXTHRLEN_POS				(uint32_t)(2ul)

/************  Bit definition for USB_HS_DEVICE_IEPEMPMSK register  ***************/
#define  USB_HS_DEVICE_IEPEMPMSK_INEPTXFEM				(uint32_t)(0xfffful << 0ul)//IN EP TxFIFO empty interrupt mask bits

#define  USB_HS_DEVICE_IEPEMPMSK_INEPTXFEM_POS			(uint32_t)(0ul)

/************  Bit definition for USB_HS_DEVICE_EACHINT register  *****************/
#define  USB_HS_DEVICE_EACHINT_OEP1INT						(uint32_t)(1ul << 17ul)		//OUT endpoint 1 interrupt bit
#define  USB_HS_DEVICE_EACHINT_IEP1INT						(uint32_t)(1ul << 1ul)		//IN endpoint 1 interrupt bit

/************  Bit definition for USB_HS_DEVICE_EACHINTMSK register  **************/
#define  USB_HS_DEVICE_EACHINTMSK_OEP1INTM				(uint32_t)(1ul << 17ul)		//OUT endpoint 1 interrupt mask bit
#define  USB_HS_DEVICE_EACHINTMSK_IEP1INTM				(uint32_t)(1ul << 1ul)		//IN endpoint 1 interrupt mask bit

/************  Bit definition for USB_HS_DEVICE_IEPEACHMSK1 register **************/
#define  USB_HS_DEVICE_IEPEACHMSK1_NAKM					(uint32_t)(1ul << 13ul)		//NAK interrupt mask
#define  USB_HS_DEVICE_IEPEACHMSK1_BIM						(uint32_t)(1ul << 9ul)		//BNA interrupt mask
#define  USB_HS_DEVICE_IEPEACHMSK1_TXFURM					(uint32_t)(1ul << 8ul)		//Tx FIFO underrun mask
#define  USB_HS_DEVICE_IEPEACHMSK1_INEPNEM				(uint32_t)(1ul << 6ul)		//IN endpoint NAK effective mask
#define  USB_HS_DEVICE_IEPEACHMSK1_INEPNMM				(uint32_t)(1ul << 5ul)		//IN token receive with EP mismatch mask
#define  USB_HS_DEVICE_IEPEACHMSK1_ITTXFEMSK				(uint32_t)(1ul << 4ul)		//IN token receive when TxFIFO empty mask
#define  USB_HS_DEVICE_IEPEACHMSK1_TOM						(uint32_t)(1ul << 3ul)		//Timeout condition mask (nonisochronous endpoint)
#define  USB_HS_DEVICE_IEPEACHMSK1_EPDM					(uint32_t)(1ul << 1ul)		//Endpoint disabled interrupt mask
#define  USB_HS_DEVICE_IEPEACHMSK1_XFRCM					(uint32_t)(1ul << 0ul)		//Transfer completed interrupt mask

/************  Bit definition for USB_HS_DEVICE_OEPEACHMSK1 register  ***************/
#define  USB_HS_DEVICE_OEPEACHMSK1_NYETM					(uint32_t)(1ul << 14ul)		//NYET interrupt mask
#define  USB_HS_DEVICE_OEPEACHMSK1_NAKM					(uint32_t)(1ul << 13ul)		//NAK interrupt mask
#define  USB_HS_DEVICE_OEPEACHMSK1_BERRM					(uint32_t)(1ul << 12ul)		//Bubble error interrupt mask
#define  USB_HS_DEVICE_OEPEACHMSK1_BOIM					(uint32_t)(1ul << 9ul)		//BNA interrupt mask
#define  USB_HS_DEVICE_OEPEACHMSK1_OPEM					(uint32_t)(1ul << 8ul)		//OUT packet error mask
#define  USB_HS_DEVICE_OEPEACHMSK1_AHBERRM				(uint32_t)(1ul << 2ul)		//AHB error mask
#define  USB_HS_DEVICE_OEPEACHMSK1_EPDM					(uint32_t)(1ul << 1ul)		//Endpoint disabled interrupt mask
#define  USB_HS_DEVICE_OEPEACHMSK1_XFRCM					(uint32_t)(1ul << 0ul)		//Transfer completed interrupt mask

/************  Bit definition for USB_HS_DEVICE_ENDPOINT_CTL register  **************/
#define  USB_HS_DEVICE_ENDPOINT_CTL_EPENA					(uint32_t)(1ul << 31ul)		//EP enable
#define  USB_HS_DEVICE_ENDPOINT_CTL_EPDIS					(uint32_t)(1ul << 30ul)		//EP disable
#define  USB_HS_DEVICE_ENDPOINT_CTL_SODDFRM				(uint32_t)(1ul << 29ul)		//Isochronous: set odd frame
#define  USB_HS_DEVICE_ENDPOINT_CTL_SD0PID				(uint32_t)(1ul << 28ul)		//Interrupt/bulk: set DATA0 PID
#define  USB_HS_DEVICE_ENDPOINT_CTL_SEVNFRM				(uint32_t)(1ul << 28ul)		//Isochronous: set even frame
#define  USB_HS_DEVICE_ENDPOINT_CTL_SNAK					(uint32_t)(1ul << 27ul)		//Set NAK
#define  USB_HS_DEVICE_ENDPOINT_CTL_CNAK					(uint32_t)(1ul << 26ul)		//Clear NAK
#define  USB_HS_DEVICE_ENDPOINT_CTL_TXFNUM				(uint32_t)(0xful << 22ul)	//TX FIFO NUM
#define  USB_HS_DEVICE_ENDPOINT_CTL_TXFNUM_POS			(uint32_t)(22ul)
#define  USB_HS_DEVICE_ENDPOINT_CTL_STALL					(uint32_t)(1ul << 21ul)		//Set STALL
#define  USB_HS_DEVICE_ENDPOINT_CTL_SNPM					(uint32_t)(1ul << 20ul)		//Snoop mode
#define  USB_HS_DEVICE_ENDPOINT_CTL_EPTYP					(uint32_t)(3ul << 18ul)		//EP type
#define  USB_HS_DEVICE_ENDPOINT_CTL_NAKSTS				(uint32_t)(1ul << 17ul)		//NAK status
#define  USB_HS_DEVICE_ENDPOINT_CTL_EONUM					(uint32_t)(1ul << 16ul)		//Isochronous: even/odd frame
#define  USB_HS_DEVICE_ENDPOINT_CTL_DPID					(uint32_t)(1ul << 16ul)		//Interrupt/bulk: data PID
#define  USB_HS_DEVICE_ENDPOINT_CTL_USBAEP				(uint32_t)(1ul << 15ul)		//USB active endpoint
#define  USB_HS_DEVICE_ENDPOINT_CTL_MPSIZ					(uint32_t)(0x7fful << 0ul)	//Maximum packet size
#define  USB_HS_DEVICE_ENDPOINT_CTL_MPSIZ0				(uint32_t)(3ul << 0ul)		//Maximum packet size for EP0

#define  USB_HS_DEVICE_ENDPOINT_CTL_EPTYP_CONTROL		(uint32_t)(0ul << 18ul)		//Control EP type
#define  USB_HS_DEVICE_ENDPOINT_CTL_EPTYP_ISOCHRONOUS	(uint32_t)(1ul << 18ul)		//Isochronous EP type
#define  USB_HS_DEVICE_ENDPOINT_CTL_EPTYP_BULK			(uint32_t)(2ul << 18ul)		//Bulk EP type
#define  USB_HS_DEVICE_ENDPOINT_CTL_EPTYP_INTERRUPT	(uint32_t)(3ul << 18ul)		//Interrupt EP type

#define  USB_HS_DEVICE_ENDPOINT_CTL_MPSIZ0_64			(uint32_t)(0x0ul << 0ul)	//EP0 maximum packet size 64 bytes
#define  USB_HS_DEVICE_ENDPOINT_CTL_MPSIZ0_32			(uint32_t)(0x1ul << 0ul)	//EP0 maximum packet size 32 bytes
#define  USB_HS_DEVICE_ENDPOINT_CTL_MPSIZ0_16			(uint32_t)(0x2ul << 0ul)	//EP0 maximum packet size 16 bytes
#define  USB_HS_DEVICE_ENDPOINT_CTL_MPSIZ0_8				(uint32_t)(0x3ul << 0ul)	//EP0 maximum packet size 8 bytes

/************  Bit definition for USB_HS_DEVICE_ENDPOINT_INT register  *************/
#define  USB_HS_DEVICE_ENDPOINT_INT_NYET					(uint32_t)(1ul << 14ul)		//NYET interrupt
#define  USB_HS_DEVICE_ENDPOINT_INT_NAK					(uint32_t)(1ul << 13ul)		//NAK interrupt
#define  USB_HS_DEVICE_ENDPOINT_INT_BERR					(uint32_t)(1ul << 12ul)		//Babble error interrupt
#define  USB_HS_DEVICE_ENDPOINT_INT_PKTDRPSTS			(uint32_t)(1ul << 11ul)		//Packet dropped status
#define  USB_HS_DEVICE_ENDPOINT_INT_BNA					(uint32_t)(1ul << 9ul)		//BNA interrupt
#define  USB_HS_DEVICE_ENDPOINT_INT_TXFIFOUDRN			(uint32_t)(1ul << 8ul)		//Tx FIFO underrun
#define  USB_HS_DEVICE_ENDPOINT_INT_OPEM					(uint32_t)(1ul << 8ul)		//OUT packet error
#define  USB_HS_DEVICE_ENDPOINT_INT_TXFE					(uint32_t)(1ul << 7ul)		//Tx FIFO empty
#define  USB_HS_DEVICE_ENDPOINT_INT_INEPNE				(uint32_t)(1ul << 6ul)		//IN: endpoint NAK effective
#define  USB_HS_DEVICE_ENDPOINT_INT_B2BSTUP				(uint32_t)(1ul << 6ul)		//OUT: back-to-back SETUP packets received
#define  USB_HS_DEVICE_ENDPOINT_INT_ITXFE					(uint32_t)(1ul << 4ul)		//IN: token received, when TxFIFO empty
#define  USB_HS_DEVICE_ENDPOINT_INT_OTEPDIS				(uint32_t)(1ul << 4ul)		//OUT: token received when endpoint disabled
#define  USB_HS_DEVICE_ENDPOINT_INT_TOC					(uint32_t)(1ul << 3ul)		//IN: timeout condition
#define  USB_HS_DEVICE_ENDPOINT_INT_STUP					(uint32_t)(1ul << 3ul)		//OUT: setup phase done
#define  USB_HS_DEVICE_ENDPOINT_INT_AHB_ERROR			(uint32_t)(1ul << 2ul)		//OUT: setup phase done
#define  USB_HS_DEVICE_ENDPOINT_INT_EPDISD				(uint32_t)(1ul << 1ul)		//Endpoint disabled interrupt
#define  USB_HS_DEVICE_ENDPOINT_INT_XFRC					(uint32_t)(1ul << 0ul)		//Transfer completed interrupt

/************  Bit definition for USB_HS_DEVICE_ENDPOINT_TSIZ register  *************/
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_PKTCNT_IN0			(uint32_t)(3ul << 19ul)		//IN0: packets count
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_IN0			(uint32_t)(0x7ful << 0ul)	//IN0: transfer size

#define  USB_HS_DEVICE_ENDPOINT_TSIZ_STUPCNT_OUT0		(uint32_t)(3ul << 29ul)		//OUT0: setup packets count
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_PKTCNT_OUT0		(uint32_t)(1ul << 19ul)		//OUT0: packets count
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_OUT0		(uint32_t)(0x7ful << 0ul)	//OUT0: transfer size

#define  USB_HS_DEVICE_ENDPOINT_TSIZ_MCNT					(uint32_t)(3ul << 29ul)		//multi count
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_RXDPID				(uint32_t)(3ul << 29ul)		//received data pid
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_PKTCNT				(uint32_t)(0x3ful << 19ul)	//packets count
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_XFRSIZ				(uint32_t)(0x7ffful << 0ul)//transfer size

#define  USB_HS_DEVICE_ENDPOINT_TSIZ_RXDPID_DATA0		(uint32_t)(0ul << 29ul)		//DATA0
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_RXDPID_DATA2		(uint32_t)(1ul << 29ul)		//DATA1
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_RXDPID_DATA1		(uint32_t)(2ul << 29ul)		//DATA1
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_RXDPID_MDATA		(uint32_t)(3ul << 29ul)		//MDATA

#define  USB_HS_DEVICE_ENDPOINT_TSIZ_MCNT_POS			(uint32_t)(29ul)
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_STUPCNT_OUT0_POS	(uint32_t)(29ul)
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_PKTCNT_POS			(uint32_t)(19ul)
#define  USB_HS_DEVICE_ENDPOINT_TSIZ_XFRSIZ_POS			(uint32_t)(0ul)

/************  Bit definition for USB_HS_DEVICE_ENDPOINT_DMA register  *************/
#define  USB_HS_DEVICE_ENDPOINT_DMA_DMAADDR				(uint32_t)(0xfffffffful << 0ul)//DMA address

#define  USB_HS_DEVICE_ENDPOINT_DMA_DMAADDR_POS			(uint32_t)(0ul)

/************  Bit definition for USB_HS_DEVICE_ENDPOINT_FSTS register  *************/
#define  USB_HS_DEVICE_ENDPOINT_FSTS_INEPTFSAV			(uint32_t)(0xfffful << 0ul)	//IN EP TxFIFO space available

#define  USB_HS_DEVICE_ENDPOINT_FSTS_INEPTFSAV_POS		(uint32_t)(0ul)

/******************************************************************************/
/*                                                                            */
/*                            USB High Speed POWER & CLOCK                    */
/*                                                                            */
/******************************************************************************/
/************  Bit definition for USB_HS_PC_GCCTL register  *******************/
#define  USB_HS_PC_GCCTL_PHYSUSP								(uint32_t)(1ul << 4ul) //phy suspended
#define  USB_HS_PC_GCCTL_GATEHCLK							(uint32_t)(1ul << 1ul) //Gate HCLK
#define  USB_HS_PC_GCCTL_STPPCLK								(uint32_t)(1ul << 0ul) //Stop PHY clock

#endif // STM32F2XX_REGSUSB_H
