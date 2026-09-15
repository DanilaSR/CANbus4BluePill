#!/bin/bash

# STM32 F1 + Cyphal/UAVCAN (libcanard) build script (Blue Pill STM32F103C8T6)
# Uses arm-none-eabi-gcc toolchain (STM32CubeCLT)

TOOLCHAIN_PATHS=(
    "/home/danila/.local/bin"
    "/c/ST/STM32CubeCLT_1.21.0/GNU-tools-for-STM32/bin"
    "/opt/STM32CubeCLT/GNU-tools-for-STM32/bin"
    "$(dirname "$(command -v arm-none-eabi-gcc 2>/dev/null)")"
)

PREFIX=""
for p in "${TOOLCHAIN_PATHS[@]}"; do
    if [ -n "$p" ] && [ -x "$p/arm-none-eabi-gcc" ]; then
        PREFIX="$p/"
        break
    fi
done

if [ -z "$PREFIX" ]; then
    if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
        PREFIX=""
    else
        echo "ERROR: arm-none-eabi-gcc not found in PATH or known toolchain dirs."
        exit 1
    fi
fi

CC="${PREFIX}arm-none-eabi-gcc"
OBJCOPY="${PREFIX}arm-none-eabi-objcopy"
SIZE="${PREFIX}arm-none-eabi-size"

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

# MCU: STM32F103C8T6 - Cortex-M3, no FPU
MCU="-mcpu=cortex-m3 -mthumb"
DEFS="-DUSE_HAL_DRIVER -DSTM32F103xB"
INCLUDES=(
    "-I${PROJECT_DIR}/Core/Inc"
    "-I${PROJECT_DIR}/lib"
    "-I${PROJECT_DIR}/uavcan/DSDLtypes"
    "-I${PROJECT_DIR}/Drivers/STM32F1xx_HAL_Driver/Inc"
    "-I${PROJECT_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Include"
    "-I${PROJECT_DIR}/Drivers/CMSIS/Include"
)

CFLAGS_BASE="${MCU} ${DEFS} ${INCLUDES[*]} -O2 -Wall -fdata-sections -ffunction-sections"
ASFLAGS="${MCU} ${DEFS} ${INCLUDES[*]} -Wall"

LDSCRIPT="${PROJECT_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/linker/STM32F103XB_FLASH.ld"
LDFLAGS="${MCU} -specs=nano.specs -T${LDSCRIPT} -lc -lm -lnosys -Wl,--gc-sections"

HAL_SRCS=(
    "${PROJECT_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c"
    "${PROJECT_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_can.c"
    "${PROJECT_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c"
    "${PROJECT_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c"
    "${PROJECT_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c"
    "${PROJECT_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c"
    "${PROJECT_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c"
    "${PROJECT_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c"
    "${PROJECT_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c"
    "${PROJECT_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c"
)

CORE_SRCS=(
    "${PROJECT_DIR}/Core/Src/main.c"
    "${PROJECT_DIR}/Core/Src/stm32f1xx_it.c"
    "${PROJECT_DIR}/lib/canard.c"
    "${PROJECT_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c"
)

ASM_SRCS=(
    "${PROJECT_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s"
)

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

OBJECTS=()

build_c() {
    local src="$1"
    local obj="${BUILD_DIR}/$(basename "${src%.c}").o"
    echo "  CC  ${src}"
    ${CC} -c ${CFLAGS_BASE} -o "${obj}" "${src}" || { echo "Compile FAILED: ${src}"; exit 1; }
    OBJECTS+=("${obj}")
}

build_asm() {
    local src="$1"
    local obj="${BUILD_DIR}/$(basename "${src%.s}").o"
    echo "  AS  ${src}"
    ${CC} -x assembler-with-cpp -c ${ASFLAGS} -o "${obj}" "${src}" || { echo "Assemble FAILED: ${src}"; exit 1; }
    OBJECTS+=("${obj}")
}

echo "Building project: ${PROJECT_DIR}"

for src in "${HAL_SRCS[@]}" "${CORE_SRCS[@]}"; do
    build_c "${src}"
done
for src in "${ASM_SRCS[@]}"; do
    build_asm "${src}"
done

echo "Linking..."
${CC} "${OBJECTS[@]}" ${LDFLAGS} -o "${BUILD_DIR}/project.elf" || { echo "LINK FAILED"; exit 1; }

"${OBJCOPY}" -O ihex "${BUILD_DIR}/project.elf" "${BUILD_DIR}/project.hex"
"${OBJCOPY}" -O binary -S "${BUILD_DIR}/project.elf" "${BUILD_DIR}/project.bin"
"${SIZE}" "${BUILD_DIR}/project.elf"

echo "Build complete!"