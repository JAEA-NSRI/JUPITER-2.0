/**
 * @addtogroup Geometry
 * @{
 * @brief Geometry support library
 *
 * @file defs.h
 * @brief Geometry data and enum definitions.
 */

#ifndef JUPITER_GEOMETRY_DEFS_H
#define JUPITER_GEOMETRY_DEFS_H

#include <stddef.h>

#ifdef __cplusplus
#define JUPITER_GEOMETRY_DECL_START extern "C" {
#define JUPITER_GEOMETRY_DECL_END }
#else
#define JUPITER_GEOMETRY_DECL_START
#define JUPITER_GEOMETRY_DECL_END
#endif

JUPITER_GEOMETRY_DECL_START

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(JUPITER_GEOMETRY_EXPORT)
#define JUPITER_GEOMETRY_DECL __declspec(dllexport)
#define JUPITER_GEOMETRY_DECL_PRIVATE
#elif defined(JUPITER_GEOMETRY_IMPORT)
#define JUPITER_GEOMETRY_DECL __declspec(dllimport)
#define JUPITER_GEOMETRY_DECL_PRIVATE
#else
#define JUPITER_GEOMETRY_DECL
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#if defined(JUPITER_GEOMETRY_EXPORT) || defined(JUPITER_GEOMETRY_IMPORT)
#define JUPITER_GEOMETRY_DECL __attribute__((visibility("default")))
#define JUPITER_GEOMETRY_DECL_PRIVATE __attribute__((visibility("hidden")))
#else
#define JUPITER_GEOMETRY_DECL
#define JUPITER_GEOMETRY_DECL_PRIVATE
#endif
#else
#define JUPITER_GEOMETRY_DECL
#define JUPITER_GEOMETRY_DECL_PRIVATE
#endif

/**
 * @brief size type used in geometry library
 *
 * You may change the type, but you can **not** use **unsigned** types.
 *
 * (Note: changing the type will break the ABI; you must recompile all
 *  sources)
 */
typedef ptrdiff_t geom_size_type;

struct geom_args_builder;
typedef struct geom_args_builder geom_args_builder;

struct geom_2d_path_data;
typedef struct geom_2d_path_data geom_2d_path_data;

struct geom_2d_path_element_iterator;
typedef struct geom_2d_path_element_iterator geom_2d_path_element_iterator;

struct geom_2d_path_element;
typedef struct geom_2d_path_element geom_2d_path_element;

struct geom_2d_path_move_element;
typedef struct geom_2d_path_move_element geom_2d_path_move_element;

struct geom_2d_path_line_element;
typedef struct geom_2d_path_line_element geom_2d_path_line_element;

struct geom_2d_path_circ_element;
typedef struct geom_2d_path_circ_element geom_2d_path_circ_element;

struct geom_2d_path_end_element;
typedef struct geom_2d_path_end_element geom_2d_path_end_element;

struct geom_surface_shape_element;
typedef struct geom_surface_shape_element geom_surface_shape_element;

struct geom_surface_shape_data;
typedef struct geom_surface_shape_data geom_surface_shape_data;

struct geom_shape_element;
typedef struct geom_shape_element geom_shape_element;

struct geom_shape_data;
typedef struct geom_shape_data geom_shape_data;

struct geom_shape_transform;
typedef struct geom_shape_transform geom_shape_transform;

struct geom_shape_args_builder;
typedef struct geom_shape_args_builder geom_shape_args_builder;

struct geom_init_args_builder;
typedef struct geom_init_args_builder geom_init_args_builder;

struct geom_surface_shape_args_builder;
typedef struct geom_surface_shape_args_builder geom_surface_shape_args_builder;

struct geom_init_element;
typedef struct geom_init_element geom_init_element;

struct geom_init_data;
typedef struct geom_init_data geom_init_data;

struct geom_data_element;
typedef struct geom_data_element geom_data_element;

