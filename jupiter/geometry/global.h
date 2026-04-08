
#ifndef JUPITER_GEOMETRY_GLOBAL_H
#define JUPITER_GEOMETRY_GLOBAL_H

#include <stdarg.h>

#include "defs.h"
#include "func_defs.h"

JUPITER_GEOMETRY_DECL_START

typedef void geom_warning_func(void *p, const char *file, long int line,
                               const char *func, const char *text);

typedef void geom_error_func(void *p, const char *file, long int line,
                             const char *func, const char *text);

JUPITER_GEOMETRY_DECL
void geom_set_warning_function(geom_warning_func *w, void *p);

JUPITER_GEOMETRY_DECL
geom_warning_func *geom_get_warning_function(void);

JUPITER_GEOMETRY_DECL
void geom_warn_x(const char *file, long int line, const char *func,
                 const char *format, ...);

JUPITER_GEOMETRY_DECL
void geom_vwarn_x(const char *file, long int line, const char *func,
                  const char *format, va_list ap);

#define geom_warn(...) geom_warn_x(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define geom_vwarn(f, a) geom_vwarn_x(__FILE__, __LINE__, __func__, (f), (a))

/**
 * @brief Install (custom) initialization function
 * @param funcs set of initialization function callbacks.
 * @return GEOM_SUCCESS if ok, GEOM_ERR_ALREADY_REGISTERED_INIT_FUNC if
 *         already registered or enum values are duplicated.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_install_init_func(geom_init_funcs *funcs);

/**
 * @brief Install (custom) shape
 * @param funcs set of shape function callbacks.
 * @return GEOM_SUCCESS if ok, GEOM_ERR_ALREADY_REGISTERED_SHAPE if
 *         already registered or enum values are duplicated.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_install_shape_func(geom_shape_funcs *funcs);

/**
 * @brief Install (custom) shape
 * @param funcs set of shape function callbacks.
 * @return GEOM_SUCCESS if ok, GEOM_ERR_ALREADY_REGISTERED_2D_SHAPE if
 *         already registered or enum values are duplicated.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_install_surface_shape_func(geom_surface_shape_funcs *funcs);

JUPITER_GEOMETRY_DECL
const geom_init_funcs *geom_get_init_func(geom_init_func f);

JUPITER_GEOMETRY_DECL
const geom_shape_funcs *geom_get_shape_func(geom_shape shape);

JUPITER_GEOMETRY_DECL
const geom_surface_shape_funcs *
geom_get_surface_shape_func(geom_surface_shape shape);

/**
 * @brief Initialize geometry internals
 */
JUPITER_GEOMETRY_DECL
void geom_initialize(void);

/**
 * @brief Test geometry is initialized
 * @return 1 if initialized, 0 if not.
 */
JUPITER_GEOMETRY_DECL
int geom_initialized(void);

JUPITER_GEOMETRY_DECL_END

#endif
