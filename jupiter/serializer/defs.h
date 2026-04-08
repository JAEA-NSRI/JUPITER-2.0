/**
 * @addtogroup Serializer
 * @brief [MessagePack](https://msgpack.org/)-inspired data
 *        serialization function definition
 *
 * Endianess of multiple bytes should be in big endian for
 * portability (If your system uses little endian, define
 * `JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN` at compilaton). But it is
 * safe to use little endian at your own risk.
 *
 * ### Format of Serialized data
 *
 * Legend of following maps
 *
 *     +----+
 *     |    | : 1 byte
 *     +----+
 *
 *     +====+
 *     |    | : Multiple bytes
 *     +====+
 *
 *     +~~~~+
 *     |    | : Multiple MessagePack objects
 *     +~~~~+
 *
 * #### Signed Integer type
 *
 *     +--------------+
 *     | 0x00 .. 0x7f | : for 0 .. 127
 *     +--------------+
 *
 *     +--------------+
 *     | 0xe0 .. 0xff | : for -32 .. -1
 *     +--------------+
 *
 *     +------+-------+
 *     | 0xd0 | int 8 | : for 128 .. 127 and -128 .. -33
 *     +------+-------+
 *
 *     +------+-------+-------+
 *     | 0xd1 |    int16      | : Signed 16 bit integer
 *     +------+-------+-------+
 *
 *     +------+-------+-------+-------+-------+
 *     | 0xd2 |             int32             | : Signed 32 bit integer
 *     +------+-------+-------+-------+-------+
 *
 *     +------+-------+-------+-------+-------+-------+-------+-------+-------+
 *     | 0xd3 |                             int64                             | : Signed 64 bit integer
 *     +------+-------+-------+-------+-------+-------+-------+-------+-------+
 *
 * #### Unsigned Integer Type
 *
 * It is allowed to use store the value between 0 and 127 with tag
 * 0xcc, but this library uses signed type format and does not
 * generate such binary.
 *
 *     +------+-------+
 *     | 0xcc | uint8 | : for 128 to 255
 *     +------+-------+
 *
 *     +------+-------+-------+
 *     | 0xcd |    uint16     | : Unsigned 16 bit integer
 *     +------+-------+-------+
 *
 *     +------+-------+-------+-------+-------+
 *     | 0xce |            uint32             | : Unsigned 32 bit integer
 *     +------+-------+-------+-------+-------+
 *
 *     +------+-------+-------+-------+-------+-------+-------+-------+-------+
 *     | 0xcf |                            uint64                             | : Unsigned 64 bit integer
 *     +------+-------+-------+-------+-------+-------+-------+-------+-------+
 *
 * #### Nil type
 *
 * 1-byte of `0xc0` is nil value.
 *
 *     +------+
 *     | 0xc0 | : nil
 *     +------+
 *
 * #### Boolean type
 *
 * 1-byte of `0xc2` will be false and `0xc3` will be true.
 *
 *     +------+
 *     | 0xc2 | : false
 *     +------+
 *
 *     +------+
 *     | 0xc3 | : true
 *     +------+
 *
 * #### Binary type
 *
 * Arbitrary length of (untyped) binary data.
 *
 *     upto 2^16-1 bytes
 *     +------+========================+=========+
 *     | 0xc4 | size of data in uint16 |  data   |
 *     +------+========================+=========+
 *
 *     upto 2^32-1 bytes
 *     +------+========================+=========+
 *     | 0xc5 | size of data in uint32 |  data   |
 *     +------+========================+=========+
 *
 *     upto 2^64-1 bytes
 *     +------+========================+=========+
 *     | 0xc6 | size of data in uint64 |  data   |
 *     +------+========================+=========+
 *
 * @note Library does not take care of endian in the `data`
 *
 * #### Float type
 *
 * Floating point value in single or double precision
 *
 *     for float type in C:
 *     +------+=================================+
 *     | 0xca | 32 bit IEEE-754 float (assumed) |
 *     +------+=================================+
 *         (actually used length in the implementation is `sizeof(float)`)
 *
 *     for double type in C:
 *     +------+=================================+
 *     | 0xcb | 64 bit IEEE-754 float (assumed) |
 *     +------+=================================+
 *         (actually used length in the implementation is `sizeof(double)`)
 *
 * @note JUPITER does not care whether the CPU architeture uses
 *       IEEE-754 format or not (for speed). All architectures which
 *       runs MS-Windows are using IEEE-754 format. POSIX does not
 *       require that floating point format is IEEE-754 format
 *       (ex. OpenVMS on DEC VAX is one of systems that supports POSIX
 *       but DEC VAX can use non-IEEE-754 format, aka. VAX-format),
 *       but almost of all architectures which runs POSIX-compliant
 *       and POSIX-like OSs are using IEEE-754 format.
 *
 * #### String type
 *
 * Arbitrary length of a string. There is no difference to binary type
 * that except binary data can handle data longer than 2^32-1 bytes.
 *
 *     upto 31 bytes:
 *     +--------+=======================+
 *     |101XXXXX| XXXXX bytes of string |
 *     +--------+=======================+
 *
 *     upto 2^8-1 bytes:
 *     +------+================+=========+
 *     | 0xd4 | size in uint8  | string  |
 *     +------+================+=========+
 *
 *     upto 2^16-1 bytes:
 *     +------+================+=========+
 *     | 0xd5 | size in uint16 | string  |
 *     +------+================+=========+
 *
 *     upto 2^32-1 bytes:
 *     +------+================+=========+
 *     | 0xd6 | size in uint32 | string  |
 *     +------+================+=========+
 *
 * @note JUPITER does not care about encoding, because the input files
 *       and file names should contain only ASCII charactors)
 *
 * #### Array type
 *
 *     upto 15 elements:
 *     +--------+~~~~~~~~~~~~~~~~~~~~~~~~+
 *     |1001XXXX| XXXX objects for array |
 *     +--------+~~~~~~~~~~~~~~~~~~~~~~~~+
 *
 *     upto 2^16-1 elements:
 *     +------+=============+~~~~~~~~~~~~~~~~~~~~~+
 *     | 0xda | N in uint16 | N objects for array |
 *     +------+=============+~~~~~~~~~~~~~~~~~~~~~+
 *
 *     upto 2^32-1 elements:
 *     +------+=============+~~~~~~~~~~~~~~~~~~~~~+
 *     | 0xdb | N in uint32 | N objects for array |
 *     +------+=============+~~~~~~~~~~~~~~~~~~~~~+
 *
 *     upto 2^64-1 elements:
 *     +------+=============+~~~~~~~~~~~~~~~~~~~~~+
 *     | 0xdc | N in uint64 | N objects for array |
 *     +------+=============+~~~~~~~~~~~~~~~~~~~~~+
 *
 *
 * #### Map type
 *
 * Pair of key and correspoinding value. This library stores the data
 * in insertion order.
 *
 *     upto 15 elements:
 *     +--------+~~~~~~~~~~~~~~~~~~~~~~~~+
 *     |1000XXXX| 2*XXXX objects for map |
 *     +--------+~~~~~~~~~~~~~~~~~~~~~~~~+
 *
 *     upto 2^16-1 elements:
 *     +------+=============+~~~~~~~~~~~~~~~~~~~~~+
 *     | 0xdd | N in uint16 | 2*N objects for map |
 *     +------+=============+~~~~~~~~~~~~~~~~~~~~~+
 *
 *     upto 2^32-1 elements:
 *     +------+=============+~~~~~~~~~~~~~~~~~~~~~+
 *     | 0xde | N in uint32 | 2*N objects for map |
 *     +------+=============+~~~~~~~~~~~~~~~~~~~~~+
 *
 *     upto 2^64-1 elements:
 *     +------+=============+~~~~~~~~~~~~~~~~~~~~~+
 *     | 0xdf | N in uint64 | 2*N objects for map |
 *     +------+=============+~~~~~~~~~~~~~~~~~~~~~+
 *
 *
 * #### Ext type
 *
 * User defined type.
 *
 *     upto 2^8-1 bytes:
 *     +------+------+================+======+
 *     | 0xc7 | type | size in uint8  | data |
 *     +------+------+================+======+
 *
 *     upto 2^16-1 bytes:
 *     +------+------+================+======+
 *     | 0xc8 | type | size in uint16 | data |
 *     +------+------+================+======+
 *
 *     upto 2^32-1 bytes
 *     +------+------+================+======+
 *     | 0xc9 | type | size in uint32 | data |
 *     +------+------+================+======+
 *
 * Negative value of type is reserved.
 *
 * Following extention types are defined (but not implemented
 * yet). Following sizes are example for `sizeof(double) == 8` and
 * `sizeof(float) == 4`; It will be different if those are not.
 *
 * 2D vector:
 *
 *             (type) (size)
 *     +------+------+------+========================+
 *     | 0xc7 | 0xd2 |  16  | (x, y) value in double |
 *     +------+------+------+========================+
 *
 *             (type) (size)
 *     +------+------+------+========================+
 *     | 0xc7 | 0xd2 |   8  | (x, y) value in float  |
 *     +------+------+------+========================+
 *
 * 3D vector:
 *
 *             (type) (size)
 *     +------+------+------+===========================+
 *     | 0xc7 | 0xd3 |  24  | (x, y, z) value in double |
 *     +------+------+------+===========================+
 *
 *             (type) (size)
 *     +------+------+------+===========================+
 *     | 0xc7 | 0xd3 |  12  | (x, y, z) value in float  |
 *     +------+------+------+===========================+
 *
 * 4D vector:
 *
 *             (type) (size)
 *     +------+------+------+==============================+
 *     | 0xc7 | 0xd4 |  32  | (x, y, z, w) value in double |
 *     +------+------+------+==============================+
 *
 *             (type) (size)
 *     +------+------+------+==============================+
 *     | 0xc7 | 0xd4 |  16  | (x, y, z, w) value in float  |
 *     +------+------+------+==============================+
 *
 * Quaternion:
 *
 *             (type) (size)
 *     +------+------+------+==============================+
 *     | 0xc7 | 0xe4 |  32  | (x, y, z, w) value in double |
 *     +------+------+------+==============================+
 *
 *             (type) (size)
 *     +------+------+------+==============================+
 *     | 0xc7 | 0xe4 |  16  | (x, y, z, w) value in float  |
 *     +------+------+------+==============================+
 *
 * 2x2 matrix:
 *
 *             (type) (size)
 *     +------+------+------+====================================+
 *     | 0xc7 | 0xf2 |  32  | 4-elements in double, column-major |
 *     +------+------+------+====================================+
 *
 *             (type) (size)
 *     +------+------+------+====================================+
 *     | 0xc7 | 0xf2 |  16  | 4-elements in float, column-major  |
 *     +------+------+------+====================================+
 *
 * 3x3 matrix:
 *
 *             (type) (size)
 *     +------+------+------+====================================+
 *     | 0xc7 | 0xf3 |  72  | 9-elements in double, column-major |
 *     +------+------+------+====================================+
 *
 *             (type) (size)
 *     +------+------+------+====================================+
 *     | 0xc7 | 0xf3 |  36  | 9-elements in float, column-major  |
 *     +------+------+------+====================================+
 *
 * 4x4 matrix:
 *
 *             (type) (size)
 *     +------+------+------+=====================================+
 *     | 0xc7 | 0xf4 | 128  | 16-elements in double, column-major |
 *     +------+------+------+=====================================+
 *
 *             (type) (size)
 *     +------+------+------+=====================================+
 *     | 0xc7 | 0xf4 |  64  | 16-elements in float, column-major  |
 *     +------+------+------+=====================================+
 *
 * 3D Vector of size:
 *
 *             (type) (size)
 *     +------+------+------+=========================+
 *     | 0xc7 | 0xc3 |   3  | (x, y, z) value in int8 |
 *     +------+------+------+=========================+
 *
 *             (type) (size)
 *     +------+------+------+==========================+
 *     | 0xc7 | 0xc3 |   6  | (x, y, z) value in int16 |
 *     +------+------+------+==========================+
 *
 *             (type) (size)
 *     +------+------+------+==========================+
 *     | 0xc7 | 0xc3 |  12  | (x, y, z) value in int32 |
 *     +------+------+------+==========================+
 *
 *             (type) (size)
 *     +------+------+------+==========================+
 *     | 0xc7 | 0xc3 |  24  | (x, y, z) value in int64 |
 *     +------+------+------+==========================+
 *
 * @ingroup Serializer
 * @file serializer/defs.h
 *
 * MessagePack-inspired data serialization function definition
 */

