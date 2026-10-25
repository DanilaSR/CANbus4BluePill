/* spacecraft.UpdateFirmware.1.0 service server (service-ID 373).
 *
 * This is the board side of the GUI's "ЗАГРУЗИТЬ ПРОШИВКУ" panel:
 *   allocate -> receive chunk (128 B) -> flash chunk -> jump
 *
 * SAFETY: the service refuses to touch the flash region it is itself running
 * from. By default only the free tail of a 64 KiB Blue Pill is writable
 * (this application ends around 0x08009308, so writing starts at 0x0800A000).
 * For a real bootloader, override the two macros below so the writable window
 * is the application slot instead:
 *
 *     -DFW_TARGET_MIN_ADDR=0x08004000UL -DFW_TARGET_MAX_ADDR=0x08010000UL
 *
 * NOTE ON RAM: the request carries `fw_size` up front, but this
 * implementation never buffers the image -- each chunk is programmed into
 * flash immediately, so a 37 KiB image fits comfortably in 20 KiB of RAM.
 */
#ifndef FIRMWARE_SERVICE_H
#define FIRMWARE_SERVICE_H

#include <canard.h>
#include <stdbool.h>
#include <stdint.h>

#define FIRMWARE_SERVICE_ID 373U

#ifndef FW_TARGET_MIN_ADDR
#define FW_TARGET_MIN_ADDR 0x0800A000UL /* running image ends ~0x08009308 */
#endif
#ifndef FW_TARGET_MAX_ADDR
#define FW_TARGET_MAX_ADDR 0x08010000UL /* end of 64 KiB flash */
#endif

/* Register the request subscription on service-ID 373. Call once after canard_new(). */
bool firmware_service_init(canard_t* self);

/* Process at most one queued request and send the response. Call from the main loop. */
void firmware_service_poll(canard_t* self);

/* True when a validated JUMP_TO_START request is pending. The response has to
 * be on the wire first, so main() calls firmware_service_jump_now() a few
 * milliseconds later instead of jumping from inside poll(). */
bool firmware_service_should_jump(void);

/* Disable IRQ/CAN/flash access and transfer control to the new image.
 * Does not return. */
void firmware_service_jump_now(void);

#endif /* FIRMWARE_SERVICE_H */
