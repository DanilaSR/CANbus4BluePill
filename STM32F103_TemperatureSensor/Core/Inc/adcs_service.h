/* spacecraft.ADCS.1.0 service server -- temperature subset (service-ID 370).
 *
 * The ground station (gui_spacecraft) reads sensor boards through this service:
 * it sends `command = CMD_GET_TEMPERATURES (3)` with `id_sensor = 15` (all
 * sensors) or a 1-based sensor index, and expects `status = STATUS_ACCEPTED (0)`
 * plus a `temperature[]` array in the response.
 *
 * Only the temperature command is implemented here; the other four arrays of
 * the response (navigation / sun_sensors / magnitometer / gyroscope) are left
 * empty (count = 0), which is valid DSDL.
 *
 * See interface_Cyphal/adcs_command.py and GUI/sudn_module/sudn_controller.py
 * on the ground-station side.
 */
#ifndef ADCS_SERVICE_H
#define ADCS_SERVICE_H

#include <canard.h>
#include <stdbool.h>
#include <stdint.h>

#define ADCS_SERVICE_ID       370U
#define ADCS_ID_ALL_SENSORS   15U /* id_sensor == 15 -> return every sensor     */
#define ADCS_MAX_TEMPERATURES 6U  /* capacity of temperature[] in the response  */

/* Register the request subscription on service-ID 370.
 * Call once after canard_new(); returns false on allocation failure. */
bool adcs_service_init(canard_t* self);

/* Answer every queued temperature request. Call from the main loop.
 *
 *   temps_c    -- up to ADCS_MAX_TEMPERATURES readings in degrees Celsius;
 *                 sensor i (1-based, as the GUI numbers the panels) is
 *                 temps_c[i - 1].
 *   count      -- how many entries of temps_c are usable (0..ADCS_MAX_TEMPERATURES).
 *   sensor_ok  -- false means no reading at all is available: the response then
 *                 carries an empty array, so the GUI keeps showing "— °C"
 *                 instead of a bogus 0.0 (it ignores the `valid` flag). */
void adcs_service_poll(canard_t* self, const float* temps_c, uint8_t count, bool sensor_ok);

#endif /* ADCS_SERVICE_H */
