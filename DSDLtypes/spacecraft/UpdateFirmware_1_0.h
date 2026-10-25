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
// Source file:   C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/373.UpdateFirmware.1.0.dsdl
// Generated at:  2026-02-15 22:07:50.761033 UTC
// Is deprecated: no
// Fixed port-ID: 373
// Full name:     spacecraft.UpdateFirmware
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

#ifndef SPACECRAFT_UPDATE_FIRMWARE_1_0_INCLUDED_
#define SPACECRAFT_UPDATE_FIRMWARE_1_0_INCLUDED_

#include <nunavut/support/serialization.h>
#include <spacecraft/SectorEraseStruct_1_0.h>
#include <stdint.h>
#include <stdlib.h>

static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_TARGET_ENDIANNESS == 434322821,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/373.UpdateFirmware.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );
static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_OMIT_FLOAT_SERIALIZATION_SUPPORT == 0,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/373.UpdateFirmware.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );
static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_ENABLE_SERIALIZATION_ASSERTS == 1,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/373.UpdateFirmware.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );
static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_ENABLE_OVERRIDE_VARIABLE_ARRAY_CAPACITY == 0,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/373.UpdateFirmware.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );
static_assert( NUNAVUT_SUPPORT_LANGUAGE_OPTION_CAST_FORMAT == 2368206204,
              "C:/spacecraft_plats/gui_spacecraft/dsdl/spacecraft/373.UpdateFirmware.1.0.dsdl is trying to use a serialization library that was compiled with "
              "different language options. This is dangerous and therefore not allowed." );

