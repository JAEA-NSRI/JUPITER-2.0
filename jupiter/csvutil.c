
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os/asprintf.h"

#include "component_data.h"
#include "component_data_defs.h"
#include "component_info_defs.h"
#include "control/defs.h"
#include "control/fv_table.h"
#include "control/write_fv_csv.h"
#include "csv.h"
#include "csvtmpl_format.h"
#include "csvutil.h"
#include "csvutil_extra.h"
#include "dccalc.h"
#include "field_control.h"
#include "geometry/bitarray.h"
#include "geometry/defs.h"
#include "geometry/error.h"
#include "optparse.h"
#include "serializer/error.h"
#include "strlist.h"
#include "struct.h"

#include "control/manager.h"
#include "tempdep_properties.h"
#include "tmcalc.h"

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

static int csv_print_error_levels_init = 0;
static jupiter_print_levels csv_print_error_levels = {0};
static int csv_print_error = 1;

static void init_jupiter_print_levels(void)
{
  if (csv_print_error_levels_init)
    return;

  csv_print_error_levels = jupiter_print_levels_non_debug();
  csv_print_error_levels_init = 1;
}

void set_jupiter_print_levels(jupiter_print_levels lvls)
{
  csv_print_error_levels = lvls;
  csv_print_error_levels_init = 1;
}

jupiter_print_levels get_jupiter_print_levels(void)
{
  init_jupiter_print_levels();
  return csv_print_error_levels;
}

/* The arguemnt is whether myrank should print */
void set_jupiter_print_rank(int flag) { csv_print_error = !!flag; }
int get_jupiter_print_rank(void) { return csv_print_error; }

static int csv_do_print(csv_error_level lvl)
{
  if (!csv_print_error)
    return 0;
  return geom_bitarray_element_get(get_jupiter_print_levels().levels, lvl);
}

#define SET_P_EMPTY

#define SET_P_CONV_DESTTYPE(type, var, ptr) \
  SET_P_DESTTYPE_##type(var) = (SET_P_DESTTYPE_##type(SET_P_EMPTY))(ptr)

#define SET_P_CONV_ERRVALTYPE(type, var, ptr) \
  SET_P_ERRVALTYPE_##type(var) = *(SET_P_ERRVALTYPE_##type(*))(ptr)

/**
 * If c in not valid or contains empty string, set *endptr to NULL.
 * If c is valid, but invalid integer value, endptr points non-'\0'.
 * On error, returns 0.
 *
 * For base, see man strtol(3) (0 for auto with prefix, or 2-36).
 *
 * The pointee of endptr is available until freeCSV is called.
 */
int csv_to_int_base(csv_column *c, const char **endptr, int base)
{
  const char *t;
  char *p;
  long int r;
  int i;

  t = getCSVValue(c);
  if (t) {
    r = strtol(t, &p, base);
    if (endptr) {
      if (p == t) {
        *endptr = NULL;
      } else {
        *endptr = p;
      }
    }
  } else {
    if (endptr) {
      *endptr = NULL;
    }
    r = 0;
  }
  i = r;
  if (i != r) { /* Reverse conversion fails: */
    errno = ERANGE;
    if (endptr) {
      *endptr = NULL;
    }
    return 0;
  } else {
    return i;
  }
}

int csv_to_int(void *dest, const void *value_on_fail, const char *fname,
               csv_column *c, csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(int, idest, dest);
  SET_P_CONV_ERRVALTYPE(int, ierrval, value_on_fail);
  const char *ep;
  int r;

  CSVASSERT(idest);

  if (!c) {
    *idest = ierrval;
    return 1;
  }

  errno = 0;
  r = csv_to_int_base(c, &ep, 10);
  if (!ep || *ep != '\0') {
    if (perror) {
      if (errno != 0) {
        csvperror_col(fname, c, errlevel, CSV_ERR_SYS, errno, 0, NULL);
      } else {
        csvperrorf_col(fname, c, errlevel, "Invalid format for int value");
      }
    }
    *idest = ierrval;
    return 2;
  }

  *idest = r;
  return 0;
}

int csv_to_double(void *dest, const void *value_on_fail, const char *fname,
                  csv_column *c, csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(double, ddest, dest);
  SET_P_CONV_ERRVALTYPE(double, errval, value_on_fail);
  const char *t;
  char *ep;
  double r;

  CSVASSERT(ddest);

  if (!c) {
    *ddest = errval;
    return 1;
  }

  t = getCSVValue(c);
  r = NAN;
  ep = NULL;
  errno = 0;

  if (t) {
    r = strtod(t, &ep);
  }
  if (!t || *ep != '\0' || ep == t) {
    if (perror) {
      if (errno != 0) {
        csvperror_col(fname, c, errlevel, CSV_ERR_SYS, errno, 0, NULL);
      } else {
        csvperrorf_col(fname, c, errlevel, "Invalid format for double value");
      }
    }
    *ddest = errval;
    return 2;
  }
  *ddest = r;
  return 0;
}

int csv_to_exact_double(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(exact_double, ddest, dest);
  SET_P_CONV_ERRVALTYPE(exact_double, errval, value_on_fail);
  const char *t;
  char *ep;
  double r;

  CSVASSERT(ddest);

  if (!c) {
    *ddest = errval;
    return 1;
  }

  t = getCSVValue(c);
  r = NAN;
  ep = NULL;
  errno = 0;

  if (t) {
    r = strtod(t, &ep);
  }
  if (!t || *ep != '\0' || ep == t) {
    *ddest = errval;
    if (perror) {
      if (errno != 0) {
        csvperror_col(fname, c, errlevel, CSV_ERR_SYS, errno, 0, NULL);
      } else {
        csvperrorf_col(fname, c, errlevel, "Invalid format for double value");
      }
    }
    return 2;
  }
  *ddest = r;
  return 0;
}

int csv_to_const_charp(void *dest, const void *value_on_fail, const char *fname,
                       csv_column *c, csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(const_charp, cdest, dest);
  SET_P_CONV_ERRVALTYPE(const_charp, errval, value_on_fail);
  struct csv_to_const_charp_data *pp;
  const char *str;

  CSVASSERT(cdest);

  str = NULL;
  if (c) {
    str = getCSVValue(c);
  }
  if (!str) {
    *cdest = errval;
    return 1;
  }
  *cdest = str;
  if (!c) {
    return 1;
  }
  return 0;
}

