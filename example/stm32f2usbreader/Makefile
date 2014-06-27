#==========================================================
#	File:	Makefile for Cortex-M3
#	Date:	2011-01-02
#==========================================================
OPTIMIZATION			= s

#----------------------------------------------------------
#PATH must be set to CodeSourcery/bin
CROSS						= arm-none-eabi-

GCC						= $(CROSS)gcc
AS							= $(CROSS)as
SIZE						= $(CROSS)size
OBJCOPY					= $(CROSS)objcopy
OBJDUMP					= $(CROSS)objdump
NM							= $(CROSS)nm

#----------------------------------------------------------
EXT_OSCILLATOR_FREQ	= 24000000
MCU_NAME					= STM32F215RG
TARGET_NAME				= stm32usb
MCU_CORE					= CORTEX_M3
#----------------------------------------------------------
BUILD_DIR				= build
OUTPUT_DIR				= output
KERNEL					= ../..
LDS_SCRIPT				= $(KERNEL)/arch/arm.ld.S
#----------------------------------------------------------
LIBS_DIR					= ../../../libs
CMSIS_DIR				= $(LIBS_DIR)/CMSIS
#CMSIS_DEVICE_DIR		= $(CMSIS_DIR)/Device/ST/STM32F10x
CMSIS_DEVICE_DIR		= $(CMSIS_DIR)/Device/ST/STM32F2xx
#CMSIS_DEVICE_DIR		= $(CMSIS_DIR)/Device/ST/STM32F4xx
#----------------------------------------------------------
OEM_LIBS					= $(CMSIS_DIR)/Include $(CMSIS_DEVICE_DIR)/Include
ARCH						= arch $(KERNEL)/arch $(KERNEL)/arch/cortex_m3 $(KERNEL)/arch/cortex_m3/stm
DRVS						= $(KERNEL)/drv_if
MOD						= $(KERNEL)/mod/console $(KERNEL)/mod/dbg_console $(KERNEL)/mod/keyboard
MOD						+= mod $(KERNEL)/mod/usb_msc $(KERNEL)/mod/storage $(KERNEL)/mod/scsi $(KERNEL)/mod/usbd $(KERNEL)/mod/sd_card mod/blinker mod/adc
MOD						+= mod/gpio_user mod/usb_desc_user mod/flash mod/aes mod/crypto_storage
TASKS						=
INCLUDE_FOLDERS		= $(KERNEL)/lib config $(KERNEL)/core $(KERNEL)/sys $(DRVS) $(MOD) $(STARTUP_FILE_DIR) $(OEM_LIBS) $(TASKS) $(ARCH)

INCLUDES					= $(INCLUDE_FOLDERS:%=-I%)
VPATH					  += $(INCLUDE_FOLDERS) $(BUILD_DIR)
#----------------------------------------------------------
SRC_AS					= delay_cortex_m3.S
SRC_AS_C					= startup_cortexm.S

SRC_C						= cortex_m3.c
#arch
SRC_C					  += rcc_stm32f2.c gpio_stm32.c timer_stm32.c uart_stm32.c usb_stm32f2xx.c sdio_stm32f2xx.c dma_stm32.c rand_stm32f2.c
#core
SRC_C					  += startup.c mem_pool.c mem.c mem_private.c error.c sys_call.c sys_time.c sys_time_private.c sys_timer.c thread_private.c
SRC_C					  += mutex_private.c event_private.c sem_private.c queue_private.c
#sys
SRC_C					  += file_private.c file.c
#lib
SRC_C					  += dlist.c time.c printf.c rand.c
#mod
SRC_C					  += gpio_user.c console.c dbg_console_private.c dbg_console.c
SRC_C					  += main.c

SRC_C					  += usb_desc.c usbd.c usbd_core.c usbd_core_io.c sd_card.c sd_card_cmd.c
SRC_C					  += usb_msc.c usb_msc_io.c scsi.c scsi_io.c scsi_page.c storage.c
SRC_C					  += usb_desc_user.c

OBJ						= $(SRC_AS_C:%.S=%.o) $(SRC_AS:%.S=%.o) $(SRC_C:%.c=%.o)
#----------------------------------------------------------
DEFINES					= -DHSE_VALUE=$(EXT_OSCILLATOR_FREQ) -D$(MCU_NAME) -D$(MCU_CORE)
PWD						= $(shell pwd)
MCU						= -mcpu=cortex-m3 -mthumb
MCU_CC					= $(MCU) -D__thumb2__=1 -mtune=cortex-m3 -msoft-float -mapcs-frame $(DEFINES)
FLAGS_AS					= $(MCU)
#USB core, provided by ST doesn't support strict aliasing, we should disable it.
FLAGS_CC					= $(INCLUDES) -I. -O$(OPTIMIZATION) -Wall -c -fmessage-length=0 $(MCU_CC) -fdata-sections -ffunction-sections -fno-hosted -fno-builtin  -nostdlib -nodefaultlibs
FLAGS_LD					= -Xlinker --gc-sections $(MCU)
#----------------------------------------------------------
all: $(TARGET_NAME).elf

%.elf:	$(OBJ) $(LD_SCRIPT)
	@$(GCC) $(INCLUDES) -I. $(DEFINES) -DLDS -E $(LDS_SCRIPT) -o $(BUILD_DIR)/script.ld.hash
	@awk '!/^(\ )*#/ {print $0}' $(BUILD_DIR)/script.ld.hash > $(BUILD_DIR)/script.ld
	@echo LD: $(OBJ)
	@$(GCC) $(FLAGS_LD) -T $(BUILD_DIR)/script.ld -o $(BUILD_DIR)/$@ $(OBJ:%.o=$(BUILD_DIR)/%.o)
	@echo '-----------------------------------------------------------'
	@$(SIZE) $(BUILD_DIR)/$(TARGET_NAME).elf
	@$(OBJCOPY) -O binary $(BUILD_DIR)/$(TARGET_NAME).elf $(BUILD_DIR)/$(TARGET_NAME).bin
	@$(OBJCOPY) -O ihex $(BUILD_DIR)/$(TARGET_NAME).elf $(BUILD_DIR)/$(TARGET_NAME).hex
	@$(OBJDUMP) -h -S -z $(BUILD_DIR)/$(TARGET_NAME).elf > $(BUILD_DIR)/$(TARGET_NAME).lss
	@$(NM) -n $(BUILD_DIR)/$(TARGET_NAME).elf > $(BUILD_DIR)/$(TARGET_NAME).sym
	@-mkdir -p $(OUTPUT_DIR)
	@mv $(BUILD_DIR)/$(TARGET_NAME).bin $(OUTPUT_DIR)/$(TARGET_NAME).bin

.c.o:
	@-mkdir -p $(BUILD_DIR)
	@echo CC: $<
	@$(GCC) $(FLAGS_CC) -c ./$< -o $(BUILD_DIR)/$@

.s.o:
	@-mkdir -p $(BUILD_DIR)
	@echo AS: $<
	@$(AS) $(FLAGS_AS) ./$< -o $(BUILD_DIR)/$@

.S.o:
	@-mkdir -p $(BUILD_DIR)
	@echo AS_C: $<
	@$(GCC) $(INCLUDES) -I. $(DEFINES) -c -x assembler-with-cpp ./$< -o $(BUILD_DIR)/$@

program:
	@st-flash write $(OUTPUT_DIR)/$(TARGET_NAME).bin 0x8000000

clean:
	@echo '-----------------------------------------------------------'
	@rm -f build/*.*	

.PHONY : all clean program flash
