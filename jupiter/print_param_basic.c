#include "print_param_basic.h"
#include "os/asprintf.h"

#include <math.h>

const char *PP_charp_name_format(void *a)
{
  return ((struct PP_charp_name_data *)a)->message;
}

const char *PP_charp_unit_format(void *a)
{
  return ((struct PP_charp_unit_data *)a)->text;
}

const char *PP_charp_value_format(void *a)
{
  return ((struct PP_charp_value_data *)a)->value;
}

const char *PP_charp_value_null_format(void *a)
{
  return ((struct PP_charp_value_data *)a)->value_null;
}

int PP_charp_okfunc(void *a)
{
  struct PP_charp_value_data *p;
  p = (struct PP_charp_value_data *)a;

  if (!p->value)
    return 0;

  if (p->okfunc)
    return p->okfunc(p->value, p->arg);
  return 1;
}

int PP_charp_nonempty_okfunc(void *a)
{
  struct PP_charp_value_data *p;
  p = (struct PP_charp_value_data *)a;

  if (!p->value)
    return 0;

  if (*p->value == '\0')
    return 0;

  if (p->okfunc)
    return p->okfunc(p->value, p->arg);
  return 1;
}

int PP_charp_null_okfunc(void *a)
{
  struct PP_charp_value_data *p;
  p = (struct PP_charp_value_data *)a;

  if (p->okfunc)
    return p->okfunc(p->value, p->arg);
  return 1;
}

//--- int

const char *PP_int_value_format(void *a)
{
  int r;
  struct PP_int_value_data *p;
  p = (struct PP_int_value_data *)a;

  PP_int_value_data_delete(a);

  r = jupiter_asprintf(&p->buf, "%d", p->value);
  if (r < 0)
    p->buf = NULL;

  return p->buf;
}

const char *PP_int_value_null_format(void *a) { return "(error)"; }

void PP_int_value_data_delete(void *a)
{
  struct PP_int_value_data *p;
  p = (struct PP_int_value_data *)a;

  free(p->buf);
  p->buf = NULL;
}

int PP_int_value_custom_okfunc(void *a)
{
  struct PP_int_value_data *p;
  p = (struct PP_int_value_data *)a;

  if (p->okfunc)
    return p->okfunc(p->value, p->arg);
  return 1;
}

//--- double

const char *PP_double_value_format(void *a)
{
  int r;
  struct PP_double_value_data *p;
  p = (struct PP_double_value_data *)a;

  PP_double_value_data_delete(a);

  r = jupiter_asprintf(&p->buf, "%.6e", p->value);
  if (r < 0)
    p->buf = NULL;

  return p->buf;
}

const char *PP_double_ns_value_format(void *a)
{
  int r;
  struct PP_double_value_data *p;
  p = (struct PP_double_value_data *)a;

#ifdef isnan
  if (isnan(p->value)) {
    p->buf = NULL;
    return "(not set)";
  }
#endif

  return PP_double_value_format(a);
}

const char *PP_double_inf_value_format(void *a)
{
  int r;
  struct PP_double_value_data *p;
  p = (struct PP_double_value_data *)a;

#ifdef isnan
  if (isnan(p->value)) {
    p->buf = NULL;
    return "(not set)";
  }
#endif
#ifdef isinf
  if (isinf(p->value)) {
    p->buf = NULL;
    return "(not set)";
  }
#endif

  return PP_double_value_format(a);
}

const char *PP_double_value_null_format(void *a) { return "(error)"; }

void PP_double_value_data_delete(void *a)
{
  struct PP_double_value_data *p;
  p = (struct PP_double_value_data *)a;

  free(p->buf);
  p->buf = NULL;
}

int PP_double_value_custom_ok_func(void *a)
{
  struct PP_double_value_data *p;
  p = (struct PP_double_value_data *)a;

  if (p->okfunc)
    return p->okfunc(p->value, p->arg);
  return 1;
}

/* These function does not use @p a */
int PP_double_default_value_okfunc(void *a)
{
  struct PP_double_value_data *p;
  p = (struct PP_double_value_data *)a;

  if (isfinite(p->value))
    return PP_double_value_custom_ok_func(a);
  return 0;
}

int PP_double_ns_value_okfunc(void *a)
{
  struct PP_double_value_data *p;
  p = (struct PP_double_value_data *)a;

  if (isfinite(p->value))
    return PP_double_value_custom_ok_func(a);
#ifdef isnan
  if (isnan(p->value))
    return 1;
#endif
  return 0;
}

int PP_double_inf_value_okfunc(void *a)
{
  struct PP_double_value_data *p;
  p = (struct PP_double_value_data *)a;

  if (isfinite(p->value))
    return PP_double_value_custom_ok_func(a);
#ifdef isnan
  if (isnan(p->value))
    return 1;
#endif
#ifdef isinf
  if (isinf(p->value))
    return 1;
#endif
  return 0;
}
