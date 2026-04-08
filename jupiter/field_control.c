#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "os/asprintf.h"

#include "field_control.h"
#include "control/comparator.h"
#include "control/extent.h"
#include "control/fv_get.h"
#include "control/mask_extent.h"
#include "control/mask_geometry.h"
#include "control/mask_lop.h"
#include "control/mask_point.h"
#include "control/postp_mask.h"
#include "control/postp_pass_arrays.h"
#include "control/postp_sum.h"
#include "control/postp_volume_integral.h"
#include "control/shared_object.h"
#include "control/write_fv_csv.h"
#include "controllable_type.h"
#include "os/os.h"
#include "csv.h"
#include "csvutil.h"
#include "func.h"
#include "geometry/data.h"
#include "geometry/init.h"
#include "geometry/shape.h"
#include "geometry/udata.h"
#include "geometry/variant.h"
#include "geometry/vector.h"
#include "init_component.h"
#include "serializer/defs.h"
#include "serializer/msgpackx.h"
#include "struct.h"

#include "control/defs.h"
#include "control/manager.h"
#include "control/executive.h"
#include "control/information.h"
#include "control/input.h"
#include "control/output.h"
#include "control/field_variable.h"
#include "geometry/defs.h"
#include "geometry/list.h"

#include "control/fv_table.h"
#include "table/table.h"
#include "update_level_set_flags.h"

write_field_variables *write_field_variables_add(domain *cdo)
{
  write_field_variables *p;
  p = (write_field_variables *)malloc(sizeof(write_field_variables));
  if (!p)
    return NULL;

  write_field_variables_init(p);
  geom_list_insert_prev(&cdo->write_field_variables_head.list, &p->list);
  return p;
}
void write_field_variables_delete(write_field_variables *p)
{
  geom_list_delete(&p->list);
  if (p->writer)
    jcntrl_write_fv_csv_delete(p->writer);
}

void write_field_variables_delete_all(write_field_variables *head)
{
  struct geom_list *lp, *ln, *lh;
  lh = &head->list;
  geom_list_foreach_safe (lp, ln, lh) {
    write_field_variables *p;
    p = write_field_variables_entry(lp);
    write_field_variables_delete(p);
  }
}

control_nametype field_control_nametype(const char *name)
{
  CSVASSERT(name);

  if (name[0] == CONTROL_KEYCHAR_GRID[0])
    return CONTROL_NAMETYPE_GRID;
  if (name[0] == CONTROL_KEYCHAR_MASK[0])
    return CONTROL_NAMETYPE_MASK;
  if (name[0] == CONTROL_KEYCHAR_GEOM[0])
    return CONTROL_NAMETYPE_GEOMETRY;
  if (name[0] == CONTROL_KEYCHAR_VARNAME[0])
    return CONTROL_NAMETYPE_VARIABLE_NAME;
  if (name[0] == CONTROL_KEYCHAR_FVAR[0])
    return CONTROL_NAMETYPE_FIELD_VARIABLE;
  if (name[0] == CONTROL_KEYCHAR_LVAR[0])
    return CONTROL_NAMETYPE_LOGICAL_VARIABLE;

  return CONTROL_NAMETYPE_INVALID;
}

/**
 * Modifying @p row is allowed but may cause unexpected behavior. Use carefully.
 */
typedef int read_control_each_row_func(csv_data *csv, const char *fname,
                                       csv_row **row, csv_column **col,
                                       const char *exname, void *exdata);

static int read_control_each_row_of(const char *keyname, int keylen,
                                    csv_data *csv, const char *fname,
                                    char keychar, int perror, int *stat,
                                    read_control_each_row_func *func,
                                    void *exdata)
{
  csv_column *col;
  csv_row *row;
  int ret;
  SET_P_INIT(csv, fname, &row, &col);

  CSVASSERT(csv);
  CSVASSERT(fname);
  CSVASSERT(keyname);
  CSVASSERT(keychar != '\0');
  CSVASSERT(keylen > 0);
  CSVASSERT(func);

  ret = 0;
  row = findCSVRow(csv, keyname, keylen);
  for (; row; row = findCSVRowNext(row)) {
    const char *name;
    char *aname;
    int l;
    int r;

    col = getColumnOfCSV(row, 1);
    if (!col) {
      if (perror)
        csvperrorf_row(fname, row, 0, CSV_EL_ERROR, "No name specified", keylen,
                       keyname);
      if (stat)
        *stat = ON;
      continue;
    }

    SET_P_CURRENT(&name, const_charp, NULL);

    l = 0;
    if (name)
      l = strlen(name);

    if (l <= 0) {
      if (perror)
        SET_P_PERROR(ERROR, "Given name is empty");
      if (stat)
        *stat = ON;
      ret = 1;
      continue;
    }

    if (l + 2 <= l) {
      if (perror)
        SET_P_PERROR(ERROR, "Given name is too long");
      if (stat)
        *stat = ON;
      ret = 1;
      continue;
    }

    l += 2;

    aname = (char *)malloc(sizeof(char) * l);
    if (!aname) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      if (stat)
        *stat = ON;
      ret = -1;
      break;
    }

    aname[0] = keychar;
    strcpy(aname + 1, name);

    r = func(csv, fname, &row, &col, aname, exdata);
    free(aname);

    if (r)
      break;
  }
  return ret;
}

typedef jcntrl_executive *executive_generator(csv_data *csv, const char *fname,
                                              csv_row **row, csv_column **col,
                                              int *stat, void *exdata);

struct read_control_names_data
{
  jcntrl_executive_manager *manager;
  executive_generator *gen;
  void *exdata;
  int *stat;
};

static int read_control_name_func(csv_data *csv, const char *fname,
                                  csv_row **row, csv_column **col,
                                  const char *exname, void *exdata)
{
  jcntrl_executive_manager_entry *e;
  jcntrl_executive *exe;

  struct read_control_names_data *d;
  d = (struct read_control_names_data *)exdata;

  e = jcntrl_executive_manager_reserve(d->manager, exname);
  if (!e) {
    if (d->stat)
      *d->stat = ON;
    return 1;
  }

  exe = d->gen(csv, fname, row, col, d->stat, exdata);
  if (!exe) {
    if (d->stat)
      *d->stat = ON;
    return 1;
  }

  if (!jcntrl_executive_manager_bind(e, exe)) {
    CSVUNREACHABLE();
    jcntrl_executive_delete(exe);
    if (d->stat)
      *d->stat = ON;
    return 1;
  }

  return 0;
}

/**
 * Read control names and genenrate jcntrl_executive data
 */
static int read_control_names_of(jcntrl_executive_manager *manager,
                                 const char *keyname, int keylen, char keychar,
                                 csv_data *csv, const char *fname,
                                 executive_generator *gen, int *stat,
                                 void *exdata)
{
  struct read_control_names_data d = {
    .manager = manager,
    .gen = gen,
    .exdata = exdata,
  };
  return read_control_each_row_of(keyname, keylen, csv, fname, keychar, 1, stat,
                                  read_control_name_func, &d);
}

#define KEYNAME_POSTPROCESS "add_postprocess"
#define KEYNAME_MASK "add_mask"
#define KEYNAME_FIELD_VARIABLE "add_field_variable"
#define KEYNAME_COND_VARIABLE "add_cond_variable"
#define KEYNAME_WRITE_FIELD_VARIABLES "write_field_variables"

static jcntrl_executive *
make_postprocess_executive(csv_data *csv, const char *fname, csv_row **row,
                           csv_column **col, int *stat, void *exdata);

static jcntrl_executive *make_mask_executive(csv_data *csv, const char *fname,
                                             csv_row **row, csv_column **col,
                                             int *stat, void *exdata);

static jcntrl_executive *
make_field_variable_executive(csv_data *csv, const char *fname, csv_row **row,
                              csv_column **col, int *stat, void *exdata);

void read_control_names(jcntrl_executive_manager *manager, csv_data *ctrl_csv,
                        const char *ctrl_file, int *stat)
{
  CSVASSERT(strlen(CONTROL_KEYCHAR_GRID) == 1);
  CSVASSERT(strlen(CONTROL_KEYCHAR_MASK) == 1);
  CSVASSERT(strlen(CONTROL_KEYCHAR_GEOM) == 1);
  CSVASSERT(strlen(CONTROL_KEYCHAR_VARNAME) == 1);
  CSVASSERT(strlen(CONTROL_KEYCHAR_FVAR) == 1);
  CSVASSERT(strlen(CONTROL_KEYCHAR_LVAR) == 1);

  jcntrl_executive_manager_reserve(manager, CONTROL_KEYNAME_FVAR("time"));
  jcntrl_executive_manager_reserve(manager, CONTROL_KEYNAME_FVAR("delta-t"));
  jcntrl_executive_manager_reserve(manager, CONTROL_KEYNAME_GRID("all"));

  if (ctrl_csv) {
    read_control_names_of(manager, KEYNAME_POSTPROCESS,
                          strlen(KEYNAME_POSTPROCESS), CONTROL_KEYCHAR_GRID[0],
                          ctrl_csv, ctrl_file, make_postprocess_executive, stat,
                          NULL);

    read_control_names_of(manager, KEYNAME_MASK, strlen(KEYNAME_MASK),
                          CONTROL_KEYCHAR_MASK[0], ctrl_csv, ctrl_file,
                          make_mask_executive, stat, NULL);

    read_control_names_of(manager, KEYNAME_FIELD_VARIABLE,
                          strlen(KEYNAME_FIELD_VARIABLE),
                          CONTROL_KEYCHAR_FVAR[0], ctrl_csv, ctrl_file,
                          make_field_variable_executive, stat, NULL);
  }
}

static jcntrl_executive *
executive_allocator(const jcntrl_shared_object_data *clsp, int *stat)
{
  jcntrl_shared_object *objp;
  jcntrl_executive *exe;

  if (!clsp) {
    if (stat)
      *stat = ON;
    return NULL;
  }

  objp = jcntrl_shared_object_new_by_meta(clsp);
  if (!objp) {
    if (stat)
      *stat = ON;
    return NULL;
  }

  exe = jcntrl_shared_object_downcast(jcntrl_executive, objp);
  if (!exe) {
    if (stat)
      *stat = ON;
    jcntrl_shared_object_delete(objp);
    return NULL;
  }

  return exe;
}

