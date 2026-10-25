/* spacecraft.ADCS.1.0 service server -- temperature subset.
 *
 * Structure deliberately mirrors lib/node_getinfo.c (the working GetInfo server
 * on this board): the RX callback only copies the request header into a small
 * queue and frees the payload, while the main loop serializes and transmits the
 * response. Keeping TX out of the IRQ callback avoids re-entering libcanard.
 */

#include "adcs_service.h"

#include <stdlib.h>
#include <string.h>

#include <spacecraft/ADCS_1_0.h>
#include <spacecraft/subject/Temperature_1_0.h>

#define ADCS_QUEUE_LEN 2U /* must be a power of two */

typedef struct
{
    uint8_t client;
    uint8_t transfer_id;
    uint8_t command;
    uint8_t id_sensor;
} adcs_request_t;

static canard_subscription_t sub_request;

static volatile adcs_request_t queue[ADCS_QUEUE_LEN];
static volatile uint8_t queue_head;
static volatile uint8_t queue_tail;

/* 506 bytes: worst-case serialization of a fully populated response. Static
 * because serialization must outlive the call until canard_poll() drains TX. */
static uint8_t response_buffer[spacecraft_ADCS_Response_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_];

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

    spacecraft_ADCS_Request_1_0 req;
    size_t size = payload.view.size;
    if ((source_node_id <= CANARD_NODE_ID_MAX) &&
        (spacecraft_ADCS_Request_1_0_deserialize_(&req, payload.view.data, &size) >= 0))
    {
        const uint8_t next = (uint8_t)((queue_head + 1U) & (ADCS_QUEUE_LEN - 1U));
        if (next != queue_tail)
        {
            queue[queue_head].client      = (uint8_t)source_node_id;
            queue[queue_head].transfer_id = (uint8_t)transfer_id;
            queue[queue_head].command     = req.command;
            queue[queue_head].id_sensor   = req.id_sensor;
            queue_head = next;
        }
    }
    release_payload(&payload);
}

static const canard_subscription_vtable_t request_vt = {.on_message = &on_request};

bool adcs_service_init(canard_t* self)
{
    if (self == NULL)
    {
        return false;
    }

    queue_head = 0U;
    queue_tail = 0U;

    return canard_subscribe_request(self, &sub_request, ADCS_SERVICE_ID,
                                    (size_t)spacecraft_ADCS_Request_1_0_EXTENT_BYTES_,
                                    5000000U, &request_vt) != NULL;
}

void adcs_service_poll(canard_t* self, const float* temps_c, uint8_t count, bool sensor_ok)
{
    while (queue_tail != queue_head)
    {
        const uint8_t client    = queue[queue_tail].client;
        const uint8_t tid       = queue[queue_tail].transfer_id;
        const uint8_t command   = queue[queue_tail].command;
        const uint8_t id_sensor = queue[queue_tail].id_sensor;
        queue_tail = (uint8_t)((queue_tail + 1U) & (ADCS_QUEUE_LEN - 1U));

        spacecraft_ADCS_Response_1_0 resp;
        memset(&resp, 0, sizeof(resp)); /* zeroes every array count */

        if (command != spacecraft_ADCS_Request_1_0_CMD_GET_TEMPERATURES)
        {
            /* Only temperatures are served by this board. */
            resp.status = spacecraft_ADCS_Response_1_0_STATUS_ERROR;
        }
        else
        {
            resp.status = spacecraft_ADCS_Response_1_0_STATUS_ACCEPTED;

            uint8_t limit = (count > ADCS_MAX_TEMPERATURES) ? ADCS_MAX_TEMPERATURES : count;
            if ((temps_c == NULL) || !sensor_ok)
            {
                limit = 0U; /* no reading -> empty array, GUI keeps "— °C" */
            }

            uint8_t n = 0U;
            for (uint8_t i = 1U; i <= limit; ++i)
            {
                if ((id_sensor != ADCS_ID_ALL_SENSORS) && (id_sensor != i))
                {
                    continue; /* single-sensor request for another index */
                }
                resp.temperature.elements[n].temperature_id = i; /* 1-based: GUI does panels[id-1] */
                resp.temperature.elements[n].temperature    = temps_c[i - 1U];
                resp.temperature.elements[n].valid          = true;
                ++n;
            }
            resp.temperature.count = n;
        }

        size_t sz = sizeof(response_buffer);
        if (spacecraft_ADCS_Response_1_0_serialize_(&resp, response_buffer, &sz) < 0)
        {
            continue;
        }

        const canard_bytes_chain_t chain = {
            .bytes = {.size = sz, .data = response_buffer},
            .next  = NULL,
        };
        const canard_us_t now = self->vtable->now(self);
        (void)canard_respond(self, now + 5000000U, canard_prio_nominal,
                             ADCS_SERVICE_ID, client, tid, chain, NULL);
    }
}
