
#ifndef JUPITER_FV_TIME_H
#define JUPITER_FV_TIME_H

#include "control/shared_object.h"
#include "struct.h"

#ifdef __cplusplus
extern "C" {
#endif

JUPITER_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jupiter_fv_time);
JUPITER_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jupiter_fv_delta_t);

JUPITER_DECL
int jupiter_fv_time_init(domain *cdo);
JUPITER_DECL
int jupiter_fv_delta_t_init(domain *cdo);
JUPITER_DECL
domain *jupiter_fv_time_downcast(jcntrl_executive *exe);
JUPITER_DECL
domain *jupiter_fv_delta_t_downcast(jcntrl_executive *exe);

JUPITER_DECL
void jupiter_fv_time_clean(domain *cdo);
JUPITER_DECL
void jupiter_fv_delta_t_clean(domain *cdo);

#ifdef __cplusplus
}
#endif

#endif
