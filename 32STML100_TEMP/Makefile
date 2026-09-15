#-------------------------------------------------------------------------------
# Makefile for STM32L100RCT6 + STLM75DS2F -> Flipper Zero
#
# Targets:
#   make all     - build firmware.elf / .hex / .bin / .map
#   make clean   - remove build artifacts
#   make flash   - flash via ST-Link (requires STM32_Programmer_CLI)
#-------------------------------------------------------------------------------

# ---- Toolchain ----
TOOLCHAIN   := /home/danila/st/stm32cubeclt_1.18.0/GNU-tools-for-STM32/bin
CC          := $(TOOLCHAIN)/arm-none-eabi-gcc
OBJCOPY     := $(TOOLCHAIN)/arm-none-eabi-objcopy
SIZE        := $(TOOLCHAIN)/arm-none-eabi-size

# ---- Project layout ----
PROJECT_DIR := $(CURDIR)
BUILD_DIR   := $(PROJECT_DIR)/build
CORE_INC    := $(PROJECT_DIR)/Core/Inc
CORE_SRC    := $(PROJECT_DIR)/Core/Src
HAL_DRIVER  := $(PROJECT_DIR)/Drivers/STM32L1xx_HAL_Driver
CMSIS       := $(PROJECT_DIR)/Drivers/CMSIS
DEVICE      := $(CMSIS)/Device/ST/STM32L1xx

# ---- MCU ----
MCU       := -mcpu=cortex-m3 -mthumb
DEFS      := -DUSE_HAL_DRIVER -DSTM32L100xC
INCLUDES  := -I$(CORE_INC) \
             -I$(HAL_DRIVER)/Inc \
             -I$(DEVICE)/Include \
             -I$(CMSIS)/Include

CFLAGS    := $(MCU) $(DEFS) $(INCLUDES) -Os -Wall -Wno-format \
             -fdata-sections -ffunction-sections -g
ASFLAGS   := $(MCU) $(DEFS) $(INCLUDES) -Wall -g

# Linker script
LDSCRIPT  := $(PROJECT_DIR)/STM32L100RCTx_FLASH.ld
LDFLAGS   := $(MCU) -specs=nano.specs -specs=nosys.specs \
             -T$(LDSCRIPT) -lc -lm -lnosys -u _printf_float \
             -Wl,--gc-sections -Wl,--print-memory-usage -Wl,-Map=$(BUILD_DIR)/firmware.map

# ---- Sources (basenames are unique across all dirs) ----
HAL_SRCS  := $(HAL_DRIVER)/Src/stm32l1xx_hal.c \
             $(HAL_DRIVER)/Src/stm32l1xx_hal_rcc.c \
             $(HAL_DRIVER)/Src/stm32l1xx_hal_rcc_ex.c \
             $(HAL_DRIVER)/Src/stm32l1xx_hal_gpio.c \
             $(HAL_DRIVER)/Src/stm32l1xx_hal_i2c.c \
             $(HAL_DRIVER)/Src/stm32l1xx_hal_uart.c \
             $(HAL_DRIVER)/Src/stm32l1xx_hal_cortex.c \
             $(HAL_DRIVER)/Src/stm32l1xx_hal_dma.c \
             $(HAL_DRIVER)/Src/stm32l1xx_hal_flash.c \
             $(HAL_DRIVER)/Src/stm32l1xx_hal_flash_ex.c \
             $(HAL_DRIVER)/Src/stm32l1xx_hal_pwr.c \
             $(HAL_DRIVER)/Src/stm32l1xx_hal_pwr_ex.c

CORE_SRCS := $(CORE_SRC)/main.c \
             $(CORE_SRC)/stm32l1xx_it.c \
             $(CORE_SRC)/stlm75.c \
             $(CORE_SRC)/uart_protocol.c \
             $(CORE_SRC)/system_stm32l1xx.c

ASM_SRC   := $(PROJECT_DIR)/startup_stm32l100xc.s

C_SRCS    := $(HAL_SRCS) $(CORE_SRCS)
OBJS      := $(addprefix $(BUILD_DIR)/,$(notdir $(C_SRCS:.c=.o))) \
             $(BUILD_DIR)/$(notdir $(ASM_SRC:.s=.o))

# ---- Build ----
TARGET    := $(BUILD_DIR)/firmware

.PHONY: all clean flash size

all: $(TARGET).elf $(TARGET).hex $(TARGET).bin
	@$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	@echo "Linking $@"
	@$(CC) $(OBJS) $(LDFLAGS) -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary -S $< $@

$(BUILD_DIR)/%.o: $(CORE_SRC)/%.c
	@mkdir -p $(BUILD_DIR)
	@echo "CC   $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: $(HAL_DRIVER)/Src/%.c
	@mkdir -p $(BUILD_DIR)
	@echo "CC   $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/startup_stm32l100xc.o: $(ASM_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "AS   $<"
	@$(CC) -c $(ASFLAGS) -o $@ $<

flash: $(TARGET).hex
	STM32_Programmer_CLI -c port=SWD -w $< -v -rst

clean:
	rm -rf $(BUILD_DIR)

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf
