#include "write_fv_csv.h"
#include "csvparser.h"
#include "data_array.h"
#include "defs.h"
#include "error.h"
#include "executive.h"
#include "executive_data.h"
#include "field_variable.h"
#include "information.h"
#include "input.h"
#include "output.h"
#include "overflow.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "static_array.h"
#include "string_array.h"

#include <math.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Uses first input port as (time) interval input.
 */
struct jcntrl_write_fv_csv
{
  jcntrl_executive exec;
  jcntrl_write_fv_csv_step_mode stepmode;
  int step_interval;
  double time_interval;
  jcntrl_char_array *filename;
  int use_scientific;
  int precision;
  int last_step;
  int header_rows;
  double last_time;
  int write_header;
};
#define jcntrl_write_fv_csv__ancestor jcntrl_executive
#define jcntrl_write_fv_csv__dnmem exec.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(jcntrl_write_fv_csv);

static jcntrl_write_fv_csv *
jcntrl_write_fv_csv_downcast_impl(jcntrl_shared_object *object)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_write_fv_csv, object);
}

static void *jcntrl_write_fv_csv_downcast_v(jcntrl_shared_object *object)
{
  return jcntrl_write_fv_csv_downcast_impl(object);
}

static int jcntrl_write_fv_csv_initializer(jcntrl_shared_object *object)
{
  jcntrl_executive *exe;
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_write_fv_csv *p = jcntrl_write_fv_csv_downcast_impl(object);

  p->filename = jcntrl_char_array_new();
  if (!p->filename)
    return 0;

  p->stepmode = JCNTRL_WRITE_FV_OUTPUT_BY_NSTEP;
  p->step_interval = 1;
  p->time_interval = 0.0;
  p->use_scientific = 0;
  p->precision = 6;
  p->header_rows = 1;
  p->last_step = -1;
  p->last_time = -HUGE_VAL;
  p->write_header = 1;

  exe = jcntrl_write_fv_csv_executive(p);
  input = jcntrl_executive_get_input(exe);
  output = jcntrl_executive_get_output(exe);

  if (!jcntrl_input_set_number_of_ports(input, 1))
    return 0;
  if (!jcntrl_output_set_number_of_ports(output, 0))
    return 0;
  return 1;
}

static void jcntrl_write_fv_csv_destructor(jcntrl_shared_object *object)
{
  jcntrl_write_fv_csv *p = jcntrl_write_fv_csv_downcast_impl(object);

  if (p->filename)
    jcntrl_char_array_delete(p->filename);
}

static jcntrl_shared_object *jcntrl_write_fv_csv_allocator(void)
{
  jcntrl_write_fv_csv *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_write_fv_csv);
  return p ? jcntrl_write_fv_csv_object(p) : NULL;
}

static void jcntrl_write_fv_csv_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int
jcntrl_write_fv_csv_fill_input_port_information_impl(jcntrl_shared_object *obj,
                                                     int index,
                                                     jcntrl_input *port)
{
  jcntrl_information *info;
  info = jcntrl_input_information(port);
  if (!jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                       JCNTRL_DATATYPE_FIELD_VAR))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_write_fv_csv, jcntrl_executive,
                    fill_input_port_information)

static void jcntrl_write_fv_csv_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_write_fv_csv_downcast_v;
  p->initializer = jcntrl_write_fv_csv_initializer;
  p->destructor = jcntrl_write_fv_csv_destructor;
  p->allocator = jcntrl_write_fv_csv_allocator;
  p->deleter = jcntrl_write_fv_csv_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_write_fv_csv, jcntrl_executive,
                          fill_input_port_information);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_write_fv_csv,
                                   jcntrl_write_fv_csv_init_func)

jcntrl_write_fv_csv *jcntrl_write_fv_csv_new(void)
{
  return jcntrl_shared_object_new(jcntrl_write_fv_csv);
}

void jcntrl_write_fv_csv_delete(jcntrl_write_fv_csv *p)
{
  jcntrl_shared_object_delete(jcntrl_write_fv_csv_object(p));
}

