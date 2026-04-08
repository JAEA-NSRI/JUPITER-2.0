
#ifndef JUPITER_CONTROL_FV_TABLE_H
#define JUPITER_CONTROL_FV_TABLE_H

#include "defs.h"
#include "shared_object.h"

#include <jupiter/table/table.h>

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_fv_table);

enum jcntrl_fv_table_extend_mode
{
  JCNTRL_FV_TABLE_EXTEND_INVALID,
  JCNTRL_FV_TABLE_EXTEND_EXTRAPOLATE,
  JCNTRL_FV_TABLE_EXTEND_NEAREST,
  JCNTRL_FV_TABLE_EXTEND_MIRROR,
  JCNTRL_FV_TABLE_EXTEND_CIRCULAR,
};
typedef enum jcntrl_fv_table_extend_mode jcntrl_fv_table_extend_mode;

struct jcntrl_fv_table;
typedef struct jcntrl_fv_table jcntrl_fv_table;

JUPITER_CONTROL_DECL
jcntrl_fv_table *jcntrl_fv_table_new(void);
JUPITER_CONTROL_DECL
void jcntrl_fv_table_delete(jcntrl_fv_table *fv);

JUPITER_CONTROL_DECL
jcntrl_fv_table *jcntrl_fv_table_downcast(jcntrl_shared_object *object);

JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_fv_table_executive(jcntrl_fv_table *fv);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_fv_table_object(jcntrl_fv_table *fv);

JUPITER_CONTROL_DECL
void jcntrl_fv_table_set_default_x(jcntrl_fv_table *fv, double x);
JUPITER_CONTROL_DECL
void jcntrl_fv_table_set_default_y(jcntrl_fv_table *fv, double y);
JUPITER_CONTROL_DECL
double jcntrl_fv_table_get_default_x(jcntrl_fv_table *fv);
JUPITER_CONTROL_DECL
double jcntrl_fv_table_get_default_y(jcntrl_fv_table *fv);

JUPITER_CONTROL_DECL
void jcntrl_fv_table_set_extend_mode(jcntrl_fv_table *fv,
                                     jcntrl_fv_table_extend_mode mode);

JUPITER_CONTROL_DECL
jcntrl_fv_table_extend_mode
jcntrl_fv_table_get_extend_mode(jcntrl_fv_table *fv);

JUPITER_CONTROL_DECL
int jcntrl_fv_table_set_table(jcntrl_fv_table *fv, table_data *table);
JUPITER_CONTROL_DECL
int jcntrl_fv_table_set_table_1d(jcntrl_fv_table *fv, int npt, const double *x,
                                 const double *v);
JUPITER_CONTROL_DECL
int jcntrl_fv_table_set_table_2d(jcntrl_fv_table *fv, int nx, int ny,
                                 const double *x, const double *y,
                                 const double *v);

JUPITER_CONTROL_DECL
double jcntrl_fv_table_get_interpolated(jcntrl_fv_table *fv, double x, double y,
                                        int *stat);

JUPITER_CONTROL_DECL
int jcntrl_install_fv_table(void);

JUPITER_CONTROL_DECL_END

#endif