static jcntrl_executive *
make_postprocess_executive(csv_data *csv, const char *fname, csv_row **row,
                           csv_column **col, int *stat, void *exdata)
{
  const jcntrl_shared_object_data *clsp;
  int postp;
  SET_P_INIT(csv, fname, row, col);

  SET_P_NEXT(&postp, postp, JCNTRL_EXE_INVALID);
  clsp = NULL;
  switch (postp) {
  case JCNTRL_POSTP_CALCULATOR:
  case JCNTRL_POSTP_ADD_VARIABLE:
  case JCNTRL_POSTP_SUM_X:
  case JCNTRL_POSTP_SUM_Y:
  case JCNTRL_POSTP_SUM_Z:
  case JCNTRL_POSTP_NEIGHBOR_SUM:
  case JCNTRL_POSTP_NEIGHBOR_SUM_X:
  case JCNTRL_POSTP_NEIGHBOR_SUM_Y:
  case JCNTRL_POSTP_NEIGHBOR_SUM_Z:
  case JCNTRL_POSTP_AVERAGE:
  case JCNTRL_POSTP_AVERAGE_X:
  case JCNTRL_POSTP_AVERAGE_Y:
  case JCNTRL_POSTP_AVERAGE_Z:
  case JCNTRL_POSTP_AVERAGE_XY:
  case JCNTRL_POSTP_AVERAGE_YZ:
  case JCNTRL_POSTP_AVERAGE_XZ:
  case JCNTRL_POSTP_AVERAGE_T:
  case JCNTRL_POSTP_NEIGHBOR_AVERAGE:
  case JCNTRL_POSTP_NEIGHBOR_AVERAGE_X:
  case JCNTRL_POSTP_NEIGHBOR_AVERAGE_Y:
  case JCNTRL_POSTP_NEIGHBOR_AVERAGE_Z:
  case JCNTRL_POSTP_NEIGHBOR_AVERAGE_XY:
  case JCNTRL_POSTP_NEIGHBOR_AVERAGE_YZ:
  case JCNTRL_POSTP_NEIGHBOR_AVERAGE_XZ:
  case JCNTRL_POSTP_VORTICITY_XY:
  case JCNTRL_POSTP_VORTICITY_YZ:
  case JCNTRL_POSTP_VORTICITY_XZ:
  case JCNTRL_POSTP_VORTICITY2:
  case JCNTRL_POSTP_UNMASK:
  case JCNTRL_POSTP_NEGATE_MASK:
    SET_P_PERROR(ERROR, "Given postprocess not supported yet");
    break;
  case JCNTRL_POSTP_DEL_VARIABLE:
    clsp = jcntrl_postp_del_variable_metadata_init();
    break;
  case JCNTRL_POSTP_DEL_VARIABLE_EXCEPT:
    clsp = jcntrl_postp_del_variable_except_metadata_init();
    break;
  case JCNTRL_POSTP_VOLUME_INTEGRAL:
    clsp = jcntrl_postp_volume_integral_metadata_init();
    break;
  case JCNTRL_POSTP_SUM:
    clsp = jcntrl_postp_sum_metadata_init();
    break;
  case JCNTRL_POSTP_MASK:
    clsp = jcntrl_postp_mask_metadata_init();
    break;
  default:
    SET_P_PERROR(ERROR, "Invalid post process specified");
    break;
  }
  return executive_allocator(clsp, stat);
}

static jcntrl_executive *make_mask_executive(csv_data *csv, const char *fname,
                                             csv_row **row, csv_column **col,
                                             int *stat, void *exdata)
{
  const jcntrl_shared_object_data *clsp;
  int maskp;
  SET_P_INIT(csv, fname, row, col);

  clsp = NULL;
  SET_P_NEXT(&maskp, maskp, JCNTRL_EXE_INVALID);
  switch (maskp) {
  case JCNTRL_MASK_GRID:
  case JCNTRL_MASK_GET:
    SET_P_PERROR(ERROR, "Given mask type not supported yet");
    break;

  case JCNTRL_MASK_ADD:
  case JCNTRL_MASK_OR:
    clsp = jcntrl_mask_or_metadata_init();
    break;
  case JCNTRL_MASK_SUB:
    clsp = jcntrl_mask_sub_metadata_init();
    break;
  case JCNTRL_MASK_MUL:
  case JCNTRL_MASK_AND:
    clsp = jcntrl_mask_and_metadata_init();
    break;
  case JCNTRL_MASK_XOR:
    clsp = jcntrl_mask_xor_metadata_init();
    break;
  case JCNTRL_MASK_EQV:
    clsp = jcntrl_mask_eqv_metadata_init();
    break;
  case JCNTRL_MASK_NOR:
    clsp = jcntrl_mask_nor_metadata_init();
    break;
  case JCNTRL_MASK_NAND:
    clsp = jcntrl_mask_nand_metadata_init();
    break;
  case JCNTRL_MASK_NEQV:
    clsp = jcntrl_mask_neqv_metadata_init();
    break;
  case JCNTRL_MASK_POINT:
    clsp = jcntrl_mask_point_metadata_init();
    break;
  case JCNTRL_MASK_GEOMETRY:
    clsp = jcntrl_mask_geometry_metadata_init();
    break;
  case JCNTRL_MASK_EXTENT:
    clsp = jcntrl_mask_extent_metadata_init();
    break;
  case JCNTRL_MASK_ALL:
    // SET_P_PERROR(ERROR, "Use predefined @all mask instead");
  case JCNTRL_MASK_NONE:
    // SET_P_PERROR(ERROR, "Use predefined @none mask instead");
    SET_P_PERROR(ERROR, "Given mask type not supported yet");
    break;
  default:
    SET_P_PERROR(ERROR, "Invalid mask type used");
    break;
  }
  return executive_allocator(clsp, stat);
}

static jcntrl_executive *
make_field_variable_executive(csv_data *csv, const char *fname, csv_row **row,
                              csv_column **col, int *stat, void *exdata)
{
  const jcntrl_shared_object_data *clsp;
  int fieldp;
  SET_P_INIT(csv, fname, row, col);

  clsp = NULL;

  SET_P_NEXT(&fieldp, fieldp, JCNTRL_EXE_INVALID);
  switch (fieldp) {
  case JCNTRL_FV_AVERAGE:
  case JCNTRL_FV_CALCULATOR:
  case JCNTRL_FV_COND:
  case JCNTRL_FV_ON_TRIGGER:
  case JCNTRL_FV_RAND:
    SET_P_PERROR(ERROR, "Given field variable type not supported yet");
    break;
  case JCNTRL_FV_GET:
    clsp = jcntrl_fv_get_metadata_init();
    break;
  case JCNTRL_FV_TABLE:
    clsp = jcntrl_fv_table_metadata_init();
    break;
  default:
    SET_P_PERROR(ERROR, "Invalid field variable type used");
    break;
  }
  return executive_allocator(clsp, stat);
}

void controllable_type_init(controllable_type *obj)
{
  CSVASSERT(obj);

  obj->current_value = 0.0;
  obj->exec = NULL;
  geom_list_init(&obj->list);
}

void controllable_type_copy(controllable_type *to, controllable_type *from)
{
  CSVASSERT(to);
  CSVASSERT(from);

  to->current_value = from->current_value;
  to->exec = from->exec;
  geom_list_insert_prev(&from->list, &to->list);
}

void controllable_type_remove_from_list(controllable_type *item)
{
  CSVASSERT(item);

  geom_list_delete(&item->list);
}

void controllable_type_add_to_list(controllable_type *head,
                                   controllable_type *item)
{
  CSVASSERT(head);
  CSVASSERT(item);

  geom_list_insert_prev(&head->list, &item->list);
}

int controllable_type_add_to_list_if_needed(controllable_type *head,
                                            controllable_type *item)
{
  CSVASSERT(head);
  CSVASSERT(item);

  if (!item->exec)
    return 0;

  controllable_type_add_to_list(head, item);
  return 1;
}

int controllable_type_format(char **buf, char fp_type, int width,
                             int fp_precision, controllable_type *item)
{
  CSVASSERT(buf);
  CSVASSERT(item);

  if (item->exec) {
    const char *exec_name;

    exec_name = jcntrl_executive_manager_entry_name(item->exec);
    if (width < 0) {
      return jupiter_asprintf(buf, "%s", exec_name);
    } else {
      return jupiter_asprintf(buf, "%*s", width, exec_name);
    }

  } else {
    int i;
    char fmt[10] = "%";
    i = 1;
    if (width >= 0) {
      fmt[i++] = '*';
    }
    if (fp_precision >= 0) {
      fmt[i++] = '.';
      fmt[i++] = '*';
    }
    switch (fp_type) {
    case 'a':
    case 'f':
    case 'e':
    case 'g':
    case 'A':
    case 'F':
    case 'E':
    case 'G':
      fmt[i] = fp_type;
      break;
    default:
      fmt[i] = 'g';
      break;
    }
    if (width >= 0) {
      if (fp_precision >= 0) {
        return jupiter_asprintf(buf, fmt, width, fp_precision,
                                item->current_value);
      }
      return jupiter_asprintf(buf, fmt, width, item->current_value);
    }
    if (fp_precision >= 0) {
      return jupiter_asprintf(buf, fmt, fp_precision, item->current_value);
    }
    return jupiter_asprintf(buf, fmt, item->current_value);
  }

  CSVUNREACHABLE();
  return -1;
}

int controllable_type_format_vec3(char **buf, char fp_type, int width,
                                  int fp_precision, controllable_type *x_item,
                                  controllable_type *y_item,
                                  controllable_type *z_item)
{
  char *tmp1, *tmp2, *tmp3;
  int r;

  r = controllable_type_format(&tmp1, fp_type, width, fp_precision, x_item);
  if (r < 0) {
    return r;
  }

  r = controllable_type_format(&tmp2, fp_type, width, fp_precision, y_item);
  if (r < 0) {
    free(tmp1);
    return r;
  }

  r = controllable_type_format(&tmp3, fp_type, width, fp_precision, z_item);
  if (r < 0) {
    free(tmp1);
    free(tmp2);
    return r;
  }

  r = jupiter_asprintf(buf, "(%s, %s, %s)", tmp1, tmp2, tmp3);
  free(tmp1);
  free(tmp2);
  free(tmp3);
  return r;
}

static int controllable_type_refresh_value(controllable_type *item)
{
  jcntrl_executive *exec;
  jcntrl_output *output;
  jcntrl_shared_object *object;
  jcntrl_information *info;
  jcntrl_field_variable *fv;
  int e;
  double d;

  exec = jcntrl_executive_manager_entry_get(item->exec);
  if (!exec) {
    return 0;
  }

  output = jcntrl_executive_get_output(exec);
  output = jcntrl_output_next_port(output);
  if (!output) {
    return 1;
  }

  info = jcntrl_output_information(output);
  if (!info) {
    return 1;
  }

  object = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  if (!object) {
    return 1;
  }

  fv = jcntrl_field_variable_downcast(object);
  if (!fv) {
    return 1;
  }

  e = 0;
  d = jcntrl_field_variable_value(fv, &e);
  if (!e) {
    item->current_value = d;
  }
  return e;
}

int controllable_type_update(controllable_type *item)
{
  jcntrl_executive *exec;
  int ret;

  CSVASSERT(item);

  if (!item->exec) {
    return 1;
  }

  exec = jcntrl_executive_manager_entry_get(item->exec);
  if (!exec) {
    return 0;
  }

  ret = jcntrl_executive_update(exec);
  if (!ret) {
    return 0;
  }

  return controllable_type_refresh_value(item);
}

