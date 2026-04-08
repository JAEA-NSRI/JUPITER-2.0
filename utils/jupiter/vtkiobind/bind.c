
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jupiter/boundary_util.h>
#include <jupiter/component_info.h>
#include <jupiter/csvtmpl_format.h>
#include <jupiter/func.h>
#include <jupiter/geometry/data.h>
#include <jupiter/geometry/udata.h>
#include <jupiter/lpt.h>
#include <jupiter/optparse.h>
#include <jupiter/os/asprintf.h>
#include <jupiter/os/os.h>
#include <jupiter/struct.h>

#include <jupiter/geometry/defs.h>
#include <time.h>

#include "bind.h"
#include "jupiter/component_data.h"
#include "jupiter/component_data_defs.h"
#include "jupiter/csv.h"
#include "jupiter/csvutil.h"
#include "jupiter/geometry/list.h"
#include "jupiter/print_param_keywords.h"
#include "jupiter/serializer/defs.h"

#ifdef LPTX
#include "jupiter/lptx/defs.h"
#endif

parameter *set_parameters_file(const char *param_file, const char *flags_file,
                               const char *geom_file, const char *control_file,
                               int *status)
{
#define NLOCAL_ARGS 14
  int arglen;
  int argc;
  int lengths[NLOCAL_ARGS];
  int i;
  const char *pname = "<unknown>";
  const char *param_f = "-input";
  const char *flags_f = "-flags";
  const char *geom_f = "-geom";
  const char *control_f = "-control";
  const char *args[NLOCAL_ARGS];
  char *argva[NLOCAL_ARGS + 1];
  char *argv_data;
  char *argp;
  char **argv;
  enum set_parameters_options opts;
  parameter *prm;

  i = 0;
  args[i++] = pname;
  args[i++] = param_f;
  args[i++] = param_file;
  args[i++] = flags_f;
  args[i++] = flags_file;
  if (geom_file) {
    args[i++] = geom_f;
    args[i++] = geom_file;
  }
  if (control_file) {
    args[i++] = control_f;
    args[i++] = control_file;
  }
#ifdef JUPITER_MPI
  args[i++] = "-print_rank";
  args[i++] = "all:-all,0:+INFO,all:+ERROR+FATAL";
#else
  args[i++] = "-print_level";
  args[i++] = "INFO,ERROR,FATAL";
#endif
  for (; i < NLOCAL_ARGS; ++i) {
    args[i] = NULL;
  }

  arglen = 0;
  for (i = 0; i < NLOCAL_ARGS; ++i) {
    if (!args[i])
      break;
    lengths[i] = strlen(args[i]) + 1;
    arglen += lengths[i];
  }

  argc = i;
  argv_data = (char *)calloc(sizeof(char *), arglen);
  if (!argv_data) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  argp = argv_data;
  for (i = 0; i < argc; ++i) {
    argva[i] = argp;
    strcpy(argp, args[i]);
    argp += lengths[i];
  }
  argva[argc] = NULL;
  argv = argva;

  opts = SET_PARAMETERS_READ_FLAGS | SET_PARAMETERS_READ_PARAM |
         SET_PARAMETERS_DOMAIN | SET_PARAMETERS_MPI;
  if (geom_file) {
    opts |= SET_PARAMETERS_READ_GEOMETRY | SET_PARAMETERS_GEOMETRY;
  }
  if (control_file) {
    opts |= SET_PARAMETERS_CONTROL | SET_PARAMETERS_READ_CONTROL |
            SET_PARAMETERS_CONTROL_GEOMETRY;
  }
  prm = set_parameters(&argc, &argv, opts, 1);
  free(argv_data);

  if (status) {
    if (prm) {
      *status = prm->status;
    } else {
      *status = ON;
    }
  }
  return prm;
}

void get_global_size(parameter *p, int size[3])
{
  size[0] = p->cdo->gnx;
  size[1] = p->cdo->gny;
  size[2] = p->cdo->gnz;
}

void get_local_size(parameter *p, int size[3])
{
  size[0] = p->cdo->nx;
  size[1] = p->cdo->ny;
  size[2] = p->cdo->nz;
}

void get_stm_size(parameter *p, int size[3])
{
  size[0] = 0;
  size[1] = 0;
  size[2] = 0;
}

void get_stp_size(parameter *p, int size[3])
{
  size[0] = 0;
  size[1] = 0;
  size[2] = 0;
}

void get_global_start(parameter *p, int size[3])
{
  mpi_param *mpi;
  domain *cdo;
  mpi = p->mpi;
  cdo = p->cdo;

  size[0] = mpi->rank_x * cdo->nx;
  size[1] = mpi->rank_y * cdo->ny;
  size[2] = mpi->rank_z * cdo->nz;
}

jupiter_options *jupiter_optparse_alloc(int *argc, char ***argv)
{
  jupiter_options *opt;
  int r;

  opt = (jupiter_options *)malloc(sizeof(jupiter_options));
  if (!opt)
    return NULL;

  jupiter_options_init(opt, "all:-all,0:+INFO,all:+ERROR+FATAL",
                       jupiter_print_levels_zero());
  r = jupiter_optparse(opt, argc, argv);
  if (r) {
    free(opt);
    return NULL;
  }
  return opt;
}

const char *jupiter_opt_flags(jupiter_options *p) { return p->flags_file; }

const char *jupiter_opt_param(jupiter_options *p) { return p->param_file; }

const char *jupiter_opt_geom(jupiter_options *p) { return p->geom_file; }

const char *jupiter_opt_plist(jupiter_options *p) { return p->plist_file; }

const char *jupiter_opt_control(jupiter_options *p) { return p->control_file; }

void jupiter_opt_delete(jupiter_options *p) { free(p); }

static int input_binary_spec_base(
  parameter *prm, const char *directory, void *data, int component_id,
  struct data_spec *spec, int mx, int my, int mz, int stmx, int stmy, int stmz,
  int stpx, int stpy, int stpz, int unit_size, int iout, int rank,
  binary_output_mode unified, int call_read, jupiter_bind_data_type type)
{
  char *path;
  int r;
  size_t usz;
  mpi_param mpi, *mpi_use;
  int i;

  path = NULL;
  if (call_read != 0) {
    r = make_file_name(&path, directory, spec->filename_template, spec->name,
                       component_id, iout, rank, unified);
    if (for_any_rank(prm->mpi, r)) {
      if (r) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
      } else {
        free(path);
      }
      return 1;
    }
  } else {
    if (for_any_rank(prm->mpi, 0)) {
      return 1;
    }
  }

  if (for_any_rank(prm->mpi, call_read == 0)) {
    unified = BINARY_OUTPUT_BYPROCESS;
#ifdef JUPITER_MPI
    r = MPI_Comm_split(prm->mpi->CommJUPITER, call_read != 0, prm->mpi->rank,
                       &mpi.CommJUPITER);
    if (r != MPI_SUCCESS) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI, 0, r,
                NULL);
      return 1;
    }

    MPI_Comm_rank(mpi.CommJUPITER, &mpi.rank);
    MPI_Comm_size(mpi.CommJUPITER, &mpi.npe);

    r = 0;
    if (call_read != 0) {
      if (mpi.npe != 1) {
        if (mpi.rank == 0) {
          csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                     "[dev] The function '%s' only supports conditions where "
                     "`call_read` is all 1 or all 0 except 1 MPI rank",
                     __func__);
        }
        r = 1;
      }
    }
    if (for_any_rank(prm->mpi, r)) {
      MPI_Comm_free(&mpi.CommJUPITER);
      return 1;
    }
