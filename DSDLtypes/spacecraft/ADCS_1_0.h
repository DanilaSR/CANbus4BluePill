// This is an AUTO-GENERATED Cyphal DSDL data type implementation. Curious? See https://opencyphal.org.
// You shouldn't attempt to edit this file.
//
// Checking this file under version control is not recommended unless it is used as part of a high-SIL
// safety-critical codebase. The typical usage scenario is to generate it as part of the build process.
//
// To avoid conflicts with definitions given in the source DSDL file, all entities created by the code generator
// are named with an underscore at the end, like foo_bar_().
//
// Generator:     nunavut-2.3.1 (serialization was enabled)
// Source file:   C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/370.ADCS.1.0.dsdl
// Generated at:  2026-02-15 22:07:50.563446 UTC
// Is deprecated: no
// Fixed port-ID: 370
// Full name:     spacecraft.ADCS
// Version:       1.0
//
// Platform
//     python_implementation:  CPython
//     python_version:  3.12.6
//     python_release_level:  final
//     python_build:  ('tags/v3.12.6:a4a2d2b', 'Sep  6 2024 20:11:23')
//     python_compiler:  MSC v.1940 64 bit (AMD64)
//     python_revision:  a4a2d2b
//     python_xoptions:  {}
//     runtime_platform:  Windows-10-10.0.19045-SP0
//
// Language Options
//     target_endianness:  little
//     omit_float_serialization_support:  False
//     enable_serialization_asserts:  True
//     enable_override_variable_array_capacity:  False
//     cast_format:  (({type}) {value})

#ifndef SPACECRAFT_ADCS_1_0_INCLUDED_
#define SPACECRAFT_ADCS_1_0_INCLUDED_

#include <nunavut/support/serialization.h>
#include <spacecraft/subject/Gyroscope_1_0.h>
#include <spacecraft/subject/Magnitometer_1_0.h>
#include <spacecraft/subject/NavigationData_1_0.h>
#include <spacecraft/subject/SunSensor_1_0.h>
#include <spacecraft/subject/Temperature_1_0.h>
#include <stdint.h>
#include <stdlib.h>

static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_TARGET_ENDIANNESS == 434322821,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/370.ADCS.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );
static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_OMIT_FLOAT_SERIALIZATION_SUPPORT == 0,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/370.ADCS.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );
static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_ENABLE_SERIALIZATION_ASSERTS == 1,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/370.ADCS.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );
static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_ENABLE_OVERRIDE_VARIABLE_ARRAY_CAPACITY == 0,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/370.ADCS.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );
static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_CAST_FORMAT == 2368206204,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/370.ADCS.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );

#ifdef __cplusplus
extern "C" {
#endif

#define spacecraft_ADCS_1_0_HAS_FIXED_PORT_ID_ true
#define spacecraft_ADCS_1_0_FIXED_PORT_ID_     370U

// +-------------------------------------------------------------------------------------------------------------------+
// | spacecraft.ADCS.1.0
// +-------------------------------------------------------------------------------------------------------------------+
#define spacecraft_ADCS_1_0_FULL_NAME_             "spacecraft.ADCS"
#define spacecraft_ADCS_1_0_FULL_NAME_AND_VERSION_ "spacecraft.ADCS.1.0"

// +-------------------------------------------------------------------------------------------------------------------+
// | spacecraft.ADCS.Request.1.0
// +-------------------------------------------------------------------------------------------------------------------+
#define spacecraft_ADCS_Request_1_0_FULL_NAME_             "spacecraft.ADCS.Request"
#define spacecraft_ADCS_Request_1_0_FULL_NAME_AND_VERSION_ "spacecraft.ADCS.Request.1.0"

/// Extent is the minimum amount of memory required to hold any serialized representation of any compatible
/// version of the data type; or, on other words, it is the the maximum possible size of received objects of this type.
/// The size is specified in bytes (rather than bits) because by definition, extent is an integer number of bytes long.
/// When allocating a deserialization (RX) buffer for this data type, it should be at least extent bytes large.
/// When allocating a serialization (TX) buffer, it is safe to use the size of the largest serialized representation
/// instead of the extent because it provides a tighter bound of the object size; it is safe because the concrete type
/// is always known during serialization (unlike deserialization). If not sure, use extent everywhere.
#define spacecraft_ADCS_Request_1_0_EXTENT_BYTES_                    1UL
#define spacecraft_ADCS_Request_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_ 1UL
static_assert(spacecraft_ADCS_Request_1_0_EXTENT_BYTES_ >= spacecraft_ADCS_Request_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_,
              "Internal constraint violation");

/// saturated uint4 CMD_GET_NAVIGATION = 1
#define spacecraft_ADCS_Request_1_0_CMD_GET_NAVIGATION (1U)

/// saturated uint4 CMD_GET_SUN_SENSORS = 2
#define spacecraft_ADCS_Request_1_0_CMD_GET_SUN_SENSORS (2U)

/// saturated uint4 CMD_GET_TEMPERATURES = 3
#define spacecraft_ADCS_Request_1_0_CMD_GET_TEMPERATURES (3U)

/// saturated uint4 CMD_GET_ACCELERATION = 4
#define spacecraft_ADCS_Request_1_0_CMD_GET_ACCELERATION (4U)

/// saturated uint4 CMD_GET_MAGNETIC_FIELD = 5
#define spacecraft_ADCS_Request_1_0_CMD_GET_MAGNETIC_FIELD (5U)

typedef struct
{
    /// saturated uint4 command
    uint8_t command;

    /// saturated uint4 id_sensor
    uint8_t id_sensor;
} spacecraft_ADCS_Request_1_0;

/// Serialize an instance into the provided buffer.
/// The lifetime of the resulting serialized representation is independent of the original instance.
/// This method may be slow for large objects (e.g., images, point clouds, radar samples), so in a later revision
/// we may define a zero-copy alternative that keeps references to the original object where possible.
///
/// @param obj      The object to serialize.
///
/// @param buffer   The destination buffer. There are no alignment requirements.
///                 @see spacecraft_ADCS_Request_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_
///
/// @param inout_buffer_size_bytes  When calling, this is a pointer to the size of the buffer in bytes.
///                                 Upon return this value will be updated with the size of the constructed serialized
///                                 representation (in bytes); this value is then to be passed over to the transport
///                                 layer. In case of error this value is undefined.
///
/// @returns Negative on error, zero on success.
static inline int8_t spacecraft_ADCS_Request_1_0_serialize_(
    const spacecraft_ADCS_Request_1_0* const obj, uint8_t* const buffer,  size_t* const inout_buffer_size_bytes)
{
    if ((obj == NULL) || (buffer == NULL) || (inout_buffer_size_bytes == NULL))
    {
        return -NUNAVUT_ERROR_INVALID_ARGUMENT;
    }
    const size_t capacity_bytes = *inout_buffer_size_bytes;
    if ((8U * (size_t) capacity_bytes) < 8UL)
    {
        return -NUNAVUT_ERROR_SERIALIZATION_BUFFER_TOO_SMALL;
    }
    // Notice that fields that are not an integer number of bytes long may overrun the space allocated for them
    // in the serialization buffer up to the next byte boundary. This is by design and is guaranteed to be safe.
    size_t offset_bits = 0U;
    {   // saturated uint4 command
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 4ULL) <= (capacity_bytes * 8U));
        uint8_t _sat0_ = obj->command;
        if (_sat0_ > 15U)
        {
            _sat0_ = 15U;
        }
        buffer[offset_bits / 8U] = (uint8_t)(_sat0_);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 4U;
    }
    {   // saturated uint4 id_sensor
        NUNAVUT_ASSERT((offset_bits + 4ULL) <= (capacity_bytes * 8U));
        uint8_t _sat1_ = obj->id_sensor;
        if (_sat1_ > 15U)
        {
            _sat1_ = 15U;
        }
        const int8_t _err0_ = nunavutSetUxx(&buffer[0], capacity_bytes, offset_bits, _sat1_, 4U);
        if (_err0_ < 0)
        {
            return _err0_;
        }
        offset_bits += 4U;
    }
    if (offset_bits % 8U != 0U)  // Pad to 8 bits. TODO: Eliminate redundant padding checks.
    {
        const uint8_t _pad0_ = (uint8_t)(8U - offset_bits % 8U);
        NUNAVUT_ASSERT(_pad0_ > 0);
        const int8_t _err1_ = nunavutSetUxx(&buffer[0], capacity_bytes, offset_bits, 0U, _pad0_);  // Optimize?
        if (_err1_ < 0)
        {
            return _err1_;
        }
        offset_bits += _pad0_;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    }
    // It is assumed that we know the exact type of the serialized entity, hence we expect the size to match.
    NUNAVUT_ASSERT(offset_bits == 8ULL);
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    *inout_buffer_size_bytes = (size_t) (offset_bits / 8U);
    return NUNAVUT_SUCCESS;
}