#ifndef JUPITER_SERIALIZER_DEFS_H
#define JUPITER_SERIALIZER_DEFS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#define JUPITER_SERIALIZER_DECL_START extern "C" {
#define JUPITER_SERIALIZER_DECL_END   }
#else
#define JUPITER_SERIALIZER_DECL_START
#define JUPITER_SERIALIZER_DECL_END
#endif

JUPITER_SERIALIZER_DECL_START

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(JUPITER_SERIALIZER_EXPORT)
#define JUPITER_SERIALIZER_DECL __declspec(dllexport)
#elif defined(JUPITER_SERIALIZER_IMPORT)
#define JUPITER_SERIALIZER_DECL __declspec(dllimport)
#else
#define JUPITER_SERIALIZER_DECL
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#if defined(JUPITER_SERIALIZER_EXPORT) || defined(JUPITER_SERIALIZER_IMPORT)
#define JUPITER_SERIALIZER_DECL __attribute__((visibility("default")))
#else
#define JUPITER_SERIALIZER_DECL
#endif
#else
#define JUPITER_SERIALIZER_DECL
#endif

/**
 * @ingroup Serializer
 * @brief Key values
 */
enum msgpackx_key {
  MSGPACKX_POSITIVE_FIXINT_MIN = 0x00, ///< Minimum value of positive fixint
  MSGPACKX_POSITIVE_FIXINT_MAX = 0x7f, ///< Maximum value of positive fixint
  MSGPACKX_FIXMAP_MIN = 0x80, ///< Minimum value of fixmap
  MSGPACKX_FIXMAP_MAX = 0x8f, ///< Maximum value of fixmap
  MSGPACKX_FIXARRAY_MIN = 0x90, ///< Minimum value of fixarray
  MSGPACKX_FIXARRAY_MAX = 0x9f, ///< Maximum value of fixarray
  MSGPACKX_FIXSTR_MIN = 0xa0, ///< Minimum length of fixstr
  MSGPACKX_FIXSTR_MAX = 0xbf, ///< Maximum length of fixstr
  MSGPACKX_NIL = 0xc0,  ///< Nil Type
  /* 0xc1 is never used */
  MSGPACKX_FALSE   = 0xc2, ///< True Type
  MSGPACKX_TRUE    = 0xc3, ///< False Type
  MSGPACKX_BIN16   = 0xc4, ///< Binary Type (16 bit size)
  MSGPACKX_BIN32   = 0xc5, ///< Binary Type (32 bit size)
  MSGPACKX_BIN64   = 0xc6, ///< Binary Type (64 bit size)
  MSGPACKX_EXT8    = 0xc7, ///< Ext Type (8 bit size)
  MSGPACKX_EXT16   = 0xc8, ///< Ext Type (16 bit size)
  MSGPACKX_EXT32   = 0xc9, ///< Ext Type (32 bit size)
  MSGPACKX_FLOAT32 = 0xca, ///< Float32 Type
  MSGPACKX_FLOAT64 = 0xcb, ///< Float64 Type
  MSGPACKX_UINT8   = 0xcc, ///< uint8 type
  MSGPACKX_UINT16  = 0xcd, ///< uint16 type
  MSGPACKX_UINT32  = 0xce, ///< uint32 type
  MSGPACKX_UINT64  = 0xcf, ///< uint64 type
  MSGPACKX_INT8    = 0xd0, ///< int8 type
  MSGPACKX_INT16   = 0xd1, ///< int16 type
  MSGPACKX_INT32   = 0xd2, ///< int32 type
  MSGPACKX_INT64   = 0xd3, ///< int64 type
  MSGPACKX_STR8    = 0xd4, ///< Str type (8 bit size)
  MSGPACKX_STR16   = 0xd5, ///< Str type (16 bit size)
  MSGPACKX_STR32   = 0xd6, ///< Str type (32 bit size)
  /* 0xd7 to 0xd9 are reserved */
  MSGPACKX_ARRAY16 = 0xda, ///< Array type (16 bit number of elements)
  MSGPACKX_ARRAY32 = 0xdb, ///< Array type (32 bit number of elements)
  MSGPACKX_ARRAY64 = 0xdc, ///< Array type (64 bit number of elements)
  MSGPACKX_MAP16   = 0xdd, ///< Map type (16 bit number of elements)
  MSGPACKX_MAP32   = 0xde, ///< Map type (32 bit number of elements)
  MSGPACKX_MAP64   = 0xdf, ///< Map type (64 bit number of elements)
  MSGPACKX_NEGATIVE_FIXINT_MIN = 0xe0, ///< Minimum value of negative fixint (-32)
  MSGPACKX_NEGATIVE_FIXINT_MAX = 0xff, ///< Maximum value of negative fixint (-1)
};
typedef enum msgpackx_key msgpackx_key;