#else
    mpi.npe = 1;
    mpi.rank = 0;
#endif
    mpi.npe_a = mpi.npe;
    mpi.rank_x = 0;
    mpi.rank_y = 0;
    mpi.rank_z = 0;
    mpi.npe_x = 1;
    mpi.npe_y = 1;
    mpi.npe_xy = 1;
    mpi.npe_z = 1;
    mpi.radiation_running = 0;
    for (i = 0; i < sizeof(mpi.nrk) / sizeof(mpi.nrk[0]); ++i) {
      mpi.nrk[i] = -1;
    }
    mpi_use = &mpi;
  } else {
#ifdef JUPITER_MPI
    mpi.CommJUPITER = MPI_COMM_NULL;
#endif
    mpi_use = prm->mpi;
  }

  if (call_read != 0) {
    CSVASSERT(path);

    switch (type) {
    case JUPITER_BIND_TYPE_INT:
      usz = sizeof(int);
      break;
    case JUPITER_BIND_TYPE_FLOAT:
      usz = sizeof(float);
      break;
    case JUPITER_BIND_TYPE_DOUBLE:
      usz = sizeof(double);
      break;
    case JUPITER_BIND_TYPE_CHAR:
      usz = sizeof(char);
      break;
    default:
      CSVUNREACHABLE();
      return 1;
    }
    usz *= unit_size;

    r = input_binary_generic(mpi_use, data, stmx, stmy, stmz, stpx, stpy, stpz,
                             mx, my, mz, usz, path, unified);
    free(path);
  }

#ifdef JUPITER_MPI
  if (mpi.CommJUPITER != MPI_COMM_NULL) {
    MPI_Comm_free(&mpi.CommJUPITER);
  }
#endif
  return r;
}

static int input_binary_spec(parameter *prm, const char *directory, void *data,
                             int component_id, struct data_spec *spec,
                             int unit_size, int iout,
                             binary_output_mode unified,
                             jupiter_bind_data_type type)
{
  int r;
  domain *cdo;

  cdo = prm->cdo;

  r = input_binary_spec_base(prm, directory, data, component_id, spec, cdo->nx,
                             cdo->ny, cdo->nz, 0, 0, 0, 0, 0, 0, unit_size,
                             iout, prm->mpi->rank, unified, 1, type);
  return r;
}

static int input_binary_spec_lpt(parameter *prm, const char *directory, int npt,
                                 void *data, int component_id,
                                 struct data_spec *spec, int unit_size,
                                 int iout, binary_output_mode unified,
                                 int call_read, jupiter_bind_data_type type)
{
  int r;
  r = input_binary_spec_base(prm, directory, data, component_id, spec, npt, 1,
                             1, 0, 0, 0, 0, 0, 0, unit_size, iout,
                             prm->mpi->rank, unified, call_read, type);
  return r;
}

static int add_attribute(const char *descriptive_name, const char *unit_name,
                         struct data_spec *spec, int soa_icompo, void *val,
                         size_t ntuple, int unit_size,
                         jupiter_bind_data_type type,
                         const char **component_names, add_attribute_func *func,
                         void *arg)
{
  return func(descriptive_name, unit_name, spec ? spec->name : NULL, soa_icompo,
              val, ntuple, unit_size, type, component_names, arg);
}

static int add_attribute_f(const char *descriptive_name, const char *unit_name,
                           struct data_spec *spec, int soa_icompo, void *val,
                           size_t ntuple, int unit_size,
                           jupiter_bind_data_type type,
                           const char **component_names,
                           const convert_fluid_funcs *funcs)
{
  return add_attribute(descriptive_name, unit_name, spec, soa_icompo, val,
                       ntuple, unit_size, type, component_names,
                       funcs->add_attribute, funcs->arg);
}

static int add_attribute_p(const char *descriptive_name, const char *unit_name,
                           struct data_spec *spec, int soa_icompo, void *val,
                           size_t ntuple, int unit_size,
                           jupiter_bind_data_type type,
                           const char **component_names,
                           const convert_particles_funcs *funcs)
{
  return add_attribute(descriptive_name, unit_name, spec, soa_icompo, val,
                       ntuple, unit_size, type, component_names,
                       funcs->add_attribute, funcs->arg);
}

static int check_status_and_add(parameter *p, int r, void **data,
                                const char *name, struct data_spec *spec,
                                int icompo, size_t ntuple, int unit_size,
                                jupiter_bind_data_type type,
                                const convert_fluid_funcs *funcs)
{
  if (for_any_rank(p->mpi, r != 0)) {
    free(*data);
    return 1;
  }
  r = 1;
  if (*data) {
    r = add_attribute_f(name, NULL, spec, icompo, *data, ntuple, unit_size,
                        type, NULL, funcs);
  }
  if (r) {
    free(*data);
  }
  r = for_any_rank(p->mpi, r != 0);
  return r;
}

static int check_status_and_add_lpt(parameter *p, int r, void **data, int npt,
                                    const char *name, struct data_spec *spec,
                                    int icompo, int unit_size, int call_read,
                                    jupiter_bind_data_type type,
                                    const convert_particles_funcs *funcs)
{
  if (for_any_rank(p->mpi, r != 0)) {
    free(*data);
    return 1;
  }
  r = 1;
  if (call_read && *data) {
    r = add_attribute_p(name, NULL, spec, icompo, *data, npt, unit_size, type,
                        NULL, funcs);
  } else {
    r = add_attribute_p(name, NULL, spec, icompo, NULL, 0, unit_size, type,
                        NULL, funcs);
  }
  if (r) {
    free(*data);
  }
  r = for_any_rank(p->mpi, r != 0);
  return r;
}