int csv_to_charp(void *dest, const void *value_on_fail, const char *fname,
                 csv_column *c, csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(charp, cdest, dest);
  SET_P_CONV_ERRVALTYPE(charp, errval, value_on_fail);
  struct csv_to_charp_data *pp;
  const char *str;
  char *a;

  CSVASSERT(cdest);

  str = NULL;
  a = NULL;
  if (c) {
    str = getCSVValue(c);
  }

  if (!str) {
    str = errval;
    if (!str) {
      *cdest = NULL;
      return 0;
    }
  }

  a = jupiter_strdup(str);
  if (!a) {
    *cdest = NULL;
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return -1;
  }

  *cdest = a;
  if (!c) {
    return 1;
  }
  return 0;
}

int format_has_mpi_rank(const char *text)
{
  int have;
  have = format_integers(NULL, text, "r", 0);
  if (have < 0) { /* Invalid % notation is included */
    return -1;
  }

  have = format_integers(NULL, text, "");
  if (have < 0) { /* %r is included */
    return 1;
  }
  return 0;
}

int format_mpi_rank(char **buf, const char *text)
{
  int rank;

#ifdef JUPITER_MPI
  int f;
  int r;

  r = MPI_SUCCESS;

  /* MPI may *not* be initialized at this point. */
  MPI_Initialized(&f); /* This function always succeeds. */
  rank = -1;
  if (f) {
    MPI_Finalized(&f);
    if (!f) {
      r = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    }
  }
  if (r != MPI_SUCCESS) {
    return -1;
  }
#else
  rank = 0;
#endif

  return format_integers(buf, text, "r", rank);
}

int format_without_mpi_rank(char **buf, const char *text)
{
  return format_integers(buf, text, "");
}

/**
 * Embed rank number. For "-", returns NULL.
 *
 * Returned pointer must be freed.
 */
int csv_to_FILEn(void *dest, const void *value_on_fail, const char *fname,
                 csv_column *c, csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(FILEn, fdest, dest);
  SET_P_CONV_ERRVALTYPE(FILEn, errval, value_on_fail);
  const char *str;
  int has_r;
  int ret;
  char *tmpfname;

  CSVASSERT(fdest);

  if (c) {
    str = getCSVValue(c);
  } else {
    str = errval;
  }

  if (!str || strcmp(str, "-") == 0) {
    *fdest->filename = NULL;
    if (fdest->has_r)
      *fdest->has_r = 0;
    if (!c)
      return 1;
    return 0;
  }

  if (*str == '\0') {
    if (fdest->has_r)
      *fdest->has_r = -1;
    *fdest->filename = NULL;
    CSVASSERT(c);
    if (!c) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG, str,
                 "The default (value_on_fail) value is an empty string, which "
                 "is not valid");
      return -2;
    }
    if (perror) {
      csvperrorf_col(fname, c, errlevel,
                     "An empty string is given for a filename");
    }
    return 2;
  }

  has_r = format_has_mpi_rank(str);
  if (fdest->has_r)
    *fdest->has_r = has_r;
  if (has_r < 0) {
    *fdest->filename = NULL;
    CSVASSERT(c);
    if (!c) {
      csvperrorf(
        __FILE__, __LINE__, 0, CSV_EL_DEBUG, str,
        "The default (value_on_fail) value is not valid format for FILEn");
      return -2;
    }
    if (perror) {
      csvperrorf_col(fname, c, errlevel,
                     "Illegally-formatted filename for FILEn");
    }
    return 2;
  }

  ret = format_mpi_rank(&tmpfname, str);
  if (ret < 0) {
#ifdef JUPITER_MPI
    int init, fin;
    MPI_Initialized(&init);
    MPI_Finalized(&fin);

    /* Calling before initialize or after finalize is basically logic error */
    CSVASSERT(init && !fin);
    if (!init) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                 "csv_to_FILEn has been called before MPI has initialized");
    } else if (fin) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                 "csv_to_FILEn has been called after MPI has finalized");
    } else {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
    }
#else
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
#endif
    *fdest->filename = NULL;
    return -1;
  }

  *fdest->filename = tmpfname;
  if (!c)
    return 1;
  return 0;
}

int csv_to_FILEn_withMPI(void *dest, const void *value_on_fail,
                         const char *fname, csv_column *c,
                         csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(FILEn_withMPI, fdest, dest);
  SET_P_CONV_ERRVALTYPE(FILEn_withMPI, errval, value_on_fail);
  const char *str;
  int ret;
  char *tmpfname;

  if (c) {
    str = getCSVValue(c);
  } else {
    str = errval;
  }

  if (!str || strcmp(str, "-") == 0) {
    *fdest = NULL;
    if (!c)
      return 1;
    return 0;
  }

  if (*str == '\0') {
    *fdest = NULL;
    CSVASSERT(c);
    if (!c) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG, str,
                 "The default (value_on_fail) value is an empty string, which "
                 "is not valid");
      return -2;
    }
    if (perror) {
      csvperrorf_col(fname, c, errlevel,
                     "An empty string is given for a filename");
    }
    return 2;
  }

  ret = format_has_mpi_rank(str);
  if (ret <= 0) {
    *fdest = NULL;
    CSVASSERT(c);
    if (!c) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG, str,
                 "The default (value_on_fail) value is invalid format for "
                 "FILEn_withMPI");
      return -2;
    }
    if (perror) {
      if (ret == 0) {
        csvperrorf_col(fname, c, errlevel,
                       "The given name does not include %%r notation for "
                       "embedding rank number");
      } else {
        csvperrorf_col(fname, c, errlevel,
                       "Illegally-formatted filename for FILEn_withMPI");
      }
    }
    return 2;
  }

  ret = format_mpi_rank(&tmpfname, str);
  if (ret < 0) {
#ifdef JUPITER_MPI
    int init, fin;
    MPI_Initialized(&init);
    MPI_Finalized(&fin);

    /* Calling before initialize or after finalize is basically logic error */
    CSVASSERT(init && !fin);
    if (!init) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                 "csv_to_FILEn_withMPI called before MPI has initialized");
    } else if (fin) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                 "csv_to_FILEn_withMPI called after MPI has finalized");
    } else {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
    }
#else
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
#endif
    return -1;
  }
  *fdest = tmpfname;
  if (!c)
    return 1;
  return 0;
}

/**
 * Same for csv_to_FILEn(). But if the text includes MPI rank conversion,
 * sets NULL. For "-", also returns NULL.
 *
 * Returned pointer must be freed.
 */
