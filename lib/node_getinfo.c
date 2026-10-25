#include "node_getinfo.h"

#include <stdlib.h>
#include <string.h>

#include <uavcan/node/GetInfo_1_0.h>

#ifndef NODE_GETINFO_UID_BASE
#define NODE_GETINFO_UID_BASE 0x1FFFF7E8UL
#endif

#define NODE_GETINFO_SERVICE_ID uavcan_node_GetInfo_1_0_FIXED_PORT_ID_
#define NODE_GETINFO_QUEUE_LEN  4U

typedef struct
{
    uint8_t client;
    uint8_t transfer_id;
} node_getinfo_request_t;

static canard_subscription_t sub_request;
static node_getinfo_config_t config;

static volatile node_getinfo_request_t queue[NODE_GETINFO_QUEUE_LEN];
static volatile uint8_t queue_head;
static volatile uint8_t queue_tail;

static uint8_t response_buffer[uavcan_node_GetInfo_Response_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_];

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

    const uint8_t next = (uint8_t)((queue_head + 1U) & (NODE_GETINFO_QUEUE_LEN - 1U));
    if ((next != queue_tail) && (source_node_id <= CANARD_NODE_ID_MAX))
    {
        queue[queue_head].client      = (uint8_t)source_node_id;
        queue[queue_head].transfer_id = (uint8_t)transfer_id;
        queue_head = next;
    }
    release_payload(&payload);
}

static const canard_subscription_vtable_t request_vt = {.on_message = &on_request};

static void fill_unique_id(uint8_t out[16])
{
    if (config.unique_id != NULL)
    {
        memcpy(out, config.unique_id, 16U);
        return;
    }

    const volatile uint32_t* const uid = (const volatile uint32_t*)NODE_GETINFO_UID_BASE;
    const uint32_t words[3] = {uid[0], uid[1], uid[2]};
    memcpy(out, words, sizeof(words));
    memset(&out[sizeof(words)], 0, 16U - sizeof(words));
}

bool node_getinfo_init(canard_t* self, const node_getinfo_config_t* cfg)
{
    if ((self == NULL) || (cfg == NULL) || (cfg->name == NULL))
    {
        return false;
    }

    config = *cfg;
    queue_head = 0U;
    queue_tail = 0U;

    return canard_subscribe_request(self, &sub_request, NODE_GETINFO_SERVICE_ID, 0U, 5000000U,
                                    &request_vt) != NULL;
}

void node_getinfo_poll(canard_t* self)
{
    while (queue_tail != queue_head)
    {
        const uint8_t client = queue[queue_tail].client;
        const uint8_t tid    = queue[queue_tail].transfer_id;
        queue_tail = (uint8_t)((queue_tail + 1U) & (NODE_GETINFO_QUEUE_LEN - 1U));

        uavcan_node_GetInfo_Response_1_0 resp;
        memset(&resp, 0, sizeof(resp));

        resp.protocol_version.major = 1U;
        resp.protocol_version.minor = 0U;

        resp.hardware_version.major = config.hardware_version_major;
        resp.hardware_version.minor = config.hardware_version_minor;

        resp.software_version.major = config.software_version_major;
        resp.software_version.minor = config.software_version_minor;

        resp.software_vcs_revision_id = config.software_vcs_revision_id;

        fill_unique_id(resp.unique_id);

        size_t name_len = strlen(config.name);
        if (name_len > uavcan_node_GetInfo_Response_1_0_name_ARRAY_CAPACITY_)
        {
            name_len = uavcan_node_GetInfo_Response_1_0_name_ARRAY_CAPACITY_;
        }
        memcpy(resp.name.elements, config.name, name_len);
        resp.name.count = name_len;

        resp.software_image_crc.count = 0U;
        resp.certificate_of_authenticity.count = 0U;

        size_t sz = sizeof(response_buffer);
        if (uavcan_node_GetInfo_Response_1_0_serialize_(&resp, response_buffer, &sz) < 0)
        {
            continue;
        }

        const canard_bytes_chain_t chain = {
            .bytes = {.size = sz, .data = response_buffer},
            .next  = NULL,
        };
        const canard_us_t now = self->vtable->now(self);
        (void)canard_respond(self, now + 5000000U, canard_prio_nominal,
                             NODE_GETINFO_SERVICE_ID, client, tid, chain, NULL);
    }
}