static int check_status_and_add_lpt_position(
  parameter *p, int r, void **data, int npt, int unit_size, int call_read,
  jupiter_bind_data_type type, const convert_particles_funcs *funcs)
{
  if (for_any_rank(p->mpi, r != 0)) {
    free(*data);
    return 1;
  }
  r = 1;
  if (call_read && *data) {
    r = funcs->set_lpt_positions(*data, npt, type, funcs->arg);
  } else {
    r = funcs->set_lpt_positions(NULL, 0, type, funcs->arg);
  }
  if (r) {
    free(*data);
  }
  r = for_any_rank(p->mpi, r != 0);
  return r;
}

static int read_data_add_outputv(
  const convert_fluid_funcs *funcs, parameter *p, const char *input_dir,
  jupiter_bind_data_type type, struct data_spec *spec, int unit_size, int iout,
  int component_id, binary_output_mode outmode, const char *name, va_list ap)
{
  void *data;
  char *nm;
  domain *cdo;
  int r;

  r = jupiter_vasprintf(&nm, name, ap);
  if (for_any_rank(p->mpi, r < 0)) {
    if (r >= 0)
      free(nm);
    return 1;
  }

  cdo = p->cdo;
  data = funcs->allocate_array(cdo->n * unit_size, type, funcs->arg);
  if (for_any_rank(p->mpi, !data)) {
    free(nm);
    free(data);
    if (!data) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
    }
    return 1;
  }

  r = input_binary_spec(p, input_dir, data, component_id, spec, unit_size, iout,
                        outmode, type);
  r = check_status_and_add(p, r, &data, nm, spec, component_id, cdo->n,
                           unit_size, type, funcs);
  free(nm);
  return r;
}

static int read_data_add_output(const convert_fluid_funcs *funcs, parameter *p,
                                const char *input_dir,
                                jupiter_bind_data_type type,
                                struct data_spec *spec, int unit_size, int iout,
                                int component_id, binary_output_mode outmode,
                                const char *name, ...)
{
  int r;
  va_list ap;
  va_start(ap, name);
  r = read_data_add_outputv(funcs, p, input_dir, type, spec, unit_size, iout,
                            component_id, outmode, name, ap);
  va_end(ap);
  return r;
}

static int read_data_add_output_lptv(const convert_particles_funcs *funcs,
                                     parameter *p, const char *input_dir,
                                     int npt, jupiter_bind_data_type type,
                                     struct data_spec *spec, int unit_size,
                                     int iout, int component_id,
                                     binary_output_mode outmode,
                                     const char *name, va_list ap)
{
  void *data;
  char *nm;
  int call_read;
  int r;

  r = jupiter_vasprintf(&nm, name, ap);
  if (for_any_rank(p->mpi, r < 0)) {
    if (r >= 0)
      free(nm);
    return 1;
  }

  call_read = (p->mpi->rank == 0);
  data = NULL;
  r = 0;
  if (call_read) {
    data = funcs->allocate_array(npt * unit_size, type, funcs->arg);
    if (!data) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      r = 1;
    }
  }
  if (for_any_rank(p->mpi, r)) {
    free(nm);
    free(data);
    return 1;
  }

  r = input_binary_spec_lpt(p, input_dir, npt, data, component_id, spec,
                            unit_size, iout, outmode, call_read, type);
  r = check_status_and_add_lpt(p, r, &data, npt, nm, spec, component_id,
                               unit_size, call_read, type, funcs);
  free(nm);
  return r;
}

static int read_data_add_output_lpt(
  const convert_particles_funcs *funcs, parameter *p, const char *input_dir,
  int npt, jupiter_bind_data_type type, struct data_spec *spec, int unit_size,
  int iout, int component_id, binary_output_mode outmode, const char *name, ...)
{
  int r;
  va_list ap;
  va_start(ap, name);
  r = read_data_add_output_lptv(funcs, p, input_dir, npt, type, spec, unit_size,
                                iout, component_id, outmode, name, ap);
  va_end(ap);
  return r;
}

static int read_data_add_output_lpt_position(
  const convert_particles_funcs *funcs, parameter *p, const char *input_dir,
  int npt, jupiter_bind_data_type type, struct data_spec *spec, int unit_size,
  int iout, int component_id, binary_output_mode outmode)
{
  void *data;
  int call_read;
  int r;

  call_read = (p->mpi->rank == 0);
  data = NULL;
  r = 0;
  if (call_read) {
    data = funcs->allocate_array(npt * unit_size, type, funcs->arg);
    if (!data) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      r = 1;
    }
  }
  if (for_any_rank(p->mpi, r)) {
    free(data);
    return 1;
  }

  r = input_binary_spec_lpt(p, input_dir, npt, data, component_id, spec,
                            unit_size, iout, outmode, call_read, type);
  r = check_status_and_add_lpt_position(p, r, &data, npt, unit_size, call_read,
                                        type, funcs);
  return r;
}

static int read_geom_dump(parameter *p, int iout, geom_data *data,
                          const convert_fluid_funcs *funcs)
{
  const char *name;
  geom_data_element *el;
  const geom_user_defined_data *ud;
  jupiter_geom_ext_eldata *extd;
  int r;
  struct data_spec spec = {
    .outf = ON,
    .name = "",
  };

  el = geom_data_get_element(data);
  for (; el; el = geom_data_element_next(el)) {
    name = geom_data_element_get_name(el);
    if (!name || strlen(name) <= 0)
      continue;

    ud = geom_data_element_get_extra_data(el);
    extd = (jupiter_geom_ext_eldata *)geom_user_defined_data_get(ud);
    if (!extd || !extd->dump_file)
      continue;

    spec.filename_template = extd->dump_file;
    r = read_data_add_output(funcs, p, ".", JUPITER_BIND_TYPE_FLOAT, &spec, 1,
                             iout, 0, extd->dump_united, "%s", name);
    if (r)
      return r;
  }
  return 0;
}

static int read_time_add_output(parameter *p, const char *input_dir, int iout,
                                struct data_spec *spec,
                                const convert_fluid_funcs *funcs)
{
  char *path;
  int r;
  int rd;
  type t;

  t = p->cdo->time;
  /*
   * `input_time_file()` will fail if read time is outside of
   * [cdo->time, cdo->tend] (and modifies `cdo->time`).
   */

  r = make_time_file_name(&path, input_dir, spec, iout);
  if (r < 0)
    return 1;

  rd = input_time_file(p->mpi, p->cdo, path);
  free(path);

  if (for_all_rank(p->mpi, rd == 0)) {
    rd = funcs->set_time(p->cdo->time, funcs->arg);
  }

  p->cdo->time = t;

  return rd;
}

static jupiter_bind_data_type get_data_type(flags *flg, int iout)
{
#ifndef JUPITER_DOUBLE
  return JUPITER_BIND_TYPE_FLOAT;
#endif

  if (iout < 0)
    return JUPITER_BIND_TYPE_DOUBLE;
  if (flg->use_double_binary == ON)
    return JUPITER_BIND_TYPE_DOUBLE;
  return JUPITER_BIND_TYPE_FLOAT;
}