struct geom_file_data;
typedef struct geom_file_data geom_file_data;

struct geom_data;
typedef struct geom_data geom_data;

struct geom_variant;
typedef struct geom_variant geom_variant;

struct geom_variant_list;
typedef struct geom_variant_list geom_variant_list;

struct geom_alloc_list;
typedef struct geom_alloc_list geom_alloc_list;

struct geom_info_map;
typedef struct geom_info_map geom_info_map;

struct geom_user_defined_data;
typedef struct geom_user_defined_data geom_user_defined_data;

struct geom_vec2;
typedef struct geom_vec2 geom_vec2;

struct geom_vec3;
typedef struct geom_vec3 geom_vec3;

struct geom_vec4;
typedef struct geom_vec4 geom_vec4;

struct geom_mat22;
typedef struct geom_mat22 geom_mat22;

struct geom_mat33;
typedef struct geom_mat33 geom_mat33;

struct geom_mat43;
typedef struct geom_mat43 geom_mat43;

struct geom_mat44;
typedef struct geom_mat44 geom_mat44;

struct geom_quat;
typedef struct geom_quat geom_quat;

enum geom_error
{
  GEOM_SUCCESS = 0, ///< No error

  /* Common errors (and may be fatal) */
  GEOM_ERR_NOMEM,    ///< Memory allocation failed
  GEOM_ERR_OVERFLOW, ///< Value range overflowed
  GEOM_ERR_RANGE,    ///< Value range invalid

  /* Errors of Global functions */
  GEOM_ERR_ALREADY_REGISTERED_INIT_FUNC,
  ///< Specified initialization function is already registered
  ///  (or identifier values are collide)

  GEOM_ERR_ALREADY_REGISTERED_SHAPE,
  ///< Specified shape already registered
  ///  (or identifier values are collide)

  GEOM_ERR_ALREADY_REGISTERED_SURFACE_SHAPE,
  ///< Specified 2D shape already registered
  ///  (or identifier values are collide)

  /* Errors of Argument builder */
  GEOM_ERR_DEPENDENCY, ///< Some other parameters, which is required
                       ///  to detemine current position's parameter
                       ///  type, are not set (Typically, number of
                       ///  arguments, degrees of equaition, etc.)
  GEOM_ERR_SHORT_LIST, ///< Length of `geom_variant_list` is shorter than
                       ///  the number required.

  /* Variant errors */
  GEOM_ERR_VARIANT_TYPE, ///< Variant type mismatch

  /* List errors */
  GEOM_ERR_LIST_HEAD,     ///< Required to be a list head
  GEOM_ERR_NOT_LIST_HEAD, ///< Required to be a list item

  /* Data errors */
  GEOM_ERR_HAS_POINTER,       ///< Specified pointer is already managed
  GEOM_ERR_POINTER_NOT_FOUND, ///< Specified pointer is not managed

  /* Init errors */
  GEOM_ERR_INVALID_INIT_FUNC, ///< Invalid (or not registered) initialization
                              ///  function used

