#include "component_data.h"
#include "component_data_defs.h"
#include "component_info_defs.h"
#include "control/defs.h"
#include "control/executive.h"
#include "csv.h"
#include "geometry/abuilder.h"
#include "geometry/data.h"
#include "geometry/defs.h"
#include "geometry/error.h"
#include "geometry/file.h"
#include "geometry/global.h"
#include "geometry/init.h"
#include "geometry/list.h"
#include "geometry/shape.h"
#include "geometry/surface_shape.h"
#include "geometry/svector.h"
#include "geometry/udata.h"
#include "geometry/variant.h"
#include "control/manager.h"

#include "boundary_util.h"
#include "csvutil_extra.h"
#include "field_control.h"
#include "geometry_source.h"
#include "if_binary.h"
#include "init_component.h"
#include "print_param_keywords.h"
#include "struct.h"
#include "func.h"
#include "csvutil.h"
#include "trip_control.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

struct set_geom_warn_data {
  const char *fname;
  csv_data *csv;
  csv_row *found_row;
  csv_column *found_col;
  int *stat;
  csv_error_level elv;
};

static
void set_geom_print_warn(void *p, const char *file, long int line,
                         const char *func, const char *text)
{
  long int col;
  const char *cp;
  csv_error_level elv;

  struct set_geom_warn_data *pp;
  pp = (struct set_geom_warn_data *)p;

  line = 0;
  col = 0;
  elv = CSV_EL_ERROR;
  cp = NULL;

  if (pp) {
    if (pp->found_col) {
      line = getCSVTextLineOrigin(pp->found_col);
      col = getCSVTextColumnOrigin(pp->found_col);
      cp = getCSVValue(pp->found_col);
    }
    if (pp->fname) {
      file = pp->fname;
    }
    elv = pp->elv;
    if (pp->stat) {
      if (elv >= CSV_EL_ERROR) {
        *pp->stat = 1;
      }
    }
  }
  csvperrorf(file, line, col, elv, cp, "%s", text);
}

static int
set_geom_shape_error_print(geom_error err,
                           geom_shape_data *data, geom_shape_element *el,
                           void *ext_data)
{
  struct set_geom_warn_data *p;
  struct jupiter_geom_ext_shp_eldata *elp;
  const struct geom_user_defined_data *udp;
  csv_error_level elv;
  int gcol;
  int brk;
  const char *file;
  long line;

  CSVASSERT(data);
  CSVASSERT(ext_data);

  line = __LINE__;
  file = __FILE__;

  elp = NULL;
  p = (struct set_geom_warn_data *)ext_data;
  elv = p->elv;
  if (p->fname) {
    file = p->fname;
  }
  if (el) {
    udp = geom_shape_element_get_extra_data(el);
    elp = (jupiter_geom_ext_shp_eldata *)geom_user_defined_data_get(udp);
  }

  gcol = 2; /* Use CSV column of shape. */
  brk = 0;
  p->elv = CSV_EL_ERROR;

  switch (err) {
  case GEOM_ERR_SHAPE_NOT_SET: /* Errors already processed */
    return 0;
  case GEOM_ERR_NOMEM:
    p->elv = CSV_EL_FATAL;
    brk = 1;
    break;
  case GEOM_ERR_SHAPE_OP_SHOULD_SET:
    p->elv = CSV_EL_WARN;
    gcol = 1; /* column of operator */
    break;
  case GEOM_ERR_SHAPE_STACK_OVERFLOW:
  case GEOM_ERR_SHAPE_STACK_UNCLOSED:
  case GEOM_ERR_INVALID_SHAPE_OP:
    gcol = 1;
    break;
  default:
    /* nop */
    break;
  }
  if (elp) {
    p->found_row = elp->row;
    p->fname = elp->file;
    p->found_col = getColumnOfCSV(p->found_row, gcol);
    elp->stk_error = err;
  } else if (el) {
    if (geom_shape_element_is_copied(el)) {
      p->found_col = getColumnOfCSV(p->found_row, gcol);
    }
  }

  set_geom_print_warn(p, file, line, "(n/a)", geom_strerror(err));

  p->elv = elv;
  return brk;
}

static int set_geom_surface_shape_error_print(geom_error err,
                                              geom_surface_shape_data *data,
                                              geom_surface_shape_element *el,
                                              void *ext_data)
{
  struct set_geom_warn_data *p;
  struct jupiter_geom_ext_sshp_eldata *elp;
  const struct geom_user_defined_data *udp;
  csv_error_level elv;
  int gcol;
  int brk;
  const char *file;
  long line;

  CSVASSERT(ext_data);

  file = __FILE__;
  line = __LINE__;
  brk = 0;
  p = (struct set_geom_warn_data *)ext_data;

  elv = p->elv;
  elp = NULL;

  if (p->fname) {
    file = p->fname;
  }
  p->elv = CSV_EL_ERROR;

  if (el) {
    udp = geom_surface_shape_element_get_extra_data(el);
    elp = (struct jupiter_geom_ext_sshp_eldata *)
      geom_user_defined_data_get(udp);
  }

  gcol = 1;
  switch(err) {
  case GEOM_ERR_NOMEM:
    brk = 1;
    p->elv = CSV_EL_FATAL;
    break;
  case GEOM_ERR_SHAPE_OP_SHOULD_SET:
    p->elv = CSV_EL_WARN;
    gcol = 1;
    break;
  case GEOM_ERR_SHAPE_STACK_OVERFLOW:
  case GEOM_ERR_SHAPE_STACK_UNCLOSED:
  case GEOM_ERR_INVALID_SHAPE_OP:
    gcol = 1;
    break;
  case GEOM_ERR_SHAPE_STACK_UNDERFLOW:
    gcol = 2;
    break;
  default:
    break;
  }

  if (elp) {
    p->found_row = elp->row;
    p->fname = elp->file;
    p->found_col = getColumnOfCSV(p->found_row, gcol);
    elp->stk_error = err;
  }

  set_geom_print_warn(p, file, line, "(n/a)", geom_strerror(err));

  p->elv = elv;
  return brk;
}

static geom_error
set_geom_with_builder(struct set_geom_warn_data *warn_data,
                      geom_args_builder *ab,
                      const char *geom_fname, csv_data *geom_csv,
                      csv_row *found_row, csv_column *found_col, int *stat,
                      jcntrl_executive_manager *manager,
                      controllable_type *control_head,
                      controllable_geometry_entry *control_geom_head)
{
  int abt;
  geom_error gerr;
  geom_error chk_err;
  geom_error set_err;
  geom_variant *var;
  geom_variant *einfo;
  controllable_geometry_entry *con_geom_p;
  SET_P_INIT(geom_csv, geom_fname, &found_row, &found_col);

  struct csv_to_controllable_type_data cnti = {
    .manager = manager,
    .head = control_head,
  };

  CSVASSERT(warn_data);

  warn_data->csv = geom_csv;
  warn_data->found_row = found_row;
  warn_data->fname = geom_fname;

  gerr = GEOM_SUCCESS;
  var = geom_variant_new(&gerr);
  if (!var) {
    if (stat) *stat = ON;
    return GEOM_ERR_NOMEM;
  }

  con_geom_p = NULL;

  /* no problem if it could not be allocated */
  einfo = geom_variant_new(&gerr);

  while ((abt = geom_args_builder_next(ab)) != GEOM_VARTYPE_NULL) {
    int optional = geom_args_builder_is_optional(ab);
    switch (abt) {
    case GEOM_VARTYPE_INT:
      {
        int iv;
        if (optional) {
          SET_P_NEXT_PASS_NOTFOUND(&iv, int, 0);
        } else {
          SET_P_NEXT(&iv, int, 0);
        }
        geom_variant_set_int(var, iv);
      }
      break;
    case GEOM_VARTYPE_DOUBLE:
      {
        con_geom_p = controllable_geometry_entry_new(con_geom_p);
        if (!con_geom_p) {
          gerr = GEOM_ERR_NOMEM;
          break;
        }

        cnti.dest = &con_geom_p->control[0];
        SET_P_NEXT(&cnti, controllable_type, 0.0);
        con_geom_p->type = GEOM_VARTYPE_DOUBLE;
        con_geom_p->index = geom_args_builder_get_loc(ab);
        controllable_type_update(&con_geom_p->control[0]);
        controllable_geometry_entry_set_to_variant(con_geom_p, var);
        if (con_geom_p->control[0].exec) {
          controllable_geometry_entry_add(control_geom_head, con_geom_p);
          con_geom_p = NULL;
        }
      }
      break;
    case GEOM_VARTYPE_STRING:
      {
        const char *p;
        if (optional) {
          SET_P_NEXT_PASS_NOTFOUND(&p, const_charp, NULL);
        } else {
          SET_P_NEXT(&p, const_charp, NULL);
        }
        geom_variant_set_string(var, p, 0);
      }
      break;
    case GEOM_VARTYPE_VECTOR2:
      {
        con_geom_p = controllable_geometry_entry_new(con_geom_p);
        if (!con_geom_p) {
          gerr = GEOM_ERR_NOMEM;
          break;
        }
        cnti.dest = &con_geom_p->control[0];
        if (optional) {
          SET_P_NEXT_PASS_NOTFOUND(&cnti, controllable_type, 0.0);
        } else {
          SET_P_NEXT(&cnti, controllable_type, 0.0);
        }
        cnti.dest = &con_geom_p->control[1];
        if (optional && !found_col) {
          SET_P_NEXT_PASS_NOTFOUND(&cnti, controllable_type, 0.0);
        } else {
          SET_P_NEXT(&cnti, controllable_type, 0.0);
        }
        con_geom_p->type = GEOM_VARTYPE_VECTOR2;
        con_geom_p->index = geom_args_builder_get_loc(ab);
        controllable_type_update(&con_geom_p->control[0]);
        controllable_type_update(&con_geom_p->control[1]);
        controllable_geometry_entry_set_to_variant(con_geom_p, var);
        if (con_geom_p->control[0].exec || con_geom_p->control[1].exec) {
          controllable_geometry_entry_add(control_geom_head, con_geom_p);
          con_geom_p = NULL;
        }
      }
      break;
    case GEOM_VARTYPE_VECTOR3:
      {
        con_geom_p = controllable_geometry_entry_new(con_geom_p);
        if (!con_geom_p) {
          gerr = GEOM_ERR_NOMEM;
          break;
        }
        cnti.dest = &con_geom_p->control[0];
        if (optional) {
          SET_P_NEXT_PASS_NOTFOUND(&cnti, controllable_type, 0.0);
        } else {
          SET_P_NEXT(&cnti, controllable_type, 0.0);
        }
        cnti.dest = &con_geom_p->control[1];
        if (optional && !found_col) {
          SET_P_NEXT_PASS_NOTFOUND(&cnti, controllable_type, 0.0);
        } else {
          SET_P_NEXT(&cnti, controllable_type, 0.0);
        }
        cnti.dest = &con_geom_p->control[2];
        if (optional && !found_col) {
          SET_P_NEXT_PASS_NOTFOUND(&cnti, controllable_type, 0.0);
        } else {
          SET_P_NEXT(&cnti, controllable_type, 0.0);
        }
        con_geom_p->type = GEOM_VARTYPE_VECTOR3;
        con_geom_p->index = geom_args_builder_get_loc(ab);
        controllable_type_update(&con_geom_p->control[0]);
        controllable_type_update(&con_geom_p->control[1]);
        controllable_type_update(&con_geom_p->control[2]);
        controllable_geometry_entry_set_to_variant(con_geom_p, var);
        if (con_geom_p->control[0].exec || con_geom_p->control[1].exec ||
            con_geom_p->control[2].exec) {
          controllable_geometry_entry_add(control_geom_head, con_geom_p);
          con_geom_p = NULL;
        }
      }
      break;
    case GEOM_VARTYPE_SIZE:
      {
        int iv;
        if (optional) {
          SET_P_NEXT_PASS_NOTFOUND(&iv, int, 0);
        } else {
          SET_P_NEXT(&iv, int, 0);
        }
        geom_variant_set_size_value(var, iv);
      }
      break;
    case GEOM_VARTYPE_SIZE_VECTOR3:
      {
        int ix, iy, iz;
        if (optional) {
          SET_P_NEXT_PASS_NOTFOUND(&ix, int, 0);
        } else {
          SET_P_NEXT(&ix, int, 0);
        }
        if (optional && !found_col) {
          SET_P_NEXT_PASS_NOTFOUND(&iy, int, 0);
        } else {
          SET_P_NEXT(&iy, int, 0);
        }
        if (optional && !found_col) {
          SET_P_NEXT_PASS_NOTFOUND(&iz, int, 0);
        } else {
          SET_P_NEXT(&iz, int, 0);
        }
        geom_variant_set_svec3(var, geom_svec3_c(ix, iy, iz));
      }
      break;
    case GEOM_VARTYPE_INT_OR_SVEC3:
      {
        csv_column *found_col_save;
        int ix, iy, iz;

        if (optional) {
          SET_P_NEXT_PASS_NOTFOUND(&ix, int, 0);
        } else {
          SET_P_NEXT(&ix, int, 0);
        }
        found_col_save = found_col;

        SET_P_NEXT_PASS_NOTFOUND(&iy, int, 0);
        if (!found_col) {
          found_col = found_col_save;
          geom_variant_set_int(var, ix);
        } else {
          SET_P_NEXT(&iz, int, 0);
          geom_variant_set_svec3(var, geom_svec3_c(ix, iy, iz));
        }
      }
      break;

    case JUPITER_VARTYPE_OUTPUT_MODE:
      {
        binary_output_mode om;
        if (optional) {
          SET_P_NEXT_PASS_NOTFOUND(&om, binary_output_mode, BINARY_OUTPUT_INVALID);
        } else {
          SET_P_NEXT(&om, binary_output_mode, BINARY_OUTPUT_INVALID);
        }
        geom_variant_set_enum(var, JUPITER_VARTYPE_OUTPUT_MODE, om);
      }
      break;

    case GEOM_VARTYPE_DATA_OPERATOR:
      {
        geom_data_operator op;
        if (optional) {
          SET_P_NEXT_PASS_NOTFOUND(&op, geom_data_op, GEOM_OP_INVALID);
        } else {
          SET_P_NEXT(&op, geom_data_op, GEOM_OP_INVALID);
        }
        geom_variant_set_data_op(var, op);
      }
      break;

    default:
      if (stat) *stat = ON;
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                 "[DEVELOPMENT NOTE] Currently, JUPITER only supports "
                 "int, double, vector[23], svector3, output_mode, "
                 "data_operator, geom_size_type or string "
                 "variant");
      continue;
    }

    if (warn_data) warn_data->found_col = found_col;

    if (!found_col) {
      geom_variant_nullify(var);
    }

    geom_variant_nullify(einfo);
    chk_err = geom_args_builder_check(ab, var, einfo);
    set_err = geom_args_builder_set_value(ab, var);

    if (chk_err == GEOM_SUCCESS) {
      chk_err = set_err;
    } else {
      if (set_err == GEOM_ERR_NOMEM) {
        chk_err = set_err;
      }
    }

    gerr = chk_err;
    if (gerr != GEOM_SUCCESS) {
      const char *str;
      csv_error_level el;
      el = CSV_EL_ERROR;
      if (gerr == GEOM_ERR_NOMEM) {
        el = CSV_EL_FATAL;
      }
      str = NULL;
      if (einfo) {
        str = geom_variant_get_string(einfo, NULL);
      }
      if (str) {
        char *buf;
        geom_error to_str_err;
        to_str_err = geom_variant_to_string(&buf, var);
        if (to_str_err == GEOM_SUCCESS) {
          SET_P_PERROR(ERROR, "%s: %s", buf, str);
          free(buf);
        } else {
          SET_P_PERROR(ERROR, "%s", buf, str);
        }
      } else {
        if (el == CSV_EL_FATAL) {
          SET_P_PERROR(FATAL, "Geometry error: %s", geom_strerror(gerr));
        } else {
          SET_P_PERROR(ERROR, "Geometry error: %s", geom_strerror(gerr));
        }
      }
      if (stat && gerr != GEOM_SUCCESS) *stat = ON;
      if (el == CSV_EL_FATAL) break;
      continue;
    }
  }

  if (con_geom_p) {
    controllable_geometry_entry_delete(con_geom_p);
  }
  geom_variant_delete(var);
  geom_variant_delete(einfo);
  return gerr;
}