static int read_comp_data(mpi_param *mpi, const char *input_dir, int iout,
                          struct data_spec *comp_data_spec,
                          component_data *comps_data_head,
                          struct component_data_metadata *metadata_out)
{
  char *comp_fname;
  int r;
  r = make_file_name(&comp_fname, input_dir, comp_data_spec->filename_template,
                     comp_data_spec->name, 0, iout, 0, BINARY_OUTPUT_UNIFY_MPI);
  if (for_any_rank(mpi, r)) {
    if (r) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
    } else {
      free(comp_fname);
    }
    return 1;
  }

  r = component_data_read(comp_fname, comps_data_head, metadata_out);
  free(comp_fname);

  return r;
}

static binary_output_mode
get_output_mode(mpi_param *mpi, flags *flg, const char *input_dir, int iout,
                struct data_spec *comp_data_spec,
                component_data *comps_data_head,
                struct component_data_metadata *metadata_out)
{
  component_data comps_tmp_head;
  component_data *head;

  struct component_data_metadata tmp_mdata;
  struct component_data_metadata *metadata;

  char *comp_fname = NULL;
  int r = 0;

  component_data_init(&comps_tmp_head);

  if (comps_data_head) {
    component_data_delete_all(comps_data_head);
    head = comps_data_head;
  } else {
    head = &comps_tmp_head;
  }
  if (metadata_out) {
    metadata = metadata_out;
  } else {
    metadata = &tmp_mdata;
  }

  r = read_comp_data(mpi, input_dir, iout, comp_data_spec, head, metadata);
  component_data_delete_all(&comps_tmp_head);

  if (r) {
    if (iout < 0)
      return flg->restart_input_mode;
    return flg->output_mode;
  }

  return metadata->output_mode;
}

static struct data_output_spec *get_data_spec(flags *flg, int iout)
{
  if (iout < 0)
    return &flg->restart_data;
  return &flg->output_data;
}

static int convert_coord_float(type *f, int n, int stm,
                               allocate_array_func *afunc,
                               set_coordinate_func *coordfunc, void *arg)
{
#ifdef JUPITER_DOUBLE
  float *af;
  int i;
  if (!(af = (float *)afunc(n + 1, JUPITER_BIND_TYPE_FLOAT, arg)))
    return 1;
  for (int i = 0; i < n + 1; ++i)
    af[i] = f[i + stm];
  return coordfunc(af, n + 1, JUPITER_BIND_TYPE_FLOAT, 1, arg);
#else
  return coordfunc(&f[stm], n + 1, JUPITER_BIND_TYPE_FLOAT, 0, arg);
#endif
}

static int convert_coord_double(type *f, int n, int stm,
                                allocate_array_func *afunc,
                                set_coordinate_func *coordfunc, void *arg)
{
#ifdef JUPITER_DOUBLE
  return coordfunc(&f[stm], n + 1, JUPITER_BIND_TYPE_DOUBLE, 0, arg);
#else
  double *af;
  int i;
  if (!(af = (double *)afunc(n + 1, JUPITER_BIND_TYPE_DOUBLE, arg)))
    return 1;
  for (int i = 0; i < n + 1; ++i)
    af[i] = f[i + stm];
  return coordfunc(af, n + 1, JUPITER_BIND_TYPE_DOUBLE, 1, arg);
#endif
}

static int convert_coords_from_cdo(domain *cdo, jupiter_bind_data_type dtype,
                                   allocate_array_func *afunc,
                                   set_coordinate_func *set_x_coord_func,
                                   set_coordinate_func *set_y_coord_func,
                                   set_coordinate_func *set_z_coord_func,
                                   void *arg)
{
  int rx, ry, rz;

  if (dtype == JUPITER_BIND_TYPE_DOUBLE) {
    rx = convert_coord_double(cdo->x, cdo->nx, cdo->stm, afunc,
                              set_x_coord_func, arg);
    ry = convert_coord_double(cdo->y, cdo->ny, cdo->stm, afunc,
                              set_y_coord_func, arg);
    rz = convert_coord_double(cdo->z, cdo->nz, cdo->stm, afunc,
                              set_z_coord_func, arg);
  } else {
    rx = convert_coord_float(cdo->x, cdo->nx, cdo->stm, afunc, set_x_coord_func,
                             arg);
    ry = convert_coord_float(cdo->y, cdo->ny, cdo->stm, afunc, set_y_coord_func,
                             arg);
    rz = convert_coord_float(cdo->z, cdo->nz, cdo->stm, afunc, set_z_coord_func,
                             arg);
  }
  return rx || ry || rz;
}

int convert_fluid_mesh(parameter *p, const convert_fluid_mesh_funcs *funcs)
{
  int r;
  jupiter_bind_data_type type;

  type = JUPITER_BIND_TYPE_DOUBLE;
#ifndef JUPITER_DOUBLE
  type = JUPITER_BIND_TYPE_FLOAT;
#endif

  r = convert_coords_from_cdo(p->cdo, type, funcs->allocate_array,
                              funcs->set_x_coordinate, funcs->set_y_coordinate,
                              funcs->set_z_coordinate, funcs->arg);
  if (for_any_rank(p->mpi, r != 0))
    return 1;
  return 0;
}

