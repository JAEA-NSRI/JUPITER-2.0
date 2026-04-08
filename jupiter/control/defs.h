
#ifndef JUPITER_CONTROL_DEFS_H
#define JUPITER_CONTROL_DEFS_H

#include <jupiter/geometry/vector.h>
#include <jupiter/serializer/defs.h>
#include <stddef.h>
#include <stdint.h>

#ifdef JCNTRL_USE_MPI
#include <mpi.h>
#endif

#ifdef __cplusplus
#define JUPITER_CONTROL_DECL_BEGIN extern "C" {
#define JUPITER_CONTROL_DECL_END }
#else
#define JUPITER_CONTROL_DECL_BEGIN
#define JUPITER_CONTROL_DECL_END
#endif

JUPITER_CONTROL_DECL_BEGIN

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(JUPITER_CONTROL_EXPORT)
#define JUPITER_CONTROL_DECL __declspec(dllexport)
#define JUPITER_CONTROL_DECL_PRIVATE
#elif defined(JUPITER_CONTROL_IMPORT)
#define JUPITER_CONTROL_DECL __declspec(dllimport)
#define JUPITER_CONTROL_DECL_PRIVATE
#else
#define JUPITER_CONTROL_DECL
#define JUPITER_CONTROL_DECL_PRIVATE
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#if defined(JUPITER_CONTROL_EXPORT) || defined(JUPITER_CONTROL_IMPORT)
#define JUPITER_CONTROL_DECL __attribute__((visibility("default")))
#define JUPITER_CONTROL_DECL_PRIVATE __attribute__((visibility("hidden")))
#else
#define JUPITER_CONTROL_DECL
#define JUPITER_CONTROL_DECL_PRIVATE
#endif
#else
#define JUPITER_CONTROL_DECL
#define JUPITER_CONTROL_DECL_PRIVATE
#endif

/**
 * @brief Storable information symbols
 */
enum jcntrl_info
{
  JCNTRL_INFO_REQUEST_UPDATE_INFORMATION, /*!< Request update information */
  JCNTRL_INFO_REQUEST_UPDATE_EXTENT,      /*!< Request update extent */
  JCNTRL_INFO_REQUEST_UPDATE_DATA,        /*!< Request update data */
  JCNTRL_INFO_REQUEST_CHECK_LOOP,  /*!< Request to check circular dependency */
  JCNTRL_INFO_WHOLE_EXTENT,        /*!< Updated value of global extent */
  JCNTRL_INFO_PIECE_EXTENT,        /*!< Updated value of local extent */
  JCNTRL_INFO_DATA_EXTENT,         /*!< Updated value of data extent */
  JCNTRL_INFO_UPDATE_TIME,         /*!< Stores current time */
  JCNTRL_INFO_DATA_OBJECT,         /*!< Stores data object */
  JCNTRL_INFO_DATATYPE,            /*!< Stores provided datatype */
  JCNTRL_INFO_REQUIRED_DATATYPE,   /*!< Stores required datatype */
  JCNTRL_INFO_REQUIREMENTS_FILLED, /*!< Stores whether the port is filled */
  JCNTRL_INFO_LOOP_MARKER,     /*!< Marker for checking circular dependency */
  JCNTRL_INFO_DATA_UP_TO_DATE, /*!< Stores whether data is up to date */
  JCNTRL_INFO_LOCKED,          /*!< Stores whether this info is
                                 read-only, this flag is used only for
                                 special purpose and no one can modify
                                 or set. */

