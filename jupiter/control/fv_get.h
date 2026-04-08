#ifndef JUPITER_CONTROL_FV_GET_H
#define JUPITER_CONTROL_FV_GET_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

/**
 * jcntrl_fv_get: Get optinally partial data of grid and generate field variable
 * from it.
 */

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_fv_get);

struct jcntrl_fv_get;
typedef struct jcntrl_fv_get jcntrl_fv_get;

JUPITER_CONTROL_DECL
int jcntrl_install_fv_get(void);

JUPITER_CONTROL_DECL
jcntrl_fv_get *jcntrl_fv_get_new(void);
JUPITER_CONTROL_DECL
void jcntrl_fv_get_delete(jcntrl_fv_get *p);

JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_fv_get_executive(jcntrl_fv_get *p);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_fv_get_object(jcntrl_fv_get *p);
JUPITER_CONTROL_DECL
jcntrl_fv_get *jcntrl_fv_get_downcast(jcntrl_shared_object *obj);

/**
 * jcntrl_fv_get does not obatain ownership of @p controller (does not increment
 * reference counter).
 */
JUPITER_CONTROL_DECL
void jcntrl_fv_get_set_controller(jcntrl_fv_get *p,
                                  const jcntrl_mpi_controller *controller);
JUPITER_CONTROL_DECL
const jcntrl_mpi_controller *jcntrl_fv_get_get_controller(jcntrl_fv_get *p);

JUPITER_CONTROL_DECL
int jcntrl_fv_get_set_varname_c(jcntrl_fv_get *p, const char *name);
JUPITER_CONTROL_DECL
int jcntrl_fv_get_set_varname(jcntrl_fv_get *p, jcntrl_data_array *array);
JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_fv_get_get_varname(jcntrl_fv_get *p);

/*
 * @param exclude_masked Whether exclude masked portion of source grid
 */
JUPITER_CONTROL_DECL
void jcntrl_fv_get_set_exclude_masked(jcntrl_fv_get *p, int exclude_masked);
JUPITER_CONTROL_DECL
int jcntrl_fv_get_get_exclude_masked(jcntrl_fv_get *p);

/*
 * @param extract_extent Whether extract in extent
 */
JUPITER_CONTROL_DECL
void jcntrl_fv_get_set_extract_extent(jcntrl_fv_get *p, int extract_extent);
JUPITER_CONTROL_DECL
int jcntrl_fv_get_get_extract_extent(jcntrl_fv_get *p);

JUPITER_CONTROL_DECL
void jcntrl_fv_get_set_extent(jcntrl_fv_get *p, const int extent[6]);
JUPITER_CONTROL_DECL
const int *jcntrl_fv_get_get_extent(jcntrl_fv_get *p);

JUPITER_CONTROL_DECL_END

#endif
