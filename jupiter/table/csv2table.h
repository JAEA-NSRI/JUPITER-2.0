
#ifndef CSV2TABLE_H
#define CSV2TABLE_H

#include <stdio.h>

#include "table.h"

JUPITER_TABLE_DECL_START

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(JUPITER_CSV2TAB_EXPORT)
#define JUPITER_CSV2TAB_DECL __declspec(dllexport)
#define JUPITER_CSV2TAB_DECL_PRIVATE
#elif defined(JUPITER_CSV2TAB_IMPORT)
#define JUPITER_CSV2TAB_DECL __declspec(dllimport)
#define JUPITER_CSV2TAB_DECL_PRIVATE
#else
#define JUPITER_CSV2TAB_DECL
#define JUPITER_CSV2TAB_DECL_PRIVATE
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#if defined(JUPITER_CSV2TAB_EXPORT) || defined(JUPITER_CSV2TAB_IMPORT)
#define JUPITER_CSV2TAB_DECL __attribute__((visibility("default")))
#define JUPITER_CSV2TAB_DECL_PRIVATE __attribute__((visibility("hidden")))
#else
#define JUPITER_CSV2TAB_DECL
#define JUPITER_CSV2TAB_DECL_PRIVATE
#endif
#else
#define JUPITER_CSV2TAB_DECL
#define JUPITER_CSV2TAB_DECL_PRIVATE
#endif

enum csv2tab_error {
  CSV2TAB_SUCCESS = 0,
  CSV2TAB_ERR_EOF,
  CSV2TAB_ERR_NOMEM,
  CSV2TAB_ERR_INVALID_TOKEN,
  CSV2TAB_ERR_GEOMETRY_UNKNOWN,
  CSV2TAB_ERR_GEOMETRY_NOT_APPLICABLE,
  CSV2TAB_ERR_FLOAT_FORMAT,
  CSV2TAB_ERR_NODATA,
  CSV2TAB_ERR_INIT_TABLE,
  CSV2TAB_ERR_SYS,
};
typedef enum csv2tab_error csv2tab_error;

enum csv2tab_status {
  CSV2TAB_STAT_CLEAR       = 0x00000000,
  CSV2TAB_STAT_NAN         = 0x00000010, /*!< value of x, y or val is nan */
  CSV2TAB_STAT_NANX        = 0x00000011,
  CSV2TAB_STAT_NANY        = 0x00000012,
  CSV2TAB_STAT_NANV        = 0x00000014,
  CSV2TAB_STAT_INF         = 0x00000020, /*!< value of x, y or val is inf */
  CSV2TAB_STAT_INFX        = 0x00000021,
  CSV2TAB_STAT_INFY        = 0x00000022,
  CSV2TAB_STAT_INFV        = 0x00000024,
  CSV2TAB_STAT_MULTIPLE_XY = 0x00000100, /*!< Multiple data for same (x,y) */
  CSV2TAB_STAT_OVERFLOW    = 0x00001000, /*!< Overflow is detected */
  CSV2TAB_STAT_UNDERFLOW   = 0x00002000, /*!< Underflow is detected */
  CSV2TAB_STAT_SUMC_NONCONSTRAINT = 0x00010000,
  /*!< Not constraint for sum-const map.
    (ex. x0 + yn != x1 + y(n-1), Nx != Ny etc.) */
  CSV2TAB_STAT_DISCARD     = 0x00020000, /*!< Discarded node */
  CSV2TAB_STAT_INTERPOLATE = 0x00040000, /*!< Interpolated node */
};
typedef enum csv2tab_status csv2tab_status;

struct csv2tab_node;
typedef struct csv2tab_node csv2tab_node;

struct csv2tab_stat_node;
typedef struct csv2tab_stat_node csv2tab_stat_node;

struct csv2tab_data;
typedef struct csv2tab_data csv2tab_data;

/**
 * @brief allocate csv2table parameter data set
 * @return pointer to allocated data, NULL if failed
 */
JUPITER_CSV2TAB_DECL
csv2tab_data *csv2tab_init(void);

/**
 * @brief deallocates csv2table required data set
 * @return obj allocated data set
 */
JUPITER_CSV2TAB_DECL
void csv2tab_free(csv2tab_data *obj);

/**
 * @brief set column index for x axis values
 * @param data csv2table data
 * @param index index value to set
 *
 * If this function were not called, column 0 (i.e., column "A") will
 * be used.
 *
 * If index is negative, this fuction does nothing.
 */