  /* error info */
  JCNTRL_INFO_ERROR_NUMBER,         /*!< Error number */
  JCNTRL_INFO_ERROR_SOURCE_FILE,    /*!< Error location of source file */
  JCNTRL_INFO_ERROR_SOURCE_LINE,    /*!< Error location of source line */
  JCNTRL_INFO_ERROR_MESSAGE,        /*!< Error message */
  JCNTRL_INFO_ERROR_ERRNO,          /*!< Stores system errno */
  JCNTRL_INFO_ERROR_MPI,            /*!< Stores MPI error code */
  JCNTRL_INFO_ERROR_GEOMETRY,       /*!< Stores error value of geometry */
  JCNTRL_INFO_ERROR_SERIALIZER,     /*!< Stores error value of serializer */
  JCNTRL_INFO_ERROR_TABLE,          /*!< Stores error value of table */
  JCNTRL_INFO_ERROR_EXEC_DOWN,      /*!< Stores downstream executive name */
  JCNTRL_INFO_ERROR_EXEC_UP,        /*!< Stores upstream executive name */
  JCNTRL_INFO_ERROR_OBJECT_GIVEN,   /*!< Stores given object data type */
  JCNTRL_INFO_ERROR_TYPE_GIVEN,     /*!< Stores given data type */
  JCNTRL_INFO_ERROR_TYPE_REQUIRED,  /*!< Stores required data type */
  JCNTRL_INFO_ERROR_INFO_KEY,       /*!< Stores info key which occured error */
  JCNTRL_INFO_ERROR_INFO_TYPE,      /*!< Stores info type which is requested */
  JCNTRL_INFO_ERROR_ELTYPE_EXPECT,  /*!< Stores expected element type */
  JCNTRL_INFO_ERROR_ELTYPE_REQUEST, /*!< Stores requested element type */
  JCNTRL_INFO_ERROR_INDEX, /*!< Stores array index number bound to an error */
  JCNTRL_INFO_ERROR_VIRTUAL_BASE,  /*!< Stores the base class */
  JCNTRL_INFO_ERROR_CLASS_CALLING, /*!< Stores the class that is calling */
  JCNTRL_INFO_ERROR_FUNCNAME,      /*!< Stores the virtual function name */
  JCNTRL_INFO_ERROR_CSV_HEADER,    /*!< Stores the CSV header in file */
};
typedef enum jcntrl_info jcntrl_info;

/**
 * @brief Datatype of each information
 */
enum jcntrl_information_datatype
{
  JCNTRL_IDATATYPE_INVALID,     /*!< Invalid data type */
  JCNTRL_IDATATYPE_REQUEST,     /*!< Request */
  JCNTRL_IDATATYPE_BOOL,        /*!< Stores a boolean value */
  JCNTRL_IDATATYPE_INTEGER,     /*!< Stores an integer */
  JCNTRL_IDATATYPE_INDEX,       /*!< Stores a 3D integral index */
  JCNTRL_IDATATYPE_INDEX_RANGE, /*!< Stores a 1D integral index range */
  JCNTRL_IDATATYPE_EXTENT,      /*!< Stores a 3D integral index range */
  JCNTRL_IDATATYPE_FLOAT,       /*!< Stores a single real value */
  JCNTRL_IDATATYPE_VECTOR,      /*!< Stores a 3D point or vector */
  JCNTRL_IDATATYPE_OBJECT,      /*!< Stores a shared object */
  JCNTRL_IDATATYPE_DATATYPE,    /*!< Stores an enum of data type */
  JCNTRL_IDATATYPE_OBJECTTYPE,  /*!< Stores a shared object type metadata */
  JCNTRL_IDATATYPE_CSTRING,     /*!< Stores a constant string */
  JCNTRL_IDATATYPE_STRING,      /*!< Stores an allocated string */
};
typedef enum jcntrl_information_datatype jcntrl_information_datatype;

/**
 * @brief Datatype of each data
 */
enum jcntrl_datatype
{
  JCNTRL_DATATYPE_INVALID,   /*!< Used for error return of specific functions */
  JCNTRL_DATATYPE_GRID,      /*!< Grid data */
  JCNTRL_DATATYPE_MASK,      /*!< Mask data */
  JCNTRL_DATATYPE_MASK_FUN,  /*!< Mask function */
  JCNTRL_DATATYPE_ANY_MASK,  /*!< (explicit) mask data or mask function */
  JCNTRL_DATATYPE_GEOMETRY,  /*!< Geomtry data */
  JCNTRL_DATATYPE_FIELD_VAR, /*!< Field variable data */
  JCNTRL_DATATYPE_FIELD_FUN, /*!< Field function data */
  JCNTRL_DATATYPE_ANY_FIELD, /*!< Field variable or function */
  JCNTRL_DATATYPE_LOGICAL_VAR, /*!< logical variable data */
};
typedef enum jcntrl_datatype jcntrl_datatype;

/**
 * @brief Predefined executives
 */