static void
set_geom_warn_duplicate_row(csv_row *prev_set, csv_row *new_row,
                            const char *fname, const char *title)
{
  long lp;
  csv_column *pcol;
  csv_column *ncol;

  CSVASSERT(prev_set);
  CSVASSERT(new_row);

  pcol = getColumnOfCSV(prev_set, 0);
  ncol = getColumnOfCSV(new_row, 0);

  CSVASSERT(pcol);
  CSVASSERT(ncol);

  lp = getCSVTextLineOrigin(pcol);

  csvperrorf_col(fname, ncol, CSV_EL_ERROR,
                 "Multiple %s defined, previously defined at line %ld",
                 title, lp);
}

static void
jupiter_geom_ext_eldata_delete(void *p)
{
  struct jupiter_geom_ext_eldata *pp;

  if (!p) return;

  pp = (struct jupiter_geom_ext_eldata *)p;
  free(pp->dump_file);
  free(p);
}

static void
jupiter_geom_ext_data_delete(void *p)
{
  free(p);
}

static void
jupiter_geom_ext_shp_eldata_delete(void *p)
{
  struct jupiter_geom_ext_shp_eldata *data;

  if (p) {
    struct geom_list *lp, *ln, *lh;
    data = (struct jupiter_geom_ext_shp_eldata *)p;

    lh = &data->control_entry_head.list;
    geom_list_foreach_safe (lp, ln, lh) {
      controllable_geometry_entry *cp;
      cp = controllable_geometry_list_entry(lp);
      free(cp);
    }
  }
  free(p);
}

static void
jupiter_geom_ext_sshp_eldata_delete(void *p)
{
  struct jupiter_geom_ext_sshp_eldata *data;

  if (p) {
    struct geom_list *lp, *ln, *lh;
    data = (struct jupiter_geom_ext_sshp_eldata *)p;

    lh = &data->control_entry_head.list;
    geom_list_foreach_safe(lp, ln, lh) {
      controllable_geometry_entry *cp;
      cp = controllable_geometry_list_entry(lp);
      free(cp);
    }
  }
  free(p);
}

static void
jupiter_geom_ext_init_eldata_delete(void *p)
{
  struct jupiter_geom_ext_init_eldata *data;

  if (p) {
    data = (struct jupiter_geom_ext_init_eldata *)p;
    struct geom_list *lp, *ln, *lh;

    lh = &data->control_entry_head.list;
    geom_list_foreach_safe (lp, ln, lh) {
      controllable_geometry_entry *cp;
      cp = controllable_geometry_list_entry(lp);
      free(cp);
    }
  }
  free(p);
}

static void
jupiter_geom_ext_file_data_delete(void *p)
{
  free(p);
}

int get_geom_num(csv_data *geom_csv, const char *geom_file, csv_column **ret)
{
  int geom_num;
  csv_row *row;
  csv_column *col;
  SET_P_INIT(geom_csv, geom_file, &row, (ret ? ret : &col));

  SET_P(&geom_num, int, "NumberOfGeom", 1, 0);
  if (geom_num < 0) {
    SET_P_PERROR(ERROR, "NumberOfGeom must be positive");
    return -1;
  }
  return geom_num;
}

static struct boundary_init_data *boundary_init_data_new(void)
{
  struct boundary_init_data *p;
  p = (struct boundary_init_data *)malloc(sizeof(struct boundary_init_data));
  if (!p)
    return NULL;

  p->dir = BOUNDARY_DIR_NONE;
  p->threshold = HUGE_VAL;

  fluid_boundary_data_init(&p->lhead);
  p->data = fluid_boundary_data_new(&p->lhead);
  if (!p->data) {
    fluid_boundary_data_delete_all(&p->lhead);
    free(p);
    return NULL;
  }

  return p;
}

static void boundary_init_data_delete(void *p)
{
  if (p) {
    struct boundary_init_data *pp;
    pp = (struct boundary_init_data *)p;
    fluid_boundary_data_delete_all(&pp->lhead);
  }
  free(p);
}

static int set_geom_boundary_inlet_id(struct inlet_component_element *e,
                                      csv_data *csv, const char *fname,
                                      csv_row **row, csv_column **col,
                                      component_data *comp_data_head,
                                      int *status)
{
  struct component_info_data edd;
  struct csv_to_component_info_data_data cidd = {
    .comp_data_head = comp_data_head,
  };
  SET_P_INIT(csv, fname, row, col);

  cidd.dest = e ? &e->comp : &edd;
  SET_P_NEXT(&cidd, component_info_data, JUPITER_ID_INVALID);
  if (!cidd.dest->d)
    if (status)
      *status = ON;

  return 0;
}

static int set_geom_fluid_inlet_const_boundary(
  fluid_boundary_data *data, csv_data *geom_csv, const char *geom_file,
  component_data *comp_data_head, csv_row *found_row, csv_column **start_col,
  int *status)
{
  int ncomp;
  int nacomp;
  int i;
  int r;

  SET_P_INIT(geom_csv, geom_file, &found_row, start_col);

  r = 0;
  controllable_type_init(&data->inlet_vel_u);
  controllable_type_init(&data->inlet_vel_v);
  controllable_type_init(&data->inlet_vel_w);

  SET_P_NEXT(&data->inlet_vel_u.current_value, exact_double, HUGE_VAL);
  if (!SET_P_PERROR_FINITE(data->inlet_vel_u.current_value, ERROR,
                           "Inlet velocity (U) must be finite")) {
    if (status)
      *status = ON;
  }

  SET_P_NEXT(&data->inlet_vel_v.current_value, exact_double, HUGE_VAL);
  if (!SET_P_PERROR_FINITE(data->inlet_vel_v.current_value, ERROR,
                           "Inlet velocity (V) must be finite")) {
    if (status)
      *status = ON;
  }

  SET_P_NEXT(&data->inlet_vel_w.current_value, exact_double, HUGE_VAL);
  if (!SET_P_PERROR_FINITE(data->inlet_vel_w.current_value, ERROR,
                           "Inlet velocity (W) must be finite")) {
    if (status)
      *status = ON;
  }

  SET_P_NEXT(&ncomp, int, -1);

  nacomp = 0;
  data->comps = inlet_component_data_new(ncomp);
  if (data->comps) {
    nacomp = inlet_component_data_ncomp(data->comps);
  } else {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    if (status)
      *status = ON;
    r = 1;
  }
  if (ncomp <= 0) {
    SET_P_PERROR(ERROR, "Number of inlet components must be positive");
    if (status) {
      *status = ON;
    }
  }

  for (i = 0; i < ncomp; ++i) {
    struct inlet_component_element *e;
    double ratio;

    e = NULL;
    if (data->comps && i < nacomp)
      e = inlet_component_data_get(data->comps, i);

    set_geom_boundary_inlet_id(e, geom_csv, geom_file, &found_row, start_col,
                               comp_data_head, status);

    SET_P_NEXT(&ratio, exact_double, HUGE_VAL);
    if (!SET_P_PERROR_GREATER(ratio, 0.0, ON, OFF, ERROR,
                              "Inlet ratio must be positive value")) {
      if (status)
        *status = ON;
    }
    if (e)
      e->ratio.current_value = ratio;
  }

  return r;
}

