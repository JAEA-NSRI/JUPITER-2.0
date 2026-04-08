#ifndef JUPITER_CONTROL_POSTP_VOLUME_INTEGRAL_H
#define JUPITER_CONTROL_POSTP_VOLUME_INTEGRAL_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_postp_volume_integral;
typedef struct jcntrl_postp_volume_integral jcntrl_postp_volume_integral;

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_postp_volume_integral);

JUPITER_CONTROL_DECL
jcntrl_postp_volume_integral *jcntrl_postp_volume_integral_new(void);

JUPITER_CONTROL_DECL
void jcntrl_postp_volume_integral_delete(jcntrl_postp_volume_integral *v);

JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_postp_volume_integral_object(jcntrl_postp_volume_integral *v);

JUPITER_CONTROL_DECL
jcntrl_executive *
jcntrl_postp_volume_integral_executive(jcntrl_postp_volume_integral *v);

JUPITER_CONTROL_DECL
jcntrl_postp_volume_integral *
jcntrl_postp_volume_integral_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
void jcntrl_postp_volume_integral_set_controller(
  jcntrl_postp_volume_integral *v, const jcntrl_mpi_controller *controller);

JUPITER_CONTROL_DECL
const jcntrl_mpi_controller *
jcntrl_postp_volume_integral_controller(jcntrl_postp_volume_integral *v);

JUPITER_CONTROL_DECL
void jcntrl_postp_volume_integral_set_root_rank(jcntrl_postp_volume_integral *v,
                                                int root);

JUPITER_CONTROL_DECL
int jcntrl_postp_volume_integral_root_rank(jcntrl_postp_volume_integral *v);

JUPITER_CONTROL_DECL
jcntrl_char_array *
jcntrl_postp_volume_integral_volume_varname(jcntrl_postp_volume_integral *v);

JUPITER_CONTROL_DECL
int jcntrl_postp_volume_integral_set_volume_varname(
  jcntrl_postp_volume_integral *v, jcntrl_data_array *d);

JUPITER_CONTROL_DECL
int jcntrl_postp_volume_integral_set_volume_varname_c(
  jcntrl_postp_volume_integral *v, const char *name, jcntrl_size_type clen);

JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_postp_volume_integral_cardinality_varname(
  jcntrl_postp_volume_integral *v);

JUPITER_CONTROL_DECL
int jcntrl_postp_volume_integral_set_cardinality_varname(
  jcntrl_postp_volume_integral *v, jcntrl_data_array *d);

JUPITER_CONTROL_DECL
int jcntrl_postp_volume_integral_set_cardinality_varname_c(
  jcntrl_postp_volume_integral *v, const char *name, jcntrl_size_type clen);

JUPITER_CONTROL_DECL_END

#endif
