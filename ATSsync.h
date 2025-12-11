///
/// \file
///

#pragma once

#include "ATSsync_types.h"

#ifndef ATSsync_API
#    define ATSsync_API
#endif

/// Represents a ATSsync physical device on the computer
struct sb_device_s;

typedef struct sb_device_s sb_device_t;

#ifdef __cplusplus
extern "C" {
#endif

/// Retrieves the number of ATSsync devices installed on the computer where this
/// function is called.
sb_rc_t ATSsync_API sb_get_device_count(size_t *count);

/// Returns the ATSsync library version.
sb_rc_t ATSsync_API sb_get_library_version(sb_version_t *version);

/// Opens a ATSsync device. This function must be called before calling any
/// `sb_device_*` function.
sb_rc_t ATSsync_API sb_device_open(size_t index, sb_device_t **handle);

/// Closes a ATSsync device. The associated device handle should not be used
/// with any `sb_device_*` function after this is called.
sb_rc_t ATSsync_API sb_device_close(sb_device_t *device);

/// Returns the driver version of a device.
sb_rc_t ATSsync_API sb_device_get_driver_version(sb_device_t *device,
                                                 sb_version_t *version);

/// Returns the firmware version of a device.
sb_rc_t ATSsync_API sb_device_get_firmware_version(sb_device_t *device,
                                                   sb_version_t *version);

/// Starts a firmware update on a device. The update status of the device must
/// be ::sb_update_status_idle before calling this.
///
/// @note Starting the update is an asynchronous call, which means that the
///       device's status will be ::sb_update_status_processing for up to a few
///       seconds. You must wait for the device's status to be
///       ::sb_update_status_running before issuing sb_device_update_write()
///       calls.
sb_rc_t ATSsync_API sb_device_update_start_async(sb_device_t *device);

/// Retrieves firmware update status of a device. Can be called regardless of
/// the current device update status.
///
/// @param[in]  device      The device to get the update status of.
/// @param[out] status      Returns the status of the device
/// @param[out] error_code  If the status is ::sb_update_status_failed, this
///                         returns a return code with the reason of the
///                         failure. Pass `NULL` if you are not interested in
///                         this value.
sb_rc_t ATSsync_API sb_device_update_get_status(sb_device_t *device,
                                                sb_update_status_t *status,
                                                sb_rc_t *error_code);

/// Writes a block of the new firmware image data to a device. Must be called
/// only when the update status is ::sb_update_status_running.
///
/// If it succeeds, this call does not change the update status.
sb_rc_t ATSsync_API sb_device_update_write(sb_device_t *device,
                                           sb_update_data_t const *data);

/// Completes the firmware upgrade process. This should be called once all the
/// data for the new firmware image has been written, only when the update
/// status is ::sb_update_status_running.
///
/// Upon success, this call changes the update status back to
/// ::sb_update_status_idle.
sb_rc_t ATSsync_API sb_device_update_complete(sb_device_t *device);

/// Resets a device's firmware upgrade process. This can be called regardless of
/// the current device update status.
///
/// Upon success, changes the update status to ::sb_update_status_idle.
sb_rc_t ATSsync_API sb_device_update_reset(sb_device_t *device);

/// Queries the status of the identification LED of a ATSsync
sb_rc_t ATSsync_API sb_device_get_id_led_state(sb_device_t *device,
                                               sb_id_led_state_t *state);

/// Turns the identification LED of a ATSsync on or off
sb_rc_t ATSsync_API sb_device_set_id_led_state(sb_device_t *device,
                                               sb_id_led_state_t state);

/// Queries the state of the trigger polarity. See
/// sb_device_get_trigger_polarity() for more information.
sb_rc_t ATSsync_API sb_device_get_trigger_polarity(
    sb_device_t *device, sb_trigger_polarity_t *polarity);

/// To synchronize acquisition between multiple boards, ATSsync must detect
/// trigger events internally. By default, it looks for rising edges in the
/// external trigger signal. You may use this function to invert the trigger
/// signal internally, and make it so that ATSsync detects falling edges.
///
/// Note that if you do so, the trigger signal coming out of ATSsync will be
/// inverted, so individual digitizers should trigger on the rising edge of the
/// trigger signal.
sb_rc_t ATSsync_API sb_device_set_trigger_polarity(
    sb_device_t *device, sb_trigger_polarity_t polarity);

/// Queries the current trigger status
sb_rc_t ATSsync_API sb_device_get_trigger_status(sb_device_t *device,
                                                 sb_trigger_status_t *status);

/// Enables or disables the trigger signal coming out of ATSsync. By default,
/// the triggger signal is disabled, which means that the output of the trigger
/// ports is low even if pulses are sent to the external trigger input.
///
/// @note   Triggers should only enabled when all digitzers are ready to acquire
///         data. Otherwise some boards may start collecting data before others.
sb_rc_t ATSsync_API sb_device_set_trigger_status(sb_device_t *device,
                                                 sb_trigger_status_t status);

/// Queries the current clock state of a ATSsync
sb_rc_t ATSsync_API sb_device_get_clock(sb_device_t *device,
                                        sb_clock_conf_t *conf);

/// Configures the clock circuit of a ATSsync device
sb_rc_t ATSsync_API sb_device_set_clock(sb_device_t *device,
                                        sb_clock_conf_t conf);

/// Fills the string pointed to by `serial` with the device's serial number.
/// `count` indicates how many characters can be copied into the string. Pass a
/// 32-character string to be sure that the serial number you receive is
/// complete.
sb_rc_t ATSsync_API sb_device_get_serial(sb_device_t *device, char *serial,
                                         size_t count);

#ifdef __cplusplus
}
#endif
