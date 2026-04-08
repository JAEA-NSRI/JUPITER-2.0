
#include "udata.h"
#include "struct_data.h"

void *
geom_user_defined_data_get(const geom_user_defined_data *d)
{
  return (void *)d->data;
}