jcntrl_shared_object *jcntrl_write_fv_csv_object(jcntrl_write_fv_csv *p)
{
  return jcntrl_executive_object(jcntrl_write_fv_csv_executive(p));
}

jcntrl_executive *jcntrl_write_fv_csv_executive(jcntrl_write_fv_csv *p)
{
  return &p->exec;
}

jcntrl_write_fv_csv *jcntrl_write_fv_csv_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_write_fv_csv, obj);
}

int jcntrl_write_fv_csv_set_output_file(jcntrl_write_fv_csv *p,
                                        jcntrl_data_array *name)
{
  jcntrl_size_type len, lensz;
  const char *namestr;
  char *nstr;

  len = jcntrl_data_array_get_ntuple(name);
  if (len <= 0) {
    jcntrl_raise_argument_error(__FILE__, __LINE__, "Output filename is empty");
    return 0;
  }

  namestr = jcntrl_data_array_get_char(name);
  JCNTRL_ASSERT(namestr);
  if (!namestr) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Given array is not string (char array)");
    return 0;
  }

  lensz = len;
  if (namestr[len - 1] != '\0') {
    if (jcntrl_s_add_overflow(len, 1, &lensz))
      return 0;
  }

  if (!jcntrl_char_array_resize(p->filename, lensz))
    return 0;

  nstr = jcntrl_char_array_get_writable(p->filename);
  JCNTRL_ASSERT(nstr);

  nstr[lensz - 1] = '\0';

  if (!jcntrl_char_array_copy(p->filename, name, len, 0, 0))
    return 0;
  return 1;
}

int jcntrl_write_fv_csv_set_output_file_c(jcntrl_write_fv_csv *p,
                                          const char *name,
                                          jcntrl_size_type namelen)
{
  jcntrl_data_array *d;
  jcntrl_static_cstr_array cstr;
  jcntrl_static_cstr_array_init_base(&cstr, name, namelen);

  d = jcntrl_static_cstr_array_data(&cstr);
  return jcntrl_write_fv_csv_set_output_file(p, d);
}

const char *jcntrl_write_fv_csv_get_output_file(jcntrl_write_fv_csv *p,
                                                jcntrl_size_type *namelen)
{
  if (namelen)
    *namelen = jcntrl_char_array_get_ntuple(p->filename);
  return jcntrl_char_array_get(p->filename);
}

void jcntrl_write_fv_csv_set_step_mode(jcntrl_write_fv_csv *p,
                                       jcntrl_write_fv_csv_step_mode mode)
{
  p->stepmode = mode;
}

jcntrl_write_fv_csv_step_mode
jcntrl_write_fv_csv_get_step_mode(jcntrl_write_fv_csv *p)
{
  return p->stepmode;
}

void jcntrl_write_fv_csv_set_step_interval(jcntrl_write_fv_csv *p, int step)
{
  p->step_interval = step;
}

int jcntrl_write_fv_csv_get_step_interval(jcntrl_write_fv_csv *p)
{
  return p->step_interval;
}

void jcntrl_write_fv_csv_set_const_time_interval(jcntrl_write_fv_csv *p,
                                                 double time)
{
  p->time_interval = time;
}

static jcntrl_input *
jcntrl_write_fv_csv_get_time_interval_port(jcntrl_write_fv_csv *p)
{
  return jcntrl_executive_input_port(jcntrl_write_fv_csv_executive(p), 0);
}

int jcntrl_write_fv_csv_set_fv_time_interval(jcntrl_write_fv_csv *p,
                                             jcntrl_output *port)
{
  jcntrl_input *input;
  input = jcntrl_write_fv_csv_get_time_interval_port(p);
  JCNTRL_ASSERT(input);

  if (port) {
    return jcntrl_input_connect(input, port);
  } else {
    return jcntrl_input_disconnect(input);
  }
}

