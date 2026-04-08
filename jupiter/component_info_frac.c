#include "component_info_frac.h"

#include <stdlib.h>
#include <string.h>

void *component_info_frac__alloc(struct general_vector_node *a, int n)
{
  struct component_info_frac *f;
  struct component_info_frac_data *p;
  f = component_info_frac__getter(a);
  p = (struct component_info_frac_data *)malloc(
    sizeof(struct component_info_frac_data) * n);
  *((struct component_info_frac_data **)&f->d) = p;
  return p;
}

void component_info_frac__delete(struct general_vector_node *a)
{
  struct component_info_frac *f;
  f = component_info_frac__getter(a);
  free(f->d);
  *((struct component_info_frac_data **)&f->d) = NULL;
}

void component_info_frac__copy(struct general_vector_node *to, int ts,
                               struct general_vector_node *from, int fs, int n)
{
  struct component_info_frac *tf, *ff;
  tf = component_info_frac__getter(to);
  ff = component_info_frac__getter(from);
  memmove(&tf->d[ts], &ff->d[fs], sizeof(struct component_info_frac_data) * n);
}
