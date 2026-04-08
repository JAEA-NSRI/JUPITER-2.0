/**
 * @addtogroup Table
 * @file table/table.h
 *
 * Table definitions and functions
 */

#ifndef JUPITER_TABLE_H
#define JUPITER_TABLE_H

#include <stddef.h>

#ifdef __cplusplus
#define JUPITER_TABLE_DECL_START extern "C" {
#define JUPITER_TABLE_DECL_END   }
#else
#define JUPITER_TABLE_DECL_START
#define JUPITER_TABLE_DECL_END
#endif

JUPITER_TABLE_DECL_START

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(JUPITER_TABLE_EXPORT)
#define JUPITER_TABLE_DECL __declspec(dllexport)
#define JUPITER_TABLE_DECL_PRIVATE
#elif defined(JUPITER_TABLE_IMPORT)
#define JUPITER_TABLE_DECL __declspec(dllimport)
#define JUPITER_TABLE_DECL_PRIVATE
#else
#define JUPITER_TABLE_DECL
#define JUPITER_TABLE_DECL_PRIVATE
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#if defined(JUPITER_TABLE_EXPORT) || defined(JUPITER_TABLE_IMPORT)
#define JUPITER_TABLE_DECL __attribute__((visibility("default")))
#define JUPITER_TABLE_DECL_PRIVATE __attribute__((visibility("hidden")))
#else
#define JUPITER_TABLE_DECL
#define JUPITER_TABLE_DECL_PRIVATE
#endif
#else
#define JUPITER_TABLE_DECL
#define JUPITER_TABLE_DECL_PRIVATE
#endif

/**
 * @ingroup Table
 * @macro TABLE_NO_IO_FUNCTIONS
 * @brief Skip compile io functions
 *
 * Table's IO functions depend that CHAR_BIT is 8.
 *
 * If you are going to build for processors
 * which CHAR_BIT is not 8, define this macro to omit IO functions.
 *
 * If you define this macro, definition of this macro is required both
 * when compiling table library itself and when using the library.
 */
//#define TABLE_NO_IO_FUNCTIONS

/**
 * @ingroup Table
 * @macro TABLE_SEARCH_ALG_PINNED
 * @brief Compile table with pinned search algorithm
 *
 * If you want to pin search algorithm, define this macro with
 * suffix after TABLE_SALG_ in enum of table_search_alg (ex. BIN_TREE).
 *
 * The advantage of using this macro is only guiding compiler
 * optimization, and generally only improves speed when the
 * inter-procedure optimization is enabled. (ex. -flto in clang and
 * gcc, -ipo (not -ipo-c) in icc)
 *
 * If you define this macro, definition of this macro is required only
 * when compiling table library itself, and should not define when
 * using the library.
 */
//#define TABLE_SEARCH_ALG_PINNED LINEAR

struct table_data;
typedef struct table_data table_data;

/**
 * @ingroup Table
 * @brief type for array size
 */
typedef ptrdiff_t table_size;
#define TABLE_SIZE_MAX PTRDIFF_MAX

/**
 * @ingroup Table
 * @brief type of array index (or iterator)
 */
typedef ptrdiff_t table_index;

/**
 * @ingroup Table
 * @brief table node
 */
struct table_node
{
  table_index x_index;
  table_index y_index;
};
typedef struct table_node table_node;

/**
 * @ingroup Table
 * @brief table geometry
 */
enum table_geometry
{
  TABLE_GEOMETRY_INVALID = 0,
  TABLE_GEOMETRY_RECTILINEAR = 1,
  TABLE_GEOMETRY_SUM_CONSTANT,
};
typedef enum table_geometry table_geometry;

/**
 * @ingroup Table
 * @brief table's interpolation mode
 */
enum table_interp_mode
{
  TABLE_INTERP_INVALID = 0,
  TABLE_INTERP_LINEAR = 1,
  TABLE_INTERP_BARYCENTRIC,
};
typedef enum table_interp_mode table_interp_mode;

/**
 * @ingroup Table
 * @brief table's search algorithms
 */
enum table_search_alg
{
  TABLE_SALG_INVALID = 0,
  TABLE_SALG_LINEAR = 1,
  TABLE_SALG_LINEAR_SAVE,
  TABLE_SALG_BIN_TREE_MAX,
  TABLE_SALG_BIN_TREE_MINMAX,
  /* TABLE_SALG_SPLAY_TREE */
};
typedef enum table_search_alg table_search_alg;

/**
 * @ingroup Table
 * @brief Error of table functions.
 */
enum table_error
{
  TABLE_SUCCESS = 0,      ///< No error
  TABLE_ERR_NOMEM = 1,    ///< Memory allocation failed
  TABLE_ERR_RANGE,        ///< Out of range (in arguement)
  TABLE_ERR_TABLE_RANGE,  ///< Out of range (in table)
  TABLE_ERR_NULLP,        ///< NULL pointer given for non-NULL argument
  TABLE_ERR_INVALID_GEOM, ///< Invalid geometry specified
  TABLE_ERR_INVALID_IPMODE, ///< Invalid interpolation mode specified
  TABLE_ERR_INVALID_SALG,   ///< Invalid seach algorithm specified
  TABLE_ERR_NOT_INITED,     ///< Table is not initialized
  TABLE_ERR_EOF,            ///< End of file while reading
  TABLE_ERR_FORMAT,         ///< Invalid binary format
  TABLE_ERR_ENDIAN,         ///< Binary file has wrong endianness
  TABLE_ERR_SYS,            ///< Please refer the errno
  TABLE_ERR_OVERFLOW,       ///< Overflow occured
};
typedef enum table_error table_error;

