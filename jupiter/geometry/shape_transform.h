/**
 * @file   shape_transform.h
 *
 * @brief Supplemental functions for geom_shape_transform.
 */


#ifndef JUPITER_GEOMETRY_SHAPE_TRANSFORM_H
#define JUPITER_GEOMETRY_SHAPE_TRANSFORM_H

#include "defs.h"
#include "func_defs.h"
#include "mat43.h"

JUPITER_GEOMETRY_DECL_START

JUPITER_GEOMETRY_DECL
const geom_shape_funcs *geom_shape_transform_func(void);

JUPITER_GEOMETRY_DECL
void geom_shape_transform_set_data(geom_shape_transform *m, geom_data *master,
                                   void *p, const geom_shape_funcs *deleg);

JUPITER_GEOMETRY_DECL
void *geom_shape_transform_get_data(geom_shape_transform *m);

JUPITER_GEOMETRY_DECL
void geom_shape_transform_set_copy_num(geom_shape_transform *m, int n);

JUPITER_GEOMETRY_DECL
int geom_shape_transform_get_copy_num(geom_shape_transform *m);

JUPITER_GEOMETRY_DECL
geom_shape geom_shape_transform_get_shape(geom_shape_transform *m);

JUPITER_GEOMETRY_DECL
geom_mat43 geom_shape_transform_get_matrix(geom_shape_transform *m);

JUPITER_GEOMETRY_DECL_END

#endif