JUPITER_CSV2TAB_DECL
void csv2tab_set_x_column(csv2tab_data *data, int index);

/**
 * @brief set column index for y axis values
 * @param data csv2table data
 * @param index index value to set
 *
 * If this function were not called, column 1 (i.e., column "B") will
 * be used.
 *
 * If index is negative, this function does nothing.
 */
JUPITER_CSV2TAB_DECL
void csv2tab_set_y_column(csv2tab_data *data, int index);

/**
 * @brief set column index for values (ex. solidus temperature, density, ...)
 * @param data csv2table data
 * @param index index value to set
 *
 * If this function were not called, column 2 (i.e., column "C") will
 * be used.
 *
 * If index is negative, this function does nothing.
 */
JUPITER_CSV2TAB_DECL
void csv2tab_set_v_column(csv2tab_data *data, int index);

/**
 * @brief set tolerance for testing equality in double comparison
 * @param data csv2table data
 * @param tol tolerance value
 *
 * If tol is negative value, tolerance is set to its absolute value.
 *
 * If tol is not finite (i.e. inf or nan), tolerance will be reverted
 * to default value.
 *
 * Default tolerance is 0. This value is absolute and the unit is
 * depend to what X or Y value means. For example, if X values are
 * temperature, the unit of tolerance will be Kelvin or such.
 *
 * In your CSV file, text representation of X (or Y) value should be
 * same for same value. So default is no tolerance (0.0).
 */
JUPITER_CSV2TAB_DECL
void csv2tab_set_tolerance(csv2tab_data *data, double tol);
JUPITER_CSV2TAB_DECL
double csv2tab_get_tolerance(csv2tab_data *data);

/**
 * @param set geometry type forcing flag
 * @paraam data csv2table data
 * @param geom Geometry to force
 *
 * If geom is invalid or not supported, automatic detection will be performed.
 */
JUPITER_CSV2TAB_DECL
void csv2tab_set_force_geometry(csv2tab_data *data, table_geometry geom);

/**
 * @param enable/disable interpolation for missing node.
 * @param data csv2table data
 * @param interpolate non-0 to enable, 0 to disable
 */
JUPITER_CSV2TAB_DECL
void csv2tab_set_interpolation(csv2tab_data *data, int interpolate);

/**
 * @param enable/disable adjust coordinates for sum-constant geometry
 * @param data csv2table data
 * @param adj_coord non-0 to enable, 0 to disable
 */
JUPITER_CSV2TAB_DECL
void csv2tab_set_adjust_coordinates(csv2tab_data *data, int adj_coord);

JUPITER_CSV2TAB_DECL
int csv2tab_convert(csv2tab_data *data, FILE *input, table_data *table,
                    csv2tab_error *errinfo, long *errline, long *errcol,
                    table_error *terror);

JUPITER_CSV2TAB_DECL
int csv2tab_test_node_flag(csv2tab_node *node, csv2tab_status flag);
JUPITER_CSV2TAB_DECL
csv2tab_status csv2tab_get_node_flag(csv2tab_node *node);
JUPITER_CSV2TAB_DECL
csv2tab_stat_node *csv2tab_get_stats(csv2tab_data *data);
JUPITER_CSV2TAB_DECL
csv2tab_stat_node *csv2tab_status_next(csv2tab_stat_node *stat);
JUPITER_CSV2TAB_DECL
csv2tab_node *csv2tab_get_node_by_stat(csv2tab_stat_node *stat);
JUPITER_CSV2TAB_DECL
csv2tab_stat_node *csv2tab_get_stat_by_node(csv2tab_node *node);
JUPITER_CSV2TAB_DECL
csv2tab_node *csv2tab_get_node_root(csv2tab_data *data);
JUPITER_CSV2TAB_DECL
double csv2tab_get_node_x(csv2tab_node *node);
JUPITER_CSV2TAB_DECL
double csv2tab_get_node_y(csv2tab_node *node);
JUPITER_CSV2TAB_DECL
double csv2tab_get_node_value(csv2tab_node *node);
JUPITER_CSV2TAB_DECL
csv2tab_node *csv2tab_get_node_x_next(csv2tab_node *node);
JUPITER_CSV2TAB_DECL
csv2tab_node *csv2tab_get_node_y_next(csv2tab_node *node);
JUPITER_CSV2TAB_DECL
long csv2tab_get_node_source_line(csv2tab_node *node);

JUPITER_CSV2TAB_DECL
const char *csv2tab_errorstr(csv2tab_error e);

JUPITER_TABLE_DECL_END

#endif
