#include "2d_path.h"
#include "defs.h"
#include "geom_assert.h"
#include "struct_data.h"
#include "vector.h"

#include <stdlib.h>

static geom_size_type geom_2d_path_data_size(geom_size_type nentry)
{
  geom_size_type size;

  GEOM_ASSERT(nentry > 0);
  size = sizeof(union geom_2d_path_element_entry) * nentry;

  GEOM_ASSERT(size + sizeof(geom_2d_path_data) > size);
  return size + sizeof(geom_2d_path_data);
}

geom_2d_path_data *geom_2d_path_data_new(geom_size_type nentry)
{
  geom_size_type size = geom_2d_path_data_size(nentry);
  geom_2d_path_data *data = (geom_2d_path_data *)malloc(size);
  if (!data)
    return NULL;

  data->number_entry = nentry;
  data->entry[0].t = GEOM_2D_PATH_INVALID;
  return data;
}

geom_2d_path_data *geom_2d_path_data_resize(geom_2d_path_data *data,
                                            geom_size_type nentry)
{
  geom_size_type size;

  if (data && data->number_entry >= nentry)
    return data;

  size = geom_2d_path_data_size(nentry);
  return (geom_2d_path_data *)realloc(data, size);
}

void geom_2d_path_data_delete(geom_2d_path_data *data)
{
  free(data);
}

geom_2d_path_element_iterator
geom_2d_path_data_get_entry(const geom_2d_path_data *data)
{
  GEOM_ASSERT(data);

  geom_2d_path_element_iterator e = {
    .entry = data->entry,
    .last_move_point = geom_vec2_c(0.0, 0.0),
    .origin_point = geom_vec2_c(0.0, 0.0),
  };
  return e;
}
