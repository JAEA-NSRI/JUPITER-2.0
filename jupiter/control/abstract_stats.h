#ifndef JUPITER_CONTROL_ABSTRACT_STATS_H
#define JUPITER_CONTROL_ABSTRACT_STATS_H

#include "defs.h"
#include "shared_object.h"
#include "shared_object_priv.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_abstract_stat;
typedef struct jcntrl_abstract_stat jcntrl_abstract_stat;

struct jcntrl_abstract_stat_all;
typedef struct jcntrl_abstract_stat_all jcntrl_abstract_stat_all;

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_abstract_stat);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_abstract_stat_all);

//---

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_abstract_stat_object(jcntrl_abstract_stat *s);

JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_abstract_stat_executive(jcntrl_abstract_stat *s);

JUPITER_CONTROL_DECL
jcntrl_abstract_stat *jcntrl_abstract_stat_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
int jcntrl_abstract_stat_root_rank(jcntrl_abstract_stat *stat);
JUPITER_CONTROL_DECL
void jcntrl_abstract_stat_set_root_rank(jcntrl_abstract_stat *stat, int rank);

//---

JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_abstract_stat_all_object(jcntrl_abstract_stat_all *s);

JUPITER_CONTROL_DECL
jcntrl_executive *
jcntrl_abstract_stat_all_executive(jcntrl_abstract_stat_all *s);

JUPITER_CONTROL_DECL
jcntrl_abstract_stat_all *
jcntrl_abstract_stat_all_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
void jcntrl_abstract_stat_all_set_controller(
  jcntrl_abstract_stat_all *s, const jcntrl_mpi_controller *controller);

JUPITER_CONTROL_DECL
const jcntrl_mpi_controller *
jcntrl_abstract_stat_all_controller(jcntrl_abstract_stat_all *s);

JUPITER_CONTROL_DECL_END

#endif