/// Deserialize an instance from the provided buffer.
/// The lifetime of the resulting object is independent of the original buffer.
/// This method may be slow for large objects (e.g., images, point clouds, radar samples), so in a later revision
/// we may define a zero-copy alternative that keeps references to the original buffer where possible.
///
/// @param obj      The object to update from the provided serialized representation.
///
/// @param buffer   The source buffer containing the serialized representation. There are no alignment requirements.
///                 If the buffer is shorter or longer than expected, it will be implicitly zero-extended or truncated,
///                 respectively; see Specification for "implicit zero extension" and "implicit truncation" rules.
///
/// @param inout_buffer_size_bytes  When calling, this is a pointer to the size of the supplied serialized
///                                 representation, in bytes. Upon return this value will be updated with the
///                                 size of the consumed fragment of the serialized representation (in bytes),
///                                 which may be smaller due to the implicit truncation rule, but it is guaranteed
///                                 to never exceed the original buffer size even if the implicit zero extension rule
///                                 was activated. In case of error this value is undefined.
///
/// @returns Negative on error, zero on success.
static inline int8_t spacecraft_ADCS_Request_1_0_deserialize_(
    spacecraft_ADCS_Request_1_0* const out_obj, const uint8_t* buffer, size_t* const inout_buffer_size_bytes)
{
    if ((out_obj == NULL) || (inout_buffer_size_bytes == NULL) || ((buffer == NULL) && (0 != *inout_buffer_size_bytes)))
    {
        return -NUNAVUT_ERROR_INVALID_ARGUMENT;
    }
    if (buffer == NULL)
    {
        buffer = (const uint8_t*)"";
    }
    const size_t capacity_bytes = *inout_buffer_size_bytes;
    const size_t capacity_bits = capacity_bytes * (size_t) 8U;
    size_t offset_bits = 0U;
    // saturated uint4 command
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    if ((offset_bits + 4U) <= capacity_bits)
    {
        out_obj->command = buffer[offset_bits / 8U] & 15U;
    }
    else
    {
        out_obj->command = 0U;
    }
    offset_bits += 4U;
    // saturated uint4 id_sensor
    out_obj->id_sensor = nunavutGetU8(&buffer[0], capacity_bytes, offset_bits, 4);
    offset_bits += 4U;
    offset_bits = (offset_bits + 7U) & ~(size_t) 7U;  // Align on 8 bits.
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    *inout_buffer_size_bytes = (size_t) (nunavutChooseMin(offset_bits, capacity_bits) / 8U);
    NUNAVUT_ASSERT(capacity_bytes >= *inout_buffer_size_bytes);
    return NUNAVUT_SUCCESS;
}

/// Initialize an instance to default values. Does nothing if @param out_obj is NULL.
/// This function intentionally leaves inactive elements uninitialized; for example, members of a variable-length
/// array beyond its length are left uninitialized; aliased union memory that is not used by the first union field
/// is left uninitialized, etc. If full zero-initialization is desired, just use memset(&obj, 0, sizeof(obj)).
static inline void spacecraft_ADCS_Request_1_0_initialize_(spacecraft_ADCS_Request_1_0* const out_obj)
{
    if (out_obj != NULL)
    {
        size_t size_bytes = 0;
        const uint8_t buf = 0;
        const int8_t err = spacecraft_ADCS_Request_1_0_deserialize_(out_obj, &buf, &size_bytes);
        NUNAVUT_ASSERT(err >= 0);
        (void) err;
    }
}

// +-------------------------------------------------------------------------------------------------------------------+
// | spacecraft.ADCS.Response.1.0
// +-------------------------------------------------------------------------------------------------------------------+
#define spacecraft_ADCS_Response_1_0_FULL_NAME_             "spacecraft.ADCS.Response"
#define spacecraft_ADCS_Response_1_0_FULL_NAME_AND_VERSION_ "spacecraft.ADCS.Response.1.0"

/// Extent is the minimum amount of memory required to hold any serialized representation of any compatible
/// version of the data type; or, on other words, it is the the maximum possible size of received objects of this type.
/// The size is specified in bytes (rather than bits) because by definition, extent is an integer number of bytes long.
/// When allocating a deserialization (RX) buffer for this data type, it should be at least extent bytes large.
/// When allocating a serialization (TX) buffer, it is safe to use the size of the largest serialized representation
/// instead of the extent because it provides a tighter bound of the object size; it is safe because the concrete type
/// is always known during serialization (unlike deserialization). If not sure, use extent everywhere.
#define spacecraft_ADCS_Response_1_0_EXTENT_BYTES_                    510UL
#define spacecraft_ADCS_Response_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_ 506UL
static_assert(spacecraft_ADCS_Response_1_0_EXTENT_BYTES_ >= spacecraft_ADCS_Response_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_,
              "Internal constraint violation");

/// saturated uint4 STATUS_ACCEPTED = 0
#define spacecraft_ADCS_Response_1_0_STATUS_ACCEPTED (0U)

/// saturated uint4 STATUS_BUSY = 1
#define spacecraft_ADCS_Response_1_0_STATUS_BUSY (1U)

/// saturated uint4 STATUS_ERROR = 15
#define spacecraft_ADCS_Response_1_0_STATUS_ERROR (15U)

/// Array metadata for: spacecraft.subject.NavigationData.1.0[<=1] navigation
#define spacecraft_ADCS_Response_1_0_navigation_ARRAY_CAPACITY_           1U
#define spacecraft_ADCS_Response_1_0_navigation_ARRAY_IS_VARIABLE_LENGTH_ true