  /* Shape errors */
  GEOM_ERR_INVALID_SHAPE,          ///< Invalid (or not registered) shape used
  GEOM_ERR_INVALID_SHAPE_OP,       ///< Invalid shape operand used. (both
                                   ///  unknown enum value and PUSH or SET
                                   ///  is used for COMB which is invalid)
  GEOM_ERR_SHAPE_NOT_SET,          ///< No shape is set to the element.
  GEOM_ERR_NO_BODY_SHAPES,         ///< No body shapes defined
  GEOM_ERR_SHAPE_STACK_UNDERFLOW,  ///< Shape stack underflow
                                   ///  (there is no shape to use `COMB`)
  GEOM_ERR_SHAPE_STACK_OVERFLOW,   ///< Shape stack overflow
                                   ///  (too much shapes stored in stack)
  GEOM_ERR_SHAPE_STACK_UNCLOSED,   ///< No required COMBs found
  GEOM_ERR_GROUP_STACK_UNDERFLOW,  ///< Group stack underflow
                                   ///  (there is no corresponding `GST`)
  GEOM_ERR_GROUP_STACK_OVERFLOW,   ///< Group stack overflow
                                   ///  (too much `GST` used)
  GEOM_ERR_GROUP_STACK_UNCLOSED,   ///< No required GEDs found
  GEOM_ERR_NO_SHAPES_TO_TRANSFORM, ///< There is no shapes to transform.
  GEOM_ERR_SINGULAR_TRANSFORM,     ///< Transformation matrix is singular.
  GEOM_ERR_COMB_WITHOUT_PUSH,      ///< COMB found without PUSH in a
                                   ///  transformation group.
  GEOM_ERR_INVALID_STRUCTURE,      ///< Invalid structure in a
                                   ///  transformation group.

  GEOM_ERR_SHAPE_OP_SHOULD_SET,
  ///< Shape operation is should be `SET` for GST or GED. This is not
  ///  a problem and no affects for the any calculations, but we
  ///  doubts what you mean.

  /* surface shape errors (most of errors share with 3d shape) */
  GEOM_ERR_INVALID_SURFACE_SHAPE, ///< Invalid surface shape

  GEOM_ERR_NO_SURFACE_IN_SHAPE, ///< No surfaces are defined in shape
  GEOM_ERR_NO_ENABLED_SURFACE,  ///< No enabled surfaces in shape
  GEOM_ERR_INACCURATE_SURFACE,  ///< Inaccurate surface point has been returned
};
typedef enum geom_error geom_error;

enum geom_variant_type
{
  GEOM_VARTYPE_NULL = 0x00,
  GEOM_VARTYPE_CHAR = 0x01,
  GEOM_VARTYPE_UCHAR = 0x02,
  GEOM_VARTYPE_INT = 0x03,
  GEOM_VARTYPE_LONG_INT = 0x04,
  GEOM_VARTYPE_SIZE = 0x0f,
  GEOM_VARTYPE_DOUBLE = 0x10,
  GEOM_VARTYPE_STRING = 0x20,
  GEOM_VARTYPE_STRING_SHORT = 0x21,

  GEOM_VARTYPE_INFO_MAP = 0x40,

  GEOM_VARTYPE_VECTOR2 = 0x51,
  GEOM_VARTYPE_VECTOR3 = 0x52,
  GEOM_VARTYPE_VECTOR4 = 0x53,
  GEOM_VARTYPE_SIZE_VECTOR3 = 0x62,
  GEOM_VARTYPE_MATRIX22 = 0x70,
  GEOM_VARTYPE_MATRIX33 = 0x71,
  GEOM_VARTYPE_MATRIX43 = 0x72,
  GEOM_VARTYPE_QUATERNION = 0x81,

  GEOM_VARTYPE_PHASE = 0x90,
  GEOM_VARTYPE_DATA_OPERATOR = 0x91,
  GEOM_VARTYPE_SHAPE_OPERATOR = 0x92,
  GEOM_VARTYPE_SHAPE = 0x93,
  GEOM_VARTYPE_INIT_FUNC = 0x94,
  GEOM_VARTYPE_ERROR = 0x96, /*!< geom_error */
  GEOM_VARTYPE_SURFACE_SHAPE = 0x97,

  GEOM_VARTYPE_LIST_HEAD = 0xff, /*!< Head item of geom_variant_list */

  GEOM_VARTYPE_EXTTYPE_MIN = 0x1000,
  GEOM_VARTYPE_EXTTYPE_MAX = 0x1fff,
  GEOM_VARTYPE_ENUMTYPE_MIN = 0x2000,
  GEOM_VARTYPE_ENUMTYPE_MAX = 0x2fff,

  GEOM_VARTYPE_STORABLE_MAX,

