OPTIMIZATION            = 2

#----------------------------------------------------------
#PATH must be set to CodeSourcery/bin
CROSS                      = arm-none-eabi-

GCC                        = $(CROSS)gcc
AS                         = $(CROSS)as
SIZE                       = $(CROSS)size
OBJCOPY                    = $(CROSS)objcopy
OBJDUMP                    = $(CROSS)objdump
NM                         = $(CROSS)nm

#----------------------------------------------------------
MCU                         = STM32F107VC
TARGET_NAME                 = stm32f1
#----------------------------------------------------------
BUILD_DIR                   = build
OUTPUT_DIR                  = output
REXOS                       = ../../rexos
KERNEL                      = $(REXOS)/kernel
USERSPACE					 	 = $(REXOS)/userspace
LIB								 = $(REXOS)/lib
LDS_SCRIPT                  = $(KERNEL)/arm.ld.S
#----------------------------------------------------------
LIBS_DIR                    = ../../3rdparty
CMSIS_DIR                   = $(LIBS_DIR)/CMSIS
CMSIS_DEVICE_DIR            = $(CMSIS_DIR)/Device/ST/STM32F10x
#CMSIS_DEVICE_DIR           = $(CMSIS_DIR)/Device/ST/STM32F2xx
#CMSIS_DEVICE_DIR           = $(CMSIS_DIR)/Device/ST/STM32F4xx
#----------------------------------------------------------
#not used in kernel
INCLUDE_FOLDERS             = $(CMSIS_DIR)/Include $(CMSIS_DEVICE_DIR)/Include
#userspace
INCLUDE_FOLDERS		+= $(USERSPACE) $(USERSPACE)/core
#kernel
INCLUDE_FOLDERS				+= $(KERNEL) $(KERNEL)/core
#lib
INCLUDE_FOLDERS            += $(LIB)
#sys
INCLUDE_FOLDERS            += $(REXOS)/drv/stm32 $(REXOS)/midware

INCLUDES                    = $(INCLUDE_FOLDERS:%=-I%)
VPATH                      += $(INCLUDE_FOLDERS)
#----------------------------------------------------------
#core-dependent part
SRC_C                       = kcortexm.c
SRC_AS                      = startup_cortexm.S cortexm.S
#kernel
SRC_C                      += kernel.c dbg.c kirq.c kprocess.c ktimer.c kipc.c kstream.c kdirect.c kblock.c kobject.c
#kernel MES
#SRC_C                      += kmutex.c kevent.c ksem.c
#lib
SRC_C                      += lib_lib.c lib_time.c lib_stdio.c lib_std.c pool.c printf.c lib_array.c lib_heap.c lib_usb.c stm32_lib_gpio.c
#userspace
SRC_C                      += stdio.c main.c comm.c
#drv
SRC_C                      += stm32_power.c stm32_gpio.c stm32_timer.c stm32_rtc.c stm32_core.c stm32_uart.c stm32_analog.c stm32_usb.c stm32_wdt.c
#midware
SRC_C								+= usbd.c cdc.c

OBJ                         = $(SRC_AS:%.S=%.o) $(SRC_C:%.c=%.o)
#----------------------------------------------------------
DEFINES                     = -D$(MCU)
MCU_FLAGS                   = -mcpu=cortex-m3 -mthumb -D__CORTEX_M3 -D__thumb2__=1 -mtune=cortex-m3 -msoft-float -mapcs-frame
NO_DEFAULTS                 = -fdata-sections -ffunction-sections -fno-hosted -fno-builtin  -nostdlib -nodefaultlibs
FLAGS_CC                    = $(INCLUDES) $(DEFINES) -I. -O$(OPTIMIZATION) -Wall -c -fmessage-length=0 $(MCU_FLAGS) $(NO_DEFAULTS)
FLAGS_LD                    = -Xlinker --gc-sections $(MCU_FLAGS)
#----------------------------------------------------------
all: $(TARGET_NAME).elf

%.elf: asm_defines.h $(OBJ) $(LDS_SCRIPT)
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
	@mkdir -p $(OUTPUT_DIR)
	@mv $(BUILD_DIR)/$(TARGET_NAME).bin $(OUTPUT_DIR)/$(TARGET_NAME).bin

.c.o:
	@-mkdir -p $(BUILD_DIR)
	@echo CC: $<
	@$(GCC) $(FLAGS_CC) -c ./$< -o $(BUILD_DIR)/$@

.S.o: asm_defines.h
	@-mkdir -p $(BUILD_DIR)
	@echo AS_C: $<
	@$(GCC) $(INCLUDES) -I. $(DEFINES) -c -x assembler-with-cpp ./$< -o $(BUILD_DIR)/$@

asm_defines.h: asm_defines.c
	$(GCC) $(FLAGS_CC) -S $< -o - | awk '($$1 == "->") { print "#define " $$2 " " $$3 }' > $@

program:
	@st-flash write $(OUTPUT_DIR)/$(TARGET_NAME).bin 0x8000000

clean:
	@echo '-----------------------------------------------------------'
	@rm -f build/*.*

test:
	@echo $(VPATH)

.PHONY : all clean program flash