enum jcntrl_executives
{
  JCNTRL_EXE_INVALID = 0, /*!< */

  /* predefined post-process executives */

  JCNTRL_POSTP_CALCULATOR,          /*!< */
  JCNTRL_POSTP_ADD_VARIABLE,        /*!< */
  JCNTRL_POSTP_DEL_VARIABLE,        /*!< */
  JCNTRL_POSTP_DEL_VARIABLE_EXCEPT, /*!< */
  JCNTRL_POSTP_SUM,                 /*!< */
  JCNTRL_POSTP_SUM_X,               /*!< */
  JCNTRL_POSTP_SUM_Y,               /*!< */
  JCNTRL_POSTP_SUM_Z,               /*!< */
  JCNTRL_POSTP_NEIGHBOR_SUM,        /*!< */
  JCNTRL_POSTP_NEIGHBOR_SUM_X,      /*!< */
  JCNTRL_POSTP_NEIGHBOR_SUM_Y,      /*!< */
  JCNTRL_POSTP_NEIGHBOR_SUM_Z,      /*!< */
  JCNTRL_POSTP_AVERAGE,             /*!< */
  JCNTRL_POSTP_AVERAGE_X,           /*!< */
  JCNTRL_POSTP_AVERAGE_Y,           /*!< */
  JCNTRL_POSTP_AVERAGE_Z,           /*!< */
  JCNTRL_POSTP_AVERAGE_XY,          /*!< */
  JCNTRL_POSTP_AVERAGE_YZ,          /*!< */
  JCNTRL_POSTP_AVERAGE_XZ,          /*!< */
  JCNTRL_POSTP_AVERAGE_T,           /*!< */
  JCNTRL_POSTP_NEIGHBOR_AVERAGE,    /*!< */
  JCNTRL_POSTP_NEIGHBOR_AVERAGE_X,  /*!< */
  JCNTRL_POSTP_NEIGHBOR_AVERAGE_Y,  /*!< */
  JCNTRL_POSTP_NEIGHBOR_AVERAGE_Z,  /*!< */
  JCNTRL_POSTP_NEIGHBOR_AVERAGE_XY, /*!< */
  JCNTRL_POSTP_NEIGHBOR_AVERAGE_YZ, /*!< */
  JCNTRL_POSTP_NEIGHBOR_AVERAGE_XZ, /*!< */
  JCNTRL_POSTP_VORTICITY_XY,        /*!< */
  JCNTRL_POSTP_VORTICITY_YZ,        /*!< */
  JCNTRL_POSTP_VORTICITY_XZ,        /*!< */
  JCNTRL_POSTP_VORTICITY2,          /*!< */
  JCNTRL_POSTP_VOLUME_INTEGRAL,     /*!< */
  JCNTRL_POSTP_MASK,                /*!< */
  JCNTRL_POSTP_UNMASK,              /*!< */
  JCNTRL_POSTP_NEGATE_MASK,         /*!< */

  /* predefined mask executives */

  JCNTRL_MASK_GEOMETRY, /*!< */
  JCNTRL_MASK_GRID,     /*!< */
  JCNTRL_MASK_ADD,      /*!< */
  JCNTRL_MASK_OR,       /*!< */
  JCNTRL_MASK_SUB,      /*!< */
  JCNTRL_MASK_MUL,      /*!< */
  JCNTRL_MASK_AND,      /*!< */
  JCNTRL_MASK_XOR,      /*!< */
  JCNTRL_MASK_EQV,      /*!< */
  JCNTRL_MASK_NOR,      /*!< */
  JCNTRL_MASK_NAND,     /*!< */
  JCNTRL_MASK_XNOR,     /*!< */
  JCNTRL_MASK_NEQV,     /*!< */
  JCNTRL_MASK_POINT,    /*!< */
  JCNTRL_MASK_EXTENT,   /*!< */
  JCNTRL_MASK_GET,      /*!< */

  JCNTRL_MASK_ALL,  /*!< */
  JCNTRL_MASK_NONE, /*!< */

  /* predefined field variable executives */

  JCNTRL_FV_CALCULATOR, /*!< */
  JCNTRL_FV_COND,       /*!< */
  JCNTRL_FV_ON_TRIGGER, /*!< */
  JCNTRL_FV_RAND,       /*!< */
  JCNTRL_FV_GET,        /*!< */
  JCNTRL_FV_SUM,        /*!< */
  JCNTRL_FV_AVERAGE,    /*!< */
  JCNTRL_FV_TABLE,      /*!< */

