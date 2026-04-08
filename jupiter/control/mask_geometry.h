#ifndef MASK_GEOMETRY_H
#define MASK_GEOMETRY_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_mask_geometry_function;
typedef struct jcntrl_mask_geometry_function jcntrl_mask_geometry_function;

struct jcntrl_mask_geometry;
typedef struct jcntrl_mask_geometry jcntrl_mask_geometry;

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_geometry_function);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_geometry);

JUPITER_CONTROL_DECL
jcntrl_mask_geometry *jcntrl_mask_geometry_new(void);
JUPITER_CONTROL_DECL
void jcntrl_mask_geometry_delete(jcntrl_mask_geometry *m);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_mask_geometry_object(jcntrl_mask_geometry *m);
JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_mask_geometry_executive(jcntrl_mask_geometry *m);
JUPITER_CONTROL_DECL
jcntrl_mask_geometry *jcntrl_mask_geometry_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_mask_geometry_geometry_input(jcntrl_mask_geometry *m);
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_mask_geometry_vof_threshold_input(jcntrl_mask_geometry *m);

JUPITER_CONTROL_DECL
double jcntrl_mask_geometry_default_vof_threshold(jcntrl_mask_geometry *m);
JUPITER_CONTROL_DECL
void jcntrl_mask_geometry_set_default_vof_threshold(jcntrl_mask_geometry *m,
                                                    double value);

JUPITER_CONTROL_DECL
jcntrl_comparator jcntrl_mask_geometry_comparator(jcntrl_mask_geometry *m);

JUPITER_CONTROL_DECL
void jcntrl_mask_geometry_set_comparator(jcntrl_mask_geometry *m,
                                         jcntrl_comparator comp);

JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_mask_geometry_function_object(jcntrl_mask_geometry_function *f);

JUPITER_CONTROL_DECL
jcntrl_mask_geometry_function *
jcntrl_mask_geometry_function_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
jcntrl_geometry *
jcntrl_mask_geometry_function_geometry(jcntrl_mask_geometry_function *f);

JUPITER_CONTROL_DECL
void jcntrl_mask_geometry_function_set_geometry(
  jcntrl_mask_geometry_function *f, jcntrl_geometry *geometry);

JUPITER_CONTROL_DECL
double
jcntrl_mask_geometry_function_vof_threshold(jcntrl_mask_geometry_function *f);

JUPITER_CONTROL_DECL
void jcntrl_mask_geometry_function_set_vof_threshold(
  jcntrl_mask_geometry_function *f, double value);

JUPITER_CONTROL_DECL
jcntrl_comparator
jcntrl_mask_geometry_function_comparator(jcntrl_mask_geometry_function *f);

JUPITER_CONTROL_DECL
void jcntrl_mask_geometry_function_set_comparator(
  jcntrl_mask_geometry_function *f, jcntrl_comparator comp);

JUPITER_CONTROL_DECL_END

#endif