int jcntrl_write_fv_csv_set_fv_time_interval_exec(jcntrl_write_fv_csv *p,
                                                  jcntrl_executive *producer,
                                                  int port)
{
  jcntrl_output *output;

  output = NULL;
  if (producer) {
    output = jcntrl_executive_output_port(producer, port);
    if (!output) {
      jcntrl_raise_argument_error(__FILE__, __LINE__,
                                  "Given port does not exist");
      return 0;
    }
  }

  return jcntrl_write_fv_csv_set_fv_time_interval(p, output);
}

static double
jcntrl_write_fv_csv_get_write_time_interval_f(jcntrl_write_fv_csv *p,
                                              int update)
{
  jcntrl_information *info;
  jcntrl_input *input;
  jcntrl_shared_object *obj;
  jcntrl_field_variable *var;
  jcntrl_data_array *d;

  input = jcntrl_write_fv_csv_get_time_interval_port(p);
  if (!jcntrl_input_is_connected(input))
    return p->time_interval;

  if (update) {
    jcntrl_executive *exe;

    exe = jcntrl_input_upstream_executive(input);
    if (!exe)
      return -1.0;

    if (!jcntrl_executive_update_data(exe))
      return -1.0;
  }

  info = jcntrl_input_upstream_information(input);
  obj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  if (!obj)
    return -1.0;

  var = jcntrl_field_variable_downcast(obj);
  if (!var)
    return -1.0;

  d = jcntrl_field_variable_array(var);
  if (!d)
    return -1.0;

  return jcntrl_data_array_get_value(d, 0);
}

double jcntrl_write_fv_csv_get_write_time_interval(jcntrl_write_fv_csv *p)
{
  return jcntrl_write_fv_csv_get_write_time_interval_f(p, 1);
}

void jcntrl_write_fv_csv_set_precision(jcntrl_write_fv_csv *p, int precision)
{
  p->precision = precision;
}

int jcntrl_write_fv_csv_get_precision(jcntrl_write_fv_csv *p)
{
  return p->precision;
}

void jcntrl_write_fv_csv_set_use_scientific(jcntrl_write_fv_csv *p,
                                            int use_scientific)
{
  p->use_scientific = !!use_scientific;
}

int jcntrl_write_fv_csv_get_use_scientific(jcntrl_write_fv_csv *p)
{
  return p->use_scientific;
}

/**
 * If the number of header rows is 1, writes first value only.
 * If the number of header rows is 2, writes all values with indices.
 */
void jcntrl_write_fv_csv_set_number_of_header_rows(jcntrl_write_fv_csv *p,
                                                   int nrows)
{
  p->header_rows = nrows;
}

int jcntrl_write_fv_csv_get_number_of_header_rows(jcntrl_write_fv_csv *p)
{
  return p->header_rows;
}

int jcntrl_write_fv_csv_set_number_of_inputs(jcntrl_write_fv_csv *p, int ninput)
{
  jcntrl_input *input;
  JCNTRL_ASSERT(ninput >= 0);
  if (jcntrl_i_add_overflow(ninput, 1, &ninput))
    return 0;

  input = jcntrl_executive_get_input(jcntrl_write_fv_csv_executive(p));
  return !!jcntrl_input_set_number_of_ports(input, ninput);
}

int jcntrl_write_fv_csv_set_input(jcntrl_write_fv_csv *p, int index,
                                  jcntrl_output *port)
{
  jcntrl_input *input;

  input = jcntrl_write_fv_csv_get_input(p, index);
  if (!input)
    return 0;

  if (port) {
    return jcntrl_input_disconnect(input);
  } else {
    return jcntrl_input_connect(input, port);
  }
}

int jcntrl_write_fv_csv_set_input_exec(jcntrl_write_fv_csv *p, int index,
                                       jcntrl_executive *producer, int port)
{
  jcntrl_output *output;

  output = NULL;
  if (producer) {
    output = jcntrl_executive_output_port(producer, port);
    if (!output) {
      jcntrl_raise_argument_error(__FILE__, __LINE__,
                                  "Given output port does not exist");
      return 1;
    }
  }

  return jcntrl_write_fv_csv_set_input(p, index, output);
}

