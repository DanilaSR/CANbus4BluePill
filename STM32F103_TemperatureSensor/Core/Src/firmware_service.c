/* spacecraft.UpdateFirmware.1.0 service server -- see firmware_service.h.
 *
 * Request/response handling follows lib/node_getinfo.c: the RX callback only
 * copies the transfer into a static buffer, and the main loop does the actual
 * flash work and transmits the response. Flash erase/program stalls the CPU on
 * STM32F1, so it must never be done from the CAN interrupt.
 */

#include "firmware_service.h"

#include <stdlib.h>
#include <string.h>

#include <main.h>
#include <spacecraft/UpdateFirmware_1_0.h>

#define FW_CHUNK_CAPACITY spacecraft_UpdateFirmware_Request_1_0_chunk_ARRAY_CAPACITY_

/* Owned by main.c; needed to shut the controller down before jumping. */
extern CAN_HandleTypeDef hcan1;

#if (FW_TARGET_MIN_ADDR % FLASH_PAGE_SIZE) != 0
#error "FW_TARGET_MIN_ADDR must be page aligned"
#endif

/* One pending request at a time: the client is synchronous, and dropping a
 * duplicate is harmless because every command is retried by the ground station. */
static uint8_t request_buffer[spacecraft_UpdateFirmware_Request_1_0_EXTENT_BYTES_];
static size_t  request_size;
static uint8_t request_client;
static uint8_t request_transfer_id;
static volatile bool request_pending;

static canard_subscription_t sub_request;

static uint8_t response_buffer[spacecraft_UpdateFirmware_Response_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_];

/* Session state: the image is programmed chunk by chunk, never buffered whole. */
static struct
{
    bool     allocated;
    uint32_t base;
    uint32_t size;
    uint32_t written;
    uint8_t  chunk[FW_CHUNK_CAPACITY];
    size_t   chunk_len;
} session;

static volatile bool jump_pending;
static uint32_t      jump_address;

static void release_payload(canard_payload_t* const payload)
{
    if (payload->origin.data != NULL)
    {
        free(payload->origin.data);
        payload->origin.data = NULL;
        payload->origin.size = 0U;
    }
}

static void on_request(canard_subscription_t* self,
                       canard_us_t            timestamp,
                       canard_prio_t          priority,
                       uint_least8_t          source_node_id,
                       uint_least8_t          transfer_id,
                       canard_payload_t       payload)
{
    (void)self;
    (void)timestamp;
    (void)priority;

    if (!request_pending && (source_node_id <= CANARD_NODE_ID_MAX) &&
        (payload.view.size <= sizeof(request_buffer)))
    {
        memcpy(request_buffer, payload.view.data, payload.view.size);
        request_size        = payload.view.size;
        request_client      = (uint8_t)source_node_id;
        request_transfer_id = (uint8_t)transfer_id;
        request_pending     = true;
    }
    release_payload(&payload);
}

static const canard_subscription_vtable_t request_vt = {.on_message = &on_request};

/* ---------------------------------------------------------
 * Flash helpers (STM32F1 medium density: 1 KiB pages, halfword programming).
 * --------------------------------------------------------- */

/* Linker-provided symbols, used to find where this image really ends in flash.
 * .data is copied at boot from flash, so the image's last byte is
 * _sidata + (_edata - _sdata). */
extern char _sidata;
extern char _sdata;
extern char _edata;

static uint32_t image_flash_end(void)
{
    return (uint32_t)&_sidata + ((uint32_t)&_edata - (uint32_t)&_sdata);
}

/* Lowest address this service may ever erase. Both the compile-time floor and
 * the real end of the running image are honoured, so growing the application
 * cannot silently make the update window overlap its own code. */
static uint32_t writable_floor(void)
{
    const uint32_t aligned = (image_flash_end() + (FLASH_PAGE_SIZE - 1U)) & ~(FLASH_PAGE_SIZE - 1U);
    return (aligned > FW_TARGET_MIN_ADDR) ? aligned : FW_TARGET_MIN_ADDR;
}

static bool range_is_writable(uint32_t addr, uint32_t len)
{
    if (len == 0U)
    {
        return false;
    }
    if (addr < writable_floor())
    {
        return false; /* would clobber the running image */
    }
    if ((addr > FW_TARGET_MAX_ADDR) || (len > (FW_TARGET_MAX_ADDR - addr)))
    {
        return false;
    }
    return true;
}

static uint32_t page_count(uint32_t addr, uint32_t len)
{
    const uint32_t first = (addr - FLASH_BASE) / FLASH_PAGE_SIZE;
    const uint32_t last  = ((addr + len - 1U) - FLASH_BASE) / FLASH_PAGE_SIZE;
    return (last - first) + 1U;
}