/// Array metadata for: spacecraft.subject.SunSensor.1.0[<=4] sun_sensors
#define spacecraft_ADCS_Response_1_0_sun_sensors_ARRAY_CAPACITY_           4U
#define spacecraft_ADCS_Response_1_0_sun_sensors_ARRAY_IS_VARIABLE_LENGTH_ true

/// Array metadata for: spacecraft.subject.Magnitometer.1.0[<=6] magnitometer
#define spacecraft_ADCS_Response_1_0_magnitometer_ARRAY_CAPACITY_           6U
#define spacecraft_ADCS_Response_1_0_magnitometer_ARRAY_IS_VARIABLE_LENGTH_ true

/// Array metadata for: spacecraft.subject.Gyroscope.1.0[<=6] gyroscope
#define spacecraft_ADCS_Response_1_0_gyroscope_ARRAY_CAPACITY_           6U
#define spacecraft_ADCS_Response_1_0_gyroscope_ARRAY_IS_VARIABLE_LENGTH_ true

/// Array metadata for: spacecraft.subject.Temperature.1.0[<=6] temperature
#define spacecraft_ADCS_Response_1_0_temperature_ARRAY_CAPACITY_           6U
#define spacecraft_ADCS_Response_1_0_temperature_ARRAY_IS_VARIABLE_LENGTH_ true

typedef struct
{
    /// saturated uint4 status
    uint8_t status;

    /// spacecraft.subject.NavigationData.1.0[<=1] navigation
    struct  /// Array address equivalence guarantee: &elements[0] == &navigation
    {
        spacecraft_subject_NavigationData_1_0 elements[spacecraft_ADCS_Response_1_0_navigation_ARRAY_CAPACITY_];
        size_t count;
    } navigation;

    /// spacecraft.subject.SunSensor.1.0[<=4] sun_sensors
    struct  /// Array address equivalence guarantee: &elements[0] == &sun_sensors
    {
        spacecraft_subject_SunSensor_1_0 elements[spacecraft_ADCS_Response_1_0_sun_sensors_ARRAY_CAPACITY_];
        size_t count;
    } sun_sensors;

    /// spacecraft.subject.Magnitometer.1.0[<=6] magnitometer
    struct  /// Array address equivalence guarantee: &elements[0] == &magnitometer
    {
        spacecraft_subject_Magnitometer_1_0 elements[spacecraft_ADCS_Response_1_0_magnitometer_ARRAY_CAPACITY_];
        size_t count;
    } magnitometer;

    /// spacecraft.subject.Gyroscope.1.0[<=6] gyroscope
    struct  /// Array address equivalence guarantee: &elements[0] == &gyroscope
    {
        spacecraft_subject_Gyroscope_1_0 elements[spacecraft_ADCS_Response_1_0_gyroscope_ARRAY_CAPACITY_];
        size_t count;
    } gyroscope;

    /// spacecraft.subject.Temperature.1.0[<=6] temperature
    struct  /// Array address equivalence guarantee: &elements[0] == &temperature
    {
        spacecraft_subject_Temperature_1_0 elements[spacecraft_ADCS_Response_1_0_temperature_ARRAY_CAPACITY_];
        size_t count;
    } temperature;
} spacecraft_ADCS_Response_1_0;

