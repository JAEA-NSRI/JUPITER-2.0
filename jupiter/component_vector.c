#include "component_vector.h"
#include "component_data_defs.h"
#include "component_vector_defs.h"

#include <stdlib.h>
#include <string.h>

void *component_vector__alloc(struct general_vector_node *a, int n)
{
  component_data **d;
  struct component_vector *c;

  c = component_vector__getter(a);
  d = (component_data **)malloc(sizeof(component_data *) * n);
  *(struct component_data ***)&c->d = d;
  return d;
}

void component_vector__delete(struct general_vector_node *a)
{
  struct component_vector *c;

  c = component_vector__getter(a);
  free(c->d);
  *(struct component_data ***)&c->d = NULL;
}

void component_vector__copy(struct general_vector_node *to, int ts,
                            struct general_vector_node *from, int fs, int n)
{
  struct component_vector *tc, *fc;
  tc = component_vector__getter(to);
  fc = component_vector__getter(from);
  memmove(tc->d + ts, fc->d + fs, sizeof(component_data *) * n);
}
