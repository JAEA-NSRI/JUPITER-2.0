#include "pvector.h"
#include "defs.h"

#include <stdlib.h>

void LPTX_particle_vector_allocate(LPTX_particle_vector *p, LPTX_idtype length)
{
  LPTX_type *d = NULL;
  if (length > 0)
    d = (LPTX_type *)malloc(sizeof(LPTX_type) * length);
  if (!d)
    length = 0;
  LPTX_particle_vector_bind(p, d, length);
}

void LPTX_particle_vector_free(LPTX_particle_vector *p)
{
  free((LPTX_type *)p->v);
}