static bool flash_erase(uint32_t addr, uint32_t len)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;
    bool ok = false;

    if (!range_is_writable(addr, len))
    {
        return false;
    }

    erase.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = addr;
    erase.NbPages     = page_count(addr, len);

    HAL_FLASH_Unlock();
    if (HAL_FLASHEx_Erase(&erase, &page_error) == HAL_OK)
    {
        ok = true;
    }
    HAL_FLASH_Lock();
    return ok;
}

static bool flash_program(uint32_t addr, const uint8_t* data, size_t len)
{
    bool ok = true;

    if (!range_is_writable(addr, (uint32_t)len))
    {
        return false;
    }

    HAL_FLASH_Unlock();
    for (size_t i = 0U; (i < len) && ok; i += 2U)
    {
        /* Odd tail byte is padded with 0xFF, which matches erased flash. */
        const uint16_t halfword =
            (uint16_t)((uint16_t)data[i] | (uint16_t)(((i + 1U) < len) ? (uint16_t)data[i + 1U] : 0x00FFU) << 8);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, halfword) != HAL_OK)
        {
            ok = false;
        }
    }
    HAL_FLASH_Lock();

    if (ok && (memcmp((const void*)addr, data, len) != 0))
    {
        ok = false; /* verify what actually landed in flash */
    }
    return ok;
}

static bool image_looks_bootable(uint32_t addr)
{
    if (!range_is_writable(addr, 4U))
    {
        return false;
    }
    const uint32_t initial_sp = *(const volatile uint32_t*)addr;
    /* SRAM on STM32F103C8 spans 0x20000000..0x20005000. */
    return (initial_sp & 0x2FFE0000UL) == 0x20000000UL;
}

/* ---------------------------------------------------------
 * Command handlers. Every one returns a STATUS_* code.
 * --------------------------------------------------------- */

static uint8_t handle_allocate(const spacecraft_UpdateFirmware_Request_1_0* req)
{
    const uint32_t offset = (req->offset.count > 0U) ? req->offset.elements[0] : 0U;
    const uint32_t size   = (req->fw_size.count > 0U) ? req->fw_size.elements[0] : 0U;

    session.allocated = false;
    session.chunk_len = 0U;

    if (!range_is_writable(offset, size))
    {
        return spacecraft_UpdateFirmware_Response_1_0_STATUS_NOT_ENOUGH_MEMORY;
    }
    if (!flash_erase(offset, size))
    {
        return spacecraft_UpdateFirmware_Response_1_0_STATUS_NOT_ENOUGH_MEMORY;
    }

    session.base      = offset;
    session.size      = size;
    session.written   = 0U;
    session.allocated = true;
    return spacecraft_UpdateFirmware_Response_1_0_STATUS_MEMORY_ALLOCATED;
}

static uint8_t handle_receive(const spacecraft_UpdateFirmware_Request_1_0* req)
{
    if (!session.allocated || (req->chunk.count > FW_CHUNK_CAPACITY))
    {
        return spacecraft_UpdateFirmware_Response_1_0_STATUS_RECEPTION_FAILED;
    }
    if ((session.written + req->chunk.count) > session.size)
    {
        return spacecraft_UpdateFirmware_Response_1_0_STATUS_RECEPTION_FAILED;
    }

    memcpy(session.chunk, req->chunk.elements, req->chunk.count);
    session.chunk_len = req->chunk.count;
    return spacecraft_UpdateFirmware_Response_1_0_STATUS_CHUNK_RECEIVED;
}

static uint8_t handle_flash(const spacecraft_UpdateFirmware_Request_1_0* req)
{
    if (!session.allocated || (session.chunk_len == 0U))
    {
        return spacecraft_UpdateFirmware_Response_1_0_STATUS_FLASH_FAILED;
    }

    const uint32_t addr = (req->offset.count > 0U) ? req->offset.elements[0]
                                                  : (session.base + session.written);
    if (!flash_program(addr, session.chunk, session.chunk_len))
    {
        return spacecraft_UpdateFirmware_Response_1_0_STATUS_FLASH_FAILED;
    }

    session.written  += session.chunk_len;
    session.chunk_len = 0U;
    return spacecraft_UpdateFirmware_Response_1_0_STATUS_FLASH_COMPLETED;
}