/**
 * @ingroup Table
 * @brief Returns string expression for table_error
 * @param e error value
 * @return pointer to string
 */
JUPITER_TABLE_DECL
const char *table_errorstr(table_error e);

/**
 * @ingroup Table
 * @brief Generate string expression for table_error
 * @param buf pointer to string to store the result.
 * @param e error value
 * @param errnoval errno value for TABLE_ERR_SYS.
 * @return Number of bytes written. -1 for allocation failure.
 *
 * If e is TABLE_ERR_SYS, this function formats string of strerror(errnoval).
 *
 * free allocated pointer by free() function.
 *
 * Returning value is usually not 0, but might be. This means
 * the text is empty, but a buffer is allocated.
 */
JUPITER_TABLE_DECL
int table_aerrorstr(char **buf, table_error e, int errnoval);

/**
 * @memberof table_data
 * @brief required data size with nx and ny data size
 * @param geom Geometry to be used
 * @param nx X size
 * @param ny Y size
 * @return required size of data, (size_t)-1 if nx or ny is too large
 *
 * Required data size for GEOMETRY_RECTILINEAR is
 *   nx * ny
 *
 * Required data size for GEOMETRY_TERNARY is
 *   1/2 * nx * (nx + 1)
 *   (Here, ny must be equal to nx. If not equal, returns `-1`).
 *
 * When overflowed, returns `-1`.
 *
 * This function is thread safe.
 */
JUPITER_TABLE_DECL
table_size table_calc_data_size(table_geometry geom, table_size nx,
                                table_size ny);

/**
 * @memberof table_data
 * @brief Compute the size for line at specified index in ternary mode.
 * @param nx X (or Y) size
 * @param index index to find for
 * @return size of Y (if you mean nx is X-axis) at index, size of X
 *         (same for Y-axis) at index, or -1 if index is out-of-range.
 */
JUPITER_TABLE_DECL
table_size table_calc_ternary_size_at(table_size nx, table_index index);

/**
 * @memberof table_data
 * @brief Allocate new empty table data.
 * @return allocated data, NULL if failed
 */
JUPITER_TABLE_DECL
table_data *table_alloc(void);

/**
 * @memberof table_data
 * @brief Initialize table data.
 * @param table Pointer to allocated table data
 * @param titile Title for table (must be ended with '\0')
 * @param geom Geometry to be used
 * @param nx Size of x-axis
 * @param ny Size of y-axis
 * @param interpmode Interpolation mode to be used
 * @param xpos X-Position list in array
 * @param ypos Y-Position list in array
 * @param data Table data in array
 *
 * See `table_calc_data_size` for rules in (geom, nx, ny)
 *
 * If data is given for xpos, ypos or data, this function copies them.
 * (To clarify what routine should response to deallocate data)
 */
JUPITER_TABLE_DECL
table_error table_init(table_data *table, const char *title,
                       table_geometry geom, table_size nx, table_size ny,
                       table_interp_mode interpmode, const double *xpos,
                       const double *ypos, const double *data);

/**
 * @memberof table_data
 * @brief test table is initialized and data is present.
 */
JUPITER_TABLE_DECL
int table_inited(table_data *table);

/**
 * @memberof table_data
 * @brief Set new search algorithm
 * @param table Table data.
 * @param alg algorithm to set.
 *
 * Set search algorithm to `alg`. If `alg` is not changed, nothing
 * will be done.
 */
JUPITER_TABLE_DECL
table_error table_set_algorithm(table_data *table, table_search_alg alg);

/**
 * @memberof table_data
 * @brief Get current search algorithm
 * @param table Table data
 */
JUPITER_TABLE_DECL
table_search_alg table_get_algorithm(table_data *table);

/**
 * @memberof table_data
 * @brief Reset internal storage to initial state.
 * @param table Table data.
 *
 * Reset internal storage to initial state.
 */
JUPITER_TABLE_DECL
table_error table_reset_search_state(table_data *table);

/**
 * @memberof table_data
 * @brief get x data in table
 *
 * You are not allowed to modify data.
 */
JUPITER_TABLE_DECL
const double *table_get_xdata(table_data *table);

/**
 * @memberof table_data
 * @brief get y data in table
 *
 * You are not allowed to modify data.
 */
JUPITER_TABLE_DECL
const double *table_get_ydata(table_data *table);

/**
 * @memberof table_data
 * @brief get table data in table
 *
 * You are not allowed to modify data.
 */
JUPITER_TABLE_DECL
const double *table_get_data(table_data *table);

/**
 * @brief get nx size
 * @return nx size, -1 if table is NULL.
 */
