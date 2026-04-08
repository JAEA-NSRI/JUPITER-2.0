#ifndef JUPITER_CONTROL_MASK_POINT_H
#define JUPITER_CONTROL_MASK_POINT_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_mask_point;
typedef struct jcntrl_mask_point jcntrl_mask_point;

struct jcntrl_mask_point_function;
typedef struct jcntrl_mask_point_function jcntrl_mask_point_function;

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_point);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_point_function);

JUPITER_CONTROL_DECL
jcntrl_mask_point *jcntrl_mask_point_new(void);
JUPITER_CONTROL_DECL
void jcntrl_mask_point_delete(jcntrl_mask_point *p);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_mask_point_object(jcntrl_mask_point *p);
JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_mask_point_executive(jcntrl_mask_point *p);
JUPITER_CONTROL_DECL
jcntrl_mask_point *jcntrl_mask_point_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
void jcntrl_mask_point_set_default_point_x(jcntrl_mask_point *p, double x);
JUPITER_CONTROL_DECL
void jcntrl_mask_point_set_default_point_y(jcntrl_mask_point *p, double y);
JUPITER_CONTROL_DECL
void jcntrl_mask_point_set_default_point_z(jcntrl_mask_point *p, double z);

JUPITER_CONTROL_DECL
double jcntrl_mask_point_default_point_x(jcntrl_mask_point *p);
JUPITER_CONTROL_DECL
double jcntrl_mask_point_default_point_y(jcntrl_mask_point *p);
JUPITER_CONTROL_DECL
double jcntrl_mask_point_default_point_z(jcntrl_mask_point *p);

JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_mask_point_x_point_port(jcntrl_mask_point *p);
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_mask_point_y_point_port(jcntrl_mask_point *p);
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_mask_point_z_point_port(jcntrl_mask_point *p);

JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_mask_point_function_object(jcntrl_mask_point_function *p);

JUPITER_CONTROL_DECL
jcntrl_mask_point_function *
jcntrl_mask_point_function_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
void jcntrl_mask_point_function_set_point_x(jcntrl_mask_point_function *f,
                                            double x);
JUPITER_CONTROL_DECL
void jcntrl_mask_point_function_set_point_y(jcntrl_mask_point_function *f,
                                            double y);
JUPITER_CONTROL_DECL
void jcntrl_mask_point_function_set_point_z(jcntrl_mask_point_function *f,
                                            double z);

JUPITER_CONTROL_DECL
double jcntrl_mask_point_function_point_x(jcntrl_mask_point_function *f);
JUPITER_CONTROL_DECL
double jcntrl_mask_point_function_point_y(jcntrl_mask_point_function *f);
JUPITER_CONTROL_DECL
double jcntrl_mask_point_function_point_z(jcntrl_mask_point_function *f);

JUPITER_CONTROL_DECL_END

#endif