  /* predefined cond variable executives */

  JCNTRL_CV_LESS,          /*!< */
  JCNTRL_CV_LESS_EQUAL,    /*!< */
  JCNTRL_CV_EQUAL,         /*!< */
  JCNTRL_CV_NOT_EQUAL,     /*!< */
  JCNTRL_CV_GREATER,       /*!< */
  JCNTRL_CV_GREATER_EQUAL, /*!< */
  JCNTRL_CV_AND,           /*!< */
  JCNTRL_CV_OR,            /*!< */
  JCNTRL_CV_XOR,           /*!< */
  JCNTRL_CV_EQV,           /*!< */
  JCNTRL_CV_NAND,          /*!< */
  JCNTRL_CV_NOR,           /*!< */
  JCNTRL_CV_XNOR,          /*!< */
  JCNTRL_CV_NEQV,          /*!< */
  JCNTRL_CV_DELAY,         /*!< */

  JCNTRL_CV_TRUE,  /*!< */
  JCNTRL_CV_FALSE, /*!< */

  /* user-defined */

  JCNTRL_EXE_USER, /*!< User-defined executive */
  JCNTRL_EXE_USER_FIRST,
  JCNTRL_EXE_USER_LAST = JCNTRL_EXE_USER_FIRST + 0x100,
  JCNTRL_EXE_MAX
};
typedef enum jcntrl_executives jcntrl_executives;

enum jcntrl_error_code
{
  JCNTRL_ERROR_UNKNOWN = 0,
  JCNTRL_ERROR_ALLOCATE,           ///< Allocation failed
  JCNTRL_ERROR_INFORMATION_LOCKED, ///< Attempted to modify locked information
  JCNTRL_ERROR_OVERFLOW,           ///< Arithmetic overflow detected
  JCNTRL_ERROR_ARGUMENT,           ///< Invalid argument passed
  JCNTRL_ERROR_INDEX,              ///< Index out-of-range
  JCNTRL_ERROR_LOOP_DETECTED,      ///< Connection loop detected
  JCNTRL_ERROR_DATATYPE_ERROR,     ///< Provided data type does not match
  JCNTRL_ERROR_PURE_VIRTUAL,       ///< Called a virtual function that is not
                                   ///  bound to any function

  JCNTRL_ERROR_CSV_HEADER_MISMATCH, ///< CSV header mismatch for append data
                                    ///  (jcntrl_write_fv_csv)

  JCNTRL_ERROR_ERRNO,      ///< Error of libc errors
  JCNTRL_ERROR_TABLE,      ///< Error of table routine
  JCNTRL_ERROR_GEOMETRY,   ///< Error of geometry routine
  JCNTRL_ERROR_SERIALIZER, ///< Error of serializer
  JCNTRL_ERROR_MPI,        ///< Error of MPI

  /* Assertion failures which should be tested even on release mode */
  JCNTRL_ERROR_INFORMATION_TYPE = 1000, ///< Type mismatch on information.
                                        ///  This error is strictly
                                        ///  prohibited, and only happens
                                        ///  when you compiled with `NDEBUG`
                                        ///  flag. You must write the code
                                        ///  never to emit this error.
  JCNTRL_ERROR_ELEMENT_TYPE,            ///< Type mismatch on data array.
                                        ///  This error is strictly
                                        ///  prohibited, and only happens
                                        ///  when you compiled with `NDEBUG`
                                        ///  flag. You must write the code
                                        ///  never to emit this error.
};
typedef enum jcntrl_error_code jcntrl_error_code;

enum jcntrl_element_type
{
  JCNTRL_EL_INVALID = 0,  ///< Invalid type
  JCNTRL_EL_CHAR = 1,     ///< Char
  JCNTRL_EL_INT = 3,      ///< Integer
  JCNTRL_EL_BOOL = 10,    ///< Boolean
  JCNTRL_EL_FLOAT = 100,  ///< Single precision float
  JCNTRL_EL_DOUBLE = 200, ///< Double precision float
  JCNTRL_EL_SIZE = 300,   ///< jcntrl_size_type
};
typedef enum jcntrl_element_type jcntrl_element_type;

