#ifndef JUPITER_PRINT_PARAM_GEOM_H
#define JUPITER_PRINT_PARAM_GEOM_H

/*
 * print param for geom_init_element, geom_shape_element and
 * geom_surface_shape_element
 */

#include "common.h"
#include "geometry/defs.h"
#include "struct.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get geom_info_map for init function in @p init_el and print it.
 */
JUPITER_DECL
void PPgeom_init_func_info(flags *flg, int indent, const char *base_unit,
                           const char *length_unit, geom_init_element *init_el,
                           int *nogood);

/**
 * Get geom_info_map for shape in @p shape_el and print it.
 */
JUPITER_DECL
void PPgeom_shape_info(flags *flg, int indent, const char *base_unit,
                       const char *length_unit, geom_shape_element *shape_el,
                       int *nogood);

/**
 * Get geom_info_map for surface shape in @p surface_shape_el and print it.
 */
JUPITER_DECL
void PPgeom_surface_shape_info(flags *flg, int indent, const char *base_unit,
                               const char *length_unit,
                               geom_surface_shape_element *surface_shape_el,
                               int *nogood);

//--- JUPITER-extended data bound to geometry elements

/**
 * print_param for init_vof_data
 */
void PPgeom_init_vof_data(flags *flg, int indent, struct init_vof_data *data,
                          int *nogood);

/**
 * print_param for init_lpt_pewall_data
 */
void PPgeom_init_lpt_pewall_data(flags *flg, int indent,
                                 struct init_lpt_pewall_data *data,
                                 int *nogood);

/**
 * print_param for boundary_init_data
 */
void PPgeom_boundary_data(flags *flg, int indent,
                          struct boundary_init_data *data, char sub_header,
                          int *nogood);

/**
 * print_param for tboundary_init_data
 */
void PPgeom_tboundary_data(flags *flg, int indent,
                           struct tboundary_init_data *data, char sub_header,
                           int *nogood);

/**
 * print_param for surface_boundary_init_data
 */
void PPgeom_surface_boundary_data(flags *flg, int indent,
                                  struct surface_boundary_init_data *data,
                                  char sub_header, int *nogood);

#ifdef __cplusplus
}
#endif

#endif
