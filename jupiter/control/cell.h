#ifndef JUPITER_CONTROL_CELL_H
#define JUPITER_CONTROL_CELL_H

/**
 * These functions are for implementation of post-processes and mask functions,
 * and not for using directly.
 */

#include "defs.h"
#include "shared_object.h"
#include "shared_object_priv.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_cell
{
  jcntrl_shared_object object;
};
#define jcntrl_cell__ancestor jcntrl_shared_object
#define jcntrl_cell__dnmem object
enum jcntrl_cell_vtable_names
{
  jcntrl_cell_number_of_points_id = JCNTRL_VTABLE_START(jcntrl_cell),
  jcntrl_cell_center_id,
  jcntrl_cell_get_point_id,
  jcntrl_cell_volume_id,
  jcntrl_cell_contain_id,
  JCNTRL_VTABLE_SIZE(jcntrl_cell),
};

enum jcntrl_cell_hex_has_neighbor
{
  JCNTRL_CELL_HEX_NEIGHBOR_W = 0x001,
  JCNTRL_CELL_HEX_NEIGHBOR_E = 0x002,
  JCNTRL_CELL_HEX_NEIGHBOR_S = 0x004,
  JCNTRL_CELL_HEX_NEIGHBOR_N = 0x008,
  JCNTRL_CELL_HEX_NEIGHBOR_B = 0x010,
  JCNTRL_CELL_HEX_NEIGHBOR_T = 0x020,
};

struct jcntrl_cell_hex
{
  jcntrl_cell cell;
  double p1[3];
  double p2[3];
  int neighbors;
};
#define jcntrl_cell_hex__ancestor jcntrl_cell
#define jcntrl_cell_hex__dnmem cell.jcntrl_cell__dnmem
JCNTRL_VTABLE_NONE(jcntrl_cell_hex);

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_cell);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_cell_hex);

// base

JUPITER_CONTROL_DECL
int jcntrl_cell_number_of_points(jcntrl_cell *c);
JUPITER_CONTROL_DECL
void jcntrl_cell_center(jcntrl_cell *c, double pnt[3]);
JUPITER_CONTROL_DECL
void jcntrl_cell_get_point(jcntrl_cell *c, int index, double pnt[3]);
JUPITER_CONTROL_DECL
double jcntrl_cell_volume(jcntrl_cell *c);
JUPITER_CONTROL_DECL
int jcntrl_cell_contain(jcntrl_cell *c, double x, double y, double z);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_cell_object(jcntrl_cell *c);
JUPITER_CONTROL_DECL
jcntrl_cell *jcntrl_cell_downcast(jcntrl_shared_object *obj);

// hex (hexahedron cell)

JUPITER_CONTROL_DECL
int jcntrl_cell_hex_init_g(jcntrl_cell_hex *h, //
                           jcntrl_grid_data *g, int i, int j, int k,
                           const int *whole_extent);
JUPITER_CONTROL_DECL
int jcntrl_cell_hex_init_s(jcntrl_cell_hex *h, //
                           const jcntrl_struct_grid *s, int i, int j, int k,
                           const int *whole_extent);

JUPITER_CONTROL_DECL
int jcntrl_cell_hex_number_of_points(jcntrl_cell_hex *c);
JUPITER_CONTROL_DECL
void jcntrl_cell_hex_center(jcntrl_cell_hex *c, double pnt[3]);
JUPITER_CONTROL_DECL
void jcntrl_cell_hex_get_point(jcntrl_cell_hex *c, int index, double pnt[3]);
JUPITER_CONTROL_DECL
double jcntrl_cell_hex_volume(jcntrl_cell_hex *c);

/**
 * @note Current implementation returns false at (x, y, z) on upper faces where
 * corresponding JCNTRL_CELL_HEX_NEIGHBOR_[ENT] is flagged.
 *
 * VTK (ParaView) implementation does not depend on cell connectivity etc, while
 * they can 'find' exactly one cell with least cell ID. VTK's strategy is hard
 * to implement for 'masking' cells.
 */