/**
 * @memberof msgpack_buffer
 * @brief Buffer copy mode
 */
enum msgpackx_buffer_copy_mode
{
  MSGPACKX_COPY_TRUNC  = 0x0001, ///< Truncate if copy data is large
  MSGPACKX_COPY_EXPAND = 0x0002, ///< expand or shrink if copy data is large
  MSGPACKX_COPY_ZERO   = 0x0004, ///< Fill 00 (ASCII NUL) if copy data is short
  MSGPACKX_COPY_SHRINK = 0x0010, ///< Shrink if copy data is short
  MSGPACKX_COPY_FLEXIBLE = MSGPACKX_COPY_EXPAND | MSGPACKX_COPY_SHRINK,
  ///< Exnpand or Shrink and move following contents.
  MSGPACKX_COPY_FIXED = MSGPACKX_COPY_TRUNC | MSGPACKX_COPY_ZERO,
  ///< Truncate or Fill zero and keep following contents left.
  MSGPACKX_COPY_CREATE = 0x0100, ///< Create New Buffer object
};

/**
 * @memberof msgpack_buffer
 * @brief buffer duplication mode
 */
enum msgpackx_buffer_dup_mode
{
  MSGPACKX_DUP_SUBSTR = 0x0000, ///< Duplicate specified substr of buffer
  MSGPACKX_DUP_WHOLE  = 0x0001, ///< Duplicate whole contiunous buffer
};

