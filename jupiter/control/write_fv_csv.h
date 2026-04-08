#ifndef JUPITER_CONTROL_WRITE_FV_CSV_H
#define JUPITER_CONTROL_WRITE_FV_CSV_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

enum jcntrl_write_fv_csv_step_mode
{
  JCNTRL_WRITE_FV_OUTPUT_BY_NONE,
  JCNTRL_WRITE_FV_OUTPUT_BY_NSTEP,
  JCNTRL_WRITE_FV_OUTPUT_BY_TIME,
};
typedef enum jcntrl_write_fv_csv_step_mode jcntrl_write_fv_csv_step_mode;

struct jcntrl_write_fv_csv;
typedef struct jcntrl_write_fv_csv jcntrl_write_fv_csv;

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_write_fv_csv);

JUPITER_CONTROL_DECL
jcntrl_write_fv_csv *jcntrl_write_fv_csv_new(void);
JUPITER_CONTROL_DECL
void jcntrl_write_fv_csv_delete(jcntrl_write_fv_csv *p);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_write_fv_csv_object(jcntrl_write_fv_csv *p);
JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_write_fv_csv_executive(jcntrl_write_fv_csv *p);
JUPITER_CONTROL_DECL
jcntrl_write_fv_csv *jcntrl_write_fv_csv_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
int jcntrl_write_fv_csv_set_output_file(jcntrl_write_fv_csv *p,
                                        jcntrl_data_array *name);

JUPITER_CONTROL_DECL
int jcntrl_write_fv_csv_set_output_file_c(jcntrl_write_fv_csv *p,
                                          const char *name,
                                          jcntrl_size_type namelen);

JUPITER_CONTROL_DECL
const char *jcntrl_write_fv_csv_get_output_file(jcntrl_write_fv_csv *p,
                                                jcntrl_size_type *namelen);

JUPITER_CONTROL_DECL
void jcntrl_write_fv_csv_set_step_mode(jcntrl_write_fv_csv *p,
                                       jcntrl_write_fv_csv_step_mode mode);
JUPITER_CONTROL_DECL
jcntrl_write_fv_csv_step_mode
jcntrl_write_fv_csv_get_step_mode(jcntrl_write_fv_csv *p);

JUPITER_CONTROL_DECL
void jcntrl_write_fv_csv_set_step_interval(jcntrl_write_fv_csv *p, int step);
JUPITER_CONTROL_DECL
int jcntrl_write_fv_csv_get_step_interval(jcntrl_write_fv_csv *p);

JUPITER_CONTROL_DECL
void jcntrl_write_fv_csv_set_const_time_interval(jcntrl_write_fv_csv *p,
                                                 double time);
JUPITER_CONTROL_DECL
int jcntrl_write_fv_csv_set_fv_time_interval(jcntrl_write_fv_csv *p,
                                             jcntrl_output *port);
JUPITER_CONTROL_DECL
int jcntrl_write_fv_csv_set_fv_time_interval_exec(jcntrl_write_fv_csv *p,
                                                  jcntrl_executive *producer,
                                                  int port);
JUPITER_CONTROL_DECL
double jcntrl_write_fv_csv_get_write_time_interval(jcntrl_write_fv_csv *p);

JUPITER_CONTROL_DECL
void jcntrl_write_fv_csv_set_precision(jcntrl_write_fv_csv *p, int precision);
JUPITER_CONTROL_DECL
int jcntrl_write_fv_csv_get_precision(jcntrl_write_fv_csv *p);

JUPITER_CONTROL_DECL
void jcntrl_write_fv_csv_set_use_scientific(jcntrl_write_fv_csv *p,
                                            int use_scientific);
JUPITER_CONTROL_DECL
int jcntrl_write_fv_csv_get_use_scientific(jcntrl_write_fv_csv *p);

/**
 * If the number of header rows is 1, writes first value only.
 * If the number of header rows is 2, writes all values with indices.
 */
JUPITER_CONTROL_DECL
void jcntrl_write_fv_csv_set_number_of_header_rows(jcntrl_write_fv_csv *p,
                                                   int nrows);
JUPITER_CONTROL_DECL
int jcntrl_write_fv_csv_get_number_of_header_rows(jcntrl_write_fv_csv *p);

JUPITER_CONTROL_DECL
int jcntrl_write_fv_csv_set_number_of_inputs(jcntrl_write_fv_csv *p,
                                             int ninput);
JUPITER_CONTROL_DECL
int jcntrl_write_fv_csv_set_input(jcntrl_write_fv_csv *p, int index,
                                  jcntrl_output *port);
JUPITER_CONTROL_DECL
int jcntrl_write_fv_csv_set_input_exec(jcntrl_write_fv_csv *p, int index,
                                       jcntrl_executive *producer, int port);
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_write_fv_csv_get_input(jcntrl_write_fv_csv *p, int index);

JUPITER_CONTROL_DECL
int jcntrl_write_fv_csv_write(jcntrl_write_fv_csv *p, int current_step,
                              double current_time);

JUPITER_CONTROL_DECL_END

#endif