static int set_geom_fluid_inlet_control_boundary(
  fluid_boundary_data *data, csv_data *geom_csv, const char *geom_file,
  component_data *comp_data_head, csv_row *found_row, csv_column **start_col,
  jcntrl_executive_manager *manager, controllable_type *control_head,
  int *status)
{
  int ncomp;
  int nacomp;
  int id;
  int i;
  int r;
  controllable_type ratio;
  struct csv_to_controllable_type_data cset = {
    .manager = manager,
    .head = control_head,
  };

  SET_P_INIT(geom_csv, geom_file, &found_row, start_col);

  r = 0;
  controllable_type_init(&ratio);

  cset.dest = &data->inlet_vel_u;
  SET_P_NEXT(&cset, controllable_type, 0.0);

  cset.dest = &data->inlet_vel_v;
  SET_P_NEXT(&cset, controllable_type, 0.0);

  cset.dest = &data->inlet_vel_w;
  SET_P_NEXT(&cset, controllable_type, 0.0);

  SET_P_NEXT(&ncomp, int, -1);

  data->comps = inlet_component_data_new(ncomp);
  nacomp = 0;
  if (data->comps) {
    nacomp = inlet_component_data_ncomp(data->comps);
  } else {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    if (status)
      *status = ON;
    r = 1;
  }
  if (ncomp <= 0) {
    SET_P_PERROR(ERROR, "Number of inlet components must be positive");
    if (status)
      *status = ON;
  }

  for (i = 0; i < ncomp; ++i) {
    struct inlet_component_element *e;

    e = NULL;
    if (data->comps && i < nacomp)
      e = inlet_component_data_get(data->comps, i);

    set_geom_boundary_inlet_id(e, geom_csv, geom_file, &found_row, start_col,
                               comp_data_head, status);

    if (e) {
      cset.dest = &e->ratio;
    } else {
      cset.dest = &ratio;
    }
    SET_P_NEXT(&cset, controllable_type, 0.0);
    if (cset.dest == &ratio)
      controllable_type_remove_from_list(&ratio);
  }

  return r;
}

static int set_geom_fluid_inlet_boundary(
  fluid_boundary_data *data, csv_data *geom_csv, const char *geom_file,
  component_data *comp_data_head, csv_row *found_row, csv_column **start_col,
  jcntrl_executive_manager *manager, controllable_type *control_head,
  int *status)
{
  SET_P_INIT(geom_csv, geom_file, &found_row, start_col);

  SET_P_NEXT(&data->control, trip_control, TRIP_CONTROL_INVALID);

  switch (data->control) {
  case TRIP_CONTROL_CONST:
    return set_geom_fluid_inlet_const_boundary(data, geom_csv, geom_file,
                                               comp_data_head, found_row,
                                               start_col, status);
  case TRIP_CONTROL_CONTROL:
    return set_geom_fluid_inlet_control_boundary(data, geom_csv, geom_file,
                                                 comp_data_head, found_row,
                                                 start_col, manager,
                                                 control_head, status);

  case TRIP_CONTROL_PULSE: /* not implemented yet */
  case TRIP_CONTROL_INVALID:
    break;
  }

  SET_P_PERROR(ERROR, "Invalid trip control method");
  if (status)
    *status = ON;
  return 0;
}

static int set_geom_fluid_out_neumann_boundary(
  fluid_boundary_data *data, csv_data *geom_csv, const char *geom_file,
  csv_row *found_row, csv_column **start_col, int *status)
{
  /* Reserved for future implementation, NOP */
  return 0;
}

static int set_geom_fluid_out_const_boundary(
  fluid_boundary_data *data, csv_data *geom_csv, const char *geom_file,
  csv_row *found_row, csv_column **start_col, jcntrl_executive_manager *manager,
  controllable_type *control_head, int *status)
{
  struct csv_to_controllable_type_data cnti = {
    .manager = manager,
    .head = control_head,
  };

  SET_P_INIT(geom_csv, geom_file, &found_row, start_col);

  cnti.dest = &data->const_p;
  SET_P_NEXT(&cnti, controllable_type, 0.0);
  return 0;
}

static int set_geom_fluid_out_boundary(
  fluid_boundary_data *data, csv_data *geom_csv, const char *geom_file,
  csv_row *found_row, csv_column **start_col, jcntrl_executive_manager *manager,
  controllable_type *control_head, int *status)
{
  SET_P_INIT(geom_csv, geom_file, &found_row, start_col);

  SET_P_NEXT(&data->out_p_cond, out_p_cond, OUT_P_COND_INVALID);

  switch (data->out_p_cond) {
  case OUT_P_COND_NEUMANN:
    return set_geom_fluid_out_neumann_boundary(data, geom_csv, geom_file,
                                               found_row, start_col, status);
  case OUT_P_COND_CONST:
    return set_geom_fluid_out_const_boundary(data, geom_csv, geom_file,
                                             found_row, start_col, manager,
                                             control_head, status);
  case OUT_P_COND_INVALID:
    break;
  }

  SET_P_PERROR(ERROR, "Invalid OUT pressure condition");
  if (status)
    *status = ON;
  return 0;
}

static int
set_geom_fluid_boundary(geom_init_element *init_el,
                        csv_data *geom_csv, const char *geom_file,
                        component_data *comp_data_head,
                        csv_row *found_row, csv_column **start_col,
                        jcntrl_executive_manager *manager,
                        controllable_type *control_head, int *status)
{
  struct boundary_init_data *bdata;
  fluid_boundary_data *data;
  int i;
  SET_P_INIT(geom_csv, geom_file, &found_row, start_col);

  CSVASSERT(init_el);

  bdata = boundary_init_data_new();
  if (!bdata) {
    if (status)
      *status = ON;
    return 1;
  }

  geom_init_element_set_component(init_el, INIT_COMPONENT_BOUNDARY, bdata,
                                  boundary_init_data_delete, "Fluid Boundary");

  CSVASSERT(bdata->data);
  data = bdata->data;

  SET_P_NEXT(&bdata->dir, boundary_dir, BOUNDARY_DIR_NONE);
  if (bdata->dir == BOUNDARY_DIR_NONE) {
    SET_P_PERROR(ERROR, "Invalid boundary direction");
    if (status)
      *status = ON;
  }

  SET_P_NEXT(&data->cond, boundary, -1);
  if (data->cond == -1) {
    SET_P_PERROR(ERROR, "Invalid fluid boundary condition");
    if (status)
      *status = ON;
  }

  SET_P_NEXT(&bdata->threshold, exact_double, HUGE_VAL);
  if (!SET_P_PERROR_FINITE(bdata->threshold,
                           ERROR, "Threshold must be finite")) {
    if (status)
      *status = ON;
  }

  if (data->cond == INLET)
    return set_geom_fluid_inlet_boundary(data, geom_csv, geom_file,
                                         comp_data_head, found_row, start_col,
                                         manager, control_head, status);

  if (data->cond == OUT)
    return set_geom_fluid_out_boundary(data, geom_csv, geom_file, found_row,
                                       start_col, manager, control_head,
                                       status);

  return 0;
}

static struct tboundary_init_data *tboundary_init_data_new(void)
{
  struct tboundary_init_data *p;
  p = (struct tboundary_init_data *)malloc(sizeof(struct tboundary_init_data));

  p->dir = BOUNDARY_DIR_NONE;
  p->threshold = HUGE_VAL;

  thermal_boundary_data_init(&p->lhead);
  p->data = thermal_boundary_data_new(&p->lhead);
  if (!p->data) {
    thermal_boundary_data_delete_all(&p->lhead);
    free(p);
    return NULL;
  }

  return p;
}

static void tboundary_init_data_delete(void *p)
{
  if (p) {
    struct tboundary_init_data *pp;
    pp = (struct tboundary_init_data *)p;
    thermal_boundary_data_delete_all(&pp->lhead);
  }
  free(p);
}

static int
set_geom_thermal_boundary(geom_init_element *init_el,
                          csv_data *geom_csv, const char *geom_file,
                          component_data *comp_data_head,
                          csv_row *found_row, csv_column **start_col,
                          jcntrl_executive_manager *manager,
                          controllable_type *control_head, int *status)
{
  struct tboundary_init_data *tdata;
  thermal_boundary_data *data;
  SET_P_INIT(geom_csv, geom_file, &found_row, start_col);

  CSVASSERT(init_el);

  tdata = tboundary_init_data_new();
  if (!tdata) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM,
              0, 0, NULL);
    if (status) {
      *status = ON;
    }
    return 1;
  }

  CSVASSERT(tdata->data);
  data = tdata->data;

  SET_P_NEXT(&tdata->dir, boundary_dir, BOUNDARY_DIR_NONE);
  if (tdata->dir == BOUNDARY_DIR_NONE) {
    SET_P_PERROR(ERROR, "Invalid boundary direction");
    if (status)
      *status = ON;
  }

  SET_P_NEXT(&data->cond, tboundary, -1);
  if (data->cond == -1) {
    SET_P_PERROR(ERROR, "Invalid thermal boundary condition");
    if (status)
      *status = ON;
  }

  SET_P_NEXT(&tdata->threshold, exact_double, HUGE_VAL);
  if (!SET_P_PERROR_FINITE(tdata->threshold, ERROR,
                           "Threshold must be finite")) {
    if (status)
      *status = ON;
  }

  if (data->cond == ISOTHERMAL) {
    SET_P_NEXT(&data->control, trip_control, TRIP_CONTROL_INVALID);

    if (data->control == TRIP_CONTROL_CONST) {
      controllable_type_init(&data->temperature);

      SET_P_NEXT(&data->temperature.current_value, exact_double, HUGE_VAL);
      SET_P_PERROR_FINITE(data->temperature.current_value, ERROR,
                          "Temperature must be finite");

    } else if (data->control == TRIP_CONTROL_CONTROL) {
      struct csv_to_controllable_type_data cset = {
        .manager = manager,
        .head = control_head,
      };

      cset.dest = &data->temperature;
      SET_P_NEXT(&cset, controllable_type, 0.0);

    } else {
      SET_P_PERROR(ERROR, "Invalid trip control method");
      if (status)
        *status = ON;
    }

  } else if (data->cond == DIFFUSION) {
    SET_P_NEXT(&data->diffusion_limit, exact_double, 0.0);
    SET_P_PERROR_FINITE(data->diffusion_limit, ERROR,
                        "Temperature limit must be finite");
  }

  geom_init_element_set_component(init_el, INIT_COMPONENT_THERMAL_BOUNDARY,
                                  tdata, tboundary_init_data_delete,
                                  "Thermal Boundary");
  return 0;
}

static struct surface_boundary_init_data *surface_boundary_init_data_new(void)
{
  struct surface_boundary_init_data *p;
  p = (struct surface_boundary_init_data *)
    malloc(sizeof(struct surface_boundary_init_data));
  if (!p)
    return NULL;