int csv_to_FILEn_noMPI(void *dest, const void *value_on_fail, const char *fname,
                       csv_column *c, csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(FILEn_noMPI, fdest, dest);
  SET_P_CONV_ERRVALTYPE(FILEn_noMPI, errval, value_on_fail);
  const char *str;
  int ret;
  char *tmpfname;

  CSVASSERT(fdest);

  if (c) {
    str = getCSVValue(c);
  } else {
    str = errval;
  }

  if (!str || strcmp(str, "-") == 0) {
    *fdest = NULL;
    if (!c)
      return 1;
    return 0;
  }

  if (*str == '\0') {
    *fdest = NULL;
    CSVASSERT(c);
    if (!c) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG, str,
                 "The default (value_on_fail) value is an empty string, which "
                 "is not valid");
      return -2;
    }
    if (perror) {
      csvperrorf_col(fname, c, errlevel,
                     "An empty string is given for a filename");
    }
    return 2;
  }

  ret = format_has_mpi_rank(str);
  if (ret != 0) {
    *fdest = NULL;
    CSVASSERT(c);
    if (!c) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG, str,
                 "Default (value_on_fail) value is invalid format for "
                 "FILEn_noMPI");
      return -2;
    }
    if (perror) {
      if (ret > 0) {
        csvperrorf_col(fname, c, errlevel,
                       "The given name includes %%r notation for embedding "
                       "rank number, which is not allowed here");
      } else {
        csvperrorf_col(fname, c, errlevel,
                       "Illegally-formatted filename for FILEn_noMPI");
      }
    }
    return 2;
  }

  ret = format_without_mpi_rank(&tmpfname, str);
  if (ret < 0) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    *fdest = NULL;
    return -1;
  }

  *fdest = tmpfname;
  if (!c)
    return 1;
  return 0;
}

int csv_to_DIRn(void *dest, const void *value_on_fail, const char *fname,
                csv_column *c, csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(DIRn, ddest, dest);
  SET_P_CONV_ERRVALTYPE(DIRn, errval, value_on_fail);
  const char *str;
  int ret;
  char *tmpfname;

  CSVASSERT(ddest);

  if (c) {
    str = getCSVValue(c);
  } else {
    str = errval;
  }

  if (!str) {
    *ddest = NULL;
    if (!c)
      return 1;
    return 0;
  }

  ret = format_has_mpi_rank(str);
  if (ret < 0) {
    *ddest = NULL;
    CSVASSERT(c);
    if (!c) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG, str,
                 "Default (value_on_fail) value is invalid format for DIRn");
      return -2;
    }
    if (perror) {
      csvperrorf_col(fname, c, errlevel,
                     "Illegally-formatted directory name for DIRn");
    }
    return 2;
  }

  ret = format_mpi_rank(&tmpfname, str);
  if (ret < 0) {
    *ddest = NULL;
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return -1;
  }

  *ddest = tmpfname;
  return 0;
}

/**
 * @brief Define csv_to_##type function using str_to_##type function.
 * @param type type name
 * @param invalid The return value from str_to_##type when illegally formatted
 *
 * @p invalid must be comparable as a scalar value, i.e., `a == b` can be used.
 */