int convert_fluid(parameter *p, int iout, const char *input_dir,
                  const convert_fluid_funcs *funcs)
{
  domain *cdo;
  flags *flg;
  float *f;
  int r;
  jupiter_bind_data_type type;
  int ic;
  struct component_data_metadata cmetadata;

  struct data_output_spec *spec;
  binary_output_mode outmode;

  flg = p->flg;
  cdo = p->cdo;

  type = get_data_type(flg, iout);
  spec = get_data_spec(flg, iout);
  outmode = get_output_mode(p->mpi, p->flg, input_dir, iout, &spec->comp_data,
                            &p->comps_data_head, &cmetadata);

  if (flg->solute_diff != (cmetadata.solute_diff ? ON : OFF)) {
    char *comp_fname;
    int r =
      make_file_name(&comp_fname, input_dir, spec->comp_data.filename_template,
                     spec->comp_data.name, 0, iout, 0, BINARY_OUTPUT_UNIFY_MPI);
    if (r)
      comp_fname = NULL;
    csvperrorf(comp_fname, 0, 0, CSV_EL_ERROR, NULL,
               "The data file stores solute_diff as '%s', but your flags "
               "input file specifies solute_diff as '%s'",
               cmetadata.solute_diff ? "ON" : "OFF",
               PP_bool_value_format_v(flg->solute_diff));
    free(comp_fname);
    return 1;
  }

  r = convert_coords_from_cdo(cdo, type, funcs->allocate_array,
                              funcs->set_x_coordinate, funcs->set_y_coordinate,
                              funcs->set_z_coordinate, funcs->arg);
  if (for_any_rank(p->mpi, r != 0))
    return 1;

  r = read_time_add_output(p, input_dir, iout, &spec->time, funcs);
  if (r)
    return 1;

  if (flg->solute_diff == ON) {
    for (ic = 0; ic < cdo->NumberOfComponent; ++ic) {
      if (spec->Y.outf == ON) {
        r = read_data_add_output(funcs, p, input_dir, type, &spec->Y, 1, iout,
                                 ic, outmode, "Mole Fraction %d [-]", ic);
        if (r)
          return 1;
      }
    }

    if (spec->Yt.outf == ON) {
      r = read_data_add_output(funcs, p, input_dir, type, &spec->Yt, 1, iout,
                               -1, outmode, "Sum of Mole Fractions [-]");
      if (r)
        return 1;
    }

    for (ic = 0; ic < cdo->NBaseComponent; ++ic) {
      if (spec->Vf.outf == ON) {
        r = read_data_add_output(funcs, p, input_dir, type, &spec->Vf, 1, iout,
                                 ic, outmode, "VOF %d [-]", ic);
        if (r)
          return 1;
      }
    }

    if (spec->fs.outf == ON) {
      r = read_data_add_output(funcs, p, input_dir, type, &spec->fs, 1, iout,
                               -1, outmode, "Solid VOF [-]");
      if (r)
        return 1;
    }

    if (spec->fl.outf == ON) {
      r = read_data_add_output(funcs, p, input_dir, type, &spec->fl, 1, iout,
                               -1, outmode, "Liquid VOF [-]");
      if (r)
        return 1;
    }

  } else {
    for (ic = 0; ic < cdo->NumberOfComponent; ++ic) {
      if (spec->fs.outf == ON) {
        r = read_data_add_output(funcs, p, input_dir, type, &spec->fs, 1, iout,
                                 ic, outmode, "Solid VOF %d [-]", ic);
        if (r)
          return 1;
      }

      if (spec->fl.outf == ON) {
        r = read_data_add_output(funcs, p, input_dir, type, &spec->fl, 1, iout,
                                 ic, outmode, "Liquid VOF %d [-]", ic);
        if (r)
          return 1;
      }
    }
  }

  if (spec->uvw.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->uvw, 3, iout, -1,
                             outmode, "Velocity [m/s]");
    if (r)
      return 1;
  }

  if (spec->u.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->u, 1, iout, -1,
                             outmode, "Staggard Velocity U [m/s]");
    if (r)
      return 1;
  }

  if (spec->v.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->v, 1, iout, -1,
                             outmode, "Staggard Velocity V [m/s]");
    if (r)
      return 1;
  }

  if (spec->w.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->w, 1, iout, -1,
                             outmode, "Staggard Velocity W [m/s]");
    if (r)
      return 1;
  }

  if (spec->t.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->t, 1, iout, -1,
                             outmode, "Temperature [K]");
    if (r)
      return 1;
  }

  if (spec->tf.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->tf, 1, iout, -1,
                             outmode, "Fluid phase Temperature [K]");
    if (r)
      return 1;
  }

  if (spec->ts.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ts, 1, iout, -1,
                             outmode, "Solid phase Temperature [K]");
    if (r)
      return 1;
  }

  if (spec->q.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->q, 1, iout, -1,
                             outmode, "Volumetric Heat Source [W/m3]");
    if (r)
      return 1;
  }

  if (spec->p.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->p, 1, iout, -1,
                             outmode, "Pressure (Relative) [Pa]");
    if (r)
      return 1;
  }

  if (spec->ll.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ll, 1, iout, -1,
                             outmode,
                             "Level Set Function (Liquid..Gas+Solid) [m]");
    if (r)
      return 1;
  }

  if (spec->ls.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ls, 1, iout, -1,
                             outmode,
                             "Level Set Function (Soild..Gas+Liquid) [m]");
    if (r)
      return 1;
  }

  if (spec->lls.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->lls, 1, iout, -1,
                             outmode,
                             "Level Set Function (Gas..Soild+Liquid) [m]");
    if (r)
      return 1;
  }

  if (spec->dens.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->dens, 1, iout,
                             -1, outmode, "Density [kg/m3]");
    if (r)
      return 1;
  }

  if (spec->denss.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->denss, 1, iout,
                             -1, outmode, "Solid Density [kg/m3]");
    if (r)
      return 1;
  }

  if (spec->densf.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->densf, 1, iout,
                             -1, outmode, "Fluid Density [kg/m3]");
    if (r)
      return 1;
  }

  if (spec->specht.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->specht, 1, iout,
                             -1, outmode, "Specific Heat [J/kg.K]");
    if (r)
      return 1;
  }

  if (spec->spechts.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->spechts, 1, iout,
                             -1, outmode, "Solid Specific Heat [J/kg.K]");
    if (r)
      return 1;
  }

  if (spec->spechtf.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->spechtf, 1, iout,
                             -1, outmode, "Fluid Specific Heat [J/kg.K]");
    if (r)
      return 1;
  }

  if (spec->mu.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->mu, 1, iout, -1,
                             outmode, "Viscosity [Pa.s]");
    if (r)
      return 1;
  }

  if (spec->thc.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->thc, 1, iout, -1,
                             outmode, "Thermal Conductivity [W/m.K]");
    if (r)
      return 1;
  }

  if (spec->thcs.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->thcs, 1, iout,
                             -1, outmode, "Solid Thermal Conductivity [W/m.K]");
    if (r)
      return 1;
  }

  if (spec->thcf.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->thcf, 1, iout,
                             -1, outmode, "Fluid Thermal Conductivity [W/m.K]");
    if (r)
      return 1;
  }

  if (spec->eps.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->eps, 1, iout, -1,
                             outmode, "Porosity [-]");
    if (r)
      return 1;
  }

  if (spec->epss.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->epss, 1, iout,
                             -1, outmode, "Effective Porosity [-]");
    if (r)
      return 1;
  }

  if (spec->perm.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->perm, 1, iout,
                             -1, outmode, "Inversed Permeability [1/m2]");
    if (r)
      return 1;
  }

  if (spec->sgm.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->sgm, 1, iout, -1,
                             outmode, "Sigma in Porous Model [J/m3.K]");
    if (r)
      return 1;
  }

  if (spec->t_liq.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->t_liq, 1, iout,
                             -1, outmode, "Liquidus Temperature [K]");
    if (r)
      return 1;
  }

  if (spec->t_soli.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->t_soli, 1, iout,
                             -1, outmode, "Solidus Temperature [K]");
    if (r)
      return 1;
  }

  if (spec->latent.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->latent, 1, iout,
                             -1, outmode, "Latent Heat [J/kg]");
    if (r)
      return 1;
  }

  if (spec->diff_g.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->diff_g, 1, iout,
                             -1, outmode, "Gas Diffusivity [m2/s]");
    if (r)
      return 1;
  }

  if (spec->rad.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->rad, 1, iout, -1,
                             outmode, "Radiation Heat Source [W/m3]");
    if (r)
      return 1;
  }

  if (spec->ox_dt.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ox_dt, 1, iout,
                             -1, outmode, "Oxidized thickness [m]");
    if (r)
      return 1;
  }

  if (spec->ox_dt_local.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ox_dt_local, 1,
                             iout, -1, outmode,
                             "Oxidized thickness (cell-local) [m]");
    if (r)
      return 1;
  }

  if (spec->ox_flag.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, JUPITER_BIND_TYPE_INT,
                             &spec->ox_flag, 1, iout, -1, outmode,
                             "Oxidation status [G-OB-IB-S-F-R]");
  }

  if (spec->ox_vof.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ox_vof, 1, iout,
                             -1, outmode, "Oxidizable Zircaloy VOF [-]");
    if (r)
      return 1;
  }

  if (spec->ox_lset.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ox_lset, 1, iout,
                             -1, outmode, "Oxidizable Zircaloy Level Set [m]");
    if (r)
      return 1;
  }

  if (spec->ox_h2.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ox_h2, 1, iout,
                             -1, outmode, "H2 Produced by Oxidation [kg/m3]");
    if (r)
      return 1;
  }

  if (spec->ox_q.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ox_q, 1, iout,
                             -1, outmode, "Produced Heat by Oxidation [W/m3]");
    if (r)
      return 1;
  }

  if (spec->ox_kp.outf == ON) {
    r =
      read_data_add_output(funcs, p, input_dir, type, &spec->ox_kp, 1, iout, -1,
                           outmode, "Reaction Rate on Oxidation [kg2/m4.s]");
    if (r)
      return 1;
  }

  if (spec->ox_dens.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ox_dens, 1, iout,
                             -1, outmode,
                             "Zircaloy Density (for Oxidation) [kg/m3]");
    if (r)
      return 1;
  }

  if (spec->ox_f_h2o.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ox_f_h2o, 1,
                             iout, -1, outmode,
                             "Region marker of enough H2O to oxidize Zr [-]");
    if (r)
      return 1;
  }

  if (spec->ox_lset_h2o.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ox_lset_h2o, 1,
                             iout, -1, outmode,
                             "Distance from enough H2O region [m]");
    if (r)
      return 1;
  }

  if (spec->ox_lset_h2o_s.outf == ON) {
    r = read_data_add_output(
      funcs, p, input_dir, type, &spec->ox_lset_h2o_s, 1, iout, -1, outmode,
      "Distance from enough H2O region at last oxidation [m]");
    if (r)
      return 1;
  }

  if (spec->ox_recess_rate.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ox_recess_rate,
                             1, iout, -1, outmode,
                             "Recession rate on oxidation [m/s2]");
    if (r)
      return 1;
  }

  if (spec->entha.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->entha, 1, iout,
                             -1, outmode, "Phase Changing Enthalphy [J/kg]");
    if (r)
      return 1;
  }

  if (spec->mushy.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, JUPITER_BIND_TYPE_INT,
                             &spec->mushy, 1, iout, -1, outmode,
                             "Mushy Zone [-1;ID]");
    if (r)
      return 1;
  }

  if (spec->df.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->df, 1, iout, -1,
                             outmode, "Melting VOF Delta [-]");
    if (r)
      return 1;
  }

  if (spec->dfs.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->dfs, 1, iout, -1,
                             outmode, "Solidification VOF Delta [-]");
    if (r)
      return 1;
  }

  if (spec->fs_ibm.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->fs_ibm, 1, iout,
                             -1, outmode, "IBM Solid VOF [-]");
    if (r)
      return 1;
  }

  if (spec->ls_ibm.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->ls_ibm, 1, iout,
                             -1, outmode, "IBM Solid Level Set [m]");
    if (r)
      return 1;
  }

  if (cmetadata.bnd_norm_u && spec->bnd_norm_u.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->bnd_norm_u, 3,
                             iout, -1, outmode,
                             "Surface boundary normal for X staggared");
    if (r)
      return 1;
  }

  if (cmetadata.bnd_norm_v && spec->bnd_norm_v.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->bnd_norm_v, 3,
                             iout, -1, outmode,
                             "Surface boundary normal for Y staggared");
    if (r)
      return 1;
  }

  if (cmetadata.bnd_norm_w && spec->bnd_norm_w.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->bnd_norm_w, 3,
                             iout, -1, outmode,
                             "Surface boundary normal for Z staggared");
    if (r)
      return 1;
  }

  if (cmetadata.qgeom && spec->qgeom.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->qgeom, 1, iout,
                             -1, outmode,
                             "Heat source by Geom_fixed_heat_source [W/m3]");
    if (r)
      return 1;
  }

  if (spec->qpt.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->qpt, 1, iout, -1,
                             outmode, "Heat source from particles [W/m3]");
    if (r)
      return 1;
  }

  if (spec->div_u.outf == ON) {
    r = read_data_add_output(funcs, p, input_dir, type, &spec->div_u, 1, iout,
                             -1, outmode, "Divergence of velocity [1/s]");
    if (r)
      return 1;
  }

  if (spec->mass_source_g.outf == ON) {
    struct geom_list *lp, *lh;
    lh = &p->comps_data_head.list;
    geom_list_foreach (lp, lh) {
      component_data *d;
      d = component_data_entry(lp);
      if (d->mass_source_g_index < 0)
        continue;

      ic = d->comp_index;
      r = read_data_add_output(funcs, p, input_dir, type, &spec->mass_source_g,
                               1, iout, ic, outmode,
                               "Mass source amount for %d [kg]", ic);
      if (r)
        return 1;
    }
  }

  if (p->geom_sets) {
    r = read_geom_dump(p, iout, p->geom_sets, funcs);
    if (r)
      return 1;
  }

  if (p->control_sets) {
    r = read_geom_dump(p, iout, p->geom_sets, funcs);
    if (r)
      return 1;
  }

  return 0;
}