  p->threshold = HUGE_VAL;

  surface_boundary_data_init(&p->lhead);
  p->data = surface_boundary_data_new(&p->lhead);
  if (!p->data) {
    surface_boundary_data_delete_all(&p->lhead);
    free(p);
    return NULL;
  }
  return p;
}

static void surface_boundary_init_data_delete(void *p)
{
  if (p) {
    struct surface_boundary_init_data *pp;
    pp = (struct surface_boundary_init_data *)p;
    surface_boundary_data_delete_all(&pp->lhead);
  }
  free(p);
}

static int set_geom_surface_boundary(geom_init_element *init_el,
                                     csv_data *geom_csv, const char *geom_file,
                                     component_data *comp_data_head,
                                     csv_row *found_row, csv_column **start_col,
                                     jcntrl_executive_manager *manager,
                                     controllable_type *control_head,
                                     int *status)
{
  struct surface_boundary_init_data *sdata;
  surface_boundary_data *data;
  SET_P_INIT(geom_csv, geom_file, &found_row, start_col);

  CSVASSERT(init_el);

  sdata = surface_boundary_init_data_new();
  if (!sdata) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
    if (status)
      *status = ON;
    return 1;
  }

  CSVASSERT(sdata->data);
  data = sdata->data;

  controllable_type_init(&data->normal_inlet_vel);

  SET_P_NEXT(&data->cond, boundary, -1);
  if (data->cond != INLET) {
    SET_P_PERROR(ERROR, "Currently supports IN only");
    if (status)
      *status = ON;
  }

  SET_P_NEXT(&sdata->threshold, double, 1.0);
  if (!SET_P_PERROR_FINITE(sdata->threshold, ERROR,
                          "Threshold value must be finite")) {
    if (status)
      *status = ON;
  }

  if (data->cond == INLET) {
    SET_P_NEXT(&data->inlet_dir, inlet_dir, SURFACE_INLET_DIR_INVALID);
    if (data->inlet_dir != SURFACE_INLET_DIR_NORMAL) {
      SET_P_PERROR(ERROR, "Currently supports NORMAL only");
      if (status)
        *status = ON;
    }

    SET_P_NEXT(&data->control, trip_control, TRIP_CONTROL_INVALID);
    if (data->control == TRIP_CONTROL_CONST) {
      int ncomp;
      int nacomp;
      int id;
      double ratio;

      if (data->inlet_dir == SURFACE_INLET_DIR_NORMAL) {

        SET_P_NEXT(&data->normal_inlet_vel.current_value, exact_double, 0.0);
        if (!SET_P_PERROR_FINITE(data->normal_inlet_vel.current_value, ERROR,
                                 "Inlet velocity must be finite")) {
          if (status)
            *status = ON;
        }

        SET_P_NEXT(&ncomp, int, 0);

        nacomp = 0;
        data->comps = inlet_component_data_new(ncomp);
        if (data->comps) {
          nacomp = inlet_component_data_ncomp(data->comps);
        } else {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                    0, NULL);
          if (status)
            *status = ON;
        }
        if (ncomp <= 0) {
          SET_P_PERROR(ERROR, "Number of inlet components must be positive");
          if (status)
            *status = ON;
        }

        for (int i = 0; i < ncomp; ++i) {
          struct inlet_component_element *e;

          e = NULL;
          if (data->comps && i < nacomp)
            e = inlet_component_data_get(data->comps, i);

          set_geom_boundary_inlet_id(e, geom_csv, geom_file, &found_row,
                                     start_col, comp_data_head, status);

          SET_P_NEXT(&ratio, exact_double, HUGE_VAL);
          if (!SET_P_PERROR_GREATER(ratio, 0.0, ON, OFF, ERROR,
                                    "Inlet ratio must be positive value")) {
            if (status)
              *status = ON;
          }
          if (e)
            e->ratio.current_value = ratio;
        }
      }
    } else if (data->control == TRIP_CONTROL_CONTROL) {
      int ncomp;
      int nacomp;
      int id;
      controllable_type ratio;
      struct csv_to_controllable_type_data cset = {
        .manager = manager,
        .head = control_head,
      };

      controllable_type_init(&ratio);

      if (data->inlet_dir == SURFACE_INLET_DIR_NORMAL) {
        cset.dest = &data->normal_inlet_vel;
        SET_P_NEXT(&cset, controllable_type, 0.0);

        SET_P_NEXT(&ncomp, int, 0);

        nacomp = 0;
        data->comps = inlet_component_data_new(ncomp);
        if (data->comps) {
          nacomp = inlet_component_data_ncomp(data->comps);
        } else {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                    0, NULL);
          if (status)
            *status = ON;
        }
        if (ncomp <= 0) {
          SET_P_PERROR(ERROR, "Number of components must be positive");
          if (status)
            *status = ON;
        }

        for (int i = 0; i < ncomp; ++i) {
          struct inlet_component_element *e;

          e = NULL;
          if (data->comps && i < nacomp)
            e = inlet_component_data_get(data->comps, i);

          set_geom_boundary_inlet_id(e, geom_csv, geom_file, &found_row,
                                     start_col, comp_data_head, status);

          if (e) {
            cset.dest = &e->ratio;
          } else {
            cset.dest = &ratio;
          }
          SET_P_NEXT(&cset, controllable_type, 0.0);
          if (cset.dest == &ratio)
            controllable_type_remove_from_list(&ratio);
        }
      }
    } else {
      SET_P_PERROR(ERROR, "Invalid or not supported trip control method");
      if (status)
        *status = ON;
    }
  }

  geom_init_element_set_component(init_el, INIT_COMPONENT_SURFACE_BOUNDARY,
                                  sdata, surface_boundary_init_data_delete,
                                  "Surface Boundary");
  if (data->cond == INLET && !data->comps) {
    return 1;
  }
  return 0;
}

static int
set_geom_init_domain_core(geom_init_element *init_el,
                          csv_data *geom_csv, const char *geom_file,
                          csv_row *found_row, csv_column **start_col,
                          jcntrl_executive_manager *manager,
                          controllable_type *control_head,
                          controllable_geometry_entry *entry_head,
                          int *status, struct set_geom_warn_data *wdata)
{
  geom_args_builder *ab;
  geom_init_args_builder *iab;
  geom_data_operator data_op;
  geom_init_func ifunc;
  double threshold;
  geom_error gerr;
  SET_P_INIT(geom_csv, geom_file, &found_row, start_col);

  CSVASSERT(start_col);

  SET_P_NEXT(&data_op, geom_data_op, GEOM_OP_INVALID);
  SET_P_NEXT(&ifunc, init_func, GEOM_INIT_FUNC_INVALID);
  SET_P_NEXT(&threshold, exact_double, 0.0);

  gerr = GEOM_SUCCESS;
  iab = geom_init_args_builder_new(ifunc, &gerr);
  if (!iab) {
    csv_error_level el;
    el = CSV_EL_ERROR;
    if (gerr == GEOM_ERR_NOMEM) {
      el = CSV_EL_FATAL;
    }
    csvperror(__FILE__, __LINE__, 0, el, NULL, CSV_ERR_GEOMETRY,
              0, gerr, NULL);
    if (el == CSV_EL_FATAL) {
      return 1;
    }
    return 0;
  }

  ab = geom_init_args_get_builder(iab);
  gerr = set_geom_with_builder(wdata, ab, geom_file, geom_csv, found_row,
                               *start_col, status, manager, control_head,
                               entry_head);

  gerr = geom_init_element_set_func(init_el, iab);
  if (gerr != GEOM_SUCCESS) {
    csvperror_row(geom_file, found_row, 0, CSV_EL_ERROR, CSV_ERR_GEOMETRY, 0,
                  gerr, NULL);
    if (status) {
      *status = ON;
    }
  }

  geom_init_args_builder_delete(iab);

  geom_init_element_set_threshold(init_el, threshold);
  geom_init_element_set_operator(init_el, data_op);
  return 0;
}

static int set_geom_init_domain_common(
  geom_init_element *init_el, csv_data *geom_csv, const char *geom_file,
  csv_row *found_row, csv_column **start_col, enum init_component_id comp_id,
  const char *comp_name, void *additional_data, geom_deallocator *dealloc,
  jcntrl_executive_manager *manager, controllable_type *control_head,
  controllable_geometry_entry *entry_head, int *status,
  struct set_geom_warn_data *wdata)
{
  int r;

  r = set_geom_init_domain_core(init_el, geom_csv, geom_file, found_row,
                                start_col, manager, control_head, entry_head,
                                status, wdata);
  geom_init_element_set_component(init_el, comp_id, additional_data, dealloc,
                                  comp_name);
  return r;
}

static int set_geom_init_domain(
  geom_init_element *init_el, csv_data *geom_csv, const char *geom_file,
  csv_row *found_row, csv_column **start_col, enum init_component_id comp_id,
  const char *comp_name, jcntrl_executive_manager *manager,
  controllable_type *control_head, controllable_geometry_entry *entry_head,
  int *status, struct set_geom_warn_data *wdata)
{
  return set_geom_init_domain_common(init_el, geom_csv, geom_file, found_row,
                                     start_col, comp_id, comp_name, NULL, NULL,
                                     manager, control_head, entry_head, status,
                                     wdata);
}

static int set_geom_check_gas_phase(csv_data *csv, csv_column *found_col,
                                    csv_row *found_row, const char *fname,
                                    const struct component_data *data,
                                    geom_vof_phase phase, int *status)
{
  SET_P_INIT(csv, fname, &found_row, &found_col);

  if (!data)
    return 1;

  if (data->jupiter_id != -1 && component_phases_is_gas_only(data->phases)) {
    if (phase != GEOM_PHASE_GAS) {
      SET_P_PERROR(ERROR, "Phase must be GAS for this ID");
      if (status)
        *status = ON;
      return 2;
    }
  }
  return 0;
}

static int set_geom_check_solid_phase(csv_data *csv, csv_column *found_col,
                                      csv_row *found_row, const char *fname,
                                      const struct component_data *data,
                                      geom_vof_phase phase, int *status)
{
  SET_P_INIT(csv, fname, &found_row, &found_col);

  if (!data)
    return 1;

  if (component_phases_is_gas_only(data->phases))
    return 0;

  if (phase == GEOM_PHASE_SOLID &&
      !component_phases_has_solid(data->phases)) {
    SET_P_PERROR(ERROR,
                 "This material has no solid (solid_form seems to be %s)",
                 PP_solid_form_value_format_v(SOLID_FORM_UNUSED));
    if (status)
      *status = ON;
    return 1;
  }
  return 0;
}

