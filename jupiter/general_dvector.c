#include "general_dvector.h"

#include <string.h>
#include <stdlib.h>

void *general_dvector__alloc(struct general_vector_node *a, int n)
{
  general_dvector *v;
  v = general_dvector__getter(a);

  *(double **)&v->d = (double *)calloc(n, sizeof(double));
  return v->d;
}

void general_dvector__delete(struct general_vector_node *a)
{
  general_dvector *v;
  v = general_dvector__getter(a);

  free(v->d);
  *(double **)&v->d = NULL;
}

void general_dvector__copy(struct general_vector_node *to, int ts,
                           struct general_vector_node *from, int fs, int n)
{
  general_dvector *vf, *vt;
  vf = general_dvector__getter(from);
  vt = general_dvector__getter(to);

  memmove(vt->d + ts, vf->d + fs, sizeof(double) * n);
}