static uint8_t handle_erase(const spacecraft_UpdateFirmware_Request_1_0* req)
{
    if (req->erase_struct.count == 0U)
    {
        return spacecraft_UpdateFirmware_Response_1_0_STATUS_ERASE_FAILED;
    }

    /* SectorEraseStruct is F7-flavoured (sector/nb_sectors). On F1 the page
     * index maps directly to an address, so translate and reuse flash_erase(). */
    const uint32_t sector = req->erase_struct.elements[0].sector;
    const uint32_t count  = req->erase_struct.elements[0].nb_sectors;
    if ((count == 0U) || (count > (FW_TARGET_MAX_ADDR - FW_TARGET_MIN_ADDR) / FLASH_PAGE_SIZE))
    {
        return spacecraft_UpdateFirmware_Response_1_0_STATUS_ERASE_FAILED;
    }

    const uint32_t addr = FLASH_BASE + (sector * FLASH_PAGE_SIZE);
    if (!flash_erase(addr, count * FLASH_PAGE_SIZE))
    {
        return spacecraft_UpdateFirmware_Response_1_0_STATUS_ERASE_FAILED;
    }
    session.allocated = false;
    return spacecraft_UpdateFirmware_Response_1_0_STATUS_ERASE_COMPLETED;
}

static uint8_t handle_jump(const spacecraft_UpdateFirmware_Request_1_0* req)
{
    const uint32_t addr = (req->offset.count > 0U) ? req->offset.elements[0] : 0U;
    if (!image_looks_bootable(addr))
    {
        /* Nothing bootable there -- refusing beats busy-looping on a hard fault. */
        return spacecraft_UpdateFirmware_Response_1_0_STATUS_UNKNOWN_FAILURE;
    }
    jump_address  = addr;
    jump_pending  = true;
    return spacecraft_UpdateFirmware_Response_1_0_STATUS_JUMPING;
}

/* ---------------------------------------------------------
 * Public API
 * --------------------------------------------------------- */

bool firmware_service_init(canard_t* self)
{
    if (self == NULL)
    {
        return false;
    }

    memset(&session, 0, sizeof(session));
    request_pending = false;
    request_size    = 0U;
    jump_pending    = false;

    return canard_subscribe_request(self, &sub_request, FIRMWARE_SERVICE_ID,
                                    (size_t)spacecraft_UpdateFirmware_Request_1_0_EXTENT_BYTES_,
                                    5000000U, &request_vt) != NULL;
}

void firmware_service_poll(canard_t* self)
{
    if (!request_pending)
    {
        return;
    }

    spacecraft_UpdateFirmware_Request_1_0 req;
    size_t size = request_size;
    request_pending = false;

    if (spacecraft_UpdateFirmware_Request_1_0_deserialize_(&req, request_buffer, &size) < 0)
    {
        return; /* malformed -- the client will retry */
    }

    uint8_t status;
    switch (req.command)
    {
        case spacecraft_UpdateFirmware_Request_1_0_COMMAND_ALLOCATE_MEMORY:
            status = handle_allocate(&req);
            break;
        case spacecraft_UpdateFirmware_Request_1_0_COMMAND_RECEIVE_CHUNK:
            status = handle_receive(&req);
            break;
        case spacecraft_UpdateFirmware_Request_1_0_COMMAND_FLASH_CHUNK:
            status = handle_flash(&req);
            break;
        case spacecraft_UpdateFirmware_Request_1_0_COMMAND_ERASE_MEMORY:
            status = handle_erase(&req);
            break;
        case spacecraft_UpdateFirmware_Request_1_0_COMMAND_JUMP_TO_START:
            status = handle_jump(&req);
            break;
        default:
            status = spacecraft_UpdateFirmware_Response_1_0_STATUS_UNKNOWN_FAILURE;
            break;
    }

    spacecraft_UpdateFirmware_Response_1_0 resp;
    memset(&resp, 0, sizeof(resp));
    resp.status = status;

    size_t sz = sizeof(response_buffer);
    if (spacecraft_UpdateFirmware_Response_1_0_serialize_(&resp, response_buffer, &sz) < 0)
    {
        return;
    }

    const canard_bytes_chain_t chain = {
        .bytes = {.size = sz, .data = response_buffer},
        .next  = NULL,
    };
    const canard_us_t now = self->vtable->now(self);
    (void)canard_respond(self, now + 2000000U, canard_prio_nominal,
                         FIRMWARE_SERVICE_ID, request_client, request_transfer_id, chain, NULL);
}

bool firmware_service_should_jump(void)
{
    return jump_pending;
}

void firmware_service_jump_now(void)
{
    typedef void (*app_entry_t)(void);

    const uint32_t app_addr = jump_address;
    const uint32_t app_sp   = *(const volatile uint32_t*)(app_addr);

    jump_pending = false;

    __disable_irq();
    HAL_CAN_Stop(&hcan1);
    HAL_CAN_DeInit(&hcan1);

    /* Point VTOR at the new image (Cortex-M3 has no VTOR relocation in HAL's
     * SystemInit for F1, so set it explicitly before the jump). */
    SCB->VTOR = app_addr;
    __DSB();
    __ISB();

    __set_MSP(app_sp);
    ((app_entry_t)(*(const volatile uint32_t*)(app_addr + 4U)))();
}
