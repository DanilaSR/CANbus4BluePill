/* uavcan.node.GetInfo.1.0 server (service-ID 430) shared by all nodes.
 *
 * The request callback runs in the CAN RX IRQ and only queues the client
 * node-ID and transfer-ID; the response is serialized and sent from the
 * main loop via node_getinfo_poll(). libcanard must not be re-entered from
 * the callback, hence the split.
 */

#ifndef NODE_GETINFO_H_INCLUDED
#define NODE_GETINFO_H_INCLUDED

#include <stdint.h>

#include <canard.h>

typedef struct
{
    uint8_t  hardware_version_major;
    uint8_t  hardware_version_minor;
    uint8_t  software_version_major;
    uint8_t  software_version_minor;
    uint64_t software_vcs_revision_id;
    const char* name;       /* <= 50 characters */
    const uint8_t* unique_id; /* optional 16-byte override; NULL -> STM32 UID */
} node_getinfo_config_t;

/* Subscribe to uavcan.node.GetInfo.1.0 requests. Call after canard_set_node_id()
 * and before the CAN RX interrupt is enabled. Returns false on failure. */
bool node_getinfo_init(canard_t* self, const node_getinfo_config_t* cfg);

/* Answer any queued GetInfo request. Call from the main loop, with the same
 * locking used around canard_poll() if the RX IRQ may preempt it. */
void node_getinfo_poll(canard_t* self);

#endif