/**
 * @memberof msgpack_buffer
 * @brief buffer seek mode
 */
enum msgpackx_buffer_seek_mode
{
  MSGPACKX_SEEK_CUR = 0, ///< Offset from current position
  MSGPACKX_SEEK_SET = 1, ///< Offset from begining of the buffer
  MSGPACKX_SEEK_END = 2, ///< Offset from end of the buffer
};

/**
 * @memberof msgpack_buffer
 * @brief buffer poiter validity
 *
 * This is a flag (bitwised-OR used for multiple states).
 */
enum msgpackx_buffer_pointer_validity
{
  MSGPACKX_POINTER_INVALID = 0x00, ///< Invalid pointer
  MSGPACKX_POINTER_VALID_AS_START = 0x01, ///< Can be use as start
  MSGPACKX_POINTER_VALID_AS_END   = 0x02, ///< Can be use as end
  MSGPACKX_POINTER_VALID = MSGPACKX_POINTER_VALID_AS_START |
  MSGPACKX_POINTER_VALID_AS_END, ///< Can be use as both start and end
};

/**
 * @memberof msgpackx_array_node
 * @brief Type of array node
 */
enum msgpackx_array_node_type
{
  MSGPACKX_ARRAY_HEAD  = 0, ///< Create a head node, i.e., create a new array.
  MSGPACKX_ARRAY_ENTRY = 1, ///< Create an array element.
};