int convert_particles(parameter *p, int iout, int npt, const char *input_dir,
                      const convert_particles_funcs *funcs)
{
  domain *cdo;
  flags *flg;
  float *f;
  int r;
  jupiter_bind_data_type type;
  int ic;

  struct data_output_spec *spec;
  binary_output_mode outmode;

  flg = p->flg;
  cdo = p->cdo;

  type = get_data_type(flg, iout);
  spec = get_data_spec(flg, iout);
  outmode = get_output_mode(p->mpi, p->flg, input_dir, iout, &spec->comp_data,
                            &p->comps_data_head, NULL);

  /* NOP for no particles */
  if (npt <= 0)
    return 0;

  /* Position is mandatory to convert */
  if (spec->lpt_pospt.outf != ON)
    return 1;

  r = read_data_add_output_lpt_position(funcs, p, input_dir, npt, type,
                                        &spec->lpt_pospt, 3, iout, -1, outmode);
  if (r)
    return 1;

  if (spec->lpt_xpt.outf == ON) {
    r =
      read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_xpt,
                               1, iout, -1, outmode, "Particle X Position [m]");
    if (r)
      return 1;
  }

  if (spec->lpt_ypt.outf == ON) {
    r =
      read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_ypt,
                               1, iout, -1, outmode, "Particle Y Position [m]");
    if (r)
      return 1;
  }

  if (spec->lpt_zpt.outf == ON) {
    r =
      read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_zpt,
                               1, iout, -1, outmode, "Particle Z Position [m]");
    if (r)
      return 1;
  }

  if (spec->lpt_pospt.outf == ON) {
    r =
      read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_pospt,
                               3, iout, -1, outmode, "Particle Position [m]");
    if (r)
      return 1;
  }

  if (spec->lpt_uxpt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_uxpt, 1, iout, -1, outmode,
                                 "Particle X Velocity [m/s]");
    if (r)
      return 1;
  }

  if (spec->lpt_uypt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_uypt, 1, iout, -1, outmode,
                                 "Particle Y Velocity [m/s]");
    if (r)
      return 1;
  }

  if (spec->lpt_uzpt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_uzpt, 1, iout, -1, outmode,
                                 "Particle Z Velocity [m/s]");
    if (r)
      return 1;
  }

  if (spec->lpt_upt.outf == ON) {
    r =
      read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_upt,
                               3, iout, -1, outmode, "Particle Velocity [m/s]");
    if (r)
      return 1;
  }

  if (spec->lpt_uf.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_uf,
                                 3, iout, -1, outmode,
                                 "Fluid Velocity around particle[m/s]");
    if (r)
      return 1;
  }

  if (spec->lpt_muf.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_muf,
                                 1, iout, -1, outmode,
                                 "Fluid viscosity around particle [Pa.s]");
    if (r)
      return 1;
  }

  if (spec->lpt_densf.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_densf, 1, iout, -1, outmode,
                                 "Fluid density around particle [kg/m3]");
    if (r)
      return 1;
  }

  if (spec->lpt_cpf.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_cpf,
                                 1, iout, -1, outmode,
                                 "Fluid specific heat around particle "
                                 "[J/kg K]");
    if (r)
      return 1;
  }

  if (spec->lpt_thcf.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_thcf, 1, iout, -1, outmode,
                                 "Fluid thermal conductivity around particle "
                                 "[W/m K]");
    if (r)
      return 1;
  }

  if (spec->lpt_pathf.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_pathf, 1, iout, -1, outmode,
                                 "Mean free path around particle [m]");
    if (r)
      return 1;
  }

  if (spec->lpt_mwf.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_mwf,
                                 1, iout, -1, outmode,
                                 "Mean molecular weight around particle "
                                 "[g/mol]");
    if (r)
      return 1;
  }

  if (spec->lpt_tempf.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_tempf, 1, iout, -1, outmode,
                                 "Fluid temperature around particle [K]");
    if (r)
      return 1;
  }

  if (spec->lpt_gradTf.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_gradTf, 3, iout, -1, outmode,
                                 "Temperature gradient around particle [K/m]");
    if (r)
      return 1;
  }

  if (spec->lpt_diapt.outf == ON) {
    r =
      read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_diapt,
                               1, iout, -1, outmode, "Particle Diameter [m]");
    if (r)
      return 1;
  }

  if (spec->lpt_timpt.outf == ON) {
    r =
      read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_timpt,
                               1, iout, -1, outmode, "Particle Start Time [s]");
    if (r)
      return 1;
  }

  if (spec->lpt_denspt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_denspt, 1, iout, -1, outmode,
                                 "Particle Density [kg/m3]");
    if (r)
      return 1;
  }

  if (spec->lpt_fuxpt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_fuxpt, 1, iout, -1, outmode,
                                 "Particle X Velocity on Scheme [m/s]");
    if (r)
      return 1;
  }

  if (spec->lpt_fuypt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_fuypt, 1, iout, -1, outmode,
                                 "Particle Y Velocity on Scheme [m/s]");
    if (r)
      return 1;
  }

  if (spec->lpt_fuzpt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_fuzpt, 1, iout, -1, outmode,
                                 "Particle Z Velocity on Scheme [m/s]");
    if (r)
      return 1;
  }

  if (spec->lpt_fupt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_fupt, 1, iout, -1, outmode,
                                 "Particle Velocity on Scheme [m/s]");
    if (r)
      return 1;
  }

  if (spec->lpt_fduxt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_fduxt, 1, iout, -1, outmode,
                                 "Particle X Velocity Difference [m/s2]");
    if (r)
      return 1;
  }

  if (spec->lpt_fduyt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_fduyt, 1, iout, -1, outmode,
                                 "Particle Y Velocity Difference [m/s2]");
    if (r)
      return 1;
  }

  if (spec->lpt_fduzt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_fduzt, 1, iout, -1, outmode,
                                 "Particle Z Velocity Difference [m/s2]");
    if (r)
      return 1;
  }

  if (spec->lpt_fdt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_fdt,
                                 1, iout, -1, outmode,
                                 "Particle Velocity Difference [m/s2]");
    if (r)
      return 1;
  }

  if (spec->lpt_temppt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_temppt, 1, iout, -1, outmode,
                                 "Particle Temperature [K]");
    if (r)
      return 1;
  }

  if (spec->lpt_cppt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_cppt, 1, iout, -1, outmode,
                                 "Particle specific heat [J/kg K]");
    if (r)
      return 1;
  }

  if (spec->lpt_thcpt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_thcpt, 1, iout, -1, outmode,
                                 "Particle thermal conductivity [W/m.K]");
    if (r)
      return 1;
  }

  if (spec->lpt_htrpt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_htrpt, 1, iout, -1, outmode,
                                 "Particle heat transfer rate [W/m2 K]");
    if (r)
      return 1;
  }

  if (spec->lpt_tothtpt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_tothtpt, 1, iout, -1, outmode,
                                 "Particle total heat transfer [W]");
    if (r)
      return 1;
  }

  if (spec->lpt_inipospt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_inipospt, 3, iout, -1, outmode,
                                 "Particle Initial Position [m]");
    if (r)
      return 1;
  }

  if (spec->lpt_iniupt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_iniupt, 3, iout, -1, outmode,
                                 "Particle Initial Velocity [m/s]");
    if (r)
      return 1;
  }

  if (spec->lpt_initemppt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_initemppt, 1, iout, -1, outmode,
                                 "Particle Initial Temperature [K]");
  }