  /* These are meta types for processing arguments well */

  GEOM_VARTYPE_INT_OR_SVEC3, /*!< Accepts int or svec3 */
};
typedef enum geom_variant_type geom_variant_type;

enum geom_vof_phase
{
  GEOM_PHASE_INVALID = 0x0000,
  GEOM_PHASE_SOLID = 0x0001,
  GEOM_PHASE_LIQUID = 0x0002,
  GEOM_PHASE_GAS = GEOM_PHASE_SOLID | GEOM_PHASE_LIQUID,
};
typedef enum geom_vof_phase geom_vof_phase;

enum geom_data_operator
{
  GEOM_OP_INVALID = -1, ///< No operation defined
  GEOM_OP_NONE = 0,     ///< Do nothing
  GEOM_OP_SET = 1,      ///< Set (overwrite)
  GEOM_OP_ADD = 2,      ///< Add
  GEOM_OP_SUB = 3,      ///< Subtract
  GEOM_OP_MUL = 4,      ///< Multiply
};
typedef enum geom_data_operator geom_data_operator;

enum geom_shape_operator
{
  GEOM_SOP_INVALID = -1, ///< No operation defined
  GEOM_SOP_SET = 0,      ///< Push stack or define new shape
  GEOM_SOP_PUSH,         ///< Same as `GEOM_SOP_SET`
  GEOM_SOP_OR,           ///< OR (aka. Union)
  GEOM_SOP_ADD,          ///< Same as `GEOM_SOP_OR` (Logical Addition)
  GEOM_SOP_AND,          ///< AND (aka. Intersection)
  GEOM_SOP_MUL,          ///< Same as `GEOM_SOP_AND` (Logical Multiplication)
  GEOM_SOP_SUB,          ///< Subtract (Remove Intersection)
  GEOM_SOP_XOR,          ///< Exclusive OR (Union but remove Intersection)
};
typedef enum geom_shape_operator geom_shape_operator;

/**
 * @brief Geometry shapes
 */
enum geom_shape
{
  GEOM_SHAPE_INVALID = -1, ///< Invalid Shape
  GEOM_SHAPE_COMB = 1,     ///< Combine two shapes (aka. pop stack)

  GEOM_SHAPE_BOX, ///< Box (aka. Right Parallelepiped (along axis))
  GEOM_SHAPE_PLA, ///< One side of infinite plane
  GEOM_SHAPE_APP, ///< Arbitrary Parallelepiped
  GEOM_SHAPE_WED, ///< Wedge (Arbitrary 6-point shape)
  GEOM_SHAPE_ARB, ///< Arbitrary 8-point shape
  GEOM_SHAPE_RPR, ///< Right Regular Prism
  GEOM_SHAPE_TRP, ///< Truncated Right Regular Pyramid
  GEOM_SHAPE_SPH, ///< Sphere
  GEOM_SHAPE_RCC, ///< Right Circular Cylinder
  GEOM_SHAPE_ELL, ///< Ellipsoid
  GEOM_SHAPE_REC, ///< Right Elliptical Cylinder
  GEOM_SHAPE_TRC, ///< Truncated Right Circular Cone
  GEOM_SHAPE_TEC, ///< Truncated Right Elliptical Cone
  GEOM_SHAPE_TOR, ///< Torus
  GEOM_SHAPE_ETO, ///< Elliptical Torus (Circular locus)

  GEOM_SHAPE_PL1, ///< Linear surface (1st degree of polynomial surface)
  GEOM_SHAPE_PL2, ///< Quadric surface (2nd degree of polynomial surface)
  GEOM_SHAPE_PL3, ///< Cubic surface (3rd degree of polynomial surface)
  GEOM_SHAPE_PL4, ///< Quartic surface (4th degree of polynomial surface)
  GEOM_SHAPE_PLN, ///< N-th degree of polynomial surface

