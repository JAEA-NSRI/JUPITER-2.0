#include "print_param_vecmat.h"
#include "geometry/mat43.h"
#include "geometry/svector.h"
#include "geometry/vector.h"
#include "print_param_core.h"

#include <string.h>

const char *PP_geom_vec2_value_format(void *a)
{
  int r;
  struct PP_geom_vec2_value_data *d;
  d = (struct PP_geom_vec2_value_data *)a;

  PP_geom_vec2_value_data_delete(a);

  r = geom_vec2_to_str(&d->buf, d->value, "%.6g", -1, -1);
  if (r < 0)
    d->buf = NULL;
  return d->buf;
}

const char *PP_geom_vec2_value_null_format(void *a) { return "(error)"; }

void PP_geom_vec2_value_data_delete(void *a)
{
  struct PP_geom_vec2_value_data *d;
  d = (struct PP_geom_vec2_value_data *)a;

  free(d->buf);
  d->buf = NULL;
}

int PP_geom_vec2_value_custom_okfunc(void *a)
{
  struct PP_geom_vec2_value_data *d;
  d = (struct PP_geom_vec2_value_data *)a;

  if (d->okfunc)
    return d->okfunc(d->value, d->arg);
  return 1;
}

const char *PP_geom_vec3_value_format(void *a)
{
  int r;
  struct PP_geom_vec3_value_data *d;
  d = (struct PP_geom_vec3_value_data *)a;

  PP_geom_vec3_value_data_delete(a);

  r = geom_vec3_to_str(&d->buf, d->value, "%.6g", -1, -1);
  if (r <= 0)
    d->buf = NULL;
  return d->buf;
}

const char *PP_geom_vec3_value_null_format(void *a) { return "(error)"; }

void PP_geom_vec3_value_data_delete(void *a)
{
  struct PP_geom_vec3_value_data *d;
  d = (struct PP_geom_vec3_value_data *)a;

  free(d->buf);
  d->buf = NULL;
}

int PP_geom_vec3_value_custom_okfunc(void *a)
{
  struct PP_geom_vec3_value_data *d;
  d = (struct PP_geom_vec3_value_data *)a;

  if (d->okfunc)
    return d->okfunc(d->value, d->arg);
  return 1;
}

const char *PP_geom_svec3_value_format(void *a)
{
  int r;
  struct PP_geom_svec3_value_data *d;
  d = (struct PP_geom_svec3_value_data *)a;

  PP_geom_svec3_value_data_delete(a);

  r = geom_svec3_to_str(&d->buf, d->value, -1, -1);
  if (r <= 0)
    d->buf = NULL;
  return d->buf;
}

const char *PP_geom_svec3_value_null_format(void *a) { return "(error)"; }

void PP_geom_svec3_value_data_delete(void *a)
{
  struct PP_geom_svec3_value_data *d;
  d = (struct PP_geom_svec3_value_data *)a;

  free(d->buf);
  d->buf = NULL;
}

int PP_geom_svec3_value_custom_okfunc(void *a)
{
  struct PP_geom_svec3_value_data *d;
  d = (struct PP_geom_svec3_value_data *)a;

  if (d->okfunc)
    return d->okfunc(d->value, d->arg);
  return 1;
}

void PP_geom_mat43_value_format(int *argc, const char ***argv, void *a)
{
  int r;
  struct PP_geom_mat43_value_data *d;
  d = (struct PP_geom_mat43_value_data *)a;

  PP_geom_mat43_value_data_delete(a);

  r = geom_mat43_to_str(d->buf, d->value, 1, ", ", "[[", "],", "%.6g", -1, -1);
  if (r < 0) {
    for (int i = 0; i < 4; ++i) {
      d->buf[i] = NULL;
      d->cbuf[i] = NULL;
    }
    *argc = 0;
    *argv = NULL;
    return;
  }

  /* Remove first opening bracket */
  for (int i = 1; i < 4; ++i)
    d->buf[i][0] = ' ';

  /* Replace comma with closing bracket */
  d->buf[3][strlen(d->buf[3]) - 1] = ']';

  for (int i = 0; i < 4; ++i)
    d->cbuf[i] = d->buf[i];

  *argc = 4;
  *argv = d->cbuf;
}

const char *PP_geom_mat43_value_null_format(void *a) { return "(error)"; }

void PP_geom_mat43_value_data_delete(void *a)
{
  struct PP_geom_mat43_value_data *d;
  d = (struct PP_geom_mat43_value_data *)a;

  free(d->buf[0]);
  d->buf[0] = NULL;
}

int PP_geom_mat43_value_custom_okfunc(void *a)
{
  struct PP_geom_mat43_value_data *d;
  d = (struct PP_geom_mat43_value_data *)a;

  if (d->okfunc)
    return d->okfunc(&d->value, d->arg);
  return 1;
}