#ifdef LPTX
  if (spec->lpt_flags.outf == ON) {
    r =
      read_data_add_output_lpt(funcs, p, input_dir, npt, JUPITER_BIND_TYPE_CHAR,
                               &spec->lpt_flags, LPTX_PTFLAG_MAX, iout, -1,
                               outmode, "Particle Flags [0/1]");
  }
#endif

  if (spec->lpt_exit.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt,
                                 JUPITER_BIND_TYPE_INT, &spec->lpt_exit, 1,
                                 iout, -1, outmode, "Exited Particle [0/1]");
    if (r)
      return 1;
  }

  if (spec->lpt_parceln.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_parceln, 1, iout, -1, outmode,
                                 "Number of particles [-]");
    if (r)
      return 1;
  }

  if (spec->lpt_fbpt.outf == ON) {
    r =
      read_data_add_output_lpt(funcs, p, input_dir, npt, type, &spec->lpt_fbpt,
                               3, iout, -1, outmode, "Brownian force [N]");
    if (r)
      return 1;
  }

  if (spec->lpt_fTpt.outf == ON) {
    r = read_data_add_output_lpt(funcs, p, input_dir, npt, type,
                                 &spec->lpt_fTpt, 3, iout, -1, outmode,
                                 "Thermophoretic force [N]");
    if (r)
      return 1;
  }

  return 0;
}

