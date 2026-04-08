#ifndef JUPITER_CONTROL_POSTP_MASK_H
#define JUPITER_CONTROL_POSTP_MASK_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_postp_mask;
typedef struct jcntrl_postp_mask jcntrl_postp_mask;

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_postp_mask);

JUPITER_CONTROL_DECL
jcntrl_postp_mask *jcntrl_postp_mask_new(void);
JUPITER_CONTROL_DECL
void jcntrl_postp_mask_delete(jcntrl_postp_mask *m);

JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_postp_mask_executive(jcntrl_postp_mask *m);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_postp_mask_object(jcntrl_postp_mask *m);
JUPITER_CONTROL_DECL
jcntrl_postp_mask *jcntrl_postp_mask_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_postp_mask_get_grid_input(jcntrl_postp_mask *m);
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_postp_mask_get_mask_input(jcntrl_postp_mask *m);

JUPITER_CONTROL_DECL
jcntrl_logical_operator jcntrl_postp_mask_get_op(jcntrl_postp_mask *m);
JUPITER_CONTROL_DECL
void jcntrl_postp_mask_set_op(jcntrl_postp_mask *m, jcntrl_logical_operator op);

JUPITER_CONTROL_DECL_END

#endif
