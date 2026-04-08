#ifndef JUPITER_CONTROL_POSTP_SUM_H
#define JUPITER_CONTROL_POSTP_SUM_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_postp_sum;
typedef struct jcntrl_postp_sum jcntrl_postp_sum;

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_postp_sum);

JUPITER_CONTROL_DECL
jcntrl_postp_sum *jcntrl_postp_sum_new(void);
JUPITER_CONTROL_DECL
void jcntrl_postp_sum_delete(jcntrl_postp_sum *s);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_postp_sum_object(jcntrl_postp_sum *s);
JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_postp_sum_executive(jcntrl_postp_sum *s);
JUPITER_CONTROL_DECL
jcntrl_postp_sum *jcntrl_postp_sum_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
void jcntrl_postp_sum_set_controller(jcntrl_postp_sum *s,
                                     const jcntrl_mpi_controller *controller);

JUPITER_CONTROL_DECL
const jcntrl_mpi_controller *jcntrl_postp_sum_controller(jcntrl_postp_sum *s);

JUPITER_CONTROL_DECL
void jcntrl_postp_sum_set_root_rank(jcntrl_postp_sum *s, int root);

JUPITER_CONTROL_DECL
int jcntrl_postp_sum_root_rank(jcntrl_postp_sum *s);

JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_postp_sum_cardinality_varname(jcntrl_postp_sum *s);

JUPITER_CONTROL_DECL
int jcntrl_postp_sum_set_cardinality_varname(jcntrl_postp_sum *s,
                                             jcntrl_data_array *d);

JUPITER_CONTROL_DECL
int jcntrl_postp_sum_set_cardinality_varname_c(jcntrl_postp_sum *s,
                                               const char *name,
                                               jcntrl_size_type clen);

JUPITER_CONTROL_DECL_END

#endif