int controllable_type_update_all(jcntrl_executive_manager *manager,
                                 controllable_type *head)
{
  struct geom_list *lh, *lp;
  int r;

  jcntrl_executive_manager_set_all_marks(manager, 0);

  r = 0;
  lh = &head->list;
  geom_list_foreach (lp, lh) {
    controllable_type *cp;
    jcntrl_executive *exec;
    int update_status;

    cp = controllable_type_entry(lp);
    if (!cp->exec) {
      continue;
    }

    exec = jcntrl_executive_manager_entry_get(cp->exec);
    if (!exec) {
      continue;
    }

    if (jcntrl_executive_manager_mark(cp->exec)) {
      continue;
    }

    /*
     * note: jcntrl_* returns truthy value on success (negative error),
     *       others return falsey value on success (positive error).
     */
    update_status = jcntrl_executive_update(exec);
    if (update_status) {
      if (controllable_type_refresh_value(cp)) {
        r = 1;
      }
    } else {
      r = 1;
    }
  }

  return r;
}

static msgpackx_map_node *
controllable_type_make_current_value_node(controllable_type *item)
{
  msgpackx_error err;
  msgpackx_node *node;
  msgpackx_map_node *mnode;

  node = msgpackx_node_new();
  if (!node) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  err = MSGPACKX_SUCCESS;
  msgpackx_node_set_str(node, "ctrl-cur-value", strlen("ctrl-cur-value"), &err);
  if (err != MSGPACKX_SUCCESS) {
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  mnode = msgpackx_map_node_new(MSGPACKX_MAP_ENTRY);
  if (!mnode) {
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  msgpackx_map_node_set_key(mnode, node, &err);
  if (err != MSGPACKX_SUCCESS) {
    msgpackx_map_node_delete(mnode);
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  node = msgpackx_node_new();
  if (!node) {
    msgpackx_map_node_delete(mnode);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  msgpackx_node_set_double(node, item->current_value, &err);
  if (err != MSGPACKX_SUCCESS) {
    msgpackx_map_node_delete(mnode);
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  msgpackx_map_node_set_value(mnode, node);
  return mnode;
}

static msgpackx_map_node *
controllable_type_make_control_node(controllable_type *item)
{
  msgpackx_error err;
  msgpackx_node *node;
  msgpackx_map_node *mnode;
  const char *name;

  node = msgpackx_node_new();
  if (!node) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  err = MSGPACKX_SUCCESS;
  msgpackx_node_set_str(node, "ctrl-fv-name", strlen("ctrl-fv-name"), &err);
  if (err != MSGPACKX_SUCCESS) {
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  mnode = msgpackx_map_node_new(MSGPACKX_MAP_ENTRY);
  if (!mnode) {
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  msgpackx_map_node_set_key(mnode, node, &err);
  if (err != MSGPACKX_SUCCESS) {
    msgpackx_map_node_delete(mnode);
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  node = msgpackx_node_new();
  if (!node) {
    msgpackx_map_node_delete(mnode);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  if (item->exec) {
    name = jcntrl_executive_manager_entry_name(item->exec);
    msgpackx_node_set_str(node, name, strlen(name), &err);
  } else {
    msgpackx_node_set_nil(node, &err);
  }
  if (err != MSGPACKX_SUCCESS) {
    msgpackx_map_node_delete(mnode);
    msgpackx_node_delete(node);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return NULL;
  }

  msgpackx_map_node_set_value(mnode, node);
  return mnode;
}

msgpackx_map_node *controllable_type_to_msgpackx(controllable_type *item)
{
  long line;
  msgpackx_error serr;
  msgpackx_map_node *head;
  msgpackx_map_node *mnode;

  head = msgpackx_map_node_new(MSGPACKX_MAP_HEAD);
  if (!head) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  mnode = controllable_type_make_current_value_node(item);
  if (!mnode) {
    msgpackx_map_node_delete_all(head);
    return NULL;
  }

  serr = MSGPACKX_SUCCESS;
  msgpackx_map_node_insert(head, mnode, &serr);
  if (serr != MSGPACKX_SUCCESS) {
    msgpackx_map_node_delete_all(head);
    msgpackx_map_node_delete(mnode);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              serr, NULL);
    return NULL;
  }

  mnode = controllable_type_make_control_node(item);
  if (!mnode) {
    msgpackx_map_node_delete_all(head);
    return NULL;
  }

  msgpackx_map_node_insert(head, mnode, &serr);
  if (serr != MSGPACKX_SUCCESS) {
    msgpackx_map_node_delete_all(head);
    msgpackx_map_node_delete(mnode);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              serr, NULL);
    return NULL;
  }

  return head;
}

int controllable_type_from_msgpackx(controllable_type *dest,
                                    msgpackx_map_node *mdata,
                                    jcntrl_executive_manager *manager)
{
  ptrdiff_t len;
  const char *fv_name;
  msgpackx_error err;
  msgpackx_map_node *mnode;
  msgpackx_node *value;

  CSVASSERT(dest);
  CSVASSERT(mdata);

  if (!msgpackx_map_node_is_head_of_map(mdata)) {
    return 1;
  }

  mnode = msgpackx_map_node_find_by_str(mdata, "ctrl-fv-name",
                                        strlen("ctrl-fv-name"), &err);
  if (!mnode) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
              err, NULL);
    return 1;
  }
  if (mnode == mdata) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Keyname \"ctrl-fv-name\" not found in packed data");
    return 1;
  }

  controllable_type_remove_from_list(dest);
  dest->exec = NULL;

  value = msgpackx_map_node_get_value(mnode);
  fv_name = msgpackx_node_get_str(value, &len, &err);
  if (fv_name) {
    if (len > 0) {
      jcntrl_executive *exec;
      jcntrl_executive_manager_entry *entry;
      char *buf;

      buf = (char *)malloc(sizeof(char) * (len + 1));
      if (!buf) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        return 1;
      }
      strncpy(buf, fv_name, len);
      buf[len] = '\0';

      exec = NULL;
      entry = jcntrl_executive_manager_has(manager, buf);
      if (entry) {
        exec = jcntrl_executive_manager_entry_get(entry);
      }
      if (!exec) {
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                   "Field variable '%s' is not defined", buf);
        free(buf);
        return 1;
      }
      free(buf);

      dest->exec = entry;
    } else {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Field variable of empty name requested");
      return 1;
    }

  } else {
    msgpackx_node *node;

    mnode = msgpackx_map_node_find_by_str(mdata, "ctrl-cur-value",
                                          strlen("ctrl-cur-value"), &err);
    if (!mnode) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
                err, NULL);
      return 1;
    }
    if (mnode == mdata) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Keyname \"ctrl-cur-value\" not found in packed data");
      return 1;
    }

    node = msgpackx_map_node_get_value(mnode);
    if (!node) {
      CSVUNREACHABLE();
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                 "Node value not set");
      return 1;
    }

    err = MSGPACKX_SUCCESS;
    dest->current_value = msgpackx_node_get_double(node, &err);
    if (err == MSGPACKX_ERR_MSG_TYPE) {
      dest->current_value = msgpackx_node_get_float(node, &err);
    }
    if (err != MSGPACKX_SUCCESS) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0,
                err, NULL);
    }
  }

  return 0;
}

controllable_geometry_entry *
controllable_geometry_entry_new(controllable_geometry_entry *oldp)
{
  controllable_geometry_entry *contr;

  if (oldp) {
    contr = oldp;
  } else {
    contr = (controllable_geometry_entry *)malloc(
      sizeof(controllable_geometry_entry));
    if (!contr) {
      return NULL;
    }
  }

  controllable_type_init(&contr->control[0]);
  controllable_type_init(&contr->control[1]);
  controllable_type_init(&contr->control[2]);
  contr->index = -1;
  contr->type = GEOM_VARTYPE_NULL;
  geom_list_init(&contr->list);
  return contr;
}

void controllable_geometry_entry_delete(controllable_geometry_entry *p)
{
  free(p);
}

void controllable_geometry_entry_add(controllable_geometry_entry *head,
                                     controllable_geometry_entry *p)
{
  geom_list_insert_prev(&head->list, &p->list);
}

geom_error
controllable_geometry_entry_set_to_variant(controllable_geometry_entry *p,
                                           geom_variant *v)
{
  CSVASSERT(p);
  CSVASSERT(v);

  switch (p->type) {
  case GEOM_VARTYPE_DOUBLE:
    return geom_variant_set_double(v, p->control[0].current_value);
  case GEOM_VARTYPE_VECTOR2:
    return geom_variant_set_vec2(v, geom_vec2_c(p->control[0].current_value,
                                                p->control[1].current_value));
  case GEOM_VARTYPE_VECTOR3:
    return geom_variant_set_vec3(v, geom_vec3_c(p->control[0].current_value,
                                                p->control[1].current_value,
                                                p->control[2].current_value));
  default:
    break;
  }
  CSVUNREACHABLE();
  return GEOM_ERR_VARIANT_TYPE;
}

static int controllable_geometry_entry_cmp(struct geom_list *la,
                                           struct geom_list *lb)
{
  controllable_geometry_entry *ca;
  controllable_geometry_entry *cb;

  ca = controllable_geometry_list_entry(la);
  cb = controllable_geometry_list_entry(lb);
  if (ca->index > cb->index) {
    return -1;
  }
  if (ca->index < cb->index) {
    return 1;
  }
  return 0;
}

void controllable_geometry_entry_sort(controllable_geometry_entry *head)
{
  geom_list_sort(&head->list, controllable_geometry_entry_cmp);
}

static void set_executive_connect(const char *fname, csv_row **row,
                                  csv_column **col,
                                  jcntrl_executive_manager_entry *entry,
                                  jcntrl_input *connecting_port, int *status)
{
  jcntrl_executive *e;
  jcntrl_output *output;
  SET_P_INIT(NULL, fname, row, col);

  e = NULL;
  if (entry)
    e = jcntrl_executive_manager_entry_get(entry);
  if (!e) {
    SET_P_PERROR(ERROR, "Specified field variable not defined");
    if (status)
      *status = ON;
    return;
  }

  output = jcntrl_executive_output_port(e, 0);
  if (!output) {
    SET_P_PERROR(ERROR, "[dev] This executive does not have output port 0",
                 jcntrl_executive_get_name(e));
    if (status)
      *status = ON;
    return;
  }

  if (!jcntrl_input_connect(connecting_port, output)) {
    if (status)
      *status = ON;
    return;
  }
}