static int set_geom_init_vof(geom_init_element *init_el, csv_data *geom_csv,
                             const char *geom_file,
                             component_data *comp_data_head, csv_row *found_row,
                             csv_column **start_col,
                             jcntrl_executive_manager *manager,
                             controllable_type *control_head,
                             controllable_geometry_entry *entry_head,
                             int *status, struct set_geom_warn_data *wdata)
{
  struct init_vof_data *vof_data;
  const char *comp_name;
  int r;
  struct csv_to_component_info_data_data ccidd = {
    .comp_data_head = comp_data_head,
  };
  SET_P_INIT(geom_csv, geom_file, &found_row, start_col);

  CSVASSERT(geom_csv);
  CSVASSERT(geom_file);
  CSVASSERT(start_col);

  vof_data = (struct init_vof_data *)malloc(sizeof(struct init_vof_data));
  if (!vof_data) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return 1;
  }

  ccidd.dest = &vof_data->comp;
  SET_P_NEXT(&ccidd, component_info_data, JUPITER_ID_INVALID);

  SET_P_NEXT(&vof_data->phase, vof_phase, GEOM_PHASE_INVALID);
  set_geom_check_gas_phase(geom_csv, *start_col, found_row, geom_file,
                           vof_data->comp.d, vof_data->phase, status);
  set_geom_check_solid_phase(geom_csv, *start_col, found_row, geom_file,
                             vof_data->comp.d, vof_data->phase, status);

  r = set_geom_init_domain_common(init_el, geom_csv, geom_file, found_row,
                                  start_col, INIT_COMPONENT_VOF, "VOF",
                                  vof_data, free, manager, control_head,
                                  entry_head, status, wdata);
  return r;
}

static int set_geom_init_lpt_pewall(
  geom_init_element *init_el, csv_data *geom_csv, const char *geom_file,
  csv_row *found_row, csv_column **start_col, int is_normal_component,
  int is_boundary, const char *comp_name, jcntrl_executive_manager *manager,
  controllable_type *control_head, controllable_geometry_entry *entry_head,
  int *status, struct set_geom_warn_data *wdata)
{
  struct init_lpt_pewall_data *pewall_data;
  int r;
  enum init_component_id icomp;
  geom_deallocator *dealloc;
  SET_P_INIT(geom_csv, geom_file, &found_row, start_col);

  CSVASSERT(geom_csv);
  CSVASSERT(geom_file);
  CSVASSERT(start_col);

  pewall_data =
    (struct init_lpt_pewall_data *)malloc(sizeof(struct init_lpt_pewall_data));
  if (!pewall_data) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return 1;
  }

  is_boundary = !!is_boundary;

  pewall_data->component = INIT_LPT_PEWALL_INVALID;
  pewall_data->is_boundary = is_boundary;
  pewall_data->dir = BOUNDARY_DIR_NONE;

  if (is_boundary) {
    SET_P_NEXT(&pewall_data->dir, boundary_dir, BOUNDARY_DIR_NONE);
    if (pewall_data->dir == BOUNDARY_DIR_NONE) {
      if (status)
        *status = ON;
    }
  }

  CSVASSERT(is_normal_component); /* Not supported yet */
  icomp = INIT_COMPONENT_LPT_PEWALL_N;
  pewall_data->component = INIT_LPT_PEWALL_NORMAL;

  r = set_geom_init_domain_common(init_el, geom_csv, geom_file, found_row,
                                  start_col, icomp, comp_name, pewall_data,
                                  free, manager, control_head, entry_head,
                                  status, wdata);
  return r;
}

static char *make_control_geom_name(const char *name)
{
  char *p;
  char *aname;
  const char *key = CONTROL_KEYCHAR_GEOM;
  size_t len;
  size_t klen;

  if (!name)
    return NULL;

  if (*name == '\0')
    return NULL;

  len = strlen(name);
  klen = strlen(key);
  CSVASSERT(klen > 0);
  if (len + klen <= len) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, "Overflow error");
    return NULL;
  }

  len += klen;
  if (len + 1 <= len) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, "Overflow error");
    return NULL;
  }
  len += 1;

  aname = (char *)malloc(sizeof(char) * len);
  if (!aname) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  p = aname;
  while ((*p++ = *key++)) {}
  --p;
  while ((*p++ = *name++)) {}
  return aname;
}

static int read_geom_file(csv_data *geom_csv, const char *geom_file,
                          int geom_iter, struct csv_to_FILEn_data *fndata,
                          csv_row **file_row, csv_column **col,
                          csv_row **next_file_row)
{
  int ret;
  SET_P_INIT(geom_csv, geom_file, file_row, col);

  ret = 0;
  *fndata->filename = NULL;
  do {
    if (geom_iter == 0) {
      ret = SET_P(fndata, FILEn, "Geom_file", 1, NULL);
    } else {
      *file_row = *next_file_row;
      if (!*file_row)
        break;
      SET_P_SOURCE_COL() = getColumnOfCSV(*file_row, 0);
      CSVASSERT(SET_P_SOURCE_COL());
      ret = SET_P_NEXT(fndata, FILEn, NULL);
    }
  } while (0);
  if (*file_row) {
    *next_file_row = findCSVRowNext(*file_row);
  } else {
    *next_file_row = NULL;
  }

  return ret;
}

void read_geom_names(csv_data *geom_csv, const char *geom_file,
                     jcntrl_executive_manager *manager,
                     int geom_num, int *status)
{
  csv_row *file_row, *next_file_row;
  csv_row *name_row;
  csv_row *row;
  csv_column *col;

  SET_P_INIT(geom_csv, geom_file, &row, &col);

  if (!geom_csv)
    return;
  if (geom_num == 0)
    return;

  for (int geom_iter = 0; geom_num < 0 || geom_iter < geom_num; ++geom_iter) {
    char *io_fname;
    int ret;
    struct csv_to_FILEn_data fndata = { .filename = &io_fname, .has_r = NULL };

    ret = read_geom_file(geom_csv, geom_file, geom_iter, &fndata, &file_row,
                         &col, &next_file_row);
    if (io_fname)
      free(io_fname);

    if (!file_row)
      break;

    row = file_row;
    if (ret > 0)
      ret = 0;
    if (ret && status)
      *status = ON;

    name_row = NULL;
    for (row = getNextRow(row); row && row != next_file_row;
         row = getNextRow(row)) {
      jupiter_geom_key keyname;
      jcntrl_executive_manager_entry *entry;
      char *aname;
      const char *geom_name;

      col = getColumnOfCSV(row, 0);
      CSVASSERT(col);

      keyname = set_geom_get_keyname(getCSVValue(col));
      if (keyname != JUPITER_GEOM_KEY_NAME)
        continue;

      if (name_row) {
        set_geom_warn_duplicate_row(name_row, row, geom_file, "geometry name");
        break;
      }

      name_row = row;
      SET_P_NEXT(&geom_name, const_charp, NULL);
      aname = make_control_geom_name(geom_name);
      if (!aname)
        continue;

      entry = jcntrl_executive_manager_has(manager, aname);
      if (!entry) {
        int ret;
        jcntrl_executive *exec;
        jupiter_geometry_source *src;
        src = jupiter_geometry_source_new();
        if (!src) {
          if (status)
            *status = ON;
          break;
        }

        ret = 1;
        do {
          exec = jupiter_geometry_source_executive(src);
          if (!jcntrl_executive_set_name(exec, aname)) {
            if (status)
              *status = ON;
            ret = 0;
            break;
          }

          if (!jcntrl_executive_manager_add(manager, exec)) {
            if (status)
              *status = ON;
            ret = 0;
          }
        } while (0);
        if (!ret)
          jcntrl_executive_delete(exec);
      } else {
        SET_P_PERROR(ERROR, "This name is already defined elsewhere");
        if (status)
          *status = ON;
      }

      free(aname);
    }
  }
}

