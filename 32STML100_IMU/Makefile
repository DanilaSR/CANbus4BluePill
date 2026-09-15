# STM32F103C8T6 + MPU-9250 build & flash Makefile

# Toolchain
TOOLCHAIN_DIR ?= /home/danila/st/stm32cubeclt_1.18.0/GNU-tools-for-STM32/bin
STM32_PROG   ?= /home/danila/st/stm32cubeclt_1.18.0/STM32CubeProgrammer/bin/STM32_Programmer_CLI

CC      := $(TOOLCHAIN_DIR)/arm-none-eabi-gcc
OBJCOPY := $(TOOLCHAIN_DIR)/arm-none-eabi-objcopy
SIZE    := $(TOOLCHAIN_DIR)/arm-none-eabi-size

# Directories
PROJECT_DIR := $(CURDIR)
BUILD_DIR   := $(PROJECT_DIR)/build
CORE_DIR    := $(PROJECT_DIR)/Core
HAL_DIR     := $(PROJECT_DIR)/Drivers/STM32F1xx_HAL_Driver
CMSIS_DEV   := $(PROJECT_DIR)/Drivers/CMSIS/Device/ST/STM32F1xx
CMSIS_CORE  := $(PROJECT_DIR)/Drivers/CMSIS

# MCU
MCU     := -mcpu=cortex-m3 -mthumb
DEFS    := -DUSE_HAL_DRIVER -DSTM32F103xB
INCLUDES := \
    -I$(CORE_DIR)/Inc \
    -I$(HAL_DIR)/Inc \
    -I$(CMSIS_DEV)/Include \
    -I$(CMSIS_CORE)/Include

CFLAGS  := $(MCU) $(DEFS) $(INCLUDES) -O2 -Wall -fdata-sections -ffunction-sections
ASFLAGS := $(MCU) $(DEFS) $(INCLUDES) -Wall
LDFLAGS := $(MCU) -specs=nano.specs -u _printf_float -T$(CMSIS_DEV)/Source/Templates/gcc/linker/STM32F103XB_FLASH.ld -lc -lm -lnosys -Wl,--gc-sections

# Sources
HAL_SRCS := \
    $(HAL_DIR)/Src/stm32f1xx_hal.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_cortex.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_dma.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_flash.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_flash_ex.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_gpio.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_i2c.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_pwr.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_rcc.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_rcc_ex.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_uart.c

CORE_SRCS := \
    $(CORE_DIR)/Src/main.c \
    $(CORE_DIR)/Src/stm32f1xx_hal_msp.c \
    $(CORE_DIR)/Src/mpu9250.c \
    $(CORE_DIR)/Src/ssd1306.c \
    $(CMSIS_DEV)/Source/Templates/system_stm32f1xx.c

ASM_SRCS := \
    $(CMSIS_DEV)/Source/Templates/gcc/startup_stm32f103xb.s

SRCS  := $(HAL_SRCS) $(CORE_SRCS)
OBJS  := $(SRCS:%.c=$(BUILD_DIR)/%.o)
OBJS  += $(ASM_SRCS:%.s=$(BUILD_DIR)/%.o)

TARGET   := $(BUILD_DIR)/project
ELF      := $(TARGET).elf
HEX      := $(TARGET).hex
BIN      := $(TARGET).bin

.PHONY: all build hex bin flash erase clean size

all: build hex bin size

build: $(ELF)

hex:   $(HEX)
bin:   $(BIN)

$(BUILD_DIR)/%.o: %.c
	@echo "  CC  $<"
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: %.s
	@echo "  AS  $<"
	@mkdir -p $(dir $@)
	$(CC) -x assembler-with-cpp -c $(ASFLAGS) -o $@ $<

$(ELF): $(OBJS)
	@echo "  LD  $@"
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary -S $< $@

size: $(ELF)
	$(SIZE) $<

# Manual flash (ST-Link V2 via SWD)
flash: build bin
	$(STM32_PROG) -c port=SWD -w $(BIN) 0x08000000 -v -rst

erase:
	$(STM32_PROG) -c port=SWD -e all -rst

clean:
	rm -rf $(BUILD_DIR)