static void set_fv_table(csv_data *control_csv, const char *control_file,
                         csv_column **col, csv_row **row,
                         jcntrl_executive_manager *manager,
                         jcntrl_fv_table *data, int *status)
{
  controllable_type tmp_head, in_x, in_y;
  jcntrl_fv_table_extend_mode tabext;
  int nd;
  int nx, ny;
  table_size np;
  table_size npi;
  int i;
  double *x;
  double *y, *ay;
  double *p;
  double ly, t, tp;
  jcntrl_input *input_head, *ip;
  SET_P_INIT(control_csv, control_file, row, col);

  struct csv_to_controllable_type_data cnti = {
    .manager = manager,
    .head = &tmp_head,
  };

  SET_P_NEXT(&tabext, fv_tabbnd, JCNTRL_FV_TABLE_EXTEND_INVALID);
  if (tabext == JCNTRL_FV_TABLE_EXTEND_INVALID) {
    if (status)
      *status = ON;
  }
  jcntrl_fv_table_set_extend_mode(data, tabext);

  SET_P_NEXT(&nd, int, 0);
  if (nd != 1 && nd != 2) {
    SET_P_PERROR(ERROR, "Number of table dimension must be 1 or 2");
    if (status)
      *status = ON;
    return;
  }

  input_head = jcntrl_executive_get_input(jcntrl_fv_table_executive(data));
  controllable_type_init(&tmp_head);

  ip = jcntrl_input_next_port(input_head);
  CSVASSERT(!jcntrl_input_is_head(ip));

  cnti.dest = &in_x;
  SET_P_NEXT(&cnti, controllable_type, 0.0);
  if (in_x.exec)
    set_executive_connect(control_file, row, col, in_x.exec, ip, status);
  jcntrl_fv_table_set_default_x(data, in_x.current_value);

  ip = jcntrl_input_next_port(ip);
  CSVASSERT(!jcntrl_input_is_head(ip));
  if (nd > 1) {
    cnti.dest = &in_y;
    SET_P_NEXT(&cnti, controllable_type, 0.0);
    if (in_y.exec)
      set_executive_connect(control_file, row, col, in_y.exec, ip, status);
  } else {
    controllable_type_init(&in_y);
    in_y.current_value = 0.0;
  }
  jcntrl_fv_table_set_default_y(data, in_y.current_value);

  SET_P_NEXT(&nx, int, 0);
  if (nx <= 1) {
    SET_P_PERROR(ERROR, "Number of points must be greater than 1");
    if (status)
      *status = ON;
  }

  if (nd > 1) {
    SET_P_NEXT(&ny, int, 0);
    if (ny < 1) {
      SET_P_PERROR(ERROR, "Number of points must be greater than 1");
      if (status)
        *status = ON;
    } else if (ny == 1) {
      SET_P_PERROR(WARN, "Consider setting number of dimension to 1 "
                         "instead of setting number of Y points to 1");
    }
  } else {
    ny = 1;
  }

  x = NULL;
  if (nx > 0) {
    x = (double *)calloc(sizeof(double), nx);
    if (!x) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      if (status)
        *status = ON;
    }
  }

  ay = NULL;
  y = NULL;
  if (ny == 1) {
    y = &ly;
  } else if (ny > 1) {
    ay = (double *)calloc(sizeof(double), ny);
    if (!ay) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      if (status)
        *status = ON;
    }
  }

  tp = -HUGE_VAL;
  for (i = 0; i < nx; i++) {
    SET_P_NEXT(&t, exact_double, HUGE_VAL);
    if (!SET_P_PERROR_FINITE(t, ERROR, "Table coordinates must be finite")) {
      if (status)
        *status = ON;
    }
    if (i > 0 && t < tp) {
      SET_P_PERROR(ERROR, "Table coordinates must be in ascending order");
      if (status)
        *status = ON;
    }

    if (x) {
      x[i] = t;
    }
    tp = t;
  }

  if (nd > 1) {
    for (i = 0; i < ny; i++) {
      SET_P_NEXT(&t, exact_double, HUGE_VAL);
      if (!SET_P_PERROR_FINITE(t, ERROR, "Table coordinates must be finite")) {
        if (status)
          *status = ON;
      }
      if (i > 0 && t < tp) {
        SET_P_PERROR(ERROR, "Table coordinates must be in ascending order");
        if (status)
          *status = ON;
      }

      if (y) {
        y[i] = t;
      }
      tp = t;
    }
  } else {
    *y = in_y.current_value;
  }

  np = 0;
  if (nx >= 1 && ny >= 1) {
    np = table_calc_data_size(TABLE_GEOMETRY_RECTILINEAR, nx, ny);
    if (np == (table_size)-1) {
      SET_P_PERROR(ERROR,
                   "Number of X or Y points is too big and causes overflow");
      if (status)
        *status = ON;
      np = 0;
    }
  }

  p = NULL;
  if (np > 0) {
    p = (double *)calloc(sizeof(double), np);
    if (!p) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      if (status)
        *status = ON;
    }
    for (npi = 0; npi < np; ++npi) {
      SET_P_NEXT(&t, exact_double, NAN);
      if (!SET_P_PERROR_FINITE(t, ERROR, "Table value must be finite")) {
        if (status)
          *status = ON;
      }

      if (p) {
        p[npi] = t;
      }
    }
  }

  if (x && y && p) {
    if (nd == 1) {
      jcntrl_fv_table_set_table_1d(data, nx, x, p);
    } else if (nd == 2) {
      jcntrl_fv_table_set_table_2d(data, nx, ny, x, y, p);
    } else {
      CSVUNREACHABLE();
      if (status)
        *status = ON;
    }
  }

  free(x);
  free(ay);
  free(p);
}

static void set_fv_get(csv_data *control_csv, const char *control_file,
                       csv_column **col, csv_row **row,
                       jcntrl_executive_manager *manager, jcntrl_fv_get *data,
                       mpi_param *mpi, int *status)
{
  const char *varname;
  jcntrl_executive_manager_entry *entry;
  struct csv_to_control_grid_data dg;
  jcntrl_input *ip;
  jcntrl_executive *exe;
  int apply_mask;
  int apply_extent;
  int extent[6];

  SET_P_INIT(control_csv, control_file, row, col);

  exe = jcntrl_fv_get_executive(data);
  ip = jcntrl_executive_input_port(exe, 0);

  dg.manager = manager;
  dg.dest = &entry;

  SET_P_NEXT(&dg, control_grid, NULL);
  set_executive_connect(control_file, row, col, entry, ip, status);

  SET_P_NEXT(&varname, control_varname, NULL);
  if (!varname || strlen(varname) == 0) {
    SET_P_PERROR(ERROR, "Empty variable name given");
    if (status)
      *status = ON;
  }

  SET_P_NEXT(&apply_mask, bool, ON);

  apply_extent = OFF;
  SET_P_NEXT(&extent[0], int, 0);
  if (*col) {
    apply_extent = ON;
    SET_P_NEXT(&extent[1], int, -1);
    SET_P_NEXT(&extent[2], int, 0);
    SET_P_NEXT(&extent[3], int, -1);
    SET_P_NEXT(&extent[4], int, 0);
    SET_P_NEXT(&extent[5], int, -1);

    if (jcntrl_extent_size(jcntrl_extent_c(extent)) <= 0) {
      SET_P_PERROR(ERROR, "Given extent is empty");
      if (status)
        *status = ON;
    }
  }

  if (!jcntrl_fv_get_set_varname_c(data, varname)) {
    if (status)
      *status = ON;
  }

  jcntrl_fv_get_set_exclude_masked(data, apply_mask == ON);
  jcntrl_fv_get_set_extract_extent(data, apply_extent == ON);
  jcntrl_fv_get_set_extent(data, extent);
  if (mpi->control_controller)
    jcntrl_fv_get_set_controller(data, mpi->control_controller);
}

typedef int set_controls_setter(csv_data *csv, const char *fname, csv_row **row,
                                csv_column **col,
                                jcntrl_executive_manager *data,
                                jcntrl_executive *exec, int *status, void *arg);

struct set_controls_data
{
  jcntrl_executive_manager *manager;
  set_controls_setter *setter;
  void *exdata;
  int *stat;
};

static int set_controls_func(csv_data *csv, const char *fname, csv_row **row,
                             csv_column **col, const char *exname, void *exdata)
{
  jcntrl_executive *exec;
  struct set_controls_data *d;
  d = (struct set_controls_data *)exdata;

  exec = jcntrl_executive_manager_get(d->manager, exname);
  if (!exec)
    return 0;

  return d->setter(csv, fname, row, col, d->manager, exec, d->stat, d->exdata);
}

static void set_controls_of(csv_data *control_csv, const char *control_file,
                            jcntrl_executive_manager *data, int *status,
                            const char *keyname, int keylen, char keychar,
                            set_controls_setter *setter, void *arg)
{
  struct set_controls_data d = {
    .manager = data,
    .setter = setter,
    .exdata = arg,
    .stat = status,
  };
  read_control_each_row_of(keyname, keylen, control_csv, control_file, keychar,
                           0, status, set_controls_func, &d);
}

static int make_variable_name(csv_data *csv, const char *fname, csv_row **row,
                              csv_column **col, char **outp, const char *name,
                              int allow_empty, int *status)
{
  int r;
  SET_P_INIT(csv, fname, row, col);

  if (!name || strlen(name) == 0) {
    if (!allow_empty) {
      SET_P_PERROR(ERROR, "Output variable name must not be empty");
      if (status)
        *status = ON;
    }
    *outp = NULL;
    return 0;
  }

  r = jupiter_asprintf(outp, "%s%s", CONTROL_KEYCHAR_VARNAME, name);
  if (r < 0) {
    *outp = NULL;
    if (status)
      *status = ON;
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return 1;
  }
  return 0;
}

static int postp_volume_integral_setter(csv_data *csv, const char *fname,
                                        csv_row **row, csv_column **col,
                                        jcntrl_executive_manager *data,
                                        jcntrl_executive *exec, mpi_param *mpi,
                                        int *status)
{
  struct csv_to_control_grid_data cvg;
  jcntrl_executive_manager_entry *ginp;
  jcntrl_executive *gexec;
  jcntrl_postp_volume_integral *vint;
  jcntrl_input *input;
  jcntrl_output *output;
  const char *vint_n_name;
  char *vint_n_a;
  jcntrl_size_type vint_n_n;
  const char *vint_v_name;
  char *vint_v_a;
  jcntrl_size_type vint_v_n;
  int r;
  SET_P_INIT(csv, fname, row, col);

  vint = jcntrl_postp_volume_integral_downcast(jcntrl_executive_object(exec));
  CSVASSERT(vint);

  gexec = NULL;

  cvg = (struct csv_to_control_grid_data){.manager = data, .dest = &ginp};
  SET_P_NEXT(&cvg, control_grid, NULL);
  if (ginp)
    gexec = jcntrl_executive_manager_entry_get(ginp);
  if (!gexec) {
    if (status)
      *status = ON;
  }

  SET_P_NEXT(&vint_n_name, const_charp, "vint-n");
  make_variable_name(csv, fname, row, col, &vint_n_a, vint_n_name, 1, status);
  vint_n_n = vint_n_a ? strlen(vint_n_a) : 0;

  SET_P_NEXT(&vint_v_name, const_charp, "vint-v");
  make_variable_name(csv, fname, row, col, &vint_v_a, vint_v_name, 1, status);
  vint_v_n = vint_v_a ? strlen(vint_v_a) : 0;

  jcntrl_postp_volume_integral_set_controller(vint, mpi->control_controller);
  if (!jcntrl_postp_volume_integral_set_volume_varname_c(vint, vint_v_a,
                                                         vint_v_n)) {
    if (status)
      *status = ON;
  }

  if (!jcntrl_postp_volume_integral_set_cardinality_varname_c(vint, vint_n_a,
                                                              vint_n_n)) {
    if (status)
      *status = ON;
  }

  if (vint_v_a)
    free(vint_v_a);
  if (vint_n_a)
    free(vint_n_a);

  if (gexec) {
    output = jcntrl_executive_output_port(gexec, 0);
    input = jcntrl_executive_input_port(exec, 0);
    CSVASSERT(input);
    CSVASSERT(output);

    if (!jcntrl_input_connect(input, output)) {
      if (status)
        *status = ON;
    }
  }
  return 0;
}