#define DEFINE_CSV_TO_KEYWORD_TYPE_BASE(type, funcname, str_conv, invalid) \
  int funcname(void *dest, const void *value_on_fail, const char *fname,   \
               csv_column *c, csv_error_level errlevel, int perror)        \
  {                                                                        \
    SET_P_CONV_DESTTYPE(type, kdest, dest);                                \
    SET_P_CONV_ERRVALTYPE(type, errval, value_on_fail);                    \
    const char *str;                                                       \
                                                                           \
    CSVASSERT(kdest);                                                      \
                                                                           \
    if (!c) {                                                              \
      *kdest = errval;                                                     \
      return 1;                                                            \
    }                                                                      \
                                                                           \
    str = getCSVValue(c);                                                  \
    if (!str) {                                                            \
      *kdest = errval;                                                     \
      return 1;                                                            \
    }                                                                      \
                                                                           \
    *kdest = str_conv(str);                                                \
    if (*kdest == (invalid)) {                                             \
      if (perror) {                                                        \
        csvperrorf_col(fname, c, errlevel, "Invalid value in " #type);     \
      }                                                                    \
      return 2;                                                            \
    }                                                                      \
    return 0;                                                              \
  }

#define DEFINE_CSV_TO_KEYWORD_TYPE(type, invalid) \
  DEFINE_CSV_TO_KEYWORD_TYPE_BASE(type, csv_to_##type, str_to_##type, invalid)

DEFINE_CSV_TO_KEYWORD_TYPE(bool, -1)
DEFINE_CSV_TO_KEYWORD_TYPE(interface_capturing_scheme, -1)
DEFINE_CSV_TO_KEYWORD_TYPE(boundary, -1)
DEFINE_CSV_TO_KEYWORD_TYPE(tboundary, -1)
DEFINE_CSV_TO_KEYWORD_TYPE(out_p_cond, OUT_P_COND_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(vof_phase, GEOM_PHASE_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(geom_data_op, GEOM_OP_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(geom_shape_op, GEOM_SOP_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(geom_shape, GEOM_SHAPE_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(geom_surface_shape, GEOM_SURFACE_SHAPE_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(init_func, GEOM_INIT_FUNC_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(trip_control, TRIP_CONTROL_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(boundary_dir, BOUNDARY_DIR_NONE)
DEFINE_CSV_TO_KEYWORD_TYPE(inlet_dir, SURFACE_INLET_DIR_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(tm_func2_model, TM_FUNC_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(dc_func2_model, DC_FUNCS_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(solid_form, SOLID_FORM_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(binary_output_mode, BINARY_OUTPUT_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(tempdep_property_type, TEMPDEP_PROPERTY_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(ox_kp_model, OX_RRMODEL_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(component_phase, COMPONENT_PHASE_MAX)
DEFINE_CSV_TO_KEYWORD_TYPE(postp, JCNTRL_EXE_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(maskp, JCNTRL_EXE_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(fieldp, JCNTRL_EXE_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(fv_tabbnd, JCNTRL_FV_TABLE_EXTEND_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(jcntrl_lop, JCNTRL_LOP_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(jcntrl_compp, JCNTRL_COMP_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(write_fv_csv_mode, JCNTRL_WRITE_FV_OUTPUT_BY_NONE)
DEFINE_CSV_TO_KEYWORD_TYPE(write_fv_form, WRITE_FIELD_VARIABLES_FORMAT_INVALID)
DEFINE_CSV_TO_KEYWORD_TYPE(non_uniform_grid_func, NON_UNIFORM_GRID_FUNC_INVALID)

DEFINE_CSV_TO_KEYWORD_TYPE(LPTts, LPTts_invalid())
DEFINE_CSV_TO_KEYWORD_TYPE(LPTht, LPTht_invalid())

int csv_to_component_info_data(void *dest, const void *value_on_fail,
                               const char *fname, csv_column *c,
                               csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(component_info_data, dest_data, dest);
  SET_P_CONV_ERRVALTYPE(component_info_data, errval, value_on_fail);
  struct component_info_data *ddest;
  const char *ep;
  int r;

  CSVASSERT(dest_data);
  CSVASSERT(dest_data->comp_data_head);
  CSVASSERT(dest_data->dest);

  ddest = dest_data->dest;

  if (!c) {
    ddest->d = NULL;
    ddest->id = errval;
    return 1;
  }

  errno = 0;
  r = csv_to_int_base(c, &ep, 10);
  if (!ep || *ep != '\0') {
    if (perror) {
      if (errno != 0) {
        csvperror_col(fname, c, errlevel, CSV_ERR_SYS, errno, 0, NULL);
      } else {
        csvperrorf_col(fname, c, errlevel, "Invalid format for int value");
      }
    }
    r = errval;
  }

  ddest->d = component_data_find_by_jupiter_id(dest_data->comp_data_head, r);
  if (!ddest->d) {
    if (perror)
      csvperrorf_col(fname, c, errlevel, "Invalid material ID");
    ddest->id = r;
    return 2;
  }

  ddest->id = r;
  return 0;
}

/**
 * @retval  0 Found
 * @retval  1 @p name does not start with @p prefix
 * @retval -1 Not found
 */
static int csv_to_executive_common(jcntrl_executive_manager *manager,
                                   const char *name, const char *prefix,
                                   int prefixlen,
                                   jcntrl_executive_manager_entry **outp)
{
  jcntrl_executive_manager_entry *entry;

  if (prefix && strncmp(name, prefix, prefixlen) != 0)
    return 1;

  entry = jcntrl_executive_manager_has(manager, name);
  if (!entry)
    return -1;

  *outp = entry;
  return 0;
}

static int csv_to_executive_any(jcntrl_executive_manager *manager,
                                const char *name,
                                jcntrl_executive_manager_entry **outp)
{
  return csv_to_executive_common(manager, name, NULL, 0, outp);
}

static int csv_to_executive_grid(jcntrl_executive_manager *manager,
                                 const char *name,
                                 jcntrl_executive_manager_entry **outp)
{
  return csv_to_executive_common(manager, name, CONTROL_KEYCHAR_GRID,
                                 strlen(CONTROL_KEYCHAR_GRID), outp);
}

static int csv_to_executive_mask(jcntrl_executive_manager *manager,
                                 const char *name,
                                 jcntrl_executive_manager_entry **outp)
{
  return csv_to_executive_common(manager, name, CONTROL_KEYCHAR_MASK,
                                 strlen(CONTROL_KEYCHAR_MASK), outp);
}

static int csv_to_executive_geom(jcntrl_executive_manager *manager,
                                 const char *name,
                                 jcntrl_executive_manager_entry **outp)
{
  return csv_to_executive_common(manager, name, CONTROL_KEYCHAR_GEOM,
                                 strlen(CONTROL_KEYCHAR_GEOM), outp);
}

static int csv_to_executive_fvar(jcntrl_executive_manager *manager,
                                 const char *name,
                                 jcntrl_executive_manager_entry **outp)
{
  return csv_to_executive_common(manager, name, CONTROL_KEYCHAR_FVAR,
                                 strlen(CONTROL_KEYCHAR_FVAR), outp);
}

int csv_to_controllable_type(void *dest, const void *value_on_fail,
                             const char *fname, csv_column *c,
                             csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(controllable_type, dest_data, dest);
  SET_P_CONV_ERRVALTYPE(controllable_type, errval, value_on_fail);
  const char *str;
  double r;
  char *ep;
  controllable_type *cdest;

  CSVASSERT(dest_data);
  CSVASSERT(dest_data->dest);
  CSVASSERT(dest_data->manager);
  CSVASSERT(dest_data->head);

  cdest = dest_data->dest;
  controllable_type_init(cdest);
  if (!c) {
    cdest->current_value = errval;
    return 1;
  }

  str = getCSVValue(c);
  if (!str) {
    cdest->current_value = errval;
    return 1;
  }

  cdest->exec = NULL;
  if (csv_to_executive_fvar(dest_data->manager, str, &cdest->exec) <= 0) {
    controllable_type_add_to_list(dest_data->head, cdest);
    if (!cdest->exec) {
      if (perror) {
        csvperrorf_col(fname, c, errlevel,
                       "Specified field variable is not defined");
      }
      cdest->exec = jcntrl_executive_manager_reserve(dest_data->manager, str);
      return 2;
    }
    return 0;
  }

  errno = 0;
  r = strtod(str, &ep);
  if (errno != 0 || *ep != '\0' || ep == str) {
    if (perror) {
      if (errno != 0) {
        csvperror_col(fname, c, errlevel, CSV_ERR_SYS, errno, 0, NULL);
      } else {
        csvperrorf_col(fname, c, errlevel,
                       "Invalid format for double value in controllable_type");
      }
    }
    cdest->current_value = errval;
    return 2;
  }

  cdest->current_value = r;
  return 0;
}

typedef int csv_to_executive_func(jcntrl_executive_manager *manager,
                                  const char *name,
                                  jcntrl_executive_manager_entry **outp);

static int csv_to_control_common(jcntrl_executive_manager *manager,
                                 jcntrl_executive_manager_entry **outp,
                                 const char *fname, csv_column *c,
                                 const char *errval, csv_error_level errlevel,
                                 int perror, csv_to_executive_func *getter,
                                 csv_to_executive_func *errval_getter,
                                 const char *control_type_name)
{
  int r;
  const char *str;

  r = 1;
  str = NULL;
  if (c) {
    str = getCSVValue(c);
    if (str) {
      r = getter(manager, str, outp);
      if (r != 0) {
        if (perror) {
          csvperrorf_col(fname, c, errlevel,
                         "Specified %s entry is not defined",
                         control_type_name ? control_type_name : "control");
        }
        r = 1;
      }
    }
  }
  if (r != 0) {
    if (errval) {
      int rr;
      rr = errval_getter(manager, errval, outp);
      if (rr != 0) {
        csvperrorf_col(fname, c, CSV_EL_ERROR,
                       "The default value of %s entry is not defined",
                       control_type_name ? control_type_name : "control");
        r = -2;
      }
    }
  }
  return r;
}

int csv_to_control_executive(void *dest, const void *value_on_fail,
                             const char *fname, csv_column *c,
                             csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(control_executive, dest_data, dest);
  SET_P_CONV_ERRVALTYPE(control_executive, errval, value_on_fail);

  return csv_to_control_common(dest_data->manager, dest_data->dest, fname, c,
                               errval, errlevel, perror, csv_to_executive_any,
                               csv_to_executive_any, "control");
}

int csv_to_control_grid(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(control_grid, dest_data, dest);
  SET_P_CONV_ERRVALTYPE(control_grid, errval, value_on_fail);

  return csv_to_control_common(dest_data->manager, dest_data->dest, fname, c,
                               errval, errlevel, perror, csv_to_executive_grid,
                               csv_to_executive_any, "grid");
}

int csv_to_control_mask(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(control_mask, dest_data, dest);
  SET_P_CONV_ERRVALTYPE(control_mask, errval, value_on_fail);

  return csv_to_control_common(dest_data->manager, dest_data->dest, fname, c,
                               errval, errlevel, perror, csv_to_executive_mask,
                               csv_to_executive_any, "mask");
}

int csv_to_control_geom(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(control_geom, dest_data, dest);
  SET_P_CONV_ERRVALTYPE(control_geom, errval, value_on_fail);

  return csv_to_control_common(dest_data->manager, dest_data->dest, fname, c,
                               errval, errlevel, perror, csv_to_executive_geom,
                               csv_to_executive_any, "geometry");
}

int csv_to_control_fvar(void *dest, const void *value_on_fail,
                        const char *fname, csv_column *c,
                        csv_error_level errlevel, int perror)
{
  SET_P_CONV_DESTTYPE(control_fvar, dest_data, dest);
  SET_P_CONV_ERRVALTYPE(control_fvar, errval, value_on_fail);

  return csv_to_control_common(dest_data->manager, dest_data->dest, fname, c,
                               errval, errlevel, perror, csv_to_executive_fvar,
                               csv_to_executive_any, "field variable");
}

int csv_to_control_varname(void *dest, const void *value_on_fail,
                           const char *fname, csv_column *c,
                           csv_error_level errlevel, int perror)
{
  const char *str;
  SET_P_CONV_DESTTYPE(control_varname, dest_data, dest);
  SET_P_CONV_ERRVALTYPE(control_varname, errval, value_on_fail);

  str = NULL;
  if (c)
    str = getCSVValue(c);
  if (!str)
    str = errval;
  *dest_data = str;
  return 0;
}

//-----

static const char *csvperror_elevel_print(csv_error_level errlevel)
{
  switch (errlevel) {
  case CSV_EL_DEBUG:
    return "DEBUG";
    break;
  case CSV_EL_INFO:
    return "INFO";
    break;
  case CSV_EL_WARN:
    return "WARN";
    break;
  case CSV_EL_ERROR:
    return "ERROR";
    break;
  case CSV_EL_FATAL:
    return "FATAL";
    break;

  case CSV_EL_MAX:
    break;
  }
  return "-----";
}

static jupiter_strlist *csvperror_make_fname_loc(const char *fname, long ln,
                                                 long cl)
{
  CSVASSERT(fname);

  if (ln > 0 && cl > 0) {
    return jupiter_strlist_asprintf("line %ld, column %ld of %s: ", ln, cl,
                                    fname);
  } else if (ln > 0) {
    return jupiter_strlist_asprintf("line %ld of %s: ", ln, fname);
  } else {
    return jupiter_strlist_asprintf("%s: ", fname);
  }
}

static int csvperror_mpi_rank(void)
{
  int rank = -1;

#ifdef JUPITER_MPI
  MPI_Errhandler eh;
  int l, r;

  rank = -1;
  l = 0;
  r = MPI_Initialized(&l);
  do {
    if (r != MPI_SUCCESS)
      break;

    if (l) {
      r = MPI_Finalized(&l);
      if (r != MPI_SUCCESS)
        break;

      l = !l;
    }

    if (l) {
      MPI_Comm_get_errhandler(MPI_COMM_WORLD, &eh);
      MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

      r = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      MPI_Comm_set_errhandler(MPI_COMM_WORLD, eh);

      if (r != MPI_SUCCESS)
        break;
    }
  } while (0);
  if (r != MPI_SUCCESS)
    rank = -1;
#endif
  return rank;
}

/**
 * This function returns NULL if MPI is not enabled.
 *
 * If MPI is enabled, NULL return means allocation error.
 */
static jupiter_strlist *csvperror_mpi_rank_header(int rank)
{
#ifdef JUPITER_MPI
  if (rank >= 0) {
    return jupiter_strlist_asprintf("[%5d]", rank);
  } else {
    return jupiter_strlist_dup_s("[RANK?]");
  }
#endif
  return NULL;
}

static int csvperror_make_message_prefix(jupiter_strlist_head *outp,
                                         const char *fname, long ln, long cl,
                                         const char *value)
{
  jupiter_strlist_head lh;
  int r = 1;
  jupiter_strlist_head_init(&lh);

  do {
    jupiter_strlist *n;

    if (fname) {
      n = csvperror_make_fname_loc(fname, ln, cl);
      if (!n)
        break;

      jupiter_strlist_append(&lh, n);
    }

    if (value) {
      n = jupiter_strlist_asprintf("%s: ", value);
      if (!n)
        break;

      jupiter_strlist_append(&lh, n);
    }

    jupiter_strlist_prepend_list(outp, &lh);
    r = 0;
  } while (0);

  jupiter_strlist_free_all(&lh);
  return r;
}

static jupiter_strlist *
csvperror_make_first_header(csv_error_level errlevel,
                            jupiter_strlist *rank_header)
{
  jupiter_strlist_head lh;
  jupiter_strlist *l = NULL;
  jupiter_strlist_head_init(&lh);

  do {
    jupiter_strlist *n;

    n = jupiter_strlist_asprintf("[%5s]: ", csvperror_elevel_print(errlevel));
    if (!n)
      break;

    jupiter_strlist_prepend(&lh, n);

    if (rank_header) {
      n = jupiter_strlist_dup_l(rank_header);
      if (!n)
        break;

      jupiter_strlist_prepend(&lh, n);
    }

    l = jupiter_strlist_join_all(&lh, NULL);
  } while (0);

  jupiter_strlist_free_all(&lh);
  return l;
}

static jupiter_strlist *csvperror_make_cont_header(jupiter_strlist *rank_header)
{
  jupiter_strlist_head lh;
  jupiter_strlist *l = NULL;
  jupiter_strlist_head_init(&lh);

  do {
    jupiter_strlist *n;

    if (rank_header) {
      n = jupiter_strlist_dup_l(rank_header);
      if (!n)
        break;

      jupiter_strlist_append(&lh, n);
    }

    n = jupiter_strlist_asprintf(" %5s | ", "");
    if (!n)
      break;

    jupiter_strlist_append(&lh, n);

    l = jupiter_strlist_join_all(&lh, "");
  } while (0);

  jupiter_strlist_free_all(&lh);
  return l;
}

void csvperrorl(const char *fname, long ln, long cl, csv_error_level errlevel,
                const char *value, jupiter_strlist_head *message_head)
{
  int r;
  int rank;
  jupiter_strlist_head llr, ll1, llc;

  if (!csv_do_print(errlevel))
    return;

  jupiter_strlist_head_init(&llr);
  jupiter_strlist_head_init(&ll1);
  jupiter_strlist_head_init(&llc);
  r = 0;
  rank = csvperror_mpi_rank();

  do {
    jupiter_strlist *lr, *l1, *lc, *lm;

    if (!message_head)
      break;

    if (jupiter_strlist_is_empty(message_head)) {
      lr = jupiter_strlist_dup_s("(** empty message **)");
      if (!lr)
        break;

      jupiter_strlist_append(message_head, lr);
    }

    if (csvperror_make_message_prefix(message_head, fname, ln, cl, value))
      break;

    lr = csvperror_mpi_rank_header(rank);
#ifdef JUPITER_MPI
    if (!lr)
      break;
#endif
    if (lr)
      jupiter_strlist_append(&llr, lr);

    l1 = csvperror_make_first_header(errlevel, lr);
    if (!l1)
      break;

    jupiter_strlist_append(&ll1, l1);

    lm = jupiter_strlist_join_all(message_head, NULL);
    if (!lm)
      break;

    jupiter_strlist_free_all(message_head);
    jupiter_strlist_append(message_head, lm);

    if (!jupiter_strlist_split_ch(lm, '\n'))
      break;

    {
      jupiter_strlist *lp;
      int first = 1;
      lc = NULL;

      r = 1;
      jupiter_strlist_foreach (lp, message_head) {
        jupiter_strlist *lh;
        if (first) {
          lh = l1;
          first = 0;
        } else {
          if (!lc) {
            lc = csvperror_make_cont_header(lr);
            if (!lc) {
              r = 0;
              break;
            }

            jupiter_strlist_append(&llc, lc);
          }
          lh = lc;
        }
        fprintf(stderr, "%s%s\n", lh->buf, lp->buf);
      }
    }
  } while (0);

  jupiter_strlist_free_all(&llr);
  jupiter_strlist_free_all(&ll1);
  jupiter_strlist_free_all(&llc);

  if (!r) {
    const char *elevel_print = csvperror_elevel_print(errlevel);
#ifdef JUPITER_MPI
    if (rank >= 0) {
      fprintf(stderr,
              "[%5d][%5s]: An error occured, but could not display it\n", rank,
              elevel_print);
    } else {
      fprintf(stderr,
              "[RANK?][%5s]: An error occured, but could not display it\n",
              elevel_print);
    }
#else
    fprintf(stderr, "[%5s]: An error occured, but could not display it\n",
            elevel_print);
#endif
  }
}

void csvperrorv(const char *fname, long ln, long cl, csv_error_level errlevel,
                const char *value, const char *format, va_list ap)
{
  jupiter_strlist_head lhm;
  jupiter_strlist *l;

  if (!csv_do_print(errlevel))
    return;

  jupiter_strlist_head_init(&lhm);
  l = jupiter_strlist_vasprintf(format, ap);
  if (l) {
    jupiter_strlist_append(&lhm, l);
    csvperrorl(fname, ln, cl, errlevel, value, &lhm);
  } else {
    csvperrorl(fname, ln, cl, errlevel, value, NULL);
  }
  jupiter_strlist_free_all(&lhm);
}

void csvperrorf(const char *fname, long l, long c, csv_error_level errlevel,
                const char *value, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  csvperrorv(fname, l, c, errlevel, value, fmt, ap);
  va_end(ap);
}

static void csvperror_get_line_col(csv_column *col, long *l, long *c)
{
  CSVASSERT(l);
  CSVASSERT(c);

  if (col) {
    *l = getCSVTextLineOrigin(col);
    *c = getCSVTextColumnOrigin(col);
  } else {
    *l = 0;
    *c = 0;
  }
}

void csvperrorv_col(const char *fname, csv_column *col,
                    csv_error_level errlevel, const char *fmt, va_list ap)
{
  long l, c;

  if (!csv_do_print(errlevel))
    return;

  csvperror_get_line_col(col, &l, &c);
  csvperrorv(fname, l, c, errlevel, getCSVValue(col), fmt, ap);
}

void csvperrorf_col(const char *fname, csv_column *col,
                    csv_error_level errlevel, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  csvperrorv_col(fname, col, errlevel, fmt, ap);
  va_end(ap);
}

void csvperror_col(const char *fname, csv_column *col, csv_error_level errlevel,
                   csv_error csv_errcode, int syscall_errcode, int ext_errcode,
                   const char *additional_info)
{
  long l, c;

  if (!csv_do_print(errlevel))
    return;

  csvperror_get_line_col(col, &l, &c);
  csvperror(fname, l, c, errlevel, getCSVValue(col), csv_errcode,
            syscall_errcode, ext_errcode, additional_info);
}

void csvperrorv_row(const char *fname, csv_row *row, int index,
                    csv_error_level errlevel, const char *fmt, va_list ap)
{
  csv_column *col;

  if (!csv_do_print(errlevel))
    return;

  col = getColumnOfCSV(row, index);
  csvperrorv_col(fname, col, errlevel, fmt, ap);
}

void csvperrorf_row(const char *fname, csv_row *row, int index,
                    csv_error_level errlevel, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  csvperrorv_row(fname, row, index, errlevel, fmt, ap);
  va_end(ap);
}

void csvperror_row(const char *fname, csv_row *row, int index,
                   csv_error_level errlevel, csv_error csv_errcode,
                   int syscall_errcode, int ext_errcode,
                   const char *additional_info)
{
  csv_column *col;

  if (!csv_do_print(errlevel))
    return;

  col = getColumnOfCSV(row, index);
  csvperror_col(fname, col, errlevel, csv_errcode, syscall_errcode, ext_errcode,
                additional_info);
}

/**
 * @brief convert and format csv_errcode to printable text.
 * @param csv_errcode error code to convert
 * @param additinoal_info additional information to know errors better
 * @return formatted text, NULL if failed.
 *
 * returning pointer must be free with `free()`.
 */
static char *csvstrerror(csv_error csv_errcode, const char *additional_info)
{
  int retv;
  char *rets;

  switch (csv_errcode) {
  case CSV_ERR_SUCC:
    retv = jupiter_asprintf(&rets, "No errors occured");
    break;

  case CSV_ERR_EOF:
    retv = jupiter_asprintf(&rets, "Unexpected end of file");
    break;

  case CSV_ERR_2BIG:
    retv = jupiter_asprintf(&rets, "Too big data found");
    break;

  case CSV_ERR_MIX_QUOTE:
    retv = jupiter_asprintf(&rets, "Quoted and Unquoted data mixed");
    break;

  case CSV_ERR_BLOCK_COMMENT_AFTER_CONT:
    retv =
      jupiter_asprintf(&rets, "Block comment found after continuation mark");
    break;

  case CSV_ERR_DATA_AFTER_CONT:
    retv = jupiter_asprintf(&rets, "Data found after continuation mark");
    break;

  case CSV_ERR_FOPEN:
    retv = jupiter_asprintf(&rets, "Could not open file");
    break;

  case CSV_ERR_NOMEM:
    retv = jupiter_asprintf(&rets, "Cannot allocate memory");
    break;

  case CSV_ERR_CMDLINE_INVAL:
    retv = jupiter_asprintf(&rets, "Invalid argument");
    break;

  case CSV_ERR_CMDLINE_RANGE:
    retv = jupiter_asprintf(&rets, "Invalid argument value");
    break;

  case CSV_ERR_SYS:
    retv = jupiter_asprintf(&rets, "System error");
    break;

  case CSV_ERR_MPI:
    retv = jupiter_asprintf(&rets, "MPI error");
    break;

  case CSV_ERR_GEOMETRY:
    retv = jupiter_asprintf(&rets, "Geometry error");
    break;

  case CSV_ERR_SERIALIZE:
    retv = jupiter_asprintf(&rets, "Serializer error");
    break;

  default:
    retv = jupiter_asprintf(&rets, "Unknown error occured");
    break;
  }
  if (retv < 0) {
    return NULL;
  }
  return rets;
}

/*
 * Pass errno as argument via syscall_errcode (because OpenMP code
 * snippet will be inserted at function start and it may change errno)
 */
void csvperror(const char *fname, long l, long c, csv_error_level errlevel,
               const char *value, csv_error csv_errcode, int syscall_errcode,
               int ext_errcode, const char *additional_info)
{
  if (!csv_do_print(errlevel))
    return;

  switch (csv_errcode) {
  case CSV_ERR_MPI:
#pragma omp critical
  {
#ifdef JUPITER_MPI
    char emsg[MPI_MAX_ERROR_STRING];
    int len;
    MPI_Error_string(ext_errcode, emsg, &len);
    if (additional_info) {
      csvperrorf(fname, l, c, errlevel, value, "%s: %s", additional_info, emsg);
    } else {
      csvperrorf(fname, l, c, errlevel, value, "%s", emsg);
    }
#else
#ifndef NDEBUG
    if (ext_errcode != 0) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG, __func__,
                 "MPI is not enabled, but ext_errcode is non-0 value: %d",
                 ext_errcode);
    }
#endif /* NDEBUG */
#endif /* MPI */
  } break;
  case CSV_ERR_SYS:
  {
    char *p;
    int errno_save;
    errno_save = errno;
    errno = 0;
    p = strerror(syscall_errcode);
    if (errno != 0) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, __func__,
                 "Invalid syscall_errcode passed: %d", syscall_errcode);
    } else {
      if (additional_info) {
        csvperrorf(fname, l, c, errlevel, value, "%s: %s", additional_info, p);
      } else {
        csvperrorf(fname, l, c, errlevel, value, "%s", p);
      }
    }
    errno = errno_save;
  } break;

  case CSV_ERR_GEOMETRY:
  {
    const char *cp;
    cp = geom_strerror(ext_errcode);
    if (additional_info) {
      csvperrorf(fname, l, c, errlevel, value, "%s: %s", cp, additional_info);
    } else {
      csvperrorf(fname, l, c, errlevel, value, "%s", cp);
    }
  } break;

  case CSV_ERR_SERIALIZE:
  {
    const char *cp;
    cp = msgpackx_strerror(ext_errcode);
    if (additional_info) {
      csvperrorf(fname, l, c, errlevel, value, "%s: %s", cp, additional_info);
    } else {
      csvperrorf(fname, l, c, errlevel, value, "%s", cp);
    }
  } break;

  default:
  {
    char *p;
    p = csvstrerror(csv_errcode, additional_info);
    if (p) {
      csvperrorf(fname, l, c, errlevel, value, "%s", p);
      free(p);
    } else {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, __func__,
                 "Could not format error message");
    }
  } break;
  }
}

#if defined(__GNUC__) && !(defined(__PGI) || defined(__NVCOMPILER))
/* GCC, Intel compiler (in Linux) and clang support __builtin_*(). PGI
 * (in Linux) also defines __GNUC__ but __builtin_trap() is not
 * available.
 *
 * The kind of signal will dependent on platform. Usually SIGSEGV or SIGILL.
 */
#define DO_TRAP __builtin_trap()
#else
/*
 * Since abort() is a library function, debugging with abort() is
 * little more complicated. Or even debugger may not stop here.
 */
#define DO_TRAP \
  do {          \
    abort();    \
  } while (1)
#endif

void csvassert_x_impl(const char *file, const char *func, long line,
                      const char *cond_text, const char *message)
{
  csvperrorf(file, line, 0, CSV_EL_FATAL, func,
             message ? "%s: %s: %s" : "%s: %s", "Assertion failed",
             cond_text ? cond_text : "(Unknown condition)", message);

  DO_TRAP;

#ifdef JUPITER_MPI
  MPI_Abort(MPI_COMM_WORLD, 100);
#endif

  exit(EXIT_FAILURE);
}

void csvunreachable_impl(const char *file, const char *func, long line)
{
  csvperrorf(file, line, 0, CSV_EL_FATAL, func, "Unreachable reached");

  DO_TRAP;

#ifdef JUPITER_MPI
  MPI_Abort(MPI_COMM_WORLD, 100);
#endif

  exit(EXIT_FAILURE);
}

static int set_p_last_error;
#ifdef _OPENMP
#pragma omp threadprivate(set_p_last_error)
#endif

int set_p_last_error_value(void) { return set_p_last_error; }

void set_p_set_last_error_value(int val) { set_p_last_error = val; }

/**
 * @brief Cell finder for SET_P* macros
 * @param fname file name of CSV
 * @param csv CSV data
 * @param keystr Keystring to find
 * @param index absolute or relative (if keystr is NULL) column index value
 * @param found_row Set to found row data.
 * @param found_col Set to found col data.
 * @param perror if non-zero value, print the error.
 * @param pelvl Error level to be used when print error.
 * @return 0 if success, otherwise failed (not found).
 *
 * This is internal function for SET_P* macros.
 *
 * If keystr is given, find the row which first column is specified keystr,
 * and find the column where specified index value.
 *
 * If keystr is not given, offset specified found_col by value of
 * index, and make it to new found_col. Here, `found_row` is used
 * during print error message, and so you should not change csv and
 * found_row while travarsing with relative moves, unless you know what
 * you are doing.
 *
 * If any errors occured and perror is not 0, prints error message
 * while finding process.
 *
 * Do not call this function directly.
 */
static int set_p_find_internal(const char *fname, csv_data *csv,
                               const char *keystr, int index,
                               csv_row **found_row, csv_column **found_col,
                               int perror, csv_error_level pelvl)
{
  csv_row *row_found;
  csv_column *col_found;
  int i;
  int rval;

  CSVASSERT(found_row);
  CSVASSERT(found_col);

  if (!csv) {
    *found_row = NULL;
    *found_col = NULL;
    return 1;
  }

  row_found = NULL;
  col_found = NULL;
  rval = 0;

  if (keystr) {
    row_found = findCSVRow(csv, keystr, strlen(keystr) + 1);
    if (row_found) {
      col_found = getColumnOfCSV(row_found, index);
      if (!col_found) {
        rval = 1;
        if (perror) {
          if (pelvl >= CSV_EL_ERROR) {
            csvperrorf_row(fname, row_found, 0, pelvl,
                           "Required data not found at CSV column %d", index);
          } else {
            csvperrorf_row(fname, row_found, 0, pelvl,
                           "Data not found at CSV column %d, using default",
                           index);
          }
        }
      }
    } else {
      rval = 1;
      if (perror) {
        if (pelvl >= CSV_EL_ERROR) {
          csvperrorf(fname, 0, 0, pelvl, keystr, "Required key not found");
        } else {
          csvperrorf(fname, 0, 0, pelvl, keystr,
                     "Key not found, using default");
        }
      }
    }

  } else {
    csv_column *c;
    c = *found_col;

    /* last valid column while tracing row. */
    if (c) {
      if (index > 0) {
        for (i = 0; i < index; ++i) {
          c = getNextColumn(c);
          if (!c)
            break;
        }
      } else if (index < 0) {
        for (i = 0; i > index; --i) {
          c = getPrevColumn(c);
          if (!c)
            break;
        }
      }
    }

    if (!c) {
      rval = 1;
      if (perror) {
        long llc;
        const char *keyname, *lcv;
        csv_column *lc, *c0;
        lc = NULL;
        c0 = NULL;
        llc = 0;
        keyname = "<error>";
        lcv = "<error>";

        if (*found_row) {
          c0 = getColumnOfCSV(*found_row, 0);
          lc = getColumnOfCSV(*found_row, -1);
        }
        if (c0) {
          llc = getCSVTextLineOrigin(c0);
          keyname = getCSVValue(c0);
        }
        if (lc) {
          llc = getCSVTextLineOrigin(lc);
          lcv = getCSVValue(lc);
        }
        if (c0) {
          if (pelvl >= CSV_EL_ERROR) {
            csvperrorf(fname, llc, 0, pelvl, keyname,
                       "Required data not found at columns right of \"%s\"",
                       lcv);
          } else {
            csvperrorf(fname, llc, 0, pelvl, keyname,
                       "Data not found at columns right of \"%s\", using "
                       "default",
                       lcv);
          }
        }
      }
    }

    row_found = *found_row;
    col_found = c;
  }

  *found_col = col_found;
  *found_row = row_found;
  return rval;
}

int set_p_internal_base(void *dest, const void *value_on_fail,
                        set_p_setter_func_type *setter, const char *type_name,
                        const char *keystr, int index, int perror_not_found,
                        int perror_invalid, csv_error_level errlevel_not_found,
                        csv_error_level errlevel_invalid, const char *fname,
                        csv_data *csv, csv_row **found_row,
                        csv_column **found_col, int *stat)
{
  int fnderr;
  int converr;

  CSVASSERT(found_col);
  CSVASSERT(found_row);

  fnderr = set_p_find_internal(fname, csv, keystr, index, found_row, found_col,
                               perror_not_found, errlevel_not_found);
  converr = setter(dest, value_on_fail, fname, *found_col, errlevel_invalid,
                   perror_invalid);
  if (fnderr || converr) {
    if (fnderr)
      converr = fnderr;
    if (stat)
      *stat = ON;
  }
  set_p_set_last_error_value(converr);
  return converr;
}

static void set_p_perror_internalv(const char *fname, csv_column *found_col,
                                   csv_error_level pelvl, const char *format,
                                   va_list ap)
{
  if (set_p_last_error_value() == 0 && found_col) {
    csvperrorv_col(fname, found_col, pelvl, format, ap);
  }
}

void set_p_perror_internal(const char *fname, csv_column *found_col,
                           csv_error_level pelvl, const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  set_p_perror_internalv(fname, found_col, pelvl, format, ap);
  va_end(ap);
}

int set_p_perror_range_internal(const char *fname, csv_column *found_col,
                                double val, double min, double max,
                                int min_incl, int max_incl,
                                csv_error_level pelvl, const char *format, ...)
{
  va_list ap;
  int accept;

  accept = !isnan(val);
  if (min_incl == ON) {
    accept = (accept && (val >= min));
  } else {
    accept = (accept && (val > min));
  }
  if (max_incl == ON) {
    accept = (accept && (val <= max));
  } else {
    accept = (accept && (val < max));
  }
  if (!accept) {
    va_start(ap, format);
    set_p_perror_internalv(fname, found_col, pelvl, format, ap);
    va_end(ap);
  }
  return accept;
}