jcntrl_input *jcntrl_write_fv_csv_get_input(jcntrl_write_fv_csv *p, int index)
{
  JCNTRL_ASSERT(index >= 0);
  if (jcntrl_i_add_overflow(index, 1, &index))
    return NULL;

  return jcntrl_executive_input_port(jcntrl_write_fv_csv_executive(p), index);
}

static const char *jcntrl_write_fv_csv_name(jcntrl_input *inp)
{
  jcntrl_executive *upexe;
  upexe = jcntrl_input_upstream_executive(inp);
  if (!upexe)
    return NULL;
  return jcntrl_executive_get_name(upexe);
}

static jcntrl_data_array *jcntrl_write_fv_csv_get_ary(jcntrl_input *inp)
{
  jcntrl_information *info;
  jcntrl_shared_object *upd;
  jcntrl_field_variable *upfv;

  info = jcntrl_input_upstream_information(inp);
  JCNTRL_ASSERT(info);
  upd = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  JCNTRL_ASSERT(upd);
  upfv = jcntrl_field_variable_downcast(upd);
  JCNTRL_ASSERT(upfv);
  return jcntrl_field_variable_array(upfv);
}

static jcntrl_size_type jcntrl_write_fv_csv_each_inputs(
  jcntrl_write_fv_csv *p,
  int (*func)(jcntrl_write_fv_csv *p, jcntrl_data_array *data, const char *name,
              jcntrl_size_type cstart, jcntrl_size_type cend, void *arg),
  void *arg)
{
  jcntrl_executive *exe;
  jcntrl_input *inp;
  jcntrl_size_type i, t;

  exe = jcntrl_write_fv_csv_executive(p);
  inp = jcntrl_executive_get_input(exe);
  inp = jcntrl_input_next_port(inp);
  if (!inp)
    return 0;

  i = 0;
  while ((inp = jcntrl_input_next_port(inp))) {
    jcntrl_data_array *d;
    jcntrl_size_type n1;
    const char *name;
    name = jcntrl_write_fv_csv_name(inp);
    if (!name)
      return -1;

    d = jcntrl_write_fv_csv_get_ary(inp);

    if (p->header_rows == 2) {
      t = 0;
      if (d) {
        t = jcntrl_data_array_get_ntuple(d);
      }
    } else {
      t = (d && jcntrl_data_array_get_ntuple(d) > 0) ? 1 : 0;
    }
    if (t == 0)
      continue;
    if (jcntrl_s_add_overflow(i, t, &t))
      return -1;

    if (!func(p, d, name, i, t, arg))
      return -1;

    i = t;
  }
  return i;
}

struct jcntrl_write_fv_csv_comp_header_data
{
  const char *fn;
  jcntrl_string_array *file_header;
};

static int jcntrl_write_fv_csv_comp_header_impl(
  jcntrl_write_fv_csv *p, jcntrl_data_array *data, const char *name,
  jcntrl_size_type cstart, jcntrl_size_type cend, void *arg)
{
  jcntrl_size_type n1;
  jcntrl_char_array *fhs;
  jcntrl_size_type nt;

  struct jcntrl_write_fv_csv_comp_header_data *a;
  a = (struct jcntrl_write_fv_csv_comp_header_data *)arg;

  n1 = strlen(name);
  nt = jcntrl_string_array_get_ntuple(a->file_header);

  for (jcntrl_size_type i = cstart; i < cend; ++i) {
    const char *fhstr;
    jcntrl_size_type n2;
    if (i >= nt) {
      jcntrl_raise_csv_header_error(a->fn, name, NULL, i);
      return 0;
    }
    fhs = jcntrl_string_array_get(a->file_header, i);
    if (!fhs) {
      jcntrl_raise_csv_header_error(a->fn, name, NULL, i);
      return 0;
    }

    n2 = jcntrl_char_array_get_ntuple(fhs);
    if (n1 != n2) {
      jcntrl_data_array *dd = jcntrl_char_array_data(fhs);
      jcntrl_raise_csv_header_error(a->fn, name, dd, i);
      return 0;
    }

    fhstr = jcntrl_char_array_get(fhs);
    if (strncmp(name, fhstr, n2) != 0) {
      jcntrl_data_array *dd = jcntrl_char_array_data(fhs);
      jcntrl_raise_csv_header_error(a->fn, name, dd, i);
      return 0;
    }
  }
  return 1;
}