/**
 * @memberof msgpackx_map_node
 * @brief Type of map node
 */
enum msgpackx_map_node_type
{
  MSGPACKX_MAP_HEAD  = 0, /// Create a head node, i.e., create a new map.
  MSGPACKX_MAP_ENTRY = 1, /// Create a map element.
};

/**
 * @ingroup Serializer
 * @brief error values for serializer
 */
enum msgpackx_error
{
  MSGPACKX_SUCCESS = 0,  ///< No error
  MSGPACKX_ERR_NOMEM,    ///< Allocation failed
  MSGPACKX_ERR_RANGE,    ///< Out of expressible range
  MSGPACKX_ERR_NO_HEAD,  ///< There is no head at map or array node
  MSGPACKX_ERR_MSG_TYPE, ///< Message type does not match to your request
  MSGPACKX_ERR_MAP_KEY_DUPLICATE,  ///< Already have that key on map
  MSGPACKX_ERR_MSG_INCOMPLETE,     ///< Incomplete message
  MSGPACKX_ERR_SYS,      ///< Subsidiary system call failed
  MSGPACKX_ERR_INDEX,    ///< Index out-of-range
  MSGPACKX_ERR_KEYNOTFOUND, ///< Specified key not found

  /* Errors in msgpackx_{make/read}_header_data */
  MSGPACKX_ERR_HEADER_SIZE, ///< Number of elements in header does not expected
  MSGPACKX_ERR_ENDIAN,      ///< Invalid Endian information
  MSGPACKX_ERR_TITLE,       ///< Title does not match to expected one
  MSGPACKX_ERR_EOF,         ///< Invalid EOF mark
};
typedef enum msgpackx_error msgpackx_error;