typedef ptrdiff_t jcntrl_size_type;
#if !defined(PTRDIFF_MAX)
#error Missing PTRDIFF_MAX
#endif
#define JCNTRL_SIZE_MIN PTRDIFF_MIN
#define JCNTRL_SIZE_MAX PTRDIFF_MAX

#ifdef JCNTRL_USE_MPI
typedef MPI_Aint jcntrl_aint_type;
#else
typedef intptr_t jcntrl_aint_type;
#endif

struct jcntrl_shared_object;
typedef struct jcntrl_shared_object jcntrl_shared_object;

struct jcntrl_shared_object_data;
typedef struct jcntrl_shared_object_data jcntrl_shared_object_data;

struct jcntrl_shared_object_funcs;
typedef struct jcntrl_shared_object_funcs jcntrl_shared_object_funcs;

struct jcntrl_struct_grid;
typedef struct jcntrl_struct_grid jcntrl_struct_grid;

struct jcntrl_input;
typedef struct jcntrl_input jcntrl_input;

struct jcntrl_output;
typedef struct jcntrl_output jcntrl_output;

struct jcntrl_executive;
typedef struct jcntrl_executive jcntrl_executive;

struct jcntrl_abstract_array;
typedef struct jcntrl_abstract_array jcntrl_abstract_array;

struct jcntrl_data_array;
typedef struct jcntrl_data_array jcntrl_data_array;

struct jcntrl_generic_data_array;
typedef struct jcntrl_generic_data_array jcntrl_generic_data_array;

struct jcntrl_char_array;
typedef struct jcntrl_char_array jcntrl_char_array;

struct jcntrl_int_array;
typedef struct jcntrl_int_array jcntrl_int_array;

struct jcntrl_bool_array;
typedef struct jcntrl_bool_array jcntrl_bool_array;

struct jcntrl_float_array;
typedef struct jcntrl_float_array jcntrl_float_array;

struct jcntrl_double_array;
typedef struct jcntrl_double_array jcntrl_double_array;

struct jcntrl_size_array;
typedef struct jcntrl_size_array jcntrl_size_array;

struct jcntrl_aint_array;
typedef struct jcntrl_aint_array jcntrl_aint_array;

struct jcntrl_data_subarray;
typedef struct jcntrl_data_subarray jcntrl_data_subarray;

struct jcntrl_string_array;
typedef struct jcntrl_string_array jcntrl_string_array;

struct jcntrl_cell_data;
typedef struct jcntrl_cell_data jcntrl_cell_data;

struct jcntrl_cell_data_entry;
typedef struct jcntrl_cell_data_entry jcntrl_cell_data_entry;

struct jcntrl_cell_data_iterator;
typedef struct jcntrl_cell_data_iterator jcntrl_cell_data_iterator;

struct jcntrl_data_object;
typedef struct jcntrl_data_object jcntrl_data_object;

struct jcntrl_field_object;
typedef struct jcntrl_field_object jcntrl_field_object;

struct jcntrl_mask_object;
typedef struct jcntrl_mask_object jcntrl_mask_object;

struct jcntrl_grid_data;
typedef struct jcntrl_grid_data jcntrl_grid_data;

struct jcntrl_mask_data;
typedef struct jcntrl_mask_data jcntrl_mask_data;

struct jcntrl_mask_function;
typedef struct jcntrl_mask_function jcntrl_mask_function;

struct jcntrl_field_variable;
typedef struct jcntrl_field_variable jcntrl_field_variable;

struct jcntrl_field_function;
typedef struct jcntrl_field_function jcntrl_field_function;

struct jcntrl_logical_variable;
typedef struct jcntrl_logical_variable jcntrl_logical_variable;

struct jcntrl_geometry;
typedef struct jcntrl_geometry jcntrl_geometry;

struct jcntrl_information;
typedef struct jcntrl_information jcntrl_information;

struct jcntrl_connection;
typedef struct jcntrl_connection jcntrl_connection;

struct jcntrl_executive_manager;
typedef struct jcntrl_executive_manager jcntrl_executive_manager;

struct jcntrl_executive_manager_entry;
typedef struct jcntrl_executive_manager_entry jcntrl_executive_manager_entry;

