
#include <stdlib.h>

#include "struct_data.h"
#include "data.h"
#include "geom_assert.h"
#include "file.h"
#include "svector.h"
#include "udata-priv.h"

geom_file_data *geom_file_data_new(geom_data_element *parent, geom_error *e)
{
  geom_data *master;
  geom_file_data *d;
  geom_error err;

  GEOM_ASSERT(parent);
  GEOM_ASSERT(!parent->file);

  master = geom_data_element_master(parent);
  GEOM_ASSERT(master);

  d = (geom_file_data *)malloc(sizeof(geom_file_data));
  if (!d) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }
  err = geom_data_add_pointer(master, d, free);
  if (err != GEOM_SUCCESS) {
    free(d);
    if (e) *e = err;
    return NULL;
  }

  parent->file = d;
  d->parent = parent;
  d->file_name = NULL;
  d->alt_file_name = NULL;
  d->offset = geom_svec3_c(0, 0, 0);
  d->size   = geom_svec3_c(0, 0, 0);
  d->repeat = geom_svec3_c(0, 0, 0);
  d->origin = geom_svec3_c(0, 0, 0);
  geom_user_defined_data_init(&d->extra_data);
  return d;
}

geom_data_element *geom_file_data_parent(geom_file_data *data)
{
  GEOM_ASSERT(data);
  return data->parent;
}

geom_data *geom_file_data_master(geom_file_data *data)
{
  GEOM_ASSERT(data);
  return geom_data_element_master(geom_file_data_parent(data));
}

void geom_file_data_delete(geom_file_data *data)
{
  if (!data) return;

  geom_user_defined_data_free(geom_file_data_master(data), &data->extra_data);
  geom_data_del_pointer(geom_file_data_master(data), data);
}

void geom_file_data_set_file_path(geom_file_data *data,
                                  const char *fname, const char *altname)
{
  GEOM_ASSERT(data);

  data->file_name = fname;
  data->alt_file_name = altname;
}

void geom_file_data_set_sizev(geom_file_data *data, geom_svec3 size)
{
  GEOM_ASSERT(data);

  data->size = size;
}

void geom_file_data_set_offsetv(geom_file_data *data, geom_svec3 offset)
{
  GEOM_ASSERT(data);

  data->offset = offset;
}

void geom_file_data_set_repeatv(geom_file_data *data, geom_svec3 rep)
{
  GEOM_ASSERT(data);

  data->repeat = rep;
}

void geom_file_data_set_originv(geom_file_data *data, geom_svec3 ori)
{
  GEOM_ASSERT(data);

  data->origin = ori;
}

const char *geom_file_data_get_file_path(geom_file_data *data)
{
  GEOM_ASSERT(data);

  return data->file_name;
}

const char *geom_file_data_get_alt_file_path(geom_file_data *data)
{
  GEOM_ASSERT(data);

  return data->alt_file_name;
}

geom_svec3 geom_file_data_get_size(geom_file_data *data)
{
  GEOM_ASSERT(data);

  return data->size;
}

geom_svec3 geom_file_data_get_offset(geom_file_data *data)
{
  GEOM_ASSERT(data);

  return data->offset;
}

geom_svec3 geom_file_data_get_repeat(geom_file_data *data)
{
  GEOM_ASSERT(data);

  return data->repeat;
}

geom_svec3 geom_file_data_get_origin(geom_file_data *data)
{
  GEOM_ASSERT(data);

  return data->origin;
}

geom_error
geom_file_data_set_extra_data(geom_file_data *data,
                              void *extra_data, geom_deallocator *dealloc)
{
  GEOM_ASSERT(data);

  return geom_user_defined_data_set(geom_file_data_master(data),
                                    &data->extra_data, extra_data, dealloc);
}

const geom_user_defined_data *
geom_file_data_get_extra_data(geom_file_data *data)
{
  GEOM_ASSERT(data);

  return &data->extra_data;
}