static int postp_pass_arrays_vars_setter(csv_data *csv, const char *fname,
                                         csv_row **row, csv_column **col,
                                         jcntrl_postp_pass_arrays *arys,
                                         const char *mode, int *status)
{
  int r;
  int n;
  SET_P_INIT(csv, fname, row, col);

  SET_P_NEXT(&n, int, 0);
  if (n < 1) {
    SET_P_PERROR(ERROR, "Number of variables to %s is 0 or negative", mode);
    if (status)
      *status = ON;
    n = 0;
  }

  r = jcntrl_postp_pass_arrays_set_number_of_variables(arys, n);
  if (!r) {
    if (status)
      *status = ON;
  }

  for (int i = 0; i < n; ++i) {
    int l;
    const char *name;
    SET_P_NEXT(&name, const_charp, NULL);

    l = 0;
    if (name)
      l = strlen(name);

    if (!jcntrl_postp_pass_arrays_set_variable_c(arys, i, name, l)) {
      if (status)
        *status = ON;
    }
  }

  return 0;
}

static int postp_del_variable_setter(csv_data *csv, const char *fname,
                                     csv_row **row, csv_column **col,
                                     jcntrl_executive_manager *data,
                                     jcntrl_executive *exec, int *status)
{
  jcntrl_executive *up;
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_executive_manager_entry *entry;
  struct csv_to_control_grid_data ctg;
  jcntrl_postp_pass_arrays *pp;
  jcntrl_postp_del_variable *p;
  SET_P_INIT(csv, fname, row, col);

  p = jcntrl_postp_del_variable_downcast(jcntrl_executive_object(exec));
  CSVASSERT(p);

  ctg.dest = &entry;
  ctg.manager = data;
  SET_P_NEXT(&ctg, control_grid, NULL);
  up = NULL;
  if (!entry) {
    if (status)
      *status = ON;
  }
  if (entry)
    up = jcntrl_executive_manager_entry_get(entry);

  pp = jcntrl_postp_del_variable_pass_arrays(p);
  if (postp_pass_arrays_vars_setter(csv, fname, row, col, pp, "delete", status))
    return 1;

  input = jcntrl_executive_input_port(exec, 0);
  CSVASSERT(input);

  output = jcntrl_executive_output_port(up, 0);
  CSVASSERT(output);

  if (!jcntrl_input_connect(input, output)) {
    if (status)
      *status = ON;
  }
  return 0;
}

static int postp_del_variable_except_setter(csv_data *csv, const char *fname,
                                            csv_row **row, csv_column **col,
                                            jcntrl_executive_manager *data,
                                            jcntrl_executive *exec, int *status)
{
  jcntrl_executive *up;
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_executive_manager_entry *entry;
  struct csv_to_control_grid_data ctg;
  jcntrl_postp_pass_arrays *pp;
  jcntrl_postp_del_variable_except *p;
  SET_P_INIT(csv, fname, row, col);

  p = jcntrl_postp_del_variable_except_downcast(jcntrl_executive_object(exec));
  CSVASSERT(p);

  ctg.dest = &entry;
  ctg.manager = data;
  SET_P_NEXT(&ctg, control_grid, NULL);
  up = NULL;
  if (!entry) {
    if (status)
      *status = ON;
  }
  if (entry)
    up = jcntrl_executive_manager_entry_get(entry);

  pp = jcntrl_postp_del_variable_except_pass_arrays(p);
  if (postp_pass_arrays_vars_setter(csv, fname, row, col, pp, "keep", status))
    return 1;

  input = jcntrl_executive_input_port(exec, 0);
  CSVASSERT(input);

  output = jcntrl_executive_output_port(up, 0);
  CSVASSERT(output);

  if (!jcntrl_input_connect(input, output)) {
    if (status)
      *status = ON;
  }
  return 0;
}

static int postp_sum_setter(csv_data *csv, const char *fname,
                            csv_row **row, csv_column **col,
                            jcntrl_executive_manager *data,
                            jcntrl_executive *exec, mpi_param *mpi,
                            int *status)
{
  struct csv_to_control_grid_data cvg;
  jcntrl_executive_manager_entry *ginp;
  jcntrl_executive *gexec;
  jcntrl_postp_sum *sump;
  jcntrl_input *input;
  jcntrl_output *output;
  const char *sum_n_name;
  char *sum_n_a;
  jcntrl_size_type sum_n_n;
  int r;
  SET_P_INIT(csv, fname, row, col);

  sump = jcntrl_postp_sum_downcast(jcntrl_executive_object(exec));
  CSVASSERT(sump);

  gexec = NULL;

  cvg = (struct csv_to_control_grid_data){.manager = data, .dest = &ginp};
  SET_P_NEXT(&cvg, control_grid, NULL);
  if (ginp)
    gexec = jcntrl_executive_manager_entry_get(ginp);
  if (!gexec) {
    if (status)
      *status = ON;
  }

  SET_P_NEXT(&sum_n_name, const_charp, "sum-n");
  make_variable_name(csv, fname, row, col, &sum_n_a, sum_n_name, 1, status);
  sum_n_n = sum_n_a ? strlen(sum_n_a) : 0;

  jcntrl_postp_sum_set_controller(sump, mpi->control_controller);
  if (!jcntrl_postp_sum_set_cardinality_varname_c(sump, sum_n_a, sum_n_n)) {
    if (status)
      *status = ON;
  }

  if (sum_n_a)
    free(sum_n_a);

  if (gexec) {
    output = jcntrl_executive_output_port(gexec, 0);
    input = jcntrl_executive_input_port(exec, 0);
    CSVASSERT(input);
    CSVASSERT(output);

    if (!jcntrl_input_connect(input, output)) {
      if (status)
        *status = ON;
    }
  }
  return 0;
}

static int postp_mask_setter(csv_data *csv, const char *fname, csv_row **row,
                             csv_column **col, jcntrl_executive_manager *data,
                             jcntrl_executive *exec, int *status)
{
  struct csv_to_control_grid_data cvg;
  struct csv_to_control_mask_data cvm;
  jcntrl_logical_operator lop;
  jcntrl_executive_manager_entry *ginp, *minp;
  jcntrl_executive *gexec, *mexec;
  jcntrl_postp_mask *mask;
  jcntrl_output *output;
  jcntrl_input *input;

  SET_P_INIT(csv, fname, row, col);

  mask = jcntrl_postp_mask_downcast(jcntrl_executive_object(exec));
  CSVASSERT(mask);

  gexec = NULL;
  mexec = NULL;

  cvg = (struct csv_to_control_grid_data){.manager = data, .dest = &ginp};
  SET_P_NEXT(&cvg, control_grid, NULL);
  if (ginp)
    gexec = jcntrl_executive_manager_entry_get(ginp);
  if (!gexec) {
    if (status)
      *status = ON;
  }

  SET_P_NEXT(&lop, jcntrl_lop, JCNTRL_LOP_INVALID);
  if (lop == JCNTRL_LOP_INVALID) {
    if (status)
      *status = ON;
  }

  cvm = (struct csv_to_control_mask_data){.manager = data, .dest = &minp};
  SET_P_NEXT(&cvm, control_mask, NULL);
  if (minp)
    mexec = jcntrl_executive_manager_entry_get(minp);
  if (!mexec) {
    if (status)
      *status = ON;
  }

  if (gexec) {
    output = jcntrl_executive_output_port(gexec, 0);
    input = jcntrl_postp_mask_get_grid_input(mask);
    CSVASSERT(output);
    CSVASSERT(input);

    if (!jcntrl_input_connect(input, output)) {
      if (status)
        *status = ON;
    }
  }
  if (mexec) {
    output = jcntrl_executive_output_port(mexec, 0);
    input = jcntrl_postp_mask_get_mask_input(mask);
    CSVASSERT(output);
    CSVASSERT(input);

    if (!jcntrl_input_connect(input, output)) {
      if (status)
        *status = ON;
    }
  }

  jcntrl_postp_mask_set_op(mask, lop);

  return 0;
}

struct parameter_data
{
  flags *flg;
  domain *cdo;
  mpi_param *mpi;
};

static int postp_setter(csv_data *csv, const char *fname, csv_row **row,
                        csv_column **col, jcntrl_executive_manager *data,
                        jcntrl_executive *exec, int *status, void *arg)
{
  int fp;
  struct parameter_data *d;
  SET_P_INIT(csv, fname, row, col);

  d = (struct parameter_data *)arg;

  SET_P_NEXT(&fp, postp, JCNTRL_EXE_INVALID);

  if (fp == JCNTRL_POSTP_VOLUME_INTEGRAL)
    return postp_volume_integral_setter(csv, fname, row, col, data, exec,
                                        d->mpi, status);

  if (fp == JCNTRL_POSTP_SUM)
    return postp_sum_setter(csv, fname, row, col, data, exec, d->mpi, status);

  if (fp == JCNTRL_POSTP_MASK)
    return postp_mask_setter(csv, fname, row, col, data, exec, status);

  if (fp == JCNTRL_POSTP_DEL_VARIABLE)
    return postp_del_variable_setter(csv, fname, row, col, data, exec, status);

  if (fp == JCNTRL_POSTP_DEL_VARIABLE_EXCEPT)
    return postp_del_variable_except_setter(csv, fname, row, col, data, exec,
                                            status);

  return 0;
}

