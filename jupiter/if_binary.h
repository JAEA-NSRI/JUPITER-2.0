
#ifndef JUPITER_IF_BINARY_H
#define JUPITER_IF_BINARY_H

#include "struct.h"
#include "geometry/defs.h"

#ifdef __cplusplus
extern "C" {
#endif

JUPITER_DECL
geom_error jupiter_install_init_func_binary(void);
JUPITER_DECL
int jupiter_init_func_binary_data_id(void);

JUPITER_DECL
void jupiter_init_func_binary_set_cdo(geom_init_element *el, domain *cdo);
JUPITER_DECL
void jupiter_init_func_binary_set_cdo_to_all_data(geom_data *data, domain *cdo);

JUPITER_DECL
int jupiter_init_func_binary_read_data(domain *cdo, mpi_param *mpi,
                                       geom_init_element *el, int force);

#ifdef __cplusplus
}
#endif

#endif