JUPITER_TABLE_DECL
table_size table_get_nx(table_data *table);

/**
 * @brief get ny size
 * @return ny size, -1 if table is NULL.
 */
JUPITER_TABLE_DECL
table_size table_get_ny(table_data *table);

/**
 * @memberof table_data
 * @brief get data size
 * @return data size, -1 if table is NULL.
 *
 * This function is equivalent to
 * ```
 * table_calc_data_size(table_get_geometry(table),
 *                      table_get_nx(table),
 *                      table_get_ny(table));
 * ```
 */
JUPITER_TABLE_DECL
table_size table_get_data_size(table_data *table);

/**
 * @memberof table_data
 * @brief get geometry info
 */
JUPITER_TABLE_DECL
table_geometry table_get_geometry(table_data *table);

/**
 * @memberof table_data
 * @brief get interpolation mode
 */
JUPITER_TABLE_DECL
table_interp_mode table_get_interp_mode(table_data *table);

/**
 * @memberof table_data
 * @brief get title of table
 * @return NULL if table is NULL or not initialized.
 *
 * Do not modify the content.
 *
 * Returned pointer is available until table_destroy, table_free,
 * table_init or table_set_title (for title is changing) is called.
 */
JUPITER_TABLE_DECL
const char *table_get_title(table_data *table);

/**
 * @memberof table_data
 * @brief Set title of table
 * @param table table data
 * @param title new title
 * @param n length of title (includes '\0')
 *
 * If title is NULL, "" is used for title.
 *
 * If n is 0, length is computed by strlen.
 *
 * This routine copies the string while set.
 */
JUPITER_TABLE_DECL
table_error table_set_title(table_data *table, const char *title, size_t n);

/**
 * @memberof table_data
 * @brief set interpolation mode
 */
JUPITER_TABLE_DECL
table_error table_set_interp_mode(table_data *table, table_interp_mode mode);

/**
 * @memberof table_data
 * @brief find the node contains (x,y) and returns the node.
 * @param x X value to calculate
 * @param y Y value to calculate
 * @param node location to store the result.
 */
JUPITER_TABLE_DECL
table_error table_search_node(table_data *table, double x, double y,
                              table_node *node);

/**
 * @memberof table_data
 * @brief Interpolate value at (x,y) with specified node
 * @param table Pointer to table data.
 * @param node Index of table
 * @param x X value to calculate
 * @param y Y value to calculate
 * @param err If you want detailed error info, pass pointer to table_error
 * @return the interpolated value, NAN if (x,y) is outside of node.
 */
JUPITER_TABLE_DECL
double table_interpolate(table_data *table, table_node node, double x, double y,
                         table_error *err);

/**
 * @memberof table_data
 * @brief find the node contains (x,y) and interpolate.
 * @param table Pointer to table data.
 * @param x X value to calculate
 * @param y Y value to calculate
 * @param err If you want detailed error info, pass pointer to table_error
 * @return the interpolated value, NAN if (x,y) is outside of table.
 */
JUPITER_TABLE_DECL
double table_search(table_data *table, double x, double y, table_error *err);

/**
 * @memberof table_data
 * @brief Destroy table data.
 * @param data Table data.
 *
 * Deallocate table content, but table_data structure itself
 * will be left allocated in empty.
 *
 * Note: There are less worth doing destroy and init than free, alloc,
 *       and then init.
 *
 */
JUPITER_TABLE_DECL
void table_destroy(table_data *table);

/**
 * @memberof table_data
 * @brief Deallocate table structure
 * @param data data to deallocate.
 */
JUPITER_TABLE_DECL
void table_free(table_data *table);

#ifndef TABLE_NO_IO_FUNCTTIONS
/**
 * @memberof table_data
 * @brief Read binary table data
 * @param table Table data
 * @param from path name read from.
 *
 * Old data will be discarded.
 */
JUPITER_TABLE_DECL
table_error table_read_binary(table_data *table, const char *from);

/**
 * @memberof table_data
 * @brief Write binary table data
 * @param table Table data
 * @param dest path name write to.
 */
JUPITER_TABLE_DECL
table_error table_write_binary(table_data *table, const char *dest);
#endif

/**
 * @memberof table_data
 * @brief Stop the execution of application, for assertion failure.
 *
 * You should not call this function directly.
 */
JUPITER_TABLE_DECL
void table_assert_impl(const char *file, long line, const char *cond_str);

/**
 * @macro TABLE_ASSERT(cond)
 * @brief assertion macro used in table functions.
 * @param cond Condition that assumes.
 */
#ifndef NDEBUG
#define TABLE_ASSERT(cond) \
  do { if ((cond) == 0) table_assert_impl(__FILE__, __LINE__, #cond); } while(0)
#define TABLE_UNREACHABLE() \
  table_assert_impl(__FILE__, __LINE__, "Unreachable reached")
#else
#define TABLE_ASSERT(cond)
#ifdef __GNUC__
#define TABLE_UNREACHABLE() __builtin_unreachable()
#else
#define TABLE_UNREACHABLE() (void)(0)
#endif
#endif

JUPITER_TABLE_DECL_END

#endif