JUPITER_CONTROL_DECL
int jcntrl_cell_hex_contain(jcntrl_cell_hex *c, double x, double y, double z);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_cell_hex_object(jcntrl_cell_hex *c);
JUPITER_CONTROL_DECL
jcntrl_cell *jcntrl_cell_hex_cell(jcntrl_cell_hex *c);
JUPITER_CONTROL_DECL
jcntrl_cell_hex *jcntrl_cell_hex_downcast(jcntrl_shared_object *obj);

// number_of_points

typedef int jcntrl_cell_number_of_points_func(jcntrl_shared_object *obj);

struct jcntrl_cell_number_of_points_args
{
  int number_of_points;
};

static inline void
jcntrl_cell_number_of_points__wrapper(jcntrl_shared_object *obj, void *arg,
                                      jcntrl_cell_number_of_points_func *func)
{
  struct jcntrl_cell_number_of_points_args *p;
  p = (struct jcntrl_cell_number_of_points_args *)arg;
  p->number_of_points = func(obj);
}

JUPITER_CONTROL_DECL
int jcntrl_cell_number_of_points__super(
  const jcntrl_shared_object_data *ancestor);

// center

typedef void jcntrl_cell_center_func(jcntrl_shared_object *obj, double pnt[3]);

struct jcntrl_cell_center_args
{
  double *pnt;
};

static inline void jcntrl_cell_center__wrapper(jcntrl_shared_object *obj,
                                               void *arg,
                                               jcntrl_cell_center_func *func)
{
  struct jcntrl_cell_center_args *p;
  p = (struct jcntrl_cell_center_args *)arg;
  func(obj, p->pnt);
}

JUPITER_CONTROL_DECL
void jcntrl_cell_center__super(const jcntrl_shared_object_data *ancestor,
                               double pnt[3]);

// get_point

typedef void jcntrl_cell_get_point_func(jcntrl_shared_object *obj, int index,
                                        double pnt[3]);

struct jcntrl_cell_get_point_args
{
  int index;
  double *pnt;
};

static inline void
jcntrl_cell_get_point__wrapper(jcntrl_shared_object *obj, void *arg,
                               jcntrl_cell_get_point_func *func)
{
  struct jcntrl_cell_get_point_args *p;
  p = (struct jcntrl_cell_get_point_args *)arg;
  func(obj, p->index, p->pnt);
}

JUPITER_CONTROL_DECL
void jcntrl_cell_get_point__super(const jcntrl_shared_object_data *ancestor,
                                  int index, double pnt[3]);

// volume

typedef double jcntrl_cell_get_volume_func(jcntrl_shared_object *obj);

struct jcntrl_cell_volume_args
{
  double volume;
};

static inline void
jcntrl_cell_volume__wrapper(jcntrl_shared_object *obj, void *arg,
                            jcntrl_cell_get_volume_func *func)
{
  struct jcntrl_cell_volume_args *p;
  p = (struct jcntrl_cell_volume_args *)arg;
  p->volume = func(obj);
}

JUPITER_CONTROL_DECL
double jcntrl_cell_volume__super(const jcntrl_shared_object_data *ancestor);

// contain

typedef int jcntrl_cell_contain_func(jcntrl_shared_object *obj, //
                                     double x, double y, double z);

struct jcntrl_cell_contain_args
{
  double x, y, z;
  int ret;
};

static inline void
jcntrl_cell_contain__wrapper(jcntrl_shared_object *obj, void *arg,
                             jcntrl_cell_contain_func *func)
{
  struct jcntrl_cell_contain_args *p;
  p = (struct jcntrl_cell_contain_args *)arg;
  p->ret = func(obj, p->x, p->y, p->z);
}

JUPITER_CONTROL_DECL
int jcntrl_cell_contain__super(const jcntrl_shared_object_data *ancestor,
                               double x, double y, double z);

JUPITER_CONTROL_DECL_END

#endif