/// Serialize an instance into the provided buffer.
/// The lifetime of the resulting serialized representation is independent of the original instance.
/// This method may be slow for large objects (e.g., images, point clouds, radar samples), so in a later revision
/// we may define a zero-copy alternative that keeps references to the original object where possible.
///
/// @param obj      The object to serialize.
///
/// @param buffer   The destination buffer. There are no alignment requirements.
///                 @see spacecraft_ADCS_Response_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_
///
/// @param inout_buffer_size_bytes  When calling, this is a pointer to the size of the buffer in bytes.
///                                 Upon return this value will be updated with the size of the constructed serialized
///                                 representation (in bytes); this value is then to be passed over to the transport
///                                 layer. In case of error this value is undefined.
///
/// @returns Negative on error, zero on success.
static inline int8_t spacecraft_ADCS_Response_1_0_serialize_(
    const spacecraft_ADCS_Response_1_0* const obj, uint8_t* const buffer,  size_t* const inout_buffer_size_bytes)
{
    if ((obj == NULL) || (buffer == NULL) || (inout_buffer_size_bytes == NULL))
    {
        return -NUNAVUT_ERROR_INVALID_ARGUMENT;
    }
    const size_t capacity_bytes = *inout_buffer_size_bytes;
    if ((8U * (size_t) capacity_bytes) < 4048UL)
    {
        return -NUNAVUT_ERROR_SERIALIZATION_BUFFER_TOO_SMALL;
    }
    // Notice that fields that are not an integer number of bytes long may overrun the space allocated for them
    // in the serialization buffer up to the next byte boundary. This is by design and is guaranteed to be safe.
    size_t offset_bits = 0U;
    {   // saturated uint4 status
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 4ULL) <= (capacity_bytes * 8U));
        uint8_t _sat2_ = obj->status;
        if (_sat2_ > 15U)
        {
            _sat2_ = 15U;
        }
        buffer[offset_bits / 8U] = (uint8_t)(_sat2_);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 4U;
    }
    if (offset_bits % 8U != 0U)  // Pad to 8 bits. TODO: Eliminate redundant padding checks.
    {
        const uint8_t _pad1_ = (uint8_t)(8U - offset_bits % 8U);
        NUNAVUT_ASSERT(_pad1_ > 0);
        const int8_t _err2_ = nunavutSetUxx(&buffer[0], capacity_bytes, offset_bits, 0U, _pad1_);  // Optimize?
        if (_err2_ < 0)
        {
            return _err2_;
        }
        offset_bits += _pad1_;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    }
    {   // spacecraft.subject.NavigationData.1.0[<=1] navigation
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 1144ULL) <= (capacity_bytes * 8U));
        if (obj->navigation.count > 1)
        {
            return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
        }
        // Array length prefix: truncated uint8
        buffer[offset_bits / 8U] = (uint8_t)(obj->navigation.count);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        for (size_t _index0_ = 0U; _index0_ < obj->navigation.count; ++_index0_)
        {
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT((offset_bits + 1136ULL) <= (capacity_bytes * 8U));
            size_t _size_bytes0_ = 142UL;  // Nested object (max) size, in bytes.
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT((offset_bits / 8U + _size_bytes0_) <= capacity_bytes);
            int8_t _err3_ = spacecraft_subject_NavigationData_1_0_serialize_(
                &obj->navigation.elements[_index0_], &buffer[offset_bits / 8U], &_size_bytes0_);
            if (_err3_ < 0)
            {
                return _err3_;
            }
            // It is assumed that we know the exact type of the serialized entity, hence we expect the size to match.
            NUNAVUT_ASSERT((_size_bytes0_ * 8U) == 1136ULL);
            offset_bits += _size_bytes0_ * 8U;  // Advance by the size of the nested object.
            NUNAVUT_ASSERT(offset_bits <= (capacity_bytes * 8U));
        }
    }
    if (offset_bits % 8U != 0U)  // Pad to 8 bits. TODO: Eliminate redundant padding checks.
    {
        const uint8_t _pad2_ = (uint8_t)(8U - offset_bits % 8U);
        NUNAVUT_ASSERT(_pad2_ > 0);
        const int8_t _err4_ = nunavutSetUxx(&buffer[0], capacity_bytes, offset_bits, 0U, _pad2_);  // Optimize?
        if (_err4_ < 0)
        {
            return _err4_;
        }
        offset_bits += _pad2_;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    }
    {   // spacecraft.subject.SunSensor.1.0[<=4] sun_sensors
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 808ULL) <= (capacity_bytes * 8U));
        if (obj->sun_sensors.count > 4)
        {
            return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
        }
        // Array length prefix: truncated uint8
        buffer[offset_bits / 8U] = (uint8_t)(obj->sun_sensors.count);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        for (size_t _index1_ = 0U; _index1_ < obj->sun_sensors.count; ++_index1_)
        {
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT((offset_bits + 200ULL) <= (capacity_bytes * 8U));
            size_t _size_bytes1_ = 25UL;  // Nested object (max) size, in bytes.
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT((offset_bits / 8U + _size_bytes1_) <= capacity_bytes);
            int8_t _err5_ = spacecraft_subject_SunSensor_1_0_serialize_(
                &obj->sun_sensors.elements[_index1_], &buffer[offset_bits / 8U], &_size_bytes1_);
            if (_err5_ < 0)
            {
                return _err5_;
            }
            // It is assumed that we know the exact type of the serialized entity, hence we expect the size to match.
            NUNAVUT_ASSERT((_size_bytes1_ * 8U) == 200ULL);
            offset_bits += _size_bytes1_ * 8U;  // Advance by the size of the nested object.
            NUNAVUT_ASSERT(offset_bits <= (capacity_bytes * 8U));
        }
    }
    if (offset_bits % 8U != 0U)  // Pad to 8 bits. TODO: Eliminate redundant padding checks.
    {
        const uint8_t _pad3_ = (uint8_t)(8U - offset_bits % 8U);
        NUNAVUT_ASSERT(_pad3_ > 0);
        const int8_t _err6_ = nunavutSetUxx(&buffer[0], capacity_bytes, offset_bits, 0U, _pad3_);  // Optimize?
        if (_err6_ < 0)
        {
            return _err6_;
        }
        offset_bits += _pad3_;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    }
    {   // spacecraft.subject.Magnitometer.1.0[<=6] magnitometer
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 632ULL) <= (capacity_bytes * 8U));
        if (obj->magnitometer.count > 6)
        {
            return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
        }
        // Array length prefix: truncated uint8
        buffer[offset_bits / 8U] = (uint8_t)(obj->magnitometer.count);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        for (size_t _index2_ = 0U; _index2_ < obj->magnitometer.count; ++_index2_)
        {
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT((offset_bits + 104ULL) <= (capacity_bytes * 8U));
            size_t _size_bytes2_ = 13UL;  // Nested object (max) size, in bytes.
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT((offset_bits / 8U + _size_bytes2_) <= capacity_bytes);
            int8_t _err7_ = spacecraft_subject_Magnitometer_1_0_serialize_(
                &obj->magnitometer.elements[_index2_], &buffer[offset_bits / 8U], &_size_bytes2_);
            if (_err7_ < 0)
            {
                return _err7_;
            }
            // It is assumed that we know the exact type of the serialized entity, hence we expect the size to match.
            NUNAVUT_ASSERT((_size_bytes2_ * 8U) == 104ULL);
            offset_bits += _size_bytes2_ * 8U;  // Advance by the size of the nested object.
            NUNAVUT_ASSERT(offset_bits <= (capacity_bytes * 8U));
        }
    }
    if (offset_bits % 8U != 0U)  // Pad to 8 bits. TODO: Eliminate redundant padding checks.
    {
        const uint8_t _pad4_ = (uint8_t)(8U - offset_bits % 8U);
        NUNAVUT_ASSERT(_pad4_ > 0);
        const int8_t _err8_ = nunavutSetUxx(&buffer[0], capacity_bytes, offset_bits, 0U, _pad4_);  // Optimize?
        if (_err8_ < 0)
        {
            return _err8_;
        }
        offset_bits += _pad4_;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    }
    {   // spacecraft.subject.Gyroscope.1.0[<=6] gyroscope
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 1208ULL) <= (capacity_bytes * 8U));
        if (obj->gyroscope.count > 6)
        {
            return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
        }
        // Array length prefix: truncated uint8
        buffer[offset_bits / 8U] = (uint8_t)(obj->gyroscope.count);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        for (size_t _index3_ = 0U; _index3_ < obj->gyroscope.count; ++_index3_)
        {
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT((offset_bits + 200ULL) <= (capacity_bytes * 8U));
            size_t _size_bytes3_ = 25UL;  // Nested object (max) size, in bytes.
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT((offset_bits / 8U + _size_bytes3_) <= capacity_bytes);
            int8_t _err9_ = spacecraft_subject_Gyroscope_1_0_serialize_(
                &obj->gyroscope.elements[_index3_], &buffer[offset_bits / 8U], &_size_bytes3_);
            if (_err9_ < 0)
            {
                return _err9_;
            }
            // It is assumed that we know the exact type of the serialized entity, hence we expect the size to match.
            NUNAVUT_ASSERT((_size_bytes3_ * 8U) == 200ULL);
            offset_bits += _size_bytes3_ * 8U;  // Advance by the size of the nested object.
            NUNAVUT_ASSERT(offset_bits <= (capacity_bytes * 8U));
        }
    }
    if (offset_bits % 8U != 0U)  // Pad to 8 bits. TODO: Eliminate redundant padding checks.
    {
        const uint8_t _pad5_ = (uint8_t)(8U - offset_bits % 8U);
        NUNAVUT_ASSERT(_pad5_ > 0);
        const int8_t _err10_ = nunavutSetUxx(&buffer[0], capacity_bytes, offset_bits, 0U, _pad5_);  // Optimize?
        if (_err10_ < 0)
        {
            return _err10_;
        }
        offset_bits += _pad5_;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    }
    {   // spacecraft.subject.Temperature.1.0[<=6] temperature
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 248ULL) <= (capacity_bytes * 8U));
        if (obj->temperature.count > 6)
        {
            return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
        }
        // Array length prefix: truncated uint8
        buffer[offset_bits / 8U] = (uint8_t)(obj->temperature.count);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        for (size_t _index4_ = 0U; _index4_ < obj->temperature.count; ++_index4_)
        {
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT((offset_bits + 40ULL) <= (capacity_bytes * 8U));
            size_t _size_bytes4_ = 5UL;  // Nested object (max) size, in bytes.
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT((offset_bits / 8U + _size_bytes4_) <= capacity_bytes);
            int8_t _err11_ = spacecraft_subject_Temperature_1_0_serialize_(
                &obj->temperature.elements[_index4_], &buffer[offset_bits / 8U], &_size_bytes4_);
            if (_err11_ < 0)
            {
                return _err11_;
            }
            // It is assumed that we know the exact type of the serialized entity, hence we expect the size to match.
            NUNAVUT_ASSERT((_size_bytes4_ * 8U) == 40ULL);
            offset_bits += _size_bytes4_ * 8U;  // Advance by the size of the nested object.
            NUNAVUT_ASSERT(offset_bits <= (capacity_bytes * 8U));
        }
    }
    if (offset_bits % 8U != 0U)  // Pad to 8 bits. TODO: Eliminate redundant padding checks.
    {
        const uint8_t _pad6_ = (uint8_t)(8U - offset_bits % 8U);
        NUNAVUT_ASSERT(_pad6_ > 0);
        const int8_t _err12_ = nunavutSetUxx(&buffer[0], capacity_bytes, offset_bits, 0U, _pad6_);  // Optimize?
        if (_err12_ < 0)
        {
            return _err12_;
        }
        offset_bits += _pad6_;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    }
    // It is assumed that we know the exact type of the serialized entity, hence we expect the size to match.
    NUNAVUT_ASSERT(offset_bits >= 48ULL);
    NUNAVUT_ASSERT(offset_bits <= 4048ULL);
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    *inout_buffer_size_bytes = (size_t) (offset_bits / 8U);
    return NUNAVUT_SUCCESS;
}