/* YSE: Read and parse geometry data */
geom_data *set_geom(csv_data *geom_csv, const char *geom_file,
                    component_data *comp_data_head, int gnx, int gny, int gnz,
                    int geom_num, int *status,
                    jcntrl_executive_manager *manager,
                    controllable_type *control_head)
{
  csv_row *row;
  csv_column *col;

  csv_row *first_init_row, *last_init_row;
  csv_row *first_shape_row, *last_shape_row;
  csv_row *first_sshape_row, *last_sshape_row;
  csv_row *file_row, *next_file_row;
  csv_row *name_row;
  csv_row *orig_row, *repeat_row, *offset_row, *size_row;
  csv_row *shape_nsub_row;
  csv_row *dump_row;

  geom_error gerr;
  geom_data *gdata;
  int geom_iter;
  int print_matrix;
  struct jupiter_geom_ext_data *ext_data;
  struct set_geom_warn_data warn_data;

  SET_P_INIT(geom_csv, geom_file, &row, &col);

  if (!geom_csv) return NULL;
  if (geom_num == 0) return NULL;

  gdata = geom_data_new(&gerr);
  if (!gdata) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
              CSV_ERR_GEOMETRY, 0, gerr, NULL);
    return NULL;
  }

  SET_P(&print_matrix, bool, "Geom_print_matrix", 1, 0);
  ext_data = (jupiter_geom_ext_data *)calloc(sizeof(jupiter_geom_ext_data), 1);
  if (ext_data) {
    ext_data->print_matrix = print_matrix;
    geom_data_set_extra_data(gdata, ext_data,
                             jupiter_geom_ext_data_delete);
  } else {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    geom_data_delete(gdata);
    return NULL;
  }

  warn_data.fname = geom_file;
  warn_data.found_col = NULL;
  warn_data.found_row = NULL;
  warn_data.csv = geom_csv;
  warn_data.elv = CSV_EL_WARN;
  warn_data.stat = status;

  if (!geom_initialized()) {
    geom_set_warning_function(set_geom_print_warn, &warn_data);
    geom_initialize();
  }

  /* Install JUPITER-defined initializations and shapes */
  jupiter_install_init_func_binary();

  gerr = GEOM_SUCCESS;

  /* Geometry input outer loop */
  file_row = NULL;
  for (geom_iter = 0; geom_num < 0 || geom_iter < geom_num; ++geom_iter) {
    struct csv_to_FILEn_data fndata;
    int fn_has_r;
    char *io_fname;
    const char *input_fname;
    int ret;
    geom_data_element *data_element;
    geom_file_data *file_data;
    geom_init_data *init_data;
    geom_shape_data *shape_data;
    geom_surface_shape_data *surface_shape_data;
    geom_svec3 shape_repeat;
    geom_svec3 file_size, file_origin, file_offset;
    geom_vec3 shape_origin, shape_offset;
    binary_output_mode rmode;
    jupiter_geom_ext_eldata *ext_eldata;
    jupiter_geom_ext_file_data *ext_fldata;
    csv_row *surf_bnd_row;

    init_data = NULL;
    shape_data = NULL;
    surface_shape_data = NULL;
    ext_eldata = NULL;
    shape_repeat = geom_svec3_c(1, 1, 1);
    shape_origin = geom_vec3_c(0.0, 0.0, 0.0);
    shape_offset = geom_vec3_c(0.0, 0.0, 0.0);
    file_origin = geom_svec3_c(0, 0, 0);
    file_offset = geom_svec3_c(0, 0, 0);
    file_size = geom_svec3_c(gnx, gny, gnz);

    fndata.filename = &io_fname;
    fndata.has_r = &fn_has_r;
    ret = read_geom_file(geom_csv, geom_file, geom_iter, &fndata,
                         &file_row, &col, &next_file_row);
    if (!file_row)
      break;

    row = file_row;
    if (ret > 0)
      ret = 0;
    if (ret && status)
      *status = ON;
    if (fn_has_r < 0 && status)
      *status = ON;
    if (io_fname) {
      gerr = geom_data_add_pointer(gdata, io_fname, free);
      if (gerr != GEOM_SUCCESS) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_GEOMETRY,
                  0, gerr, NULL);
        free(io_fname);
        io_fname = NULL;
        break;
      }
    }

    data_element = geom_data_element_new(gdata, &gerr);
    if (gerr != GEOM_SUCCESS) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                CSV_ERR_GEOMETRY, 0, gerr, NULL);
      break;
    }

    input_fname = io_fname;

    {
      binary_output_mode def_mode;

      if (!io_fname || fn_has_r < 0 || ret) {
        def_mode = BINARY_OUTPUT_UNIFY_MPI;
      } else if (fn_has_r > 0) {
        def_mode = BINARY_OUTPUT_BYPROCESS;
      } else {
        def_mode = BINARY_OUTPUT_UNIFY_MPI;
      }
      SET_P_NEXT_PASS_NOTFOUND(&rmode, binary_output_mode, def_mode);

      if (!io_fname || fn_has_r < 0 || ret) {
        /* Ignore the user inputted value */
        rmode = BINARY_OUTPUT_UNIFY_MPI;
      } else if (fn_has_r > 0) {
        if (rmode != BINARY_OUTPUT_BYPROCESS) {
          SET_P_PERROR(WARN,
                       "The given filename represents splitted-by-rank "
                       "data. In this case, the last column of Geom_file "
                       "should be BYPROCESS if specified.");
          rmode = BINARY_OUTPUT_BYPROCESS;
        }
      } else {
        CSVASSERT(fn_has_r == 0);
        if (rmode != BINARY_OUTPUT_UNIFY_MPI &&
            rmode != BINARY_OUTPUT_UNIFY_GATHER) {
          SET_P_PERROR(
            WARN,
            "The given filename represents unified data. In this case, the "
            "last column of Geom_file should be UNIFY, UNIFY_MPI or "
            "UNIFY_GATHER if specified. Assumed UNIFY_MPI.");
          rmode = BINARY_OUTPUT_UNIFY_MPI;
        }
      }
    }

    file_data = NULL;
    ext_fldata = NULL;
    if (io_fname) {
      file_data = geom_file_data_new(data_element, &gerr);
      if (!file_data) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                  CSV_ERR_GEOMETRY, 0, gerr, NULL);
        break;
      }
      geom_file_data_set_file_path(file_data, input_fname, io_fname);

      ext_fldata = (jupiter_geom_ext_file_data *)
        malloc(sizeof(jupiter_geom_ext_file_data));
      if (!ext_fldata) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                  CSV_ERR_NOMEM, 0, 0, NULL);
        gerr = GEOM_ERR_NOMEM;
        geom_file_data_delete(file_data);
        break;
      }
      gerr = geom_file_data_set_extra_data(file_data, ext_fldata,
                                           jupiter_geom_ext_file_data_delete);
      if (gerr != GEOM_SUCCESS) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                  CSV_ERR_GEOMETRY, 0, gerr, NULL);
        geom_file_data_delete(file_data);
        break;
      }
      ext_fldata->read_mode = rmode;
    }

    name_row = NULL;
    orig_row = NULL;
    offset_row = NULL;
    size_row = NULL;
    repeat_row = NULL;
    shape_nsub_row = NULL;
    dump_row = NULL;
    first_shape_row = NULL;
    first_sshape_row = NULL;
    first_init_row = NULL;
    last_shape_row = NULL;
    last_sshape_row = NULL;
    last_init_row = NULL;
    surf_bnd_row = NULL;

    for (row = getNextRow(row); row && row != next_file_row;
         row = getNextRow(row)) {
      const char *key;
      jupiter_geom_key keyname;

      col = getColumnOfCSV(row, 0);
      CSVASSERT(col);

      key = getCSVValue(col);
      keyname = set_geom_get_keyname(key);

      switch(keyname) {
      case JUPITER_GEOM_KEY_INVALID:
        continue;
      case JUPITER_GEOM_KEY_FILE:
        /*
         * If keyname definition of Geom_file mismatches between
         * set_geom (above) and set_geom_get_keyname, we may reach here.
         */
        CSVUNREACHABLE();
        continue;

      case JUPITER_GEOM_KEY_NAME:
        if (name_row) {
          set_geom_warn_duplicate_row(name_row, row, geom_file,
                                      "geometry name");
          break;
        }
        name_row = row;
        {
          char *aname;
          const char *geom_name;
          SET_P_NEXT(&geom_name, const_charp, NULL);
          aname = make_control_geom_name(geom_name);
          if (aname) {
            geom_data_add_pointer(gdata, aname, free);
            geom_data_element_set_name(data_element, aname);
          }
        }
        break;

      case JUPITER_GEOM_KEY_SIZE:
        if (size_row) {
          set_geom_warn_duplicate_row(size_row, row, geom_file,
                                      "geometry size");
          break;
        }
        size_row = row;
        if (file_data) {
          int x, y, z;
          SET_P_NEXT(&x, int, gnx);
          SET_P_NEXT(&y, int, gny);
          SET_P_NEXT(&z, int, gnz);
          if (rmode == BINARY_OUTPUT_BYPROCESS) {
            csvperrorf_row(geom_file, row, 0, CSV_EL_ERROR,
                           "Cannot set size of splitted-by-rank data");
          } else {
            file_size = geom_svec3_c(x, y, z);

            if (rmode == BINARY_OUTPUT_UNIFY_GATHER &&
                !geom_svec3_eql(file_size, geom_svec3_c(gnx, gny, gnz))) {
              csvperrorf_row(geom_file, file_row, 0, CSV_EL_WARN,
                             "Use UNIFY[_MPI] mode for partial geometry data");
              rmode = BINARY_OUTPUT_UNIFY_MPI;
            }
          }
        } else {
          csvperrorf_col(geom_file, col, CSV_EL_ERROR,
                         "Cannot set size of non-file geometry data");
        }
        break;

      case JUPITER_GEOM_KEY_OFFSET:
        if (offset_row) {
          set_geom_warn_duplicate_row(offset_row, row, geom_file,
                                      "repetition offset");
          break;
        }
        offset_row = row;
        if (file_data) {
          int x, y, z;
          SET_P_NEXT(&x, int, 0);
          SET_P_NEXT(&y, int, 0);
          SET_P_NEXT(&z, int, 0);
          if (rmode == BINARY_OUTPUT_BYPROCESS) {
            csvperrorf_row(geom_file, row, 0, CSV_EL_ERROR,
                           "Cannot set offset for splitted-by-rank data");
          } else {
            file_offset = geom_svec3_c(x, y, z);
          }
        } else {
          double x, y, z;
          SET_P_NEXT(&x, exact_double, 0.0);
          SET_P_NEXT(&y, exact_double, 0.0);
          SET_P_NEXT(&z, exact_double, 0.0);
          shape_offset = geom_vec3_c(x, y, z);
        }
        break;

      case JUPITER_GEOM_KEY_ORIGIN:
        if (orig_row) {
          set_geom_warn_duplicate_row(orig_row, row, geom_file,
                                      "geometry origin");
          break;
        }
        orig_row = row;
        if (file_data) {
          int x, y, z;
          SET_P_NEXT(&x, int, 0);
          SET_P_NEXT(&y, int, 0);
          SET_P_NEXT(&z, int, 0);
          if (rmode == BINARY_OUTPUT_BYPROCESS) {
            csvperrorf_row(geom_file, row, 0, CSV_EL_ERROR,
                           "Cannot set origin for splitted-by-rank data");
          } else {
            file_origin = geom_svec3_c(x, y, z);

            if (rmode == BINARY_OUTPUT_UNIFY_GATHER &&
                !geom_svec3_eql(file_origin, geom_svec3_c(0, 0, 0))) {
              csvperrorf_row(geom_file, file_row, 0, CSV_EL_WARN,
                             "Use UNIFY[_MPI] mode for partial geometry data");
              rmode = BINARY_OUTPUT_UNIFY_MPI;
            }
          }
        } else {
          double x, y, z;
          SET_P_NEXT(&x, exact_double, 0.0);
          SET_P_NEXT(&y, exact_double, 0.0);
          SET_P_NEXT(&z, exact_double, 0.0);
          shape_origin = geom_vec3_c(x, y, z);
        }
        break;

      case JUPITER_GEOM_KEY_REPEAT:
        if (repeat_row) {
          set_geom_warn_duplicate_row(repeat_row, row, geom_file,
                                      "repetition number");
          break;
        }
        repeat_row = row;
        {
          int x, y, z;
          SET_P_NEXT(&x, int, 1);
          SET_P_NEXT(&y, int, 1);
          SET_P_NEXT(&z, int, 1);
          shape_repeat = geom_svec3_c(x, y, z);
          if (file_data) {
            if (rmode == BINARY_OUTPUT_BYPROCESS) {
              csvperrorf_row(geom_file, row, 0, CSV_EL_ERROR,
                             "Cannot set repeat for splitted-by-rank data");
            } else if (rmode == BINARY_OUTPUT_UNIFY_GATHER &&
                       !geom_svec3_eql(shape_repeat, geom_svec3_c(1, 1, 1))) {
              csvperrorf_row(geom_file, row, 0, CSV_EL_WARN,
                             "Use UNIFY[_MPI] mode for partial geometry data");
              rmode = BINARY_OUTPUT_UNIFY_MPI;
            }
          }
        }
        break;

      case JUPITER_GEOM_KEY_NUM_SUBCELL:
        if (shape_nsub_row) {
          set_geom_warn_duplicate_row(shape_nsub_row, row, geom_file,
                                      "number of subcell for shape calc");
          break;
        }
        shape_nsub_row = row;
        break;

      case JUPITER_GEOM_KEY_SHAPE:
        if (!first_shape_row) {
          first_shape_row = row;
        }
        last_shape_row = row;
        break;

      case JUPITER_GEOM_KEY_SURFACE_SHAPE:
        if (!first_sshape_row) {
          first_sshape_row = row;
        }
        last_sshape_row = row;
        break;

      case JUPITER_GEOM_KEY_BOUNDARY:
      case JUPITER_GEOM_KEY_TBOUNDARY:
      case JUPITER_GEOM_KEY_SURFACE_BOUNDARY:
      case JUPITER_GEOM_KEY_VOF:
      case JUPITER_GEOM_KEY_PRESSURE:
      case JUPITER_GEOM_KEY_TEMPERATURE:
      case JUPITER_GEOM_KEY_VELOCITY_U:
      case JUPITER_GEOM_KEY_VELOCITY_V:
      case JUPITER_GEOM_KEY_VELOCITY_W:
      case JUPITER_GEOM_KEY_FIXED_HSOURCE:
      case JUPITER_GEOM_KEY_LPT_WALLREF_BN:
      case JUPITER_GEOM_KEY_LPT_WALLREF_IN:
        if (!first_init_row) {
          first_init_row = row;
        }
        last_init_row = row;
        break;

      case JUPITER_GEOM_KEY_DUMP:
        dump_row = row;
        break;

        /* Obsoleted keynames */
      case JUPITER_GEOM_KEY_MATERIAL:
        csvperrorf_col(geom_file, col, CSV_EL_WARN,
                       "Geom_material is obsoleted. Use Geom_vof.");
        break;
      case JUPITER_GEOM_KEY_VELOCITY:
        /*
         * csvperrorf_col(fname, found_col, CSV_EL_WARN,
         *            "Geom_velocity is obsoleted. Use Geom_velocity_[uvw].");
         */
        break;
      }
      continue;
    }

    if (file_data) {
      geom_file_data_set_sizev(file_data, file_size);
      geom_file_data_set_offsetv(file_data, file_offset);
      geom_file_data_set_originv(file_data, file_origin);
      geom_file_data_set_repeatv(file_data, shape_repeat);
    }

    if (first_init_row) {
      /* Initializations iterated by getNextRow (because key varies) */
      last_init_row = getNextRow(last_init_row);

      init_data = geom_init_data_new(data_element, &gerr);
      if (!init_data) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                  CSV_ERR_GEOMETRY, 0, gerr, NULL);
        break;
      }
    } else {
      csvperrorf_row(geom_file, file_row, 0, CSV_EL_WARN,
                     "No initialization defined. "
                     "This set of data won\'t affect to the calculation");
    }

    if (first_shape_row) {
      /* Shapes iterated by findCSVRowNext (because unique key is used) */
      last_shape_row = findCSVRowNext(last_shape_row);

      if (file_data) {
        long ll;
        ll = getCSVTextLineOrigin(getColumnOfCSV(first_shape_row, 0));
        csvperrorf_row(geom_file, file_row, 0, CSV_EL_ERROR,
                       "Can not set both of file and shape. "
                       "First shape is defined at line %ld", ll);
        if (status)
          *status = ON;
      }

      shape_data = geom_shape_data_new(data_element, &gerr);
      if (!shape_data) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                  CSV_ERR_GEOMETRY, 0, gerr, NULL);
        break;
      }

      if (offset_row) {
        geom_shape_data_set_offsetv(shape_data, shape_offset);
      }
      if (repeat_row) {
        geom_shape_data_set_repeatv(shape_data, shape_repeat);
      }
      if (orig_row) {
        geom_shape_data_set_originv(shape_data, shape_origin);
      }
      if (shape_nsub_row) {
        int nsub;
        SET_P_SOURCE_ROW() = shape_nsub_row;
        SET_P_SOURCE_COL() = getColumnOfCSV(shape_nsub_row, 0);
        SET_P_NEXT(&nsub, int, 1);
        if (nsub <= 0) {
          SET_P_PERROR(ERROR, "Number of subcell must be positive");
        }
        geom_shape_data_set_nsub_cell(shape_data, nsub);
      }

    } else { /* No shape data */
      if (!file_data) {
        /*
         * SET_P() for Geom_file returned NULL and no shapes are defined:
         * Whole region
         */
        if (orig_row) {
          csvperrorf_row(geom_file, orig_row, 0, CSV_EL_WARN,
                         "For whole region, origin specification ignored");
        }
        if (offset_row) {
          csvperrorf_row(geom_file, offset_row, 0, CSV_EL_WARN,
                         "For whole region, offset specification ignored");
        }
        if (repeat_row) {
          csvperrorf_row(geom_file, repeat_row, 0, CSV_EL_WARN,
                         "For whole region, repeat specification ignored");
        }
      }
      if (shape_nsub_row) {
        csvperrorf_row(geom_file, shape_nsub_row, 0, CSV_EL_WARN,
                       "While no shape definition, Geom_num_subcell has no effect");
      }
    }

    /* Shape setting main loop */
    for (row = first_shape_row; row && row != last_shape_row;
         row = findCSVRowNext(row)) {
      geom_shape_element *shape_el;
      geom_shape_args_builder *sab;
      geom_args_builder *ab;
      geom_shape shape;
      geom_shape_operator shape_op;
      jupiter_geom_ext_shp_eldata *shp_ext_data;

      col = getColumnOfCSV(row, 0);

      shape_el = geom_shape_element_new(shape_data, &gerr);
      if (!shape_el) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                  CSV_ERR_GEOMETRY, 0, gerr, NULL);
        break;
      }

      shp_ext_data = (jupiter_geom_ext_shp_eldata *)
        malloc(sizeof(jupiter_geom_ext_shp_eldata));
      if (shp_ext_data) {
        shp_ext_data->row = row;
        shp_ext_data->file = geom_file;
        geom_list_init(&shp_ext_data->control_entry_head.list);
        gerr =
          geom_shape_element_set_extra_data(shape_el, shp_ext_data,
                                            jupiter_geom_ext_shp_eldata_delete);
        if (gerr != GEOM_SUCCESS) {
          free(shp_ext_data);
          shp_ext_data = NULL;
          csvperror(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
                    CSV_ERR_GEOMETRY, 0, gerr, NULL);
        }
      } else {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
                  CSV_ERR_NOMEM, 0, 0, NULL);
      }

      SET_P_NEXT(&shape_op, geom_shape_op, GEOM_SOP_INVALID);
      SET_P_NEXT(&shape, geom_shape, GEOM_SHAPE_INVALID);

      sab = geom_shape_args_builder_new(shape, &gerr);
      if (!sab) {
        if (gerr == GEOM_ERR_NOMEM) {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                    CSV_ERR_GEOMETRY, 0, gerr, NULL);
          break;
        } else {
          csvperror_col(geom_file, col, CSV_EL_ERROR,
                        CSV_ERR_GEOMETRY, 0, gerr, NULL);
          continue;
        }
      }

      ab = geom_shape_args_get_builder(sab);
      gerr = set_geom_with_builder(&warn_data, ab, geom_file, geom_csv,
                                   row, col, status, manager, control_head,
                                   &shp_ext_data->control_entry_head);
      col = warn_data.found_col;

      gerr = geom_shape_element_set_shape(shape_el, shape_op, sab);
      if (gerr != GEOM_SUCCESS && gerr != GEOM_ERR_SHAPE_NOT_SET) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                  CSV_ERR_GEOMETRY, 0, gerr, NULL);
      }
      geom_shape_args_builder_delete(sab);

      if (geom_shape_element_get_shape_type(shape_el) == GEOM_SHPT_TRANS) {
        int copy;
        SET_P_NEXT(&copy, int, 0);
        if (copy < 0) {
          SET_P_PERROR(ERROR, "Number of copy by transformation must be positive or 0 for no copy");
        }

        geom_shape_element_set_transformation_copy_n(shape_el, copy);
      }

      switch(gerr) {
      case GEOM_SUCCESS:
        break;
      case GEOM_ERR_NOMEM:
        CSVUNREACHABLE();
        break;
      case GEOM_ERR_SHAPE_NOT_SET:
        /* We passed through errors at `geom_shape_element_set_shape`. */
        if (shape == GEOM_SHAPE_INVALID) {
          break;
        }
        CSV_DOFALLTHRU(); /* fall through */
      default:
        {
          csvperror_row(geom_file, row, 0, CSV_EL_ERROR,
                        CSV_ERR_GEOMETRY, 0, gerr, NULL);
        }
        break;
      }

      geom_shape_data_add_element(shape_el);
    }

    if (shape_data) {
      gerr = geom_shape_data_update_all_transform(shape_data,
                                                  set_geom_shape_error_print,
                                                  &warn_data, NULL);
      if (gerr == GEOM_ERR_SHAPE_OP_SHOULD_SET) {
        gerr = GEOM_SUCCESS;
      }
    }
    if (status) {
      if (gerr != GEOM_SUCCESS) *status = ON;
    }

    if (first_sshape_row) {
      int nsurfs;
      int nbody;

      nbody = 0;
      nsurfs = 0;
      if (shape_data) {
        geom_shape_element *el;
        for (el = geom_shape_data_get_element(shape_data); el;
             el = geom_shape_element_next(el)) {
          if (geom_shape_element_get_shape_type(el) == GEOM_SHPT_BODY)
            nbody += 1;
          nsurfs += geom_shape_element_n_enabled_surface(el);
        }
      }
      if (nsurfs <= 0) {
        if (nbody > 0) {
          csvperrorf_row(
            geom_file, first_sshape_row, 0, CSV_EL_ERROR,
            "No shapes provide their surfaces to set surface shapes");
          if (status)
            *status = ON;
        } else if (shape_data) {
          /* No body is different error */
        } else {
          csvperrorf_row(geom_file, first_sshape_row, 0, CSV_EL_ERROR,
                         "Surface shapes can not be used for whole region or "
                         "file-based geometry");
          if (status)
            *status = ON;
        }
      }

      /* Shapes iterated by findCSVRowNext (because unique key is used) */
      last_sshape_row = findCSVRowNext(last_sshape_row);

      surface_shape_data = geom_surface_shape_data_new(data_element, &gerr);
      if (!surface_shape_data) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_GEOMETRY,
                  0, gerr, NULL);
        if (status)
          *status = ON;
      }
    }

    /* Surface shape setting main loop */
    for (row = first_sshape_row; row && row != last_sshape_row;
         row = findCSVRowNext(row)) {
      geom_surface_shape_element *shape_el;
      geom_surface_shape_args_builder *sab;
      geom_args_builder *ab;
      geom_shape_operator shape_op;
      geom_surface_shape shape;
      jupiter_geom_ext_sshp_eldata *sshp_ext_data;

      col = getColumnOfCSV(row, 0);

      shape_el = geom_surface_shape_element_new(surface_shape_data, &gerr);
      if (!shape_el) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                  CSV_ERR_GEOMETRY, 0, gerr, NULL);
        break;
      }

      sshp_ext_data = (jupiter_geom_ext_sshp_eldata *)
        calloc(1, sizeof(jupiter_geom_ext_sshp_eldata));
      if (sshp_ext_data) {
        sshp_ext_data->row = row;
        sshp_ext_data->file = geom_file;
        geom_list_init(&sshp_ext_data->control_entry_head.list);

        gerr = geom_surface_shape_element_set_extra_data(
          shape_el, sshp_ext_data, jupiter_geom_ext_sshp_eldata_delete);
        if (gerr != GEOM_SUCCESS) {
          free(sshp_ext_data);
          sshp_ext_data = NULL;
          csvperror(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL, CSV_ERR_GEOMETRY,
                    0, gerr, NULL);
        }
      } else {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
                  CSV_ERR_NOMEM, 0, 0, NULL);
      }

      SET_P_NEXT(&shape_op, geom_shape_op, GEOM_SOP_INVALID);
      SET_P_NEXT(&shape, geom_surface_shape, GEOM_SURFACE_SHAPE_INVALID);

      sab = geom_surface_shape_args_builder_new(shape, &gerr);
      if (!sab) {
        if (gerr == GEOM_ERR_NOMEM) {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_GEOMETRY,
                    0, gerr, NULL);
          break;
        } else {
          csvperror_col(geom_file, col, CSV_EL_ERROR, CSV_ERR_GEOMETRY, 0,
                        gerr, NULL);
        }
      }

      ab = geom_surface_shape_args_get_builder(sab);
      gerr = set_geom_with_builder(&warn_data, ab, geom_file, geom_csv, row,
                                   col, status, manager, control_head,
                                   &sshp_ext_data->control_entry_head);

      col = warn_data.found_col;
      gerr = geom_surface_shape_element_set_shape(shape_el, shape_op, sab);
      if (gerr != GEOM_SUCCESS && gerr != GEOM_ERR_SHAPE_NOT_SET) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_GEOMETRY,
                  0, gerr, NULL);
        if (status)
          *status = ON;
      }
      geom_surface_shape_args_builder_delete(sab);

      switch(gerr) {
      case GEOM_SUCCESS:
        break;
      case GEOM_ERR_NOMEM:
        CSVUNREACHABLE();
        break;
      case GEOM_ERR_SHAPE_NOT_SET:
        if (shape == GEOM_SURFACE_SHAPE_INVALID)
          break;
        CSV_DOFALLTHRU(); /* fall through */
      default:
        csvperror_row(geom_file, row, 0, CSV_EL_ERROR, CSV_ERR_GEOMETRY, 0,
                      gerr, NULL);
        break;
      }

      geom_surface_shape_data_add_element(shape_el);
    }

    if (surface_shape_data) {
      warn_data.found_row = first_sshape_row;
      warn_data.found_col = getColumnOfCSV(first_sshape_row, 0);
      gerr = geom_surface_shape_data_check(surface_shape_data,
                                           set_geom_surface_shape_error_print,
                                           &warn_data, NULL);
      if (gerr != GEOM_SUCCESS && status)
        *status = ON;
    }

    /* Initialization setting main loop */
    for (row = first_init_row; row && row != last_init_row;
         row = getNextRow(row)) {
      geom_init_element *init_el;
      const char *keystr;
      jupiter_geom_key keyname;
      int comp_id;
      int r;
      jupiter_geom_ext_init_eldata *init_ext_data;

      col = getColumnOfCSV(row, 0);
      keystr = getCSVValue(col);
      keyname = set_geom_get_keyname(keystr);

      init_el = geom_init_element_new(init_data, &gerr);
      if (!init_el) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                  CSV_ERR_GEOMETRY, 0, gerr, NULL);
        if (status)
          *status = ON;
        break;
      }

      init_ext_data = (jupiter_geom_ext_init_eldata *)
        malloc(sizeof(jupiter_geom_ext_init_eldata));
      if (init_ext_data) {
        geom_list_init(&init_ext_data->control_entry_head.list);
        gerr =
          geom_init_element_set_extra_data(init_el, init_ext_data,
                                           jupiter_geom_ext_init_eldata_delete);
        if (gerr != GEOM_SUCCESS) {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                    CSV_ERR_GEOMETRY, 0, gerr, NULL);
          if (status)
            *status = ON;
          break;
        }
      } else {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        if (status)
          *status = ON;
        break;
      }

      switch(keyname) {
      case JUPITER_GEOM_KEY_BOUNDARY:
        r = set_geom_fluid_boundary(init_el, geom_csv, geom_file,
                                    comp_data_head, row, &col,
                                    manager, control_head, status);
        break;
      case JUPITER_GEOM_KEY_TBOUNDARY:
        r = set_geom_thermal_boundary(init_el, geom_csv, geom_file,
                                      comp_data_head, row, &col,
                                      manager, control_head, status);
        break;
      case JUPITER_GEOM_KEY_SURFACE_BOUNDARY:
        if (surf_bnd_row) {
          csvperrorf_row(
            geom_file, row, 0, CSV_EL_WARN,
            "Using multiple Geom_surface_boundary in single set is just "
            "overwriting the previous setting. Last Geom_surface_boundary is "
            "at line %ld. (These conditions may be set to different positions "
            "if their threshold values are different, but I don't think this "
            "is what you want to do.)",
            getCSVTextLineOrigin(getColumnOfCSV(surf_bnd_row, 0)));
        } else if (!first_sshape_row) {
          csvperrorf_row(geom_file, row, 0, CSV_EL_WARN,
                         "No Geom_surface_shape is set. Geom_surface_boundary "
                         "will have no effect.");
        }
        surf_bnd_row = row;
        r = set_geom_surface_boundary(init_el, geom_csv, geom_file,
                                      comp_data_head, row, &col,
                                      manager, control_head, status);
        break;
      case JUPITER_GEOM_KEY_TEMPERATURE:
        r = set_geom_init_domain(init_el, geom_csv, geom_file,
                                 row, &col, INIT_COMPONENT_TEMPERATURE,
                                 "Temperature", manager, control_head,
                                 &init_ext_data->control_entry_head, status,
                                 &warn_data);
        break;
      case JUPITER_GEOM_KEY_VELOCITY_U:
        r = set_geom_init_domain(init_el, geom_csv, geom_file,
                                 row, &col, INIT_COMPONENT_VELOCITY_U,
                                 "Velocity U", manager, control_head,
                                 &init_ext_data->control_entry_head, status,
                                 &warn_data);
        break;
      case JUPITER_GEOM_KEY_VELOCITY_V:
        r = set_geom_init_domain(init_el, geom_csv, geom_file,
                                 row, &col, INIT_COMPONENT_VELOCITY_V,
                                 "Velocity V", manager, control_head,
                                 &init_ext_data->control_entry_head, status,
                                 &warn_data);
        break;
      case JUPITER_GEOM_KEY_VELOCITY_W:
        r = set_geom_init_domain(init_el, geom_csv, geom_file,
                                 row, &col, INIT_COMPONENT_VELOCITY_W,
                                 "Velocity W", manager, control_head,
                                 &init_ext_data->control_entry_head, status,
                                 &warn_data);
        break;
      case JUPITER_GEOM_KEY_PRESSURE:
        r = set_geom_init_domain(init_el, geom_csv, geom_file,
                                 row, &col, INIT_COMPONENT_PRESSURE, "Pressure",
                                 manager, control_head,
                                 &init_ext_data->control_entry_head, status,
                                 &warn_data);
        break;
      case JUPITER_GEOM_KEY_FIXED_HSOURCE:
        r = set_geom_init_domain(init_el, geom_csv, geom_file,
                                 row, &col, INIT_COMPONENT_FIXED_HSOURCE,
                                 "Heat Source", manager, control_head,
                                 &init_ext_data->control_entry_head, status,
                                 &warn_data);
        break;
      case JUPITER_GEOM_KEY_LPT_WALLREF_BN:
        r =
          set_geom_init_lpt_pewall(init_el, geom_csv, geom_file, row, &col, 1,
                                   1, "LPT Boundary Restitution Coeff., Normal",
                                   manager, control_head,
                                   &init_ext_data->control_entry_head, status,
                                   &warn_data);
        break;
      case JUPITER_GEOM_KEY_LPT_WALLREF_IN:
        r =
          set_geom_init_lpt_pewall(init_el, geom_csv, geom_file, row, &col, 1,
                                   0, "LPT Internal Restitution Coeff., Normal",
                                   manager, control_head,
                                   &init_ext_data->control_entry_head, status,
                                   &warn_data);
        break;
      case JUPITER_GEOM_KEY_VOF:
        r = set_geom_init_vof(init_el, geom_csv, geom_file, comp_data_head,
                              row, &col, manager, control_head,
                              &init_ext_data->control_entry_head, status,
                              &warn_data);
        break;
      default:
        r = 1;
        break;
      }

      if (r) {
        geom_init_element_delete(init_el);
      } else {
        geom_init_data_add_element(init_el);
      }
    }

    if (dump_row) {
      if (!ext_eldata) {
        ext_eldata = (jupiter_geom_ext_eldata *)
          calloc(sizeof(jupiter_geom_ext_eldata), 1);
        if (!ext_eldata) {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
                    CSV_ERR_NOMEM, 0, 0, NULL);
        }
      }
      if (ext_eldata) {
        struct csv_to_FILEn_data fndata;
        int ret, has_r;
        binary_output_mode rmode, def_mode;

        fndata.filename = &ext_eldata->dump_file;
        fndata.has_r = &has_r;
        SET_P_SOURCE_ROW() = dump_row;
        SET_P_SOURCE_COL() = getColumnOfCSV(dump_row, 0);
        ret = SET_P_NEXT(&fndata, FILEn, NULL);
        if (ret > 0)
          ret = 0;

        if (!ext_eldata->dump_file || has_r < 0 || ret) {
          SET_P_PERROR(ERROR, "Binary data cannot be written in stdout");
          if (status)
            *status = ON;
          def_mode = BINARY_OUTPUT_UNIFY_MPI;
        } else if (has_r > 0) {
          def_mode = BINARY_OUTPUT_BYPROCESS;
        } else {
          CSVASSERT(has_r == 0);
          def_mode = BINARY_OUTPUT_UNIFY_MPI;
        }
        SET_P_NEXT_PASS_NOTFOUND(&rmode, binary_output_mode, def_mode);
        if (!ext_eldata->dump_file || has_r < 0 || ret) {
          rmode = BINARY_OUTPUT_UNIFY_MPI;
        } else if (has_r > 0) {
          if (rmode != BINARY_OUTPUT_BYPROCESS) {
            SET_P_PERROR(WARN,
                         "The given filename represents splitted-by-rank data. "
                         "In this case, the output mode should be BYPROCESS");
            rmode = BINARY_OUTPUT_BYPROCESS;
          }
        } else {
          if (rmode != BINARY_OUTPUT_UNIFY_MPI &&
              rmode != BINARY_OUTPUT_UNIFY_GATHER) {
            SET_P_PERROR(WARN,
                         "The given filename represents unified data. In this "
                         "case, the output mode should be UNIFY, UNIFY_MPI or "
                         "UNIFY_GATHER. Using UNIFY_MPI instead.");
            rmode = BINARY_OUTPUT_UNIFY_MPI;
          }
        }
        ext_eldata->dump_united = rmode;
      }
    }

    if (ext_eldata) {
      gerr = geom_data_element_set_extra_data(data_element, ext_eldata,
                                              jupiter_geom_ext_eldata_delete);
      if (gerr != GEOM_SUCCESS) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                  CSV_ERR_GEOMETRY, 0, gerr, NULL);
        jupiter_geom_ext_eldata_delete(ext_eldata);
      }
    }

    gerr = geom_data_add_element(data_element);
    if (gerr != GEOM_SUCCESS) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                CSV_ERR_GEOMETRY, 0, gerr, NULL);
      break;
    }
  }

  geom_set_warning_function(NULL, NULL);

  if (gerr == GEOM_ERR_NOMEM) {
    geom_data_delete(gdata);
    gdata = NULL;
  }
  if (status && gerr != GEOM_SUCCESS) {
    *status = ON;
  }

  return gdata;
}

