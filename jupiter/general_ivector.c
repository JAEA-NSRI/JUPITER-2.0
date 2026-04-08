#include "general_ivector.h"

#include <string.h>
#include <stdlib.h>

void *general_ivector__alloc(struct general_vector_node *a, int n)
{
  general_ivector *v;
  v = general_ivector__getter(a);

  *(int **)&v->d = (int *)calloc(n, sizeof(int));
  return v->d;
}

void general_ivector__delete(struct general_vector_node *a)
{
  general_ivector *v;
  v = general_ivector__getter(a);

  free(v->d);
  *(int **)&v->d = NULL;
}

void general_ivector__copy(struct general_vector_node *to, int ts,
                           struct general_vector_node *from, int fs, int n)
{
  general_ivector *vf, *vt;
  vf = general_ivector__getter(from);
  vt = general_ivector__getter(to);

  memmove(vt->d + ts, vf->d + fs, sizeof(int) * n);
}
