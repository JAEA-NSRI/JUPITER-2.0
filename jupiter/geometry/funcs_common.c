
#include "funcs_common.h"
#include "defs.h"
#include "func_data.h"
#include "data.h"
#include "geom_assert.h"
#include "global.h"
#include "variant.h"
#include "abuilder.h"

void *geom_funcs_common_allocate(const struct geom_funcs_common *funcs,
                                 geom_data *master, geom_error *e)
{
  geom_error err;
  void *data;

  GEOM_ASSERT(funcs);
  GEOM_ASSERT(master);

  data = NULL;
  if (funcs->allocator && funcs->deallocator) {
    data = funcs->allocator();
    if (!data) {
      if (e) *e = GEOM_ERR_NOMEM;
      return NULL;
    }
    err = geom_data_add_pointer(master, data, funcs->deallocator);
    if (err != GEOM_SUCCESS) {
      if (e) *e = err;
      funcs->deallocator(data);
      return NULL;
    }
  }
  if (e) *e = GEOM_SUCCESS;
  return data;
}

void  geom_funcs_common_deallocate(const struct geom_funcs_common *funcs,
                                   void *data, geom_data *master)
{
  GEOM_ASSERT(funcs);
  GEOM_ASSERT(master);

  if (!data) return;

  geom_data_del_pointer(master, data);
}

geom_error
geom_funcs_common_set_data(const struct geom_funcs_common *funcs, void *data,
                           geom_args_builder *ab)
{
  geom_error err;
  geom_variant_list *vl, *curs;
  geom_size_type icnt;
  geom_size_type reqsize;

  GEOM_ASSERT(funcs);

  if (!data) return GEOM_SUCCESS;

  GEOM_ASSERT(funcs->n_params);
  GEOM_ASSERT(funcs->set_value);

  vl = geom_args_builder_get_list(ab);
  GEOM_ASSERT(vl);

  for (icnt = 0, curs = geom_variant_list_next(vl); curs != vl;
       icnt++, curs = geom_variant_list_next(curs)) {
    err = funcs->set_value(data, icnt, geom_variant_list_get(curs));
    if (err != GEOM_SUCCESS) {
      return err;
    }
  }

  reqsize = funcs->n_params(NULL, ab);
  if (reqsize >= 0 && icnt < reqsize) {
    return GEOM_ERR_SHORT_LIST;
  }

  return GEOM_SUCCESS;
}

void *geom_funcs_common_copy_data(const struct geom_funcs_common *funcs,
                                  void *a, geom_data *master, geom_error *e)
{
  geom_error err;
  void *np;

  GEOM_ASSERT(funcs);
  GEOM_ASSERT(master);

  if (!a) return NULL;

  GEOM_ASSERT(funcs->copy);
  GEOM_ASSERT(funcs->deallocator);

  err = GEOM_SUCCESS;
  np = funcs->copy(a);
  if (!np) {
    err = GEOM_ERR_NOMEM;
  } else {
    err = geom_data_add_pointer(master, np, funcs->deallocator);
  }
  if (e) *e = err;
  return np;
}