static int mask_geometry_setter(csv_data *csv, const char *fname, csv_row **row,
                                csv_column **col,
                                jcntrl_executive_manager *data,
                                jcntrl_executive *exec, int *status)
{
  controllable_type h;
  controllable_type t;
  struct csv_to_controllable_type_data ctd;
  struct csv_to_control_geom_data ctg;
  jcntrl_executive_manager_entry *entry;
  jcntrl_executive *geom_exec;
  jcntrl_executive *fvar_exec;
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_mask_geometry *mask_geometry;
  jcntrl_comparator compp;
  SET_P_INIT(csv, fname, row, col);

  mask_geometry = jcntrl_mask_geometry_downcast(jcntrl_executive_object(exec));
  CSVASSERT(mask_geometry);

  controllable_type_init(&h);
  controllable_type_init(&t);

  ctg.manager = data;
  ctg.dest = &entry;
  SET_P_NEXT(&ctg, control_geom, NULL);
  geom_exec = NULL;
  if (entry)
    geom_exec = jcntrl_executive_manager_entry_get(entry);
  if (!geom_exec) {
    if (status)
      *status = ON;
  }

  ctd.manager = data;
  ctd.head = &h;
  ctd.dest = &t;
  SET_P_NEXT(&ctd, controllable_type, 0.0);
  fvar_exec = NULL;
  if (t.exec) {
    fvar_exec = jcntrl_executive_manager_entry_get(t.exec);
    if (!fvar_exec) {
      if (status)
        *status = ON;
    }
  }

  SET_P_NEXT(&compp, jcntrl_compp, JCNTRL_COMP_INVALID);
  if (!jcntrl_comparator_is_valid(compp)) {
    if (status)
      *status = ON;
  }

  if (geom_exec) {
    input = jcntrl_mask_geometry_geometry_input(mask_geometry);
    CSVASSERT(input);

    output = jcntrl_executive_output_port(geom_exec, 0);
    CSVASSERT(output);

    if (!jcntrl_input_connect(input, output)) {
      if (status)
        *status = ON;
    }
  }

  if (fvar_exec) {
    input = jcntrl_mask_geometry_vof_threshold_input(mask_geometry);
    CSVASSERT(input);

    output = jcntrl_executive_output_port(fvar_exec, 0);
    CSVASSERT(output);

    if (!jcntrl_input_connect(input, output)) {
      if (status)
        *status = ON;
    }
  }

  jcntrl_mask_geometry_set_default_vof_threshold(mask_geometry,
                                                 t.current_value);
  jcntrl_mask_geometry_set_comparator(mask_geometry, compp);
  return 0;
}

static int mask_lop_setter(csv_data *csv, const char *fname, csv_row **row,
                           csv_column **col, jcntrl_executive_manager *data,
                           jcntrl_executive *exec, int *status)
{
  int n;
  jcntrl_mask_lop *mask_lop;
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_executive *iexec;
  int r;
  SET_P_INIT(csv, fname, row, col);

  mask_lop = jcntrl_mask_lop_downcast(jcntrl_executive_object(exec));
  CSVASSERT(mask_lop);

  SET_P_NEXT(&n, int, 0);
  if (n <= 0) {
    SET_P_PERROR(ERROR, "Number of masks must be positive");
    if (status)
      *status = ON;
    n = 0;
  }

  input = jcntrl_executive_get_input(exec);
  r = !jcntrl_input_set_number_of_ports(input, n);
  if (r) {
    if (status)
      *status = ON;
  }

  if (n > 0) {
    jcntrl_executive *iexec;
    jcntrl_executive_manager_entry *entry;
    struct csv_to_control_mask_data ctm;
    input = jcntrl_input_next_port(input);

    ctm.dest = &entry;
    ctm.manager = data;
    for (int i = 0; i < n; ++i) {
      SET_P_NEXT(&ctm, control_mask, NULL);
      iexec = NULL;
      if (entry)
        iexec = jcntrl_executive_manager_entry_get(entry);
      if (!iexec) {
        if (status)
          *status = ON;
      }

      if (!input)
        continue;

      if (iexec) {
        output = jcntrl_executive_output_port(iexec, 0);
        CSVASSERT(output);

        if (!jcntrl_input_connect(input, output)) {
          if (status)
            *status = ON;
        }
      }
      input = jcntrl_input_next_port(input);
    }
  }

  return 0;
}

static int mask_extent_setter(csv_data *csv, const char *fname, csv_row **row,
                              csv_column **col, jcntrl_executive_manager *data,
                              jcntrl_executive *exec, int *status)
{
  int extent[6];
  jcntrl_mask_extent *mask_extent;
  SET_P_INIT(csv, fname, row, col);

  mask_extent = jcntrl_mask_extent_downcast(jcntrl_executive_object(exec));
  CSVASSERT(mask_extent);

  SET_P_NEXT(&extent[0], int, 0);
  SET_P_NEXT(&extent[1], int, -1);
  SET_P_NEXT(&extent[2], int, 0);
  SET_P_NEXT(&extent[3], int, -1);
  SET_P_NEXT(&extent[4], int, 0);
  SET_P_NEXT(&extent[5], int, -1);
  if (extent[1] <= extent[0] || extent[3] <= extent[2] ||
      extent[5] <= extent[4]) {
    SET_P_PERROR(WARN, "Given extent is empty");
  }

  jcntrl_mask_extent_set_extent(mask_extent, extent);
  return 0;
}

static void mask_point_connect(jcntrl_input *input,
                               jcntrl_executive_manager_entry *f, int *status)
{
  jcntrl_output *output;
  jcntrl_executive *exec;

  CSVASSERT(input);
  CSVASSERT(f);

  exec = jcntrl_executive_manager_entry_get(f);
  if (!exec) {
    if (status)
      *status = ON;
    return;
  }

  output = jcntrl_executive_output_port(exec, 0);
  if (!output) {
    if (status)
      *status = ON;
  }

  if (!jcntrl_input_connect(input, output)) {
    if (status)
      *status = ON;
  }
}

static int mask_point_setter(csv_data *csv, const char *fname, csv_row **row,
                             csv_column **col, jcntrl_executive_manager *data,
                             jcntrl_executive *exec, int *status)
{
  struct csv_to_controllable_type_data ctc;
  controllable_type h, t;
  jcntrl_mask_point *mask_point;
  SET_P_INIT(csv, fname, row, col);

  mask_point = jcntrl_mask_point_downcast(jcntrl_executive_object(exec));
  CSVASSERT(mask_point);

  controllable_type_init(&h);
  controllable_type_init(&t);

  ctc.head = &h;
  ctc.dest = &t;
  ctc.manager = data;
  SET_P_NEXT(&ctc, controllable_type, 0.0);
  controllable_type_remove_from_list(&t);
  jcntrl_mask_point_set_default_point_x(mask_point, t.current_value);
  if (t.exec) {
    mask_point_connect(jcntrl_mask_point_x_point_port(mask_point), t.exec,
                       status);
  }

  SET_P_NEXT(&ctc, controllable_type, 0.0);
  controllable_type_remove_from_list(&t);
  jcntrl_mask_point_set_default_point_y(mask_point, t.current_value);
  if (t.exec) {
    mask_point_connect(jcntrl_mask_point_y_point_port(mask_point), t.exec,
                       status);
  }

  SET_P_NEXT(&ctc, controllable_type, 0.0);
  controllable_type_remove_from_list(&t);
  jcntrl_mask_point_set_default_point_z(mask_point, t.current_value);
  if (t.exec) {
    mask_point_connect(jcntrl_mask_point_z_point_port(mask_point), t.exec,
                       status);
  }
  return 0;
}

static int mask_setter(csv_data *csv, const char *fname, csv_row **row,
                       csv_column **col, jcntrl_executive_manager *data,
                       jcntrl_executive *exec, int *status, void *arg)
{
  int fp;
  struct parameter_data *d;
  SET_P_INIT(csv, fname, row, col);

  d = (struct parameter_data *)arg;

  SET_P_NEXT(&fp, maskp, JCNTRL_EXE_INVALID);

  if (fp == JCNTRL_MASK_GEOMETRY)
    return mask_geometry_setter(csv, fname, row, col, data, exec, status);

  if (fp == JCNTRL_MASK_EXTENT)
    return mask_extent_setter(csv, fname, row, col, data, exec, status);

  if (fp == JCNTRL_MASK_POINT)
    return mask_point_setter(csv, fname, row, col, data, exec, status);

  switch (fp) {
  case JCNTRL_MASK_ADD:
  case JCNTRL_MASK_OR:
  case JCNTRL_MASK_SUB:
  case JCNTRL_MASK_MUL:
  case JCNTRL_MASK_AND:
  case JCNTRL_MASK_XOR:
  case JCNTRL_MASK_EQV:
  case JCNTRL_MASK_NOR:
  case JCNTRL_MASK_NAND:
  case JCNTRL_MASK_NEQV:
    return mask_lop_setter(csv, fname, row, col, data, exec, status);
  }

  return 0;
}

static int field_variables_setter(csv_data *csv, const char *fname,
                                  csv_row **row, csv_column **col,
                                  jcntrl_executive_manager *data,
                                  jcntrl_executive *exec, int *status,
                                  void *arg)
{
  struct parameter_data *d;
  int fp;
  SET_P_INIT(csv, fname, row, col);

  d = (struct parameter_data *)arg;

  SET_P_NEXT(&fp, fieldp, JCNTRL_EXE_INVALID);

  if (fp == JCNTRL_FV_TABLE) {
    jcntrl_fv_table *fv_table;
    fv_table = jcntrl_fv_table_downcast(jcntrl_executive_object(exec));
    CSVASSERT(fv_table);
    set_fv_table(csv, fname, col, row, data, fv_table, status);
    return 0;
  }
  if (fp == JCNTRL_FV_GET) {
    jcntrl_fv_get *fv_get;
    fv_get = jcntrl_fv_get_downcast(jcntrl_executive_object(exec));
    CSVASSERT(fv_get);
    set_fv_get(csv, fname, col, row, data, fv_get, d->mpi, status);
    return 0;
  }
  return 0;
}