int add_geom_sources(jcntrl_executive_manager *manager, domain *cdo,
                     geom_data *data)
{
  int r = 0;
  geom_data_element *el;

  CSVASSERT(data);
  CSVASSERT(manager);

  el = geom_data_get_element(data);
  for (; el; el = geom_data_element_next(el)) {
    if (add_geom_source_for_element(manager, el, cdo, NULL)) {
      r = 1;
    }
  }
  return r;
}

int add_geom_source_for_element(jcntrl_executive_manager *manager,
                                geom_data_element *element, domain *cdo,
                                jupiter_geometry_source **source)
{
  jupiter_geometry_source *src;
  jcntrl_executive *exe;
  jcntrl_executive_manager_entry *mentry;
  const char *name;
  const char *keychar;
  char *cntrl_name;
  char *p;
  size_t len;
  size_t xlen;
  int r;

  CSVASSERT(manager);
  CSVASSERT(element);

  name = geom_data_element_get_name(element);
  if (!name || *name == '\0') {
    if (source) {
      *source = NULL;
    }
    return 0;
  }

  src = NULL;
  mentry = jcntrl_executive_manager_has(manager, name);
  if (mentry) {
    exe = jcntrl_executive_manager_get(manager, name);
    if (exe) {
      src = jupiter_geometry_source_downcast(exe);
      if (!src) {
        return 1;
      }
    }
  } else {
    mentry = jcntrl_executive_manager_reserve(manager, name);
    if (!mentry) {
      return 1;
    }
  }

  if (!src) {
    src = jupiter_geometry_source_new();
    if (!src) {
      return 1;
    }

    exe = jupiter_geometry_source_executive(src);
    if (!jcntrl_executive_manager_bind(mentry, exe)) {
      jupiter_geometry_source_delete(src);
      return 1;
    }
  }

  if (!jupiter_geometry_source_set_geometry(src, element)) {
    return 1;
  }
  if (!jupiter_geometry_source_set_domain(src, cdo)) {
    return 1;
  }

  if (source) {
    *source = src;
  }
  return 0;
}
