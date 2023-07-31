OPTIMIZATION            = s

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
MCU                         = STM32F100RB
TARGET_NAME                 = stm32-discovery
#----------------------------------------------------------
BUILD_DIR                   = build
OUTPUT_DIR                  = output
REXOS                       = ../../../../rexos
KERNEL                      = $(REXOS)/kernel
USERSPACE                   = $(REXOS)/userspace
LIB                         = $(REXOS)/lib
LDS_SCRIPT                  = $(KERNEL)/arm.ld.S
#----------------------------------------------------------
CMSIS_DIR                   = $(REXOS)/CMSIS
#CMSIS_DEVICE_DIR            = $(CMSIS_DIR)/Device/ST/STM32L0xx
CMSIS_DEVICE_DIR            = $(CMSIS_DIR)/Device/ST/STM32F10x
#CMSIS_DEVICE_DIR           = $(CMSIS_DIR)/Device/ST/STM32F2xx
#CMSIS_DEVICE_DIR           = $(CMSIS_DIR)/Device/ST/STM32F4xx
#CMSIS_DEVICE_DIR            = $(CMSIS_DIR)/Device/NXP/LPC11Uxx
#----------------------------------------------------------
#not used in kernel
INCLUDE_FOLDERS             = $(CMSIS_DIR)/Include $(CMSIS_DEVICE_DIR)/Include
#kernel
INCLUDE_FOLDERS            += $(REXOS) $(KERNEL) $(KERNEL)/core
#lib
INCLUDE_FOLDERS            += $(LIB)
#userspace
INCLUDE_FOLDERS            += $(USERSPACE) $(USERSPACE)/core $(USERSPACE)/stm32
#sys
INCLUDE_FOLDERS            += $(REXOS)/kernel/drv $(REXOS)/kernel/stm32 $(REXOS)/midware $(REXOS)/midware/usbd $(REXOS)/midware/tcpips

INCLUDES                    = $(INCLUDE_FOLDERS:%=-I%)
VPATH                      += $(INCLUDE_FOLDERS)
#----------------------------------------------------------
#core-dependent part
SRC_C                       = kcortexm.c
SRC_AS                      = startup_cortexm.S cortexm.S
#kernel
SRC_C                      += kernel.c dbg.c kstdlib.c karray.c kso.c kirq.c kprocess.c ksystime.c kipc.c kstream.c kobject.c kio.c kerror.c kexo.c
#lib
SRC_C                      += lib_lib.c lib_systime.c pool.c printf.c lib_std.c lib_stdio.c lib_array.c lib_so.c
#drv
SRC_C                      += stm32_pin.c stm32_gpio.c stm32_power.c stm32_timer.c stm32_rtc.c stm32_exo.c stm32_uart.c
#userspace lib
SRC_C                      += ipc.c io.c process.c stdio.c stdlib.c systime.c time.c uart.c power.c stream.c pin.c
#userspace lib
SRC_C                      += app.c

OBJ                         = $(SRC_AS:%.S=%.o) $(SRC_C:%.c=%.o)
#----------------------------------------------------------
DEFINES                     = -D$(MCU)
MCU_FLAGS                   = -mcpu=cortex-m3 -mthumb -D__CORTEX_M3 -D__thumb2__=1 -mtune=cortex-m3 -msoft-float -mapcs-frame
NO_DEFAULTS                 = -fdata-sections -ffunction-sections -fno-hosted -fno-builtin  -nostdlib -nodefaultlibs
FLAGS_CC                    = $(INCLUDES) $(DEFINES) -I. -O$(OPTIMIZATION) -Wall -c -fmessage-length=0 $(MCU_FLAGS) $(NO_DEFAULTS)
FLAGS_LD                    = -Xlinker --gc-sections $(MCU_FLAGS)
#----------------------------------------------------------
all: $(TARGET_NAME).elf

%.elf: $(OBJ) $(LDS_SCRIPT)
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

.S.o:
	@-mkdir -p $(BUILD_DIR)
	@echo AS_C: $<
	@$(GCC) $(INCLUDES) -I. $(DEFINES) -c -x assembler-with-cpp ./$< -o $(BUILD_DIR)/$@

program:
	@st-flash write $(OUTPUT_DIR)/$(TARGET_NAME).bin 0x8000000
#	@openocd -f stm32f1.cfg -c "program $(OUTPUT_DIR)/$(TARGET_NAME).bin 0x08000000 reset exit"

menuconfig:
	@kconfig-mconf KConfig

clean:
	@echo '-----------------------------------------------------------'
	@rm -f build/*.*

test:
	@echo $(VPATH)

.PHONY : all clean program flash