static void write_field_variables_setter(csv_data *csv, const char *fname,
                                         jcntrl_executive_manager *data,
                                         domain *cdo, int *status)
{
  controllable_type dh;
  struct csv_to_controllable_type_data ctd = {.head = &dh, .manager = data};
  struct csv_to_control_fvar_data cfd = {.manager = data};
  csv_column *col;
  csv_row *row;
  SET_P_INIT(csv, fname, &row, &col);

  controllable_type_init(&dh);

  row = findCSVRow(csv, KEYNAME_WRITE_FIELD_VARIABLES,
                   sizeof(KEYNAME_WRITE_FIELD_VARIABLES));
  for (; row; row = findCSVRowNext(row)) {
    char *outfname;
    struct csv_to_FILEn_data fnd;
    enum write_field_variables_format fmt;
    jcntrl_write_fv_csv *writer;
    controllable_type cntr;
    jcntrl_executive_manager_entry **entries;

    col = getColumnOfCSV(row, 0);
    if (!col)
      continue;

    fnd.filename = &outfname;
    fnd.has_r = NULL;
    SET_P_NEXT(&fnd, FILEn, NULL);
    if (!outfname) {
      SET_P_PERROR(ERROR, "Data cannot be written in stdout");
      if (status)
        *status = ON;
    }

    entries = NULL;
    controllable_type_init(&cntr);
    writer = NULL;
    do {
      int nhead;
      write_field_variables *wp;
      jcntrl_write_fv_csv_step_mode stepmode;
      int nstep;
      int precision;
      int use_scientific;
      int nvars;

      SET_P_NEXT(&fmt, write_fv_form, WRITE_FIELD_VARIABLES_FORMAT_INVALID);
      if (fmt != WRITE_FIELD_VARIABLES_FORMAT_CSV) {
        if (status)
          *status = ON;
        break;
      }

      SET_P_NEXT(&nhead, int, 1);
      if (nhead != 1 && nhead != 2) {
        SET_P_PERROR(ERROR, "Number of CSV header is 1 or 2");
        if (status)
          *status = ON;
      }

      SET_P_NEXT(&stepmode, write_fv_csv_mode, JCNTRL_WRITE_FV_OUTPUT_BY_NONE);

      nstep = -1;
      cntr.current_value = -1.0;
      cntr.exec = NULL;

      switch (stepmode) {
      case JCNTRL_WRITE_FV_OUTPUT_BY_NSTEP:
        SET_P_NEXT(&nstep, int, 0);
        if (nstep < 0) {
          SET_P_PERROR(WARN, "Negative value of step will disable the output");
        }
        break;
      case JCNTRL_WRITE_FV_OUTPUT_BY_TIME:
        ctd.dest = &cntr;
        SET_P_NEXT(&ctd, controllable_type, 0.0);
        if (!cntr.exec) {
          SET_P_PERROR_GREATER(
            cntr.current_value, 0.0, ON, OFF, WARN,
            "Negative value of time step width will disable the output");
        }
        break;
      case JCNTRL_WRITE_FV_OUTPUT_BY_NONE:
        break;
      }

      SET_P_NEXT(&precision, int, 0);
      SET_P_NEXT(&use_scientific, bool, OFF);
      use_scientific = use_scientific == ON;

      SET_P_NEXT(&nvars, int, 0);
      if (nvars <= 0)
        SET_P_PERROR(WARN, "No variables will be written (including time)");

      if (nvars > 0) {
        entries = (jcntrl_executive_manager_entry **)
          calloc(nvars, sizeof(jcntrl_executive_manager_entry *));
        if (!entries) {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                    0, NULL);
          break;
        }
      }

      for (int i = 0; i < nvars; ++i) {
        jcntrl_executive_manager_entry *dummy;

        cfd.dest = entries ? &entries[i] : &dummy;
        SET_P_NEXT(&cfd, control_fvar, NULL);
        if (!*cfd.dest || !jcntrl_executive_manager_entry_get(*cfd.dest)) {
          SET_P_PERROR(ERROR, "Given field variable is not defined");
          if (status)
            *status = ON;
        }
      }

      wp = write_field_variables_add(cdo);
      if (!wp)
        break;

      writer = jcntrl_write_fv_csv_new();
      if (!writer)
        break;

      if (!jcntrl_write_fv_csv_set_output_file_c(writer, outfname,
                                                 strlen(outfname)))
        break;

      if (!jcntrl_write_fv_csv_set_number_of_inputs(writer, nvars))
        break;

      jcntrl_write_fv_csv_set_number_of_header_rows(writer, nhead);
      jcntrl_write_fv_csv_set_step_mode(writer, stepmode);
      jcntrl_write_fv_csv_set_step_interval(writer, nstep);
      jcntrl_write_fv_csv_set_const_time_interval(writer, cntr.current_value);
      if (cntr.exec) {
        jcntrl_executive *exe = jcntrl_executive_manager_entry_get(cntr.exec);
        if (exe)
          jcntrl_write_fv_csv_set_fv_time_interval_exec(writer, exe, 0);
      }
      jcntrl_write_fv_csv_set_precision(writer, precision);
      jcntrl_write_fv_csv_set_use_scientific(writer, use_scientific);

      if (entries) {
        for (int i = 0; i < nvars; ++i) {
          jcntrl_executive *exe;
          jcntrl_output *output;
          jcntrl_input *input;

          if (!entries[i])
            continue;

          exe = jcntrl_executive_manager_entry_get(entries[i]);
          if (!exe)
            continue;

          input = jcntrl_write_fv_csv_get_input(writer, i);
          CSVASSERT(input);

          output = jcntrl_executive_output_port(exe, 0);
          CSVASSERT(output);

          if (!jcntrl_input_connect(input, output)) {
            if (status)
              *status = ON;
          }
        }
      }

      wp->writer = writer;
      writer = NULL;
    } while (0);

    if (entries)
      free(entries);
    if (outfname)
      free(outfname);
    if (writer)
      jcntrl_write_fv_csv_delete(writer);

    controllable_type_remove_from_list(&cntr);
  }
}

void set_controls(csv_data *control_csv, const char *control_file,
                  jcntrl_executive_manager *data, int *status, flags *flg,
                  domain *cdo, mpi_param *mpi)
{
  struct parameter_data d = {
    .flg = flg,
    .cdo = cdo,
    .mpi = mpi,
  };

  set_controls_of(control_csv, control_file, data, status, KEYNAME_POSTPROCESS,
                  strlen(KEYNAME_POSTPROCESS), CONTROL_KEYCHAR_GRID[0],
                  postp_setter, &d);

  set_controls_of(control_csv, control_file, data, status, KEYNAME_MASK,
                  strlen(KEYNAME_MASK), CONTROL_KEYCHAR_MASK[0], mask_setter,
                  &d);

  set_controls_of(control_csv, control_file, data, status,
                  KEYNAME_FIELD_VARIABLE, strlen(KEYNAME_FIELD_VARIABLE),
                  CONTROL_KEYCHAR_FVAR[0], field_variables_setter, &d);

  write_field_variables_setter(control_csv, control_file, data, cdo, status);
}

void print_field_variable_values(parameter *prm)
{
  int first;
  struct geom_list *lp;

  CSVASSERT(prm);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->mpi);

  if (prm->cdo->viewflg == 1 && prm->mpi->rank == 0) {
    jcntrl_executive_manager_set_all_marks(prm->controls, 0);
    first = 1;
    geom_list_foreach (lp, &prm->control_head.list) {
      controllable_type *ctrl;
      jcntrl_executive_manager_entry *ent;
      jcntrl_executive *ex;

      ctrl = controllable_type_entry(lp);
      ent = ctrl->exec;
      ex = NULL;
      if (ent) {
        ex = jcntrl_executive_manager_entry_get(ent);
        if (jcntrl_executive_manager_mark(ent)) {
          ex = NULL;
        } else {
          jcntrl_executive_manager_set_mark(ent, 1);
        }
      }

      if (ex) {
        const char *name;

        if (first) {
          fprintf(stdout, "============================================\n");
          first = 0;
        }

        name = jcntrl_executive_get_name(ex);
        if (name && strlen(name) > 20) {
          fprintf(stdout, " %s\n %20s = %14.6e\n", name, "",
                  ctrl->current_value);
        } else {
          fprintf(stdout, " %-20s = %14.6e\n", name, ctrl->current_value);
        }
      }
    }
    if (!first) {
      fprintf(stdout, "============================================\n");
      first = 0;
    }
  }
}

static void update_all_parameters_error_handler(const char *fname,
                                                geom_error err, geom_variant *v,
                                                geom_variant *einfo, int *stat)
{
  geom_error ferr;
  csv_error_level el;
  char *buf;
  const char *cstr;

  CSVASSERT(v);

  if (err == GEOM_SUCCESS && (!einfo || geom_variant_is_null(einfo)))
    return;

  ferr = geom_variant_to_string(&buf, v);
  if (ferr != GEOM_SUCCESS) {
    buf = NULL;
  }

  if (err != GEOM_SUCCESS) {
    el = CSV_EL_ERROR;
    if (stat)
      *stat = 1;
  } else {
    el = CSV_EL_WARN;
  }

  cstr = geom_variant_get_string(einfo, NULL);
  if (cstr) {
    csvperrorf(fname, 0, 0, el, buf, "%s", cstr);
  } else {
    csvperror(fname, 0, 0, el, buf, CSV_ERR_GEOMETRY, 0, err, NULL);
  }
  if (buf) {
    free(buf);
  }
}

typedef geom_error update_all_parameters_setter_func(void *data,
                                                     geom_size_type index,
                                                     geom_variant *value,
                                                     geom_variant *einfo);

static int
update_all_parameters_setter(controllable_geometry_entry *head, void *data,
                             const char *fname, geom_variant **einfo,
                             update_all_parameters_setter_func *setter)
{
  controllable_geometry_entry *le;
  geom_variant *verr;
  geom_variant *v;
  geom_error err;
  int r;

  CSVASSERT(einfo);

  le = controllable_geometry_entry_next(head);
  if (le == head) /* empty */
    return 0;

  v = geom_variant_new(&err);
  if (!v) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_GEOMETRY, 0,
              err, NULL);
    return 1;
  }

  verr = *einfo;
  r = 0;
  for (; le != head; le = controllable_geometry_entry_next(le)) {
    err = controllable_geometry_entry_set_to_variant(le, v);
    if (err != GEOM_SUCCESS) {
      csvperror(fname, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_GEOMETRY, 0, err,
                NULL);
      r = 1;
      continue;
    }

    if (!verr) {
      verr = geom_variant_new(NULL);
      *einfo = verr;
    }

    err = setter(data, le->index, v, verr);
    update_all_parameters_error_handler(fname, err, v, verr, &r);
  }

  geom_variant_delete(v);
  return r;
}

static geom_error update_all_parameters_of_shape_setter(void *data,
                                                        geom_size_type index,
                                                        geom_variant *value,
                                                        geom_variant *einfo)
{
  return geom_shape_element_set_parameter((geom_shape_element *)data, index,
                                          value, einfo);
}

static int update_all_parameters_of_shape(const char *fname,
                                          geom_shape_element *el,
                                          geom_variant **einfo)
{
  const geom_user_defined_data *ud;
  controllable_geometry_entry *lh;
  jupiter_geom_ext_shp_eldata *extdata;

  CSVASSERT(el);

  ud = geom_shape_element_get_extra_data(el);
  extdata = (jupiter_geom_ext_shp_eldata *)geom_user_defined_data_get(ud);
  if (!extdata)
    return 0;

  lh = &extdata->control_entry_head;
  return update_all_parameters_setter(lh, el, fname, einfo,
                                      update_all_parameters_of_shape_setter);
}

static geom_error update_all_parameters_of_init_setter(void *data,
                                                       geom_size_type index,
                                                       geom_variant *value,
                                                       geom_variant *einfo)
{
  return geom_init_element_set_parameter((geom_init_element *)data, index,
                                         value, einfo);
}

static int update_all_parameters_of_init(const char *fname,
                                         geom_init_element *el,
                                         geom_variant **einfo)
{
  const geom_user_defined_data *ud;
  controllable_geometry_entry *lh;
  jupiter_geom_ext_init_eldata *extdata;

  CSVASSERT(el);

  ud = geom_init_element_get_extra_data(el);
  extdata = (jupiter_geom_ext_init_eldata *)geom_user_defined_data_get(ud);
  if (!extdata)
    return 0;

  lh = &extdata->control_entry_head;
  return update_all_parameters_setter(lh, el, fname, einfo,
                                      update_all_parameters_of_init_setter);
}