struct msgpackx_data;
typedef struct msgpackx_data msgpackx_data;

struct msgpackx_node;
typedef struct msgpackx_node msgpackx_node;

struct msgpackx_array_node;
typedef struct msgpackx_array_node msgpackx_array_node;

struct msgpackx_map_node;
typedef struct msgpackx_map_node msgpackx_map_node;

struct msgpackx_buffer;
typedef struct msgpackx_buffer msgpackx_buffer;

#if !defined(PTRDIFF_MAX)
#error "PTRDIFF_MAX is not defined (needs C99 compliant compiler)"
#endif

#if defined(UINT64_MAX)
#if PTRDIFF_MAX > UINT64_MAX
#define MSGPACKX_UINT64_MAX UINT64_MAX
#else
#define MSGPACKX_UINT64_MAX PTRDIFF_MAX
#endif
#define MSGPACKX_SIZE_MAX MSGPACKX_UINT64_MAX
#endif

#if defined(UINT32_MAX)
#if PTRDIFF_MAX > UINT32_MAX
#define MSGPACKX_UINT32_MAX UINT32_MAX
#else
#define MSGPACKX_UINT32_MAX PTRDIFF_MAX
#endif
#if !defined(MSGPACKX_SIZE_MAX)
#define MSGPACKX_SIZE_MAX MSGPACKX_UINT32_MAX
#endif
#endif

#if defined(UINT16_MAX)
#if PTRDIFF_MAX > UINT16_MAX
#define MSGPACKX_UINT16_MAX UINT16_MAX
#else
#define MSGPACKX_UINT16_MAX PTRDIFF_MAX
#endif
#if !defined(MSGPACKX_SIZE_MAX)
#define MSGPACKX_SIZE_MAX MSGPACKX_UINT16_MAX
#endif
#endif

#if !defined(MSGPACKX_SIZE_MAX)
#error "Impossible to deteremine MSGPACKX_SIZE_MAX (uint32_t or uint64_t is required at least)"
#endif

JUPITER_SERIALIZER_DECL_END

#endif
