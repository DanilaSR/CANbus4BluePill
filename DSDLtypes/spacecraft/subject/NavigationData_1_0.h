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
// Source file:   C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/subject/NavigationData.1.0.dsdl
// Generated at:  2026-02-15 22:07:50.799973 UTC
// Is deprecated: no
// Fixed port-ID: None
// Full name:     spacecraft.subject.NavigationData
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

#ifndef SPACECRAFT_SUBJECT_NAVIGATION_DATA_1_0_INCLUDED_
#define SPACECRAFT_SUBJECT_NAVIGATION_DATA_1_0_INCLUDED_

#include <nunavut/support/serialization.h>
#include <stdint.h>
#include <stdlib.h>

static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_TARGET_ENDIANNESS == 434322821,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/subject/NavigationData.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );
static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_OMIT_FLOAT_SERIALIZATION_SUPPORT == 0,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/subject/NavigationData.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );
static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_ENABLE_SERIALIZATION_ASSERTS == 1,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/subject/NavigationData.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );
static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_ENABLE_OVERRIDE_VARIABLE_ARRAY_CAPACITY == 0,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/subject/NavigationData.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );
static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_CAST_FORMAT == 2368206204,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/subject/NavigationData.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );

#ifdef __cplusplus
extern "C" {
#endif

/// This type does not have a fixed port-ID. See https://forum.opencyphal.org/t/choosing-message-and-service-ids/889
#define spacecraft_subject_NavigationData_1_0_HAS_FIXED_PORT_ID_ false

// +-------------------------------------------------------------------------------------------------------------------+
// | spacecraft.subject.NavigationData.1.0
// +-------------------------------------------------------------------------------------------------------------------+
#define spacecraft_subject_NavigationData_1_0_FULL_NAME_             "spacecraft.subject.NavigationData"
#define spacecraft_subject_NavigationData_1_0_FULL_NAME_AND_VERSION_ "spacecraft.subject.NavigationData.1.0"

/// Extent is the minimum amount of memory required to hold any serialized representation of any compatible
/// version of the data type; or, on other words, it is the the maximum possible size of received objects of this type.
/// The size is specified in bytes (rather than bits) because by definition, extent is an integer number of bytes long.
/// When allocating a deserialization (RX) buffer for this data type, it should be at least extent bytes large.
/// When allocating a serialization (TX) buffer, it is safe to use the size of the largest serialized representation
/// instead of the extent because it provides a tighter bound of the object size; it is safe because the concrete type
/// is always known during serialization (unlike deserialization). If not sure, use extent everywhere.
#define spacecraft_subject_NavigationData_1_0_EXTENT_BYTES_                    142UL
#define spacecraft_subject_NavigationData_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_ 142UL
static_assert(spacecraft_subject_NavigationData_1_0_EXTENT_BYTES_ >= spacecraft_subject_NavigationData_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_,
              "Internal constraint violation");

typedef struct
{
    /// saturated float64 x
    double x;

    /// saturated float64 y
    double y;

    /// saturated float64 z
    double z;

    /// saturated float64 vx
    double vx;

    /// saturated float64 vy
    double vy;

    /// saturated float64 vz
    double vz;

    /// saturated uint32 channels_mask
    uint32_t channels_mask;

    /// saturated uint32 diff_correction_mask
    uint32_t diff_correction_mask;

    /// saturated int32 year
    int32_t year;

    /// saturated int32 month
    int32_t month;

    /// saturated int32 day
    int32_t day;

    /// saturated int32 hour
    int32_t hour;

    /// saturated int32 minute
    int32_t minute;

    /// saturated int32 second
    int32_t second;

    /// saturated uint32 internal_time
    uint32_t _internal_time;

    /// saturated float64 filtered_x
    double filtered_x;

    /// saturated float64 filtered_y
    double filtered_y;

    /// saturated float64 filtered_z
    double filtered_z;

    /// saturated float64 filtered_vx
    double filtered_vx;

    /// saturated float64 filtered_vy
    double filtered_vy;

    /// saturated float64 filtered_vz
    double filtered_vz;

    /// saturated uint32 invalid_measures
    uint32_t invalid_measures;

    /// saturated uint32 last_fix
    uint32_t last_fix;

    /// saturated uint8 is_syncro
    uint8_t is_syncro;

    /// saturated uint8 is_valid
    uint8_t is_valid;
} spacecraft_subject_NavigationData_1_0;

/// Serialize an instance into the provided buffer.
/// The lifetime of the resulting serialized representation is independent of the original instance.
/// This method may be slow for large objects (e.g., images, point clouds, radar samples), so in a later revision
/// we may define a zero-copy alternative that keeps references to the original object where possible.
///
/// @param obj      The object to serialize.
///
/// @param buffer   The destination buffer. There are no alignment requirements.
///                 @see spacecraft_subject_NavigationData_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_
///
/// @param inout_buffer_size_bytes  When calling, this is a pointer to the size of the buffer in bytes.
///                                 Upon return this value will be updated with the size of the constructed serialized
///                                 representation (in bytes); this value is then to be passed over to the transport
///                                 layer. In case of error this value is undefined.
///
/// @returns Negative on error, zero on success.
static inline int8_t spacecraft_subject_NavigationData_1_0_serialize_(
    const spacecraft_subject_NavigationData_1_0* const obj, uint8_t* const buffer,  size_t* const inout_buffer_size_bytes)
{
    if ((obj == NULL) || (buffer == NULL) || (inout_buffer_size_bytes == NULL))
    {
        return -NUNAVUT_ERROR_INVALID_ARGUMENT;
    }
    const size_t capacity_bytes = *inout_buffer_size_bytes;
    if ((8U * (size_t) capacity_bytes) < 1136UL)
    {
        return -NUNAVUT_ERROR_SERIALIZATION_BUFFER_TOO_SMALL;
    }
    // Notice that fields that are not an integer number of bytes long may overrun the space allocated for them
    // in the serialization buffer up to the next byte boundary. This is by design and is guaranteed to be safe.
    size_t offset_bits = 0U;
    {   // saturated float64 x
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 64ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- assume the native representation of float64 is conformant.
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        (void) memmove(&buffer[offset_bits / 8U], &obj->x, 8U);
        offset_bits += 64U;
    }
    {   // saturated float64 y
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 64ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- assume the native representation of float64 is conformant.
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        (void) memmove(&buffer[offset_bits / 8U], &obj->y, 8U);
        offset_bits += 64U;
    }
    {   // saturated float64 z
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 64ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- assume the native representation of float64 is conformant.
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        (void) memmove(&buffer[offset_bits / 8U], &obj->z, 8U);
        offset_bits += 64U;
    }
    {   // saturated float64 vx
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 64ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- assume the native representation of float64 is conformant.
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        (void) memmove(&buffer[offset_bits / 8U], &obj->vx, 8U);
        offset_bits += 64U;
    }
    {   // saturated float64 vy
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 64ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- assume the native representation of float64 is conformant.
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        (void) memmove(&buffer[offset_bits / 8U], &obj->vy, 8U);
        offset_bits += 64U;
    }
    {   // saturated float64 vz
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 64ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- assume the native representation of float64 is conformant.
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        (void) memmove(&buffer[offset_bits / 8U], &obj->vz, 8U);
        offset_bits += 64U;
    }
    {   // saturated uint32 channels_mask
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 32ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        (void) memmove(&buffer[offset_bits / 8U], &obj->channels_mask, 4U);
        offset_bits += 32U;
    }
    {   // saturated uint32 diff_correction_mask
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 32ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        (void) memmove(&buffer[offset_bits / 8U], &obj->diff_correction_mask, 4U);
        offset_bits += 32U;
    }
    {   // saturated int32 year
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 32ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        (void) memmove(&buffer[offset_bits / 8U], &obj->year, 4U);
        offset_bits += 32U;
    }
    {   // saturated int32 month
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 32ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        (void) memmove(&buffer[offset_bits / 8U], &obj->month, 4U);
        offset_bits += 32U;
    }
    {   // saturated int32 day
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 32ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        (void) memmove(&buffer[offset_bits / 8U], &obj->day, 4U);
        offset_bits += 32U;
    }
    {   // saturated int32 hour
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 32ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        (void) memmove(&buffer[offset_bits / 8U], &obj->hour, 4U);
        offset_bits += 32U;
    }
    {   // saturated int32 minute
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 32ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        (void) memmove(&buffer[offset_bits / 8U], &obj->minute, 4U);
        offset_bits += 32U;
    }
    {   // saturated int32 second
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 32ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        (void) memmove(&buffer[offset_bits / 8U], &obj->second, 4U);
        offset_bits += 32U;
    }
    {   // saturated uint32 internal_time
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 32ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        (void) memmove(&buffer[offset_bits / 8U], &obj->_internal_time, 4U);
        offset_bits += 32U;
    }
    {   // saturated float64 filtered_x
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 64ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- assume the native representation of float64 is conformant.
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        (void) memmove(&buffer[offset_bits / 8U], &obj->filtered_x, 8U);
        offset_bits += 64U;
    }
    {   // saturated float64 filtered_y
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 64ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- assume the native representation of float64 is conformant.
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        (void) memmove(&buffer[offset_bits / 8U], &obj->filtered_y, 8U);
        offset_bits += 64U;
    }
    {   // saturated float64 filtered_z
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 64ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- assume the native representation of float64 is conformant.
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        (void) memmove(&buffer[offset_bits / 8U], &obj->filtered_z, 8U);
        offset_bits += 64U;
    }
    {   // saturated float64 filtered_vx
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 64ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- assume the native representation of float64 is conformant.
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        (void) memmove(&buffer[offset_bits / 8U], &obj->filtered_vx, 8U);
        offset_bits += 64U;
    }
    {   // saturated float64 filtered_vy
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 64ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- assume the native representation of float64 is conformant.
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        (void) memmove(&buffer[offset_bits / 8U], &obj->filtered_vy, 8U);
        offset_bits += 64U;
    }
    {   // saturated float64 filtered_vz
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 64ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- assume the native representation of float64 is conformant.
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        static_assert(NUNAVUT_PLATFORM_IEEE754_DOUBLE, "Native IEEE754 binary64 required. TODO: relax constraint");
        (void) memmove(&buffer[offset_bits / 8U], &obj->filtered_vz, 8U);
        offset_bits += 64U;
    }
    {   // saturated uint32 invalid_measures
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 32ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        (void) memmove(&buffer[offset_bits / 8U], &obj->invalid_measures, 4U);
        offset_bits += 32U;
    }
    {   // saturated uint32 last_fix
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 32ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        (void) memmove(&buffer[offset_bits / 8U], &obj->last_fix, 4U);
        offset_bits += 32U;
    }
    {   // saturated uint8 is_syncro
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 8ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        buffer[offset_bits / 8U] = (uint8_t)(obj->is_syncro);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
    }
    {   // saturated uint8 is_valid
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 8ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        buffer[offset_bits / 8U] = (uint8_t)(obj->is_valid);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
    }
    if (offset_bits % 8U != 0U)  // Pad to 8 bits. TODO: Eliminate redundant padding checks.
    {
        const uint8_t _pad0_ = (uint8_t)(8U - offset_bits % 8U);
        NUNAVUT_ASSERT(_pad0_ > 0);
        const int8_t _err0_ = nunavutSetUxx(&buffer[0], capacity_bytes, offset_bits, 0U, _pad0_);  // Optimize?
        if (_err0_ < 0)
        {
            return _err0_;
        }
        offset_bits += _pad0_;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    }
    // It is assumed that we know the exact type of the serialized entity, hence we expect the size to match.
    NUNAVUT_ASSERT(offset_bits == 1136ULL);
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
static inline int8_t spacecraft_subject_NavigationData_1_0_deserialize_(
    spacecraft_subject_NavigationData_1_0* const out_obj, const uint8_t* buffer, size_t* const inout_buffer_size_bytes)
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
    // saturated float64 x
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->x = nunavutGetF64(&buffer[0], capacity_bytes, offset_bits);
    offset_bits += 64U;
    // saturated float64 y
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->y = nunavutGetF64(&buffer[0], capacity_bytes, offset_bits);
    offset_bits += 64U;
    // saturated float64 z
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->z = nunavutGetF64(&buffer[0], capacity_bytes, offset_bits);
    offset_bits += 64U;
    // saturated float64 vx
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->vx = nunavutGetF64(&buffer[0], capacity_bytes, offset_bits);
    offset_bits += 64U;
    // saturated float64 vy
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->vy = nunavutGetF64(&buffer[0], capacity_bytes, offset_bits);
    offset_bits += 64U;
    // saturated float64 vz
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->vz = nunavutGetF64(&buffer[0], capacity_bytes, offset_bits);
    offset_bits += 64U;
    // saturated uint32 channels_mask
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->channels_mask = nunavutGetU32(&buffer[0], capacity_bytes, offset_bits, 32);
    offset_bits += 32U;
    // saturated uint32 diff_correction_mask
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->diff_correction_mask = nunavutGetU32(&buffer[0], capacity_bytes, offset_bits, 32);
    offset_bits += 32U;
    // saturated int32 year
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->year = nunavutGetI32(&buffer[0], capacity_bytes, offset_bits, 32);
    offset_bits += 32U;
    // saturated int32 month
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->month = nunavutGetI32(&buffer[0], capacity_bytes, offset_bits, 32);
    offset_bits += 32U;
    // saturated int32 day
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->day = nunavutGetI32(&buffer[0], capacity_bytes, offset_bits, 32);
    offset_bits += 32U;
    // saturated int32 hour
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->hour = nunavutGetI32(&buffer[0], capacity_bytes, offset_bits, 32);
    offset_bits += 32U;
    // saturated int32 minute
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->minute = nunavutGetI32(&buffer[0], capacity_bytes, offset_bits, 32);
    offset_bits += 32U;
    // saturated int32 second
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->second = nunavutGetI32(&buffer[0], capacity_bytes, offset_bits, 32);
    offset_bits += 32U;
    // saturated uint32 internal_time
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->_internal_time = nunavutGetU32(&buffer[0], capacity_bytes, offset_bits, 32);
    offset_bits += 32U;
    // saturated float64 filtered_x
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->filtered_x = nunavutGetF64(&buffer[0], capacity_bytes, offset_bits);
    offset_bits += 64U;
    // saturated float64 filtered_y
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->filtered_y = nunavutGetF64(&buffer[0], capacity_bytes, offset_bits);
    offset_bits += 64U;
    // saturated float64 filtered_z
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->filtered_z = nunavutGetF64(&buffer[0], capacity_bytes, offset_bits);
    offset_bits += 64U;
    // saturated float64 filtered_vx
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->filtered_vx = nunavutGetF64(&buffer[0], capacity_bytes, offset_bits);
    offset_bits += 64U;
    // saturated float64 filtered_vy
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->filtered_vy = nunavutGetF64(&buffer[0], capacity_bytes, offset_bits);
    offset_bits += 64U;
    // saturated float64 filtered_vz
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->filtered_vz = nunavutGetF64(&buffer[0], capacity_bytes, offset_bits);
    offset_bits += 64U;
    // saturated uint32 invalid_measures
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->invalid_measures = nunavutGetU32(&buffer[0], capacity_bytes, offset_bits, 32);
    offset_bits += 32U;
    // saturated uint32 last_fix
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    out_obj->last_fix = nunavutGetU32(&buffer[0], capacity_bytes, offset_bits, 32);
    offset_bits += 32U;
    // saturated uint8 is_syncro
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->is_syncro = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->is_syncro = 0U;
    }
    offset_bits += 8U;
    // saturated uint8 is_valid
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->is_valid = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->is_valid = 0U;
    }
    offset_bits += 8U;
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
static inline void spacecraft_subject_NavigationData_1_0_initialize_(spacecraft_subject_NavigationData_1_0* const out_obj)
{
    if (out_obj != NULL)
    {
        size_t size_bytes = 0;
        const uint8_t buf = 0;
        const int8_t err = spacecraft_subject_NavigationData_1_0_deserialize_(out_obj, &buf, &size_bytes);
        NUNAVUT_ASSERT(err >= 0);
        (void) err;
    }
}

#ifdef __cplusplus
}
#endif
#endif // SPACECRAFT_SUBJECT_NAVIGATION_DATA_1_0_INCLUDED_