static int jcntrl_write_fv_csv_comp_header(jcntrl_write_fv_csv *p,
                                           const char *fn,
                                           jcntrl_string_array *file_header)
{
  jcntrl_size_type r;
  struct jcntrl_write_fv_csv_comp_header_data arg = {
    .fn = fn,
    .file_header = file_header,
  };

  r = jcntrl_write_fv_csv_each_inputs(p, jcntrl_write_fv_csv_comp_header_impl,
                                      &arg);
  return r >= 0;
}

static int jcntrl_write_fv_csv_check_header(jcntrl_write_fv_csv *p,
                                            const char *fname, FILE *fp)
{
  int r;
  jcntrl_string_array *file_header;
  jcntrl_csvparser *psr;

  psr = NULL;
  r = 1;

  p->write_header = 1;
  do {
    if (!r)
      break;

    psr = jcntrl_csvparser_new();
    if (!psr) {
      r = 0;
      break;
    }

    jcntrl_csvparser_set_input_file(psr, fname);
    jcntrl_csvparser_set_stream(psr, fp);
    file_header = jcntrl_csvparser_read_row(psr);

    /* Empty file */
    if (jcntrl_string_array_get_ntuple(file_header) == 0)
      break;

    if (!jcntrl_write_fv_csv_comp_header(p, fname, file_header)) {
      r = 0;
      break;
    }

    p->write_header = 0;
  } while (0);

  if (psr)
    jcntrl_csvparser_delete(psr);
  return r;
}

struct jcntrl_write_fv_csv_write_data
{
  const char *fname;
  FILE *fp;
};

static int jcntrl_write_fv_csv_write_header_cell(
  jcntrl_write_fv_csv *p, jcntrl_data_array *data, const char *name,
  jcntrl_size_type cstart, jcntrl_size_type cend, void *arg)
{
  struct jcntrl_write_fv_csv_write_data *a;
  jcntrl_size_type n1;

  a = (struct jcntrl_write_fv_csv_write_data *)arg;
  n1 = strlen(name);

  for (; cstart < cend; ++cstart) {
    if (cstart != 0)
      fputc(',', a->fp);
    fputc('"', a->fp);
    for (jcntrl_size_type i = 0; i < n1; ++i) {
      fputc(name[i], a->fp);
      if (name[i] == '"')
        fputc(name[i], a->fp);
    }
    fputc('"', a->fp);
  }

  return 1;
}

static int jcntrl_write_fv_csv_write_index_cell(
  jcntrl_write_fv_csv *p, jcntrl_data_array *data, const char *name,
  jcntrl_size_type cstart, jcntrl_size_type cend, void *arg)
{
  struct jcntrl_write_fv_csv_write_data *a;
  a = (struct jcntrl_write_fv_csv_write_data *)arg;

  for (jcntrl_size_type i = 0; cstart < cend; ++i, ++cstart) {
    if (cstart != 0)
      fputc(',', a->fp);
    fprintf(a->fp, "%" PRIdMAX "", (intmax_t)i);
  }
  return 1;
}

static int jcntrl_write_fv_csv_write_header(jcntrl_write_fv_csv *p,
                                            const char *fname, FILE *fp)
{
  int r;
  struct jcntrl_write_fv_csv_write_data arg = {
    .fname = fname,
    .fp = fp,
  };
  r = jcntrl_write_fv_csv_each_inputs(p, jcntrl_write_fv_csv_write_header_cell,
                                      &arg);
  if (!r)
    return 0;
  fputc('\n', fp);

  if (p->header_rows == 2) {
   r = jcntrl_write_fv_csv_each_inputs(p, jcntrl_write_fv_csv_write_index_cell,
                                       &arg);
   if (!r)
     return 0;
   fputc('\n', fp);
  }

  return r;
}