int update_geometry_parameters(const char *fname, geom_data *data)
{
  geom_variant *einfo;
  geom_data_element *el;
  int r;

  r = 0;
  einfo = NULL;

  el = geom_data_get_element(data);
  for (; el; el = geom_data_element_next(el)) {
    geom_shape_data *shapes;
    geom_init_data *inits;

    shapes = geom_data_element_get_shape(el);
    inits = geom_data_element_get_init(el);

    if (shapes) {
      geom_shape_element *shp_el;
      shp_el = geom_shape_data_get_element(shapes);
      for (; shp_el; shp_el = geom_shape_element_next(shp_el)) {
        if (update_all_parameters_of_shape(fname, shp_el, &einfo)) {
          r = 1;
        }
      }
    }

    if (inits) {
      geom_init_element *init_el;
      init_el = geom_init_data_get_element(inits);
      for (; init_el; init_el = geom_init_element_next(init_el)) {
        if (update_all_parameters_of_init(fname, init_el, &einfo)) {
          r = 1;
        }
      }
    }
  }
  if (einfo) {
    geom_variant_delete(einfo);
  }
  return r;
}

type update_control_values(variable *val, material *mtl, parameter *prm)
{
  type startt;
  type endt;
  int r;

  CSVASSERT(prm);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->mpi);

  startt = cpu_time();
  r = controllable_type_update_all(prm->controls, &prm->control_head);
  if (r) {
    prm->status = ON;
  }

  if (!geom_list_empty(&prm->cdo->write_field_variables_head.list)) {
    struct geom_list *lp, *lh;
    lh = &prm->cdo->write_field_variables_head.list;
    geom_list_foreach (lp, lh) {
      write_field_variables *p;
      jcntrl_executive *exec;
      p = write_field_variables_entry(lp);
      if (!p->writer)
        continue;

      exec = jcntrl_write_fv_csv_executive(p->writer);
      if (!jcntrl_executive_update(exec)) {
        prm->status = ON;
        continue;
      }

      if (!jcntrl_write_fv_csv_write(p->writer, prm->cdo->icnt,
                                     prm->cdo->time)) {
        prm->status = ON;
        continue;
      }
    }
  }

  if (prm->control_sets) {
    init_component comps = init_component_all();
    init_component modified = init_component_zero();

    r = update_geometry_parameters(prm->control_file, prm->control_sets);
    if (r) {
      prm->status = ON;
    }

    init_component_for_update_control(&comps, prm);
    r = geometry_in_with(val, prm, comps, &modified, prm->control_sets);
    if (r) {
      prm->status = ON;
    }

    if (init_component_is_set(&modified, INIT_COMPONENT_VOF) ||
        init_component_is_set(&modified, INIT_COMPONENT_BOUNDARY)) {
      int icompo;
      int j;
      domain *cdo;
      flags *flg;

      cdo = prm->cdo;
      flg = prm->flg;

      materials(mtl, val, prm);
      bcf_VOF(0, val->fs, val, prm);
      bcf_VOF(1, val->fl, val, prm);

      if (init_component_is_set(&modified, INIT_COMPONENT_BOUNDARY)) {
        /* Mark to update level-set ll(s) if liquid inlet exists */
        update_level_set_flags *flags[] = {
          &flg->update_level_set_lls,
          // &flg->update_level_set_ll
        };
        int nflags = sizeof(flags) / sizeof(flags[0]);

        update_level_set_flags_mark_if_liquid_inlet_exists(
          &cdo->fluid_boundary_head, nflags, flags,
          UPDATE_LEVEL_SET_BY_LIQUID_INLET);
      }

      if (init_component_is_set(&modified, INIT_COMPONENT_VOF)) {
#pragma omp parallel
        {
          int m = cdo->m;
          int ncompo = cdo->NumberOfComponent;
          int nbcompo = cdo->NBaseComponent;
          phase_value_component *comps = prm->phv->comps;

          if (prm->flg->solute_diff == ON) {
#pragma omp for
            for (j = 0; j < m; ++j) {
              int icompo;
              type valY;
              valY = 0.0;
              for (icompo = 0; icompo < nbcompo; icompo++)
                valY += val->Y[j + icompo * m];
              val->Yt[j] = valY;
              val->fs_sum[j] = val->fs[j];
              val->fl_sum[j] = val->fl[j];
            }
          } else {
#pragma omp for
            for (j = 0; j < m; ++j) {
              int icompo;
              type vals, vall;
              vals = 0.0;
              vall = 0.0;
              for (icompo = 0; icompo < ncompo; icompo++) {
                vals += val->fs[j + icompo * prm->cdo->m];
                vall += val->fl[j + icompo * prm->cdo->m];
              }
              val->fs_sum[j] = vals;
              val->fl_sum[j] = vall;
            }

            if (val->fs_ibm) {
#pragma omp for
              for (j = 0; j < prm->cdo->m; ++j) {
                int icompo;
                type vals_ibm = 0.0;
                for (icompo = 0; icompo < ncompo; ++icompo) {
                  if (comps[icompo].sform == SOLID_FORM_IBM) {
                    vals_ibm += val->fs[j + icompo * prm->cdo->m];
                  }
                }
                val->fs_ibm[j] = vals_ibm;
              }
            }
          }
#pragma omp for
          for (j = 0; j < prm->cdo->m; ++j) {
            val->fls[j] = val->fs_sum[j] + val->fl_sum[j];
          }
        }

        /* Request update level set if fs != 0 */
        {
          update_level_set_flags *flags[] = {
            &flg->update_level_set_lls,
            &flg->update_level_set_ls,
          };
          int nflags = sizeof(flags) / sizeof(flags[0]);

          update_level_set_flags_mark_if_fl_exists(
            val->fs_sum, cdo, prm->mpi, nflags, flags,
            UPDATE_LEVEL_SET_BY_CONTROLLED_VOF);
        }

        /* Request update level set if fl != 0 */
        {
          update_level_set_flags *flags[] = {
            &flg->update_level_set_lls,
            // &flg->update_level_set_ll,
          };
          int nflags = sizeof(flags) / sizeof(flags[0]);

          update_level_set_flags_mark_if_fl_exists(
            val->fl_sum, cdo, prm->mpi, nflags, flags,
            UPDATE_LEVEL_SET_BY_CONTROLLED_VOF);
        }

        /* Total mass may be changed */
        init_partial_volume(val, mtl, prm);
      }

      if (prm->flg->solute_diff == ON) {
        bcs(val->Y, val->Vf, val, prm, mtl);
      }

      if (prm->flg->oxidation == ON) {
        if (init_component_is_set(&modified, INIT_COMPONENT_VOF)) {
          csvperrorf(prm->control_file, 0, 0, CSV_EL_ERROR, NULL,
                     "Modifying VOF domain value is not supported with "
                     "Zircaloy oxidation model yet");
          prm->status = ON;
        }
      }
    }
    if (init_component_is_set(&modified, INIT_COMPONENT_TEMPERATURE)) {
      for (int j = 0; j < prm->cdo->m; ++j)
        val->t_pre[j] = val->t[j];
    }
    if (init_component_is_set(&modified, INIT_COMPONENT_TEMPERATURE) ||
        init_component_is_set(&modified, INIT_COMPONENT_THERMAL_BOUNDARY)) {
      bct(val->t, val, prm);
    }
    if (init_component_is_set(&modified, INIT_COMPONENT_VELOCITY_U) ||
        init_component_is_set(&modified, INIT_COMPONENT_VELOCITY_V) ||
        init_component_is_set(&modified, INIT_COMPONENT_VELOCITY_W) ||
        init_component_is_set(&modified, INIT_COMPONENT_BOUNDARY)) {
      if (init_component_is_set(&modified, INIT_COMPONENT_VOF)) {
        bcu(val->u, val->v, val->w, val, mtl, prm);
      } else {
        bcu_correct(val->u, val->v, val->w, val, mtl, prm);
      }
    }
  }

  print_field_variable_values(prm);

  endt = cpu_time();
  return endt - startt;
}

int field_control_errorhandler(void *data, jcntrl_information *info)
{
  int eno = JCNTRL_ERROR_UNKNOWN;
  int errcode;
  const char *fname = NULL;
  const char *message = NULL;
  long line = -1;

  if (jcntrl_information_has(info, JCNTRL_INFO_ERROR_SOURCE_FILE))
    fname = jcntrl_information_get_string(info, JCNTRL_INFO_ERROR_SOURCE_FILE);

  if (jcntrl_information_has(info, JCNTRL_INFO_ERROR_SOURCE_LINE))
    line = jcntrl_information_get_integer(info, JCNTRL_INFO_ERROR_SOURCE_LINE);

  if (jcntrl_information_has(info, JCNTRL_INFO_ERROR_NUMBER))
    eno = jcntrl_information_get_integer(info, JCNTRL_INFO_ERROR_NUMBER);

  if (jcntrl_information_has(info, JCNTRL_INFO_ERROR_MESSAGE))
    message = jcntrl_information_get_string(info, JCNTRL_INFO_ERROR_MESSAGE);

  if (eno == JCNTRL_ERROR_ERRNO) {
    errcode = jcntrl_information_get_integer(info, JCNTRL_INFO_ERROR_ERRNO);
    csvperror(fname, line, 0, CSV_EL_ERROR, message, CSV_ERR_SYS, errcode, 0,
              NULL);
  } else if (eno == JCNTRL_ERROR_SERIALIZER) {
    errcode =
      jcntrl_information_get_integer(info, JCNTRL_INFO_ERROR_SERIALIZER);
    csvperror(fname, line, 0, CSV_EL_ERROR, message, CSV_ERR_SERIALIZE, 0,
              errcode, NULL);
  } else if (eno == JCNTRL_ERROR_GEOMETRY) {
    errcode = jcntrl_information_get_integer(info, JCNTRL_INFO_ERROR_GEOMETRY);
    csvperror(fname, line, 0, CSV_EL_ERROR, message, CSV_ERR_GEOMETRY, 0,
              errcode, NULL);
  } else if (eno == JCNTRL_ERROR_MPI) {
    errcode = jcntrl_information_get_integer(info, JCNTRL_INFO_ERROR_MPI);
    csvperror(fname, line, 0, CSV_EL_ERROR, message, CSV_ERR_MPI, 0, errcode,
              NULL);
  } else {
    if (eno == JCNTRL_ERROR_TABLE) {
      errcode = jcntrl_information_get_integer(info, JCNTRL_INFO_ERROR_TABLE);
      message = table_errorstr(errcode);
    } else if (!message) {
      message = "Unknown control library error";
    }
    csvperrorf(fname, line, 0, CSV_EL_ERROR, NULL, "%s (%d)", message, eno);
  }

  return 0;
}