int get_mpi_rank(parameter *p) { return p->mpi->rank; }

int get_mpi_nproc(parameter *p) { return p->mpi->npe; }

static struct data_spec *get_time_data_spec(parameter *prm, int is_restart)
{
  if (is_restart) {
    return &prm->flg->restart_data.time;
  } else {
    return &prm->flg->output_data.time;
  }
}

char *get_time_file_pattern(parameter *prm, int is_restart)
{
  struct data_spec *spec;
  char *buf;
  int r;

  spec = get_time_data_spec(prm, is_restart);

  r = make_glob_pattern(&buf, spec->filename_template);
  if (r < 0)
    return NULL;

  return buf;
}

int input_time_file_get_index(parameter *prm, const char *base,
                              const char *file, int is_restart, double *time,
                              int *idx)
{
  int r;
  char *cur;
  char *full_template;
  char *time_component;
  char *norm_file;
  type time_save;
  struct data_spec *spec;
  struct format_integers_match_data match_data[4] =
    {{.key = 'c', .type = FORMAT_MATCH_STR, .string = &time_component},
     {.key = 'n', .type = FORMAT_MATCH_INT, .string = NULL},
     {.key = 'i', .type = FORMAT_MATCH_INT, .string = NULL},
     {.key = 'r', .type = FORMAT_MATCH_INT, .string = NULL}};

  spec = get_time_data_spec(prm, is_restart);

  errno = 0;
  r = get_current_directory(&cur);
  if (r < 0) {
    if (errno == 0) {
      csvperror(file, 0, 0, CSV_EL_FATAL,
                "Cannot obtain current directory name", CSV_ERR_NOMEM, 0, 0,
                NULL);
    } else {
      csvperror(file, 0, 0, CSV_EL_FATAL,
                "Cannot obtain current directory name", CSV_ERR_SYS, errno, 0,
                NULL);
    }
    return -1;
  }

  r = join_filenames(&full_template, cur, base);
  free(cur);
  if (r < 0) {
    csvperror(file, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
    return -1;
  }

  cur = full_template;
  r = join_filenames(&full_template, cur, spec->filename_template);
  free(cur);
  if (r < 0) {
    csvperror(file, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
    return -1;
  }

  cur = full_template;
  full_template = canonicalize_path(cur);
  free(cur);
  if (!full_template) {
    csvperror(file, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
    return -1;
  }

  norm_file = canonicalize_path(file);
  if (!norm_file) {
    csvperror(file, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
    return -1;
  }

  r = format_integers_match(norm_file, full_template, 4, match_data);
  if (r <= 0) {
    if (r == 0) {
#ifndef NDEBUG
      csvperrorf(norm_file, 0, 0, CSV_EL_DEBUG, NULL,
                 "Does not match to the template \"%s\"", full_template);
#endif
    } else {
      csvperror(file, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
    }
    free(norm_file);
    free(time_component);
    free(full_template);
    return -1;
  }
  free(full_template);

  if (time_component && strcmp(time_component, "time") != 0) {
    csvperrorf(norm_file, 0, 0, CSV_EL_WARN, NULL,
               "Has component name \"%s\" (expected \"time\")", time_component);
    free(norm_file);
    free(time_component);
    return -1;
  }
  free(time_component);

  if (!match_data[1 /* n */].matched) {
    csvperrorf(norm_file, 0, 0, CSV_EL_WARN, NULL,
               "Can not find matched value for \"%%n\"");
    *idx = -1;
  } else {
    *idx = match_data[1].value;
    if (*idx < 0) {
      csvperrorf(norm_file, 0, 0, CSV_EL_WARN, NULL,
                 "Matched to negative time index value, %d", *idx);
    }
  }
  free(norm_file);

  time_save = prm->cdo->time;
  prm->cdo->time = -HUGE_VAL;

  r = input_time_file(prm->mpi, prm->cdo, file);

  *time = prm->cdo->time;
  prm->cdo->time = time_save;

  if (r) {
    return 0;
  }

  csvperrorf(file, 0, 0, CSV_EL_INFO, NULL,
             "Found data for index %d, time %.7g", *idx, *time);
  return 1;
}

const char *get_input_directory(parameter *prm, int is_restart)
{
  if (is_restart) {
    return prm->flg->restart_data.readdir;
  } else {
    return prm->flg->output_data.readdir;
  }
}

int get_number_of_particles(parameter *prm, int iout, const char *dir)
{
  flags *flg;

  CSVASSERT(prm);

  flg = prm->flg;

#ifdef HAVE_LPT
  if (flg->lpt_calc == ON) {
    jupiter_lpt_ctrl_data *data;
    int r;
    int npt;
    struct data_output_spec *spec;
    struct data_spec *lpt_ctrl_spec;
    binary_output_mode outmode;

    if (iout < 0) {
      spec = &flg->restart_data;
    } else {
      spec = &flg->output_data;
    }
    outmode = flg->output_mode;

    lpt_ctrl_spec = &spec->lpt_ctrl;

    r = input_binary_lpt_ctrl(prm, dir, lpt_ctrl_spec, 0, &data, iout, outmode);
    if (r != 0)
      return -1;

    npt = data->npt;
    delete_lpt_ctrl_data(data);

    return npt;
  } else {
    return 0;
  }
#else
  return 0;
#endif
}