static int jcntrl_write_fv_csv_write_data_cell(
  jcntrl_write_fv_csv *p, jcntrl_data_array *data, const char *name,
  jcntrl_size_type cstart, jcntrl_size_type cend, void *arg)
{
  struct jcntrl_write_fv_csv_write_data *a;
  a = (struct jcntrl_write_fv_csv_write_data *)arg;

  for (jcntrl_size_type i = 0; cstart < cend; ++i, ++cstart) {
    double value;

    if (cstart != 0)
      fputc(',', a->fp);

    value = jcntrl_data_array_get_value(data, i);
    if (p->use_scientific) {
      if (p->precision > 0) {
        fprintf(a->fp, "%.*e", p->precision, value);
      } else {
        fprintf(a->fp, "%e", value);
      }
    } else {
      if (p->precision > 0) {
        fprintf(a->fp, "%.*g", p->precision, value);
      } else {
        fprintf(a->fp, "%g", value);
      }
    }
  }
  return 1;
}

static int jcntrl_write_fv_csv_write_data(jcntrl_write_fv_csv *p,
                                          const char *fname, FILE *fp)
{
  struct jcntrl_write_fv_csv_write_data arg = {
    .fname = fname,
    .fp = fp,
  };
  if (!jcntrl_write_fv_csv_each_inputs(p, jcntrl_write_fv_csv_write_data_cell,
                                       &arg))
    return 0;

  fputc('\n', fp);
  return 1;
}

static int jcntrl_write_fv_csv_write_row(jcntrl_write_fv_csv *p,
                                         const char *fname, FILE *fp)
{
  if (p->write_header) {
    if (!jcntrl_write_fv_csv_write_header(p, fname, fp))
      return 0;
  }
  return jcntrl_write_fv_csv_write_data(p, fname, fp);
}

int jcntrl_write_fv_csv_write(jcntrl_write_fv_csv *p, int current_step,
                              double current_time)
{
  double time;
  const char *fname;
  FILE *fp;
  int update_file;
  int r;

  update_file = 0;
  switch (p->stepmode) {
  case JCNTRL_WRITE_FV_OUTPUT_BY_NSTEP:
    if (p->step_interval < 0)
      return 0;
    if (p->step_interval > 0 && current_step < p->last_step + p->step_interval)
      return 1;
    update_file = 1;
    break;

  case JCNTRL_WRITE_FV_OUTPUT_BY_TIME:
    time = jcntrl_write_fv_csv_get_write_time_interval(p);
    if (time < 0.0)
      return 0;
    if (time >= 0.0 && current_time < p->last_time + time)
      return 1;
    update_file = 1;
    break;

  case JCNTRL_WRITE_FV_OUTPUT_BY_NONE:
    return 1;
  }
  JCNTRL_ASSERT(update_file);
  if (!update_file)
    return 0;

  p->last_step = current_step;
  p->last_time = current_time;

  fname = jcntrl_char_array_get(p->filename);
  if (!fname) {
    jcntrl_raise_argument_error(__FILE__, __LINE__, "Output filename is empty");
    return 0;
  }

  errno = 0;
  fp = fopen(fname, "r+b");
  if (!fp) {
#ifdef ENOENT
    if (errno != 0 && errno != ENOENT) {
      jcntrl_raise_errno_error(fname, 0, errno, "Failed to open output file");
      return 0;
    }
#endif
  }

  if (fp) {
    if (!jcntrl_write_fv_csv_check_header(p, fname, fp)) {
      fclose(fp);
      return 0;
    }

    if (fseek(fp, -1, SEEK_END) != 0) {
      fclose(fp);
      jcntrl_raise_errno_error(fname, 0, errno, "Does not support seek");
      return 0;
    }
    if (fgetc(fp) != '\n') {
      fputc('\n', fp);
    }
  }

  if (!fp) {
    fp = fopen(fname, "wb");
    if (!fp) {
      jcntrl_raise_errno_error(fname, 0, errno, "Failed to open output file");
      return 0;
    }
  }

  r = jcntrl_write_fv_csv_write_row(p, fname, fp);
  fclose(fp);

  if (r) {
    p->last_step = current_step;
    p->last_time = current_time;
  }

  return r;
}
