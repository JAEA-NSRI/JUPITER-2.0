
#ifndef JUPITER_CONTROL_ERROR_H
#define JUPITER_CONTROL_ERROR_H

#include "defs.h"

JUPITER_CONTROL_DECL_BEGIN

#ifdef __GNUC__
#define JCNTRL_EXPECT(cond) __builtin_expect(!!(cond), 1)
#else
#define JCNTRL_EXPECT(cond) ((!!(cond)) == 1)
#endif

JUPITER_CONTROL_DECL
void jcntrl_assert_impl(const char *file, long line, const char *func,
                        const char *cond, const char *format, ...);

#ifndef NDEBUG
#define JCNTRL_ASSERT_X(cond, ...)                                          \
  do {                                                                      \
    if (!JCNTRL_EXPECT(cond))                                               \
      jcntrl_assert_impl(__FILE__, __LINE__, __func__, #cond, __VA_ARGS__); \
  } while (0)

#define JCNTRL_UNREACHABLE() \
  jcntrl_assert_impl(__FILE__, __LINE__, __func__, NULL, "Unreachable reached");

#else
#define JCNTRL_ASSERT_X(cond, ...) ((void)0)

#ifdef __GNUC__
#define JCNTRL_UNREACHABLE() __builtin_unreachable()
#else
#define JCNTRL_UNREACHABLE()
#endif
#endif

#define JCNTRL_ASSERT(cond) JCNTRL_ASSERT_X(cond, NULL)

/*
 * Following functions are defined as error reporting module, but
 * defined in information.c, because callback to use information
 * object locally.
 */

/**
 * @brief set error callback function
 * @param func Callback to set
 * @param data Pointer to extra data to pass callback function
 * @return Old callback function
 *
 * Setting NULL to supress all error outputs. Default function outputs
 * error message to stderr.
 *
 * This function is not thread safe and callback is process global data.
 */
JUPITER_CONTROL_DECL
jcntrl_error_callback *jcntrl_error_callback_set(jcntrl_error_callback *func,
                                                 void *data);

/**
 * @brief get error callback function
 * @return current callback function
 */
JUPITER_CONTROL_DECL
jcntrl_error_callback *jcntrl_error_callback_get(void);

/**
 * @brief raise allocation failed error
 * @param file source file name where the error occured
 * @param line source file line number where the error occured
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_allocation_failed(const char *file, int line);

/**
 * @brief raise locked information error
 * @param file source file name where the error occured
 * @param line source file line number where the error occured
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_information_locked_error(const char *file, int line);

/**
 * @brief raise arithmetic overflow error
 * @param file source file name where the error occured
 * @param line source file line number where the error occured
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_overflow_error(const char *file, int line);

/**
 * @brief raise generic argument error
 * @param file source file name where the error occured
 * @param line source file line number where the error occuered
 * @param message Detailed Error information
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_argument_error(const char *file, int line,
                                const char *message);

/**
 * @brief raise array index error
 * @param file source file location where the error occured
 * @param line source file line number where the error occured
 * @param index Index which will be out-of-range
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_index_error(const char *file, int line,
                             jcntrl_size_type index);

/**
 * @brief raise loop detected error
 * @param file source file location where the error occuered
 * @param line source file line number where the error occuered
 * @param upstream_executive_name Upstream executive name
 * @param downstream_executive_name Downstream executive name
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_loop_detected_error(const char *file, int line,
                                     const char *upstream_executive_name,
                                     const char *downstream_executive_name);

/**
 * @brief raise upstream type error
 * @param file source file location where the error occured
 * @param line source file line number where the error occuered
 * @param upstream_executive_name Upstream executive name
 * @param downstream_executive_name Downstream executive name
 * @param required Required data type for the executive on @p port
 * @param provided Provided data type from upstream executive.
 * @param object_type_provided Provided data type as object type
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_upstream_type_error(
  const char *file, int line, const char *upstream_executive_name,
  const char *downstream_executive_name, jcntrl_datatype required,
  jcntrl_datatype provided,
  const jcntrl_shared_object_data *object_type_provided);

/**
 * @brief raise libc errno error
 * @param file source file location where the error occured
 * @param line source line number where the error occured
 * @param errno The errno value
 * @param msg Message that describes error, or NULL to use strerror().
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_errno_error(const char *file, int line, int errno_code,
                             const char *msg);

/**
 * @brief raise table library error
 * @param file source file location where the error occured
 * @param line source line number where the error occured
 * @param table_error_code Error code of table library
 * @param msg Message that describes error, or NULL to use table_errorstr().
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_table_error(const char *file, int line, int table_error_code,
                             const char *msg);

/**
 * @brief raise geometry library error
 * @param file source file location where the error occured
 * @param line source line number where the error occured
 * @param geom_error_code Error code of geometry library
 * @param msg Message that describes error, or NULL to use geom_strerror().
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_geom_error(const char *file, int line, int geom_error_code,
                            const char *msg);

/**
 * @brief raise serializer library error
 * @param file source file location where the error occured
 * @param line source line number where the error occured
 * @param serializer_error_code Error code of serializer library.
 * @param msg Message that describes error, or NULL to use msgpackx_strerror().
 * @return The return value returned by callback function.
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_serializer_error(const char *file, int line,
                                  int serializer_error_code, const char *msg);

/**
 * @brief raise MPI library error
 * @param file source file location where the error occuered
 * @param line source line number where the error occured
 * @param mpi_error_code Error code of MPI library.
 * @param msg Message that describes error, or NULL to use MPI_Error_string().
 * @return The return value returned by callback function.
 *
 * Control library does not take care of MPI error handler. If you set your
 * error handler to corresponding MPI communicator, it'll be called at
 * first. However, control library may also raises this error.
 *
 * If corresponding MPI communicator's error handler is `MPI_ERRORS_ARE_FATAL`,
 * the program will be aborted before calling the error handler of control
 * library.
 *
 * The error handler for this error should abort or return. Do not continue
 * processing using jump. Another required synchronization can remain since this
 * error will be raised immediately after the MPI calls.
 *
 * This function is available even if MPI is not linked. Is this case, the error
 * message will be "MPI error raised, but MPI is not linked".
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_mpi_error(const char *file, int line, int mpi_error_code,
                           const char *msg);

/**
 * @brief raise information type error
 * @param file source file location where  the error occuered
 * @param line source file line number where the error occuered
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_information_type_error(const char *file, int line,
                                        jcntrl_info key,
                                        jcntrl_information_datatype requested);

/**
 * @brief raise element type error
 * @param file source file location where the error occured.
 * @param line source file line number where the error occured.
 * @param expected Expected element type (element type set to array)
 * @param requested Requested elment type (element type trying to assign)
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_element_type_error(const char *file, int line,
                                    jcntrl_element_type expected,
                                    jcntrl_element_type requested);

/**
 * @brief raise pure virtual error
 * @param file source file location where the error occured.
 * @param line source file line number where the error occured.
 * @param base Base class which the virtual has been function declared
 * @param thisp The class that is calling
 * @param funcname Function name
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_pure_virtual_error(const char *file, int line,
                                    const jcntrl_shared_object_data *base,
                                    const jcntrl_shared_object_data *thisp,
                                    const char *funcname);

/**
 * @brief raise CSV Header file
 * @param file CSV file name
 * @param required_name Upstream exec name
 * @param csv_header_name CSV Header name
 * @param index Column index
 * @return The return value returned by callback function
 */
JUPITER_CONTROL_DECL
int jcntrl_raise_csv_header_error(const char *file, const char *required_name,
                                  jcntrl_data_array *csv_header_name,
                                  jcntrl_size_type index);

JUPITER_CONTROL_DECL_END

#endif