#ifdef __cplusplus
extern "C" {
#endif

#define spacecraft_UpdateFirmware_1_0_HAS_FIXED_PORT_ID_ true
#define spacecraft_UpdateFirmware_1_0_FIXED_PORT_ID_     373U

// +-------------------------------------------------------------------------------------------------------------------+
// | spacecraft.UpdateFirmware.1.0
// +-------------------------------------------------------------------------------------------------------------------+
#define spacecraft_UpdateFirmware_1_0_FULL_NAME_             "spacecraft.UpdateFirmware"
#define spacecraft_UpdateFirmware_1_0_FULL_NAME_AND_VERSION_ "spacecraft.UpdateFirmware.1.0"

// +-------------------------------------------------------------------------------------------------------------------+
// | spacecraft.UpdateFirmware.Request.1.0
// +-------------------------------------------------------------------------------------------------------------------+
#define spacecraft_UpdateFirmware_Request_1_0_FULL_NAME_             "spacecraft.UpdateFirmware.Request"
#define spacecraft_UpdateFirmware_Request_1_0_FULL_NAME_AND_VERSION_ "spacecraft.UpdateFirmware.Request.1.0"

/// Extent is the minimum amount of memory required to hold any serialized representation of any compatible
/// version of the data type; or, on other words, it is the the maximum possible size of received objects of this type.
/// The size is specified in bytes (rather than bits) because by definition, extent is an integer number of bytes long.
/// When allocating a deserialization (RX) buffer for this data type, it should be at least extent bytes large.
/// When allocating a serialization (TX) buffer, it is safe to use the size of the largest serialized representation
/// instead of the extent because it provides a tighter bound of the object size; it is safe because the concrete type
/// is always known during serialization (unlike deserialization). If not sure, use extent everywhere.
#define spacecraft_UpdateFirmware_Request_1_0_EXTENT_BYTES_                    164UL
#define spacecraft_UpdateFirmware_Request_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_ 161UL
static_assert(spacecraft_UpdateFirmware_Request_1_0_EXTENT_BYTES_ >= spacecraft_UpdateFirmware_Request_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_,
              "Internal constraint violation");

/// saturated uint8 COMMAND_ALLOCATE_MEMORY = 1
#define spacecraft_UpdateFirmware_Request_1_0_COMMAND_ALLOCATE_MEMORY (1U)

/// saturated uint8 COMMAND_RECEIVE_CHUNK = 2
#define spacecraft_UpdateFirmware_Request_1_0_COMMAND_RECEIVE_CHUNK (2U)

/// saturated uint8 COMMAND_FLASH_CHUNK = 3
#define spacecraft_UpdateFirmware_Request_1_0_COMMAND_FLASH_CHUNK (3U)

/// saturated uint8 COMMAND_JUMP_TO_START = 4
#define spacecraft_UpdateFirmware_Request_1_0_COMMAND_JUMP_TO_START (4U)

/// saturated uint8 COMMAND_ERASE_MEMORY = 5
#define spacecraft_UpdateFirmware_Request_1_0_COMMAND_ERASE_MEMORY (5U)

/// Array metadata for: saturated uint8[<=128] chunk
#define spacecraft_UpdateFirmware_Request_1_0_chunk_ARRAY_CAPACITY_           128U
#define spacecraft_UpdateFirmware_Request_1_0_chunk_ARRAY_IS_VARIABLE_LENGTH_ true

/// Array metadata for: saturated uint32[<=1] offset
#define spacecraft_UpdateFirmware_Request_1_0_offset_ARRAY_CAPACITY_           1U
#define spacecraft_UpdateFirmware_Request_1_0_offset_ARRAY_IS_VARIABLE_LENGTH_ true

/// Array metadata for: saturated uint32[<=1] fw_size
#define spacecraft_UpdateFirmware_Request_1_0_fw_size_ARRAY_CAPACITY_           1U
#define spacecraft_UpdateFirmware_Request_1_0_fw_size_ARRAY_IS_VARIABLE_LENGTH_ true

/// Array metadata for: spacecraft.SectorEraseStruct.1.0[<=1] erase_struct
#define spacecraft_UpdateFirmware_Request_1_0_erase_struct_ARRAY_CAPACITY_           1U
#define spacecraft_UpdateFirmware_Request_1_0_erase_struct_ARRAY_IS_VARIABLE_LENGTH_ true

typedef struct
{
    /// saturated uint8 command
    uint8_t command;

    /// saturated uint8[<=128] chunk
    struct  /// Array address equivalence guarantee: &elements[0] == &chunk
    {
        uint8_t elements[spacecraft_UpdateFirmware_Request_1_0_chunk_ARRAY_CAPACITY_];
        size_t count;
    } chunk;

    /// saturated uint32[<=1] offset
    struct  /// Array address equivalence guarantee: &elements[0] == &offset
    {
        uint32_t elements[spacecraft_UpdateFirmware_Request_1_0_offset_ARRAY_CAPACITY_];
        size_t count;
    } offset;

    /// saturated uint32[<=1] fw_size
    struct  /// Array address equivalence guarantee: &elements[0] == &fw_size
    {
        uint32_t elements[spacecraft_UpdateFirmware_Request_1_0_fw_size_ARRAY_CAPACITY_];
        size_t count;
    } fw_size;

    /// spacecraft.SectorEraseStruct.1.0[<=1] erase_struct
    struct  /// Array address equivalence guarantee: &elements[0] == &erase_struct
    {
        spacecraft_SectorEraseStruct_1_0 elements[spacecraft_UpdateFirmware_Request_1_0_erase_struct_ARRAY_CAPACITY_];
        size_t count;
    } erase_struct;
} spacecraft_UpdateFirmware_Request_1_0;

/// Serialize an instance into the provided buffer.
/// The lifetime of the resulting serialized representation is independent of the original instance.
/// This method may be slow for large objects (e.g., images, point clouds, radar samples), so in a later revision
/// we may define a zero-copy alternative that keeps references to the original object where possible.
///
/// @param obj      The object to serialize.
///
/// @param buffer   The destination buffer. There are no alignment requirements.
///                 @see spacecraft_UpdateFirmware_Request_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_
///
/// @param inout_buffer_size_bytes  When calling, this is a pointer to the size of the buffer in bytes.
///                                 Upon return this value will be updated with the size of the constructed serialized
///                                 representation (in bytes); this value is then to be passed over to the transport
///                                 layer. In case of error this value is undefined.
///
/// @returns Negative on error, zero on success.
static inline int8_t spacecraft_UpdateFirmware_Request_1_0_serialize_(
    const spacecraft_UpdateFirmware_Request_1_0* const obj, uint8_t* const buffer,  size_t* const inout_buffer_size_bytes)
{
    if ((obj == NULL) || (buffer == NULL) || (inout_buffer_size_bytes == NULL))
    {
        return -NUNAVUT_ERROR_INVALID_ARGUMENT;
    }
    const size_t capacity_bytes = *inout_buffer_size_bytes;
    if ((8U * (size_t) capacity_bytes) < 1288UL)
    {
        return -NUNAVUT_ERROR_SERIALIZATION_BUFFER_TOO_SMALL;
    }
    // Notice that fields that are not an integer number of bytes long may overrun the space allocated for them
    // in the serialization buffer up to the next byte boundary. This is by design and is guaranteed to be safe.
    size_t offset_bits = 0U;
    {   // saturated uint8 command
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 8ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        buffer[offset_bits / 8U] = (uint8_t)(obj->command);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
    }
    {   // saturated uint8[<=128] chunk
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 1032ULL) <= (capacity_bytes * 8U));
        if (obj->chunk.count > 128)
        {
            return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
        }
        // Array length prefix: truncated uint8
        buffer[offset_bits / 8U] = (uint8_t)(obj->chunk.count);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        // Optimization prospect: this item is aligned at the byte boundary, so it is possible to use memmove().
        nunavutCopyBits(&buffer[0], offset_bits, obj->chunk.count * 8U, &obj->chunk.elements[0], 0U);
        offset_bits += obj->chunk.count * 8U;
    }
    {   // saturated uint32[<=1] offset
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 40ULL) <= (capacity_bytes * 8U));
        if (obj->offset.count > 1)
        {
            return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
        }
        // Array length prefix: truncated uint8
        buffer[offset_bits / 8U] = (uint8_t)(obj->offset.count);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        // Saturation code not emitted -- assume the native representation is conformant.
        // Optimization prospect: this item is aligned at the byte boundary, so it is possible to use memmove().
        nunavutCopyBits(&buffer[0], offset_bits, obj->offset.count * 32UL, &obj->offset.elements[0], 0U);
        offset_bits += obj->offset.count * 32UL;
    }
    {   // saturated uint32[<=1] fw_size
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 40ULL) <= (capacity_bytes * 8U));
        if (obj->fw_size.count > 1)
        {
            return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
        }
        // Array length prefix: truncated uint8
        buffer[offset_bits / 8U] = (uint8_t)(obj->fw_size.count);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        // Saturation code not emitted -- assume the native representation is conformant.
        // Optimization prospect: this item is aligned at the byte boundary, so it is possible to use memmove().
        nunavutCopyBits(&buffer[0], offset_bits, obj->fw_size.count * 32UL, &obj->fw_size.elements[0], 0U);
        offset_bits += obj->fw_size.count * 32UL;
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
    {   // spacecraft.SectorEraseStruct.1.0[<=1] erase_struct
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 168ULL) <= (capacity_bytes * 8U));
        if (obj->erase_struct.count > 1)
        {
            return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
        }
        // Array length prefix: truncated uint8
        buffer[offset_bits / 8U] = (uint8_t)(obj->erase_struct.count);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        for (size_t _index0_ = 0U; _index0_ < obj->erase_struct.count; ++_index0_)
        {
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT((offset_bits + 160ULL) <= (capacity_bytes * 8U));
            size_t _size_bytes0_ = 16UL;  // Nested object (max) size, in bytes.
            // Constant delimiter header can be written ahead of the nested object.
            (void) memmove(&buffer[offset_bits / 8U], &_size_bytes0_, 4U);
            offset_bits += 32U;
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            NUNAVUT_ASSERT((offset_bits / 8U + _size_bytes0_) <= capacity_bytes);
            int8_t _err1_ = spacecraft_SectorEraseStruct_1_0_serialize_(
                &obj->erase_struct.elements[_index0_], &buffer[offset_bits / 8U], &_size_bytes0_);
            if (_err1_ < 0)
            {
                return _err1_;
            }
            // It is assumed that we know the exact type of the serialized entity, hence we expect the size to match.
            NUNAVUT_ASSERT((_size_bytes0_ * 8U) == 128ULL);
            offset_bits += _size_bytes0_ * 8U;  // Advance by the size of the nested object.
            NUNAVUT_ASSERT(offset_bits <= (capacity_bytes * 8U));
        }
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
    // It is assumed that we know the exact type of the serialized entity, hence we expect the size to match.
    NUNAVUT_ASSERT(offset_bits >= 40ULL);
    NUNAVUT_ASSERT(offset_bits <= 1288ULL);
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
static inline int8_t spacecraft_UpdateFirmware_Request_1_0_deserialize_(
    spacecraft_UpdateFirmware_Request_1_0* const out_obj, const uint8_t* buffer, size_t* const inout_buffer_size_bytes)
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
    // saturated uint8 command
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->command = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->command = 0U;
    }
    offset_bits += 8U;
    // saturated uint8[<=128] chunk
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    // Array length prefix: truncated uint8
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->chunk.count = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->chunk.count = 0U;
    }
    offset_bits += 8U;
    if (out_obj->chunk.count > 128U)
    {
        return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
    }
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    nunavutGetBits(&out_obj->chunk.elements[0], &buffer[0], capacity_bytes, offset_bits, out_obj->chunk.count * 8U);
    offset_bits += out_obj->chunk.count * 8U;
    // saturated uint32[<=1] offset
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    // Array length prefix: truncated uint8
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->offset.count = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->offset.count = 0U;
    }
    offset_bits += 8U;
    if (out_obj->offset.count > 1U)
    {
        return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
    }
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    nunavutGetBits(&out_obj->offset.elements[0], &buffer[0], capacity_bytes, offset_bits, out_obj->offset.count * 32U);
    offset_bits += out_obj->offset.count * 32U;
    // saturated uint32[<=1] fw_size
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    // Array length prefix: truncated uint8
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->fw_size.count = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->fw_size.count = 0U;
    }
    offset_bits += 8U;
    if (out_obj->fw_size.count > 1U)
    {
        return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
    }
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    nunavutGetBits(&out_obj->fw_size.elements[0], &buffer[0], capacity_bytes, offset_bits, out_obj->fw_size.count * 32U);
    offset_bits += out_obj->fw_size.count * 32U;
    offset_bits = (offset_bits + 7U) & ~(size_t) 7U;  // Align on 8 bits.
    // spacecraft.SectorEraseStruct.1.0[<=1] erase_struct
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    // Array length prefix: truncated uint8
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->erase_struct.count = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->erase_struct.count = 0U;
    }
    offset_bits += 8U;
    if (out_obj->erase_struct.count > 1U)
    {
        return -NUNAVUT_ERROR_REPRESENTATION_BAD_ARRAY_LENGTH;
    }
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    for (size_t _index1_ = 0U; _index1_ < out_obj->erase_struct.count; ++_index1_)
    {
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        {
            // Delimiter header: truncated uint32
            size_t _size_bytes1_ = 0U;
            _size_bytes1_ = nunavutGetU32(&buffer[0], capacity_bytes, offset_bits, 32);
            offset_bits += 32U;
            if (_size_bytes1_ > (capacity_bytes - nunavutChooseMin((offset_bits / 8U), capacity_bytes)))
            {
                return -NUNAVUT_ERROR_REPRESENTATION_BAD_DELIMITER_HEADER;
            }
            const size_t _dh0_ = _size_bytes1_;  // Store the original delimiter header value.
            NUNAVUT_ASSERT(offset_bits % 8U == 0U);
            const int8_t _err3_ = spacecraft_SectorEraseStruct_1_0_deserialize_(
                &out_obj->erase_struct.elements[_index1_], &buffer[offset_bits / 8U], &_size_bytes1_);
            if (_err3_ < 0)
            {
                return _err3_;
            }
            // Advance the offset by the size of the delimiter header, even if the nested deserialization routine
            // consumed fewer bytes of data. This behavior implements the implicit truncation rule for nested objects.
            offset_bits += _dh0_ * 8U;
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
static inline void spacecraft_UpdateFirmware_Request_1_0_initialize_(spacecraft_UpdateFirmware_Request_1_0* const out_obj)
{
    if (out_obj != NULL)
    {
        size_t size_bytes = 0;
        const uint8_t buf = 0;
        const int8_t err = spacecraft_UpdateFirmware_Request_1_0_deserialize_(out_obj, &buf, &size_bytes);
        NUNAVUT_ASSERT(err >= 0);
        (void) err;
    }
}

// +-------------------------------------------------------------------------------------------------------------------+
// | spacecraft.UpdateFirmware.Response.1.0
// +-------------------------------------------------------------------------------------------------------------------+
#define spacecraft_UpdateFirmware_Response_1_0_FULL_NAME_             "spacecraft.UpdateFirmware.Response"
#define spacecraft_UpdateFirmware_Response_1_0_FULL_NAME_AND_VERSION_ "spacecraft.UpdateFirmware.Response.1.0"

/// Extent is the minimum amount of memory required to hold any serialized representation of any compatible
/// version of the data type; or, on other words, it is the the maximum possible size of received objects of this type.
/// The size is specified in bytes (rather than bits) because by definition, extent is an integer number of bytes long.
/// When allocating a deserialization (RX) buffer for this data type, it should be at least extent bytes large.
/// When allocating a serialization (TX) buffer, it is safe to use the size of the largest serialized representation
/// instead of the extent because it provides a tighter bound of the object size; it is safe because the concrete type
/// is always known during serialization (unlike deserialization). If not sure, use extent everywhere.
#define spacecraft_UpdateFirmware_Response_1_0_EXTENT_BYTES_                    2UL
#define spacecraft_UpdateFirmware_Response_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_ 1UL
static_assert(spacecraft_UpdateFirmware_Response_1_0_EXTENT_BYTES_ >= spacecraft_UpdateFirmware_Response_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_,
              "Internal constraint violation");

/// saturated uint8 STATUS_UNKNOWN_FAILURE = 255
#define spacecraft_UpdateFirmware_Response_1_0_STATUS_UNKNOWN_FAILURE (255U)

/// saturated uint8 STATUS_MEMORY_ALLOCATED = 1
#define spacecraft_UpdateFirmware_Response_1_0_STATUS_MEMORY_ALLOCATED (1U)

/// saturated uint8 STATUS_NOT_ENOUGH_MEMORY = 2
#define spacecraft_UpdateFirmware_Response_1_0_STATUS_NOT_ENOUGH_MEMORY (2U)

/// saturated uint8 STATUS_CHUNK_RECEIVED = 3
#define spacecraft_UpdateFirmware_Response_1_0_STATUS_CHUNK_RECEIVED (3U)

/// saturated uint8 STATUS_RECEPTION_FAILED = 4
#define spacecraft_UpdateFirmware_Response_1_0_STATUS_RECEPTION_FAILED (4U)

/// saturated uint8 STATUS_FLASH_COMPLETED = 5
#define spacecraft_UpdateFirmware_Response_1_0_STATUS_FLASH_COMPLETED (5U)

/// saturated uint8 STATUS_FLASH_FAILED = 6
#define spacecraft_UpdateFirmware_Response_1_0_STATUS_FLASH_FAILED (6U)

/// saturated uint8 STATUS_JUMPING = 7
#define spacecraft_UpdateFirmware_Response_1_0_STATUS_JUMPING (7U)

/// saturated uint8 STATUS_ERASE_COMPLETED = 8
#define spacecraft_UpdateFirmware_Response_1_0_STATUS_ERASE_COMPLETED (8U)

/// saturated uint8 STATUS_ERASE_FAILED = 9
#define spacecraft_UpdateFirmware_Response_1_0_STATUS_ERASE_FAILED (9U)

typedef struct
{
    /// saturated uint8 status
    uint8_t status;
} spacecraft_UpdateFirmware_Response_1_0;

/// Serialize an instance into the provided buffer.
/// The lifetime of the resulting serialized representation is independent of the original instance.
/// This method may be slow for large objects (e.g., images, point clouds, radar samples), so in a later revision
/// we may define a zero-copy alternative that keeps references to the original object where possible.
///
/// @param obj      The object to serialize.
///
/// @param buffer   The destination buffer. There are no alignment requirements.
///                 @see spacecraft_UpdateFirmware_Response_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_
///
/// @param inout_buffer_size_bytes  When calling, this is a pointer to the size of the buffer in bytes.
///                                 Upon return this value will be updated with the size of the constructed serialized
///                                 representation (in bytes); this value is then to be passed over to the transport
///                                 layer. In case of error this value is undefined.
///
/// @returns Negative on error, zero on success.
static inline int8_t spacecraft_UpdateFirmware_Response_1_0_serialize_(
    const spacecraft_UpdateFirmware_Response_1_0* const obj, uint8_t* const buffer,  size_t* const inout_buffer_size_bytes)
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
    {   // saturated uint8 status
        NUNAVUT_ASSERT(offset_bits % 8U == 0U);
        NUNAVUT_ASSERT((offset_bits + 8ULL) <= (capacity_bytes * 8U));
        // Saturation code not emitted -- native representation matches the serialized representation.
        buffer[offset_bits / 8U] = (uint8_t)(obj->status);  // C std, 6.3.1.3 Signed and unsigned integers
        offset_bits += 8U;
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
static inline int8_t spacecraft_UpdateFirmware_Response_1_0_deserialize_(
    spacecraft_UpdateFirmware_Response_1_0* const out_obj, const uint8_t* buffer, size_t* const inout_buffer_size_bytes)
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
    // saturated uint8 status
    NUNAVUT_ASSERT(offset_bits % 8U == 0U);
    if ((offset_bits + 8U) <= capacity_bits)
    {
        out_obj->status = buffer[offset_bits / 8U] & 255U;
    }
    else
    {
        out_obj->status = 0U;
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
static inline void spacecraft_UpdateFirmware_Response_1_0_initialize_(spacecraft_UpdateFirmware_Response_1_0* const out_obj)
{
    if (out_obj != NULL)
    {
        size_t size_bytes = 0;
        const uint8_t buf = 0;
        const int8_t err = spacecraft_UpdateFirmware_Response_1_0_deserialize_(out_obj, &buf, &size_bytes);
        NUNAVUT_ASSERT(err >= 0);
        (void) err;
    }
}

#ifdef __cplusplus
}
#endif
#endif // SPACECRAFT_UPDATE_FIRMWARE_1_0_INCLUDED_