struct jcntrl_mpi_controller;
typedef struct jcntrl_mpi_controller jcntrl_mpi_controller;

struct jcntrl_reduce_op;
typedef struct jcntrl_reduce_op jcntrl_reduce_op;

struct jcntrl_irange;
typedef struct jcntrl_irange jcntrl_irange;

struct jcntrl_extent;
typedef struct jcntrl_extent jcntrl_extent;

struct jcntrl_cell;
typedef struct jcntrl_cell jcntrl_cell;

struct jcntrl_cell_hex;
typedef struct jcntrl_cell_hex jcntrl_cell_hex;

/**
 * @brief Prototype of executive callback
 * @param request Request information of operation
 * @param input_head Head to input informations for each input ports
 * @param output_head Head to input informations for each output ports
 * @param data Executive object data
 * @return non-0 on success, 0 if error occured
 */
typedef int jcntrl_process_request(jcntrl_information *request,
                                   jcntrl_input *input_head,
                                   jcntrl_output *output_head,
                                   jcntrl_shared_object *data);

/**
 * @brief Prototype of input port information filler
 * @param data Executive object data
 * @param index Index number of the port
 * @param input Port that is required to update information
 * @return non-0 on success, 0 if error occuered
 */
typedef int jcntrl_fill_input_port_information(jcntrl_shared_object *data,
                                               int index, jcntrl_input *input);

/**
 * @brief Prototype of input port information filler
 * @param data Executive object data
 * @param index Index number of the port
 * @param output Port that is required to update information
 * @return non-0 on success, 0 if error occuered
 */
typedef int jcntrl_fill_output_port_information(jcntrl_shared_object *data,
                                                int index,
                                                jcntrl_output *output);

typedef jcntrl_process_request jcntrl_update_information;
typedef jcntrl_process_request jcntrl_update_extent;
typedef jcntrl_process_request jcntrl_update_data;
typedef const char *jcntrl_class_name(void);
typedef int jcntrl_restart_read(msgpackx_data *mdata,
                                jcntrl_shared_object *data);
typedef int jcntrl_restart_write(msgpackx_data *mdata,
                                 jcntrl_shared_object *data);

typedef double jcntrl_scalar_field_function(void *arg, //
                                            double x, double y, double z);
typedef geom_vec3 jcntrl_vector_field_function(void *arg, //
                                               double x, double y, double z);

typedef int jcntrl_error_callback(void *arg, jcntrl_information *info);

enum jcntrl_logical_operator
{
  JCNTRL_LOP_INVALID = -1, ///< Invalid
  JCNTRL_LOP_SET,  ///< Replace the upstream value (only for 'upstream' exists)
  JCNTRL_LOP_ADD,  ///< (Same meaning to OR)
  JCNTRL_LOP_OR,   ///< OR, Union
  JCNTRL_LOP_SUB,  ///< Subtract (unmask "minuend" masks)
  JCNTRL_LOP_MUL,  ///< (Same meaning to AND)
  JCNTRL_LOP_AND,  ///< AND
  JCNTRL_LOP_EQV,  ///< Equivalence
  JCNTRL_LOP_XOR,  ///< eXclusive OR
  JCNTRL_LOP_NOR,  ///< Not OR
  JCNTRL_LOP_NAND, ///< Not AND
  JCNTRL_LOP_XNOR, ///< eXclusive NOR
  JCNTRL_LOP_NEQV, ///< Not Equivalence
};
typedef enum jcntrl_logical_operator jcntrl_logical_operator;

enum jcntrl_comparator
{
  JCNTRL_COMP_INVALID = -1, ///< Invalid
  JCNTRL_COMP_LESS,         ///< Less
  JCNTRL_COMP_LESS_EQ,      ///< Less or equal
  JCNTRL_COMP_EQUAL,        ///< Equal
  JCNTRL_COMP_NOT_EQ,       ///< Not equal
  JCNTRL_COMP_GREATER,      ///< Greater
  JCNTRL_COMP_GREATER_EQ,   ///< Greater or equal
};
typedef enum jcntrl_comparator jcntrl_comparator;

JUPITER_CONTROL_DECL_END

#endif /* JUPITER_CONTROL_DEFS_H */