  GEOM_SHAPE_GST, ///< Transformation Group start
  GEOM_SHAPE_GED, ///< Transformation Group end

  GEOM_SHAPE_TRA, ///< [Transformation] Translate
  GEOM_SHAPE_ROT, ///< [Transformation] Rotation
  GEOM_SHAPE_SCA, ///< [Transformation] Scale
  GEOM_SHAPE_MAT, ///< [Transformation] Direct specification of transformation
                  ///< matrix

  GEOM_SHAPE_USER, ///< Minimum value for user defined shape
};
typedef enum geom_shape geom_shape;

enum geom_shape_type
{
  GEOM_SHPT_INVALID = -1, ///< Invalid shape type
  GEOM_SHPT_BODY = 0,     ///< Body shape
  GEOM_SHPT_TRANS,        ///< Transformation
  GEOM_SHPT_SPECIAL,      ///< Internal specials
};
typedef enum geom_shape_type geom_shape_type;

/**
 * @brief Size of shape test stack.
 *
 * Fixed, because if we make this parameter modifiable dynamically, we
 * need to allocate stack buffer while running shape calculation, so
 * you *must* error handling for allocation error there. That is
 * painful.
 */
#define GEOM_SHAPE_STACK_SIZE 10

enum geom_surface_shape
{
  GEOM_SURFACE_SHAPE_INVALID,       ///< Invalid shape type
  GEOM_SURFACE_SHAPE_COMB,          ///< Combine two shapes
  GEOM_SURFACE_SHAPE_PARALLELOGRAM, ///< Parallelogram
  GEOM_SURFACE_SHAPE_USER,          ///< Minimum value for user-defined 2D shape
};
typedef enum geom_surface_shape geom_surface_shape;

enum geom_2d_path_element_type
{
  GEOM_2D_PATH_INVALID,
  GEOM_2D_PATH_MOVE,    ///< Move to a vertex
  GEOM_2D_PATH_LINE,    ///< Line to
  GEOM_2D_PATH_CIRC,    ///< Circular Arc to
  GEOM_2D_PATH_RELMOVE, ///< Move to a vertex, relative
  GEOM_2D_PATH_RELLINE, ///< Line to, relative
  GEOM_2D_PATH_RELCIRC, ///< Circular Arc to, relative
  GEOM_2D_PATH_END,     ///< End and close the path
};
typedef enum geom_2d_path_element_type geom_2d_path_element_type;

enum geom_init_func
{
  GEOM_INIT_FUNC_INVALID = -1, ///< Invalid
  GEOM_INIT_FUNC_NONE = 0,     ///< No initialization (by library)
  GEOM_INIT_FUNC_CONST,        ///< Constant
  GEOM_INIT_FUNC_LINEAR,       ///< (Simple) Linear equation
  GEOM_INIT_FUNC_POLY,         ///< Polynomial equation
  GEOM_INIT_FUNC_POLY_N,       ///< Polynomial equation (alternative input mode)
  GEOM_INIT_FUNC_EXP_POLY,     ///< Exponential of Polynomial
  GEOM_INIT_FUNC_USER,         ///< Minimum value for user defined function
};
typedef enum geom_init_func geom_init_func;

/* General allocation/deallocation function prototype */

/**
 * @brief Allocator function prototype to be used with geometry library
 *
 * The required size of memory must be defined in a reasonable way,
 * because we do not know it.
 */
typedef void *geom_allocator(void);

/**
 * @brief Deallocator function prototype to be used with geometry library
 */
typedef void geom_deallocator(void *p);

/**
 * @macro GEOM_DEPRECATED
 * @brief If the compiler supports, emit deprecated warning
 * @param message Warning message
 */
#if defined(__GNUC__)
#define GEOM_DEPRECATED(message) __attribute__((__deprecated__(message)))
#else
#define GEOM_DEPRECATED(message)
#endif

JUPITER_GEOMETRY_DECL_END

#endif
/** @} */
