///
/// \file
///

#pragma once

#ifndef ATS_DRIVER
#    include "stdint.h"
#else
#    ifdef _WIN32
// We cannot use stdint.h from driver code on Windows, so we need to redefine
// fixed-sized integers manually.
#        ifndef INT8_MAX
typedef __int8 int8_t;
#        endif
#        ifndef UINT8_MAX
typedef unsigned __int8 uint8_t;
#        endif
#        ifndef INT32_MAX
typedef __int32 int32_t;
#        endif
#        ifndef UINT32_MAX
typedef unsigned __int32 uint32_t;
#        endif
#        ifndef INT64_MAX
typedef __int64 int64_t;
#        endif
#        ifndef UINT64_MAX
typedef unsigned __int64 uint64_t;
#        endif
#    else
#        include "linux/types.h"
#    endif
#endif

/// ATSsync Return Type
typedef enum {
    sb_rc_success = 0,
    sb_rc_internal_error = -1,
    sb_rc_ad9516_communication_failure = -2,

    /// The ATSsync device is invalid
    sb_rc_invalid_device = -3,

    /// A function parameter was null
    sb_rc_null_parameter = -4,

    /// Generic failure.
    sb_rc_failed = -5,

    /// Memory not available
    sb_rc_no_memory = -6,

    /// Invalid type was detected for the product
    sb_rc_invalid_device_type = -7,

    /// The requested update operation cannot be performed because the board's
    /// update status is not correct. Please check the documentation.
    sb_rc_update_invalid_status = -8,

    /// Data passed to one of the APIs was invalid. Please check the
    /// documentation.
    sb_rc_invalid_data = -9,

    /// Operation timed out
    sb_rc_timeout = -10,

    /// PLL clock circuit failed to lock.
    sb_rc_pll_not_locked = -11,

    /// A CRC was invalid.
    sb_rc_invalid_crc = -12,

    /// The desired frequency cannot be outputted. Please check the
    /// documentation and ATSsync log file.
    sb_rc_invalid_frequency = -13,
} sb_rc_t;

static inline const char *sb_rc_name(sb_rc_t rc) {
    switch (rc) {
    case sb_rc_success:
        return "success";
    case sb_rc_internal_error:
        return "internal_error";
    case sb_rc_ad9516_communication_failure:
        return "ad9516_communication_error";
    case sb_rc_invalid_device:
        return "invalid_device";
    case sb_rc_null_parameter:
        return "null_parameter";
    case sb_rc_failed:
        return "failed";
    case sb_rc_no_memory:
        return "no_memory";
    case sb_rc_invalid_device_type:
        return "invalid_device_type";
    case sb_rc_update_invalid_status:
        return "update_invalid_status";
    case sb_rc_invalid_data:
        return "invalid_data";
    case sb_rc_timeout:
        return "timeout";
    case sb_rc_pll_not_locked:
        return "pll_not_locked";
    case sb_rc_invalid_crc:
        return "invalid_rc";
    case sb_rc_invalid_frequency:
        return "invalid_frequency";
    default:
        return "invalid_rc";
    }
}

/// Represents the version of one of the components of ATSsync
typedef struct {
    /// Major version number
    int32_t major;

    /// Minor version number
    int32_t minor;

    /// Patch version number
    int32_t patch;

    /// Full version number, potentially including pre-release information, for
    /// example "1.2.3-dev2".
    char full[32];
} sb_version_t;

/// Status of the firmware upgrade. See sb_device_update_get_status().
typedef enum {
    /// No update is ongoing. You may start an update with
    /// sb_device_update_start_async().
    sb_update_status_idle = 0,

    /// An operation is being processed on the device. Do not issue any update
    /// commands until the status switches to a different value.
    sb_update_status_processing = 1,

    /// The firmware update has started. You may issue sb_device_update_write()
    /// or sb_device_update_complete() commands.
    sb_update_status_running = 2,

    /// The firmware update failed. The error code return value of
    /// sb_device_update_get_status() indicates why the update is in this state.
    /// You can reset the firmware update engine to the idle state by calling
    /// sb_device_update_reset().
    sb_update_status_failed = 3,
} sb_update_status_t;

/// Information returned by sb_device_update_get_status()
typedef struct {
    sb_update_status_t status; ///< Status of the update
    sb_rc_t rc; ///< If the status is `sb_update_status_failed`, this indicates
                ///< the associated error code
} sb_update_status_pair_t;

/// Represents a block of ATSsync's firmware, sent to the device during an
/// update.
typedef struct {
    /// Location of the block in the firmware image.
    int32_t offset_bytes;

    /// Size of the current block. Note that this cannot exceed `sizeof(data)`
    int32_t size_bytes;

    /// Data block. If the block's size is less than the size of this array, the
    /// data is located at the start of the array.
    uint8_t data[0x400];
} sb_update_data_t;

/// State of the ATSsync's identification LED
typedef enum {
    sb_id_led_state_off = 0,
    sb_id_led_state_on = 1,
} sb_id_led_state_t;

/// Specifies how the clock signals output by ATSsync are generated
typedef enum {
    /// Signal is generated internally by ATSsync
    sb_clock_source_internal = 0,

    /// Signal is copied from the external clock input port
    sb_clock_source_external = 1,
} sb_clock_source_t;

typedef struct {
    sb_clock_source_t source;

    /// Indicates the frequency in Hertz of the clock to generate. Leave this
    /// field empty if `source` is `sb_clock_source_external`
    int64_t sample_rate_hz;
} sb_clock_conf_t;

typedef enum {
    sb_trigger_polarity_rising = 0,
    sb_trigger_polarity_falling = 1,
} sb_trigger_polarity_t;

typedef enum {
    sb_trigger_status_disabled = 0,
    sb_trigger_status_enabled = 1,
} sb_trigger_status_t;
