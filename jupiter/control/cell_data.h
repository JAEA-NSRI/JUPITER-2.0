#ifndef JUPITER_CONTROL_CELL_DATA_H
#define JUPITER_CONTROL_CELL_DATA_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_cell_data);

JUPITER_CONTROL_DECL
jcntrl_cell_data *jcntrl_cell_data_new(void);

JUPITER_CONTROL_DECL
void jcntrl_cell_data_delete(jcntrl_cell_data *data);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_cell_data_object(jcntrl_cell_data *data);

JUPITER_CONTROL_DECL
jcntrl_cell_data *jcntrl_cell_data_downcast(jcntrl_shared_object *object);

JUPITER_CONTROL_DECL
int jcntrl_cell_data_add_array(jcntrl_cell_data *data,
                               jcntrl_data_array *array);

/**
 * This function counts number of arrays (i.e., O(n))
 */
JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_cell_data_get_number_of_arrays(jcntrl_cell_data *data);

JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_cell_data_get_array(jcntrl_cell_data *data,
                                              jcntrl_size_type index);

JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_cell_data_get_array_by_name(jcntrl_cell_data *data,
                                                      const char *name,
                                                      jcntrl_size_type namelen);

JUPITER_CONTROL_DECL
void jcntrl_cell_data_remove_all_arrays(jcntrl_cell_data *data);

JUPITER_CONTROL_DECL
int jcntrl_cell_data_remove_array(jcntrl_cell_data *data,
                                  jcntrl_size_type index);

JUPITER_CONTROL_DECL
int jcntrl_cell_data_remove_array_by_name(jcntrl_cell_data *data,
                                          const char *name,
                                          jcntrl_size_type namelen);

/**
 * Copy arrays from @p from to @p dest without copying the array themselves.
 */
JUPITER_CONTROL_DECL
int jcntrl_cell_data_shallow_copy(jcntrl_cell_data *dest,
                                  jcntrl_cell_data *from);

/**
 * Copy arrays from @p from to @p dest with copying the array themselves.
 */
JUPITER_CONTROL_DECL
int jcntrl_cell_data_deep_copy(jcntrl_cell_data *dest, jcntrl_cell_data *from);

/**
 * @note deleting the current item except for using
 * jcntrl_cell_data_iterator_remove() makes the iterator invalid.
 */
struct jcntrl_cell_data_iterator
{
  jcntrl_cell_data *const celldata;     // do not modify directly
  jcntrl_cell_data_entry *const cursor; // do not modify directly
};

static inline struct jcntrl_cell_data_iterator
jcntrl_cell_data_iterator_init(jcntrl_cell_data *celldata)
{
  return (struct jcntrl_cell_data_iterator){
    .celldata = celldata,
    .cursor = NULL,
  };
}

JUPITER_CONTROL_DECL
void jcntrl_cell_data_iterator_goto_first(jcntrl_cell_data_iterator *iter);
JUPITER_CONTROL_DECL
void jcntrl_cell_data_iterator_goto_last(jcntrl_cell_data_iterator *iter);
JUPITER_CONTROL_DECL
int jcntrl_cell_data_iterator_is_end(jcntrl_cell_data_iterator *iter);

JUPITER_CONTROL_DECL
jcntrl_data_array *
jcntrl_cell_data_iterator_get(jcntrl_cell_data_iterator *iter);

JUPITER_CONTROL_DECL
void jcntrl_cell_data_iterator_next(jcntrl_cell_data_iterator *iter);

JUPITER_CONTROL_DECL
void jcntrl_cell_data_iterator_prev(jcntrl_cell_data_iterator *iter);

JUPITER_CONTROL_DECL
void jcntrl_cell_data_iterator_remove(jcntrl_cell_data_iterator *iter);

static inline jcntrl_cell_data_iterator jcntrl_cell_data_iterator_init_f(
  jcntrl_cell_data *celldata, void (*to_first)(jcntrl_cell_data_iterator *it))
{
  jcntrl_cell_data_iterator it = jcntrl_cell_data_iterator_init(celldata);
  to_first(&it);
  return it;
}

#define jcntrl_cell_data_array_foreach_base(cell_data, aryp, to_first, next, \
                                            iter)                            \
  for (jcntrl_cell_data_iterator iter =                                      \
         jcntrl_cell_data_iterator_init_f(cell_data, to_first);              \
       (aryp = jcntrl_cell_data_iterator_get(&iter)),                        \
                                 !jcntrl_cell_data_iterator_is_end(&iter);   \
       next(&iter))

/**
 * Iterate over arrays in cell data.
 *
 * @p iter is just for the iterator variable name. No need to declare it
 * explicitly.
 */
#define jcntrl_cell_data_array_foreach(cell_data, aryp, iter)               \
  jcntrl_cell_data_array_foreach_base(cell_data, aryp,                      \
                                      jcntrl_cell_data_iterator_goto_first, \
                                      jcntrl_cell_data_iterator_next, iter)

#define jcntrl_cell_data_array_reverse_foreach(cell_data, aryp, iter)      \
  jcntrl_cell_data_array_foreach_base(cell_data, aryp,                     \
                                      jcntrl_cell_data_iterator_goto_last, \
                                      jcntrl_cell_data_iterator_prev, iter)

JUPITER_CONTROL_DECL_END

#endif