/// Deserialize an instance from the provided buffer.
/// The lifetime of the resulting object is independent of the original buffer.
/// This method may be slow for large objects (e.g., images, point clouds, radar samples), so in a later revision
/// we may define a zero-copy alternative that keeps references to the original buffer where possible.
///
/// @param obj      The object to update from the provided serialized representation.
///
/// @param buffer   The source buffer containing the serialized representation. There are no alignment requirements.
///                 If the buffer is shorter or longer than expected, it will be implicitly zero-extended or truncated,
///                 respectively; see Specification for "implicit zero extension" and "implicit truncation" rules.
///
/// @param inout_buffer_size_bytes  When calling, this is a pointer to the size of the supplied serialized
///                                 representation, in bytes. Upon return this value will be updated with the
///                                 size of the consumed fragment of the serialized representation (in bytes),
///                                 which may be smaller due to the implicit truncation rule, but it is guaranteed
///                                 to never exceed the original buffer size even if the implicit zero extension rule
///                                 was activated. In case of error this value is undefined.
///
/// @returns Negative on error, zero on success.
static inline int8_t spacecraft_ADCS_Response_1_0_deserialize_(
    spacecraft_ADCS_Response_1_0* const out_obj, const uint8_t* buffer, size_t* const inout_buffer_size_bytes)
{
    if ((out_obj == NULL) || (inout_buffer_size_bytes == NULL) || ((buffer == NULL) && (0 != *inout_buffer_size_bytes)))
    {
        return -NUNAVUT_ERROR_INVALID_ARGUMENT;
    }
    if (buffer == NULL)
    {
        buffer = (const uint8_t*)"";
    }
    const size_t capacity_bytes = *inout_buffer_size_bytes;
    const size_t capacity_bits = capacity_bytes * (size_t) 8U;
    size_t offset_bits = 0U;
    // saturated uint4 status
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    if ((offset_bits + 4U) <= capacity_bits)
    {
        out_obj->status = buffer[offset_bits / 8U] & 15U;
    }
    else
    {
        out_obj->status = 0U;
    }
    offset_bits += 4U;
    offset_bits = (offset_bits + 7U) & ~(size_t) 7U;  // Align on 8 bits.
    // spacecraft.subject.NavigationData.1.0[<=1] navigation
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    // Array length prefix: truncated uint8
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->navigation.count = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->navigation.count = 0U;
    }
    offset_bits += 8U;
    if (out_obj->navigation.count > 1U)
    {
        return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
    }
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    for (size_t _index5_ = 0U; _index5_ < out_obj->navigation.count; ++_index5_)
    {
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        {
            size_t _size_bytes5_ = (size_t)(capacity_bytes - nunavutChooseMin((offset_bits / 8U), capacity_bytes));
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            const int8_t _err13_ = spacecraft_subject_NavigationData_1_0_deserialize_(
                &out_obj->navigation.elements[_index5_], &buffer[offset_bits / 8U], &_size_bytes5_);
            if (_err13_ < 0)
            {
                return _err13_;
            }
            offset_bits += _size_bytes5_ * 8U;  // Advance by the size of the nested serialized representation.
        }
    }
    offset_bits = (offset_bits + 7U) & ~(size_t) 7U;  // Align on 8 bits.
    // spacecraft.subject.SunSensor.1.0[<=4] sun_sensors
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    // Array length prefix: truncated uint8
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->sun_sensors.count = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->sun_sensors.count = 0U;
    }
    offset_bits += 8U;
    if (out_obj->sun_sensors.count > 4U)
    {
        return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
    }
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    for (size_t _index6_ = 0U; _index6_ < out_obj->sun_sensors.count; ++_index6_)
    {
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        {
            size_t _size_bytes6_ = (size_t)(capacity_bytes - nunavutChooseMin((offset_bits / 8U), capacity_bytes));
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            const int8_t _err14_ = spacecraft_subject_SunSensor_1_0_deserialize_(
                &out_obj->sun_sensors.elements[_index6_], &buffer[offset_bits / 8U], &_size_bytes6_);
            if (_err14_ < 0)
            {
                return _err14_;
            }
            offset_bits += _size_bytes6_ * 8U;  // Advance by the size of the nested serialized representation.
        }
    }
    offset_bits = (offset_bits + 7U) & ~(size_t) 7U;  // Align on 8 bits.
    // spacecraft.subject.Magnitometer.1.0[<=6] magnitometer
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    // Array length prefix: truncated uint8
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->magnitometer.count = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->magnitometer.count = 0U;
    }
    offset_bits += 8U;
    if (out_obj->magnitometer.count > 6U)
    {
        return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
    }
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    for (size_t _index7_ = 0U; _index7_ < out_obj->magnitometer.count; ++_index7_)
    {
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        {
            size_t _size_bytes7_ = (size_t)(capacity_bytes - nunavutChooseMin((offset_bits / 8U), capacity_bytes));
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            const int8_t _err15_ = spacecraft_subject_Magnitometer_1_0_deserialize_(
                &out_obj->magnitometer.elements[_index7_], &buffer[offset_bits / 8U], &_size_bytes7_);
            if (_err15_ < 0)
            {
                return _err15_;
            }
            offset_bits += _size_bytes7_ * 8U;  // Advance by the size of the nested serialized representation.
        }
    }
    offset_bits = (offset_bits + 7U) & ~(size_t) 7U;  // Align on 8 bits.
    // spacecraft.subject.Gyroscope.1.0[<=6] gyroscope
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    // Array length prefix: truncated uint8
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->gyroscope.count = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->gyroscope.count = 0U;
    }
    offset_bits += 8U;
    if (out_obj->gyroscope.count > 6U)
    {
        return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
    }
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    for (size_t _index8_ = 0U; _index8_ < out_obj->gyroscope.count; ++_index8_)
    {
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        {
            size_t _size_bytes8_ = (size_t)(capacity_bytes - nunavutChooseMin((offset_bits / 8U), capacity_bytes));
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            const int8_t _err16_ = spacecraft_subject_Gyroscope_1_0_deserialize_(
                &out_obj->gyroscope.elements[_index8_], &buffer[offset_bits / 8U], &_size_bytes8_);
            if (_err16_ < 0)
            {
                return _err16_;
            }
            offset_bits += _size_bytes8_ * 8U;  // Advance by the size of the nested serialized representation.
        }
    }
    offset_bits = (offset_bits + 7U) & ~(size_t) 7U;  // Align on 8 bits.
    // spacecraft.subject.Temperature.1.0[<=6] temperature
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    // Array length prefix: truncated uint8
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->temperature.count = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->temperature.count = 0U;
    }
    offset_bits += 8U;
    if (out_obj->temperature.count > 6U)
    {
        return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
    }
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    for (size_t _index9_ = 0U; _index9_ < out_obj->temperature.count; ++_index9_)
    {
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        {
            size_t _size_bytes9_ = (size_t)(capacity_bytes - nunavutChooseMin((offset_bits / 8U), capacity_bytes));
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            const int8_t _err17_ = spacecraft_subject_Temperature_1_0_deserialize_(
                &out_obj->temperature.elements[_index9_], &buffer[offset_bits / 8U], &_size_bytes9_);
            if (_err17_ < 0)
            {
                return _err17_;
            }
            offset_bits += _size_bytes9_ * 8U;  // Advance by the size of the nested serialized representation.
        }
    }
    offset_bits = (offset_bits + 7U) & ~(size_t) 7U;  // Align on 8 bits.
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    *inout_buffer_size_bytes = (size_t) (nunavutChooseMin(offset_bits, capacity_bits) / 8U);
    NUNAVUT_ASSERT(capacity_bytes >= *inout_buffer_size_bytes);
    return NUNAVUT_SUCCESS;
}

/// Initialize an instance to default values. Does nothing if @param out_obj is NULL.
/// This function intentionally leaves inactive elements uninitialized; for example, members of a variable-length
/// array beyond its length are left uninitialized; aliased union memory that is not used by the first union field
/// is left uninitialized, etc. If full zero-initialization is desired, just use memset(&obj, 0, sizeof(obj)).
static inline void spacecraft_ADCS_Response_1_0_initialize_(spacecraft_ADCS_Response_1_0* const out_obj)
{
    if (out_obj != NULL)
    {
        size_t size_bytes = 0;
        const uint8_t buf = 0;
        const int8_t err = spacecraft_ADCS_Response_1_0_deserialize_(out_obj, &buf, &size_bytes);
        NUNAVUT_ASSERT(err >= 0);
        (void) err;
    }
}

#ifdef __cplusplus
}
#endif
#endif // SPACECRAFT_ADCS_1_0_INCLUDED_
