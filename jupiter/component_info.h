#ifndef JUPITER_COMPONENT_INFO_H
#define JUPITER_COMPONENT_INFO_H

#include "component_data_defs.h"
#include "component_info_defs.h"
#include "general_vector_node.h"
#include "csv.h"
#include "geometry/util.h"
#include "jupiter/common.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline component_info *
component_info__getter(struct general_vector_node *node)
{
  return geom_container_of(node, struct component_info, node);
}

JUPITER_DECL
void *component_info__alloc(struct general_vector_node *a, int n);
JUPITER_DECL
void component_info__delete(struct general_vector_node *a);
JUPITER_DECL
void component_info__copy(struct general_vector_node *to, int ts,
                          struct general_vector_node *from, int fs, int n);

static inline void component_info__assign(struct general_vector_node *to,
                                          struct general_vector_node *from)
{
  struct component_info *vf, *vt;
  vf = component_info__getter(from);
  vt = component_info__getter(to);

  *(struct component_info_data **)&vt->d = vf->d;
}

static inline void component_info__nullify(struct general_vector_node *a)
{
  struct component_info *v;
  v = component_info__getter(a);

  *(struct component_info_data **)&v->d = NULL;
}

static inline void component_info__swap(struct general_vector_node *p, int i,
                                        struct general_vector_node *q, int j,
                                        void *arg)
{
  struct component_info *pp, *qq;
  struct component_info_data t;

  pp = component_info__getter(p);
  qq = component_info__getter(q);
  t = pp->d[i];
  pp->d[i] = qq->d[j];
  qq->d[j] = t;
}

/**
 * @return negative when a < b, 0 when a == b, positive when a > b.
 */
typedef int component_info_comp1(const struct component_info_data *a,
                                 const struct component_info_data *b,
                                 void *arg);

/**
 * @retval 0 keep @p a.
 * @retval 1 keep @p b.
 */
typedef int component_info_merge1(struct component_info_data *a,
                                  struct component_info_data *b, void *arg);

struct component_info_comp1_data
{
  component_info_comp1 *cmp;
  component_info_merge1 *merge;
  void *arg;
};

static inline int component_info__comp1(struct general_vector_node *p, int i,
                                        struct general_vector_node *q, int j,
                                        void *arg)
{
  struct component_info *pp, *qq;
  struct component_info_comp1_data *d;
  pp = component_info__getter(p);
  qq = component_info__getter(q);
  d = (struct component_info_comp1_data *)arg;
  return d->cmp(&pp->d[i], &qq->d[j], d->arg);
}

static inline int component_info__merge1(struct general_vector_node *p, int i,
                                         struct general_vector_node *q, int j,
                                         void *arg)
{
  struct component_info *pp, *qq;
  struct component_info_comp1_data *d;
  pp = component_info__getter(p);
  qq = component_info__getter(q);
  d = (struct component_info_comp1_data *)arg;
  if (d->merge)
    return d->merge(&pp->d[i], &qq->d[j], d->arg);
  return 0;
}

typedef int component_info_compn(const struct component_info_data *a,
                                 void *arg);

/**
 * Include needle data in @p arg.
 */
struct component_info_compn_data
{
  component_info_compn *cmp;
  void *arg;
};

static inline int component_info__compn(struct general_vector_node *n, int i,
                                        void *arg)
{
  struct component_info *v;
  struct component_info_compn_data *d;
  v = component_info__getter(n);
  d = (struct component_info_compn_data *)arg;
  return d->cmp(&v->d[i], d->arg);
}

static inline const struct general_vector_callbacks *
component_info__funcs(struct general_vector_callbacks *f)
{
  *f = (struct general_vector_callbacks){
    .alloc = component_info__alloc,
    .copy = component_info__copy,
    .assign = component_info__assign,
    .deleter = component_info__delete,
    .nullify = component_info__nullify,
    .swap = component_info__swap,
    .comp1 = component_info__comp1,
    .compn = component_info__compn,
    .merge = component_info__merge1,
  };
  return f;
}

#define component_info_funcs() general_vector_funcs(component_info__funcs)

//----

static inline void component_info_data_init(struct component_info_data *info)
{
  info->d = NULL;
  info->id = JUPITER_ID_INVALID;
}

static inline int component_info_data_geti(struct component_info_data *info)
{
  return info->id;
}

static inline struct component_data *
component_info_data_getc(struct component_info_data *info)
{
  return info->d;
}

//---

static inline void component_info_init(struct component_info *info)
{
  general_vector_node_init(&info->node, component_info_funcs());
}

static inline void component_info_clear(struct component_info *info)
{
  general_vector_node_clear(&info->node, component_info_funcs());
}

/**
 * Share the same array of @p from to @p to.
 */
static inline void component_info_share(struct component_info *to,
                                        struct component_info *from)
{
  general_vector_node_share(&to->node, &from->node, component_info_funcs());
}

/**
 * Resize array.
 *
 * @note This function has been reimplemented using
 *       general_vector_node_resize(), but this function returns @p info rather
 *       than `int`. To be same as old API.
 */
static inline struct component_info *
component_info_resize(struct component_info *info, int ncompo, int copy)
{
  int r;
  component_info tmp;
  component_info_init(&tmp);
  r = general_vector_node_resize(&info->node, &tmp.node, ncompo, copy,
                                 component_info_funcs());
  if (r)
    return info;
  return NULL;
}

static inline void component_info_seti(struct component_info *info, int index,
                                       int compo)
{
  info->d[index] = (struct component_info_data){.d = NULL, .id = compo};
}

static inline void component_info_setc(struct component_info *info, int index,
                                       struct component_data *d)
{
  info->d[index] = (struct component_info_data){.d = d, .id = 0};
}

static inline struct component_info_data *
component_info_getp(struct component_info *info, int index)
{
  return &info->d[index];
}

static inline int component_info_geti(struct component_info *info, int index)
{
  return component_info_data_geti(component_info_getp(info, index));
}

static inline struct component_data *
component_info_getc(struct component_info *info, int index)
{
  return component_info_data_getc(component_info_getp(info, index));
}

static inline int component_info_ncompo(struct component_info *info)
{
  return info->node.n;
}

/**
 * Comparison function for component_info_sort().
 */
static inline int component_info_sort_comp1(const struct component_info_data *a,
                                            const struct component_info_data *b,
                                            void *arg)
{
  if (a->d && b->d)
    return a->d->comp_index - b->d->comp_index;
  if (a->d)
    return 1;
  if (b->d)
    return -1;
  return a->id - b->id;
}

/**
 * Comparison function for component_info_merge().
 *
 * Let merge JUPITER ID is given ID without pointer.
 */
static inline int
component_info_merge_comp1(const struct component_info_data *a,
                           const struct component_info_data *b, void *arg)
{
  if (a->d || b->d) {
    if (a->d && b->d)
      return component_info_sort_comp1(a, b, arg);
    if (a->d && a->d->jupiter_id == b->id)
      return 0;
    if (b->d && b->d->jupiter_id == a->id)
      return 0;
    return component_info_sort_comp1(a, b, arg);
  }
  return component_info_sort_comp1(a, b, arg);
}

/**
 * Merge selector for component_info_merge()
 */
static inline int component_info_merge_merge1(struct component_info_data *a,
                                              struct component_info_data *b,
                                              void *arg)
{
  if (!a->d && b->d)
    return 1;
  return 0;
}

struct component_info_sort_compn_data
{
  int ic;
};

static inline int component_info_sort_compn(const struct component_info_data *a,
                                            void *arg)
{
  struct component_info_sort_compn_data *d;
  d = (struct component_info_sort_compn_data *)arg;
  if (a->d)
    return a->d->comp_index - d->ic;
  return 1;
}

/**
 * @brief sort component info array in specified order.
 */
static inline void component_info_sort_base(struct component_info *info,
                                            component_info_comp1 *cmp,
                                            void *arg)
{
  struct component_info_comp1_data d = {
    .cmp = cmp,
    .merge = NULL,
    .arg = arg,
  };
  general_vector_node_sort(&info->node, component_info_funcs(), &d);
}

/**
 * @brief sort component info array in ascending order on component indices.
 */
static inline void component_info_sort(struct component_info *info)
{
  component_info_sort_base(info, component_info_sort_comp1, NULL);
}

/**
 * @brief find the array index of given condition in component_info array
 *
 * This function uses binary search. Array needs to be sorted in the way of
 * given comparison.
 */
static inline int component_info_find_sorted_base(struct component_info *info,
                                                  component_info_compn *comp,
                                                  void *arg)
{
  struct component_info_compn_data d = {
    .cmp = comp,
    .arg = arg,
  };
  return general_vector_node_bsearch(&info->node, component_info_funcs(), &d);
}

/**
 * @brief find the array index of a component index in component_info array.
 * @param ic Component index to look for
 * @param info Array to find
 * @return index of the array
 * @retval -1 not found
 *
 * @note Array **must** be sorted. Otherwise this function can return
 * invalid result.
 *
 * @note If the array contains same ID twice or more, the returning
 * index will be inconsistent.
 */
static inline int component_info_find_sorted(int ic,
                                             struct component_info *info)
{
  struct component_info_sort_compn_data sd = {.ic = ic};
  return component_info_find_sorted_base(info, component_info_sort_compn, &sd);
}

/**
 * @brief find the array index of given condition in component_info array
 */
static inline int component_info_find_base(struct component_info *info,
                                           component_info_compn *cmp, void *arg)
{
  struct component_info_compn_data d = {
    .cmp = cmp,
    .arg = arg,
  };
  return general_vector_node_lsearch(&info->node, component_info_funcs(), &d);
}

/**
 * @brief find the array index of a component index in component_info array.
 * @param ic ID to look for
 * @param info Array to find
 * @return index of the array
 * @retval -1 not found
 *
 * This function performs linear search.
 */
static inline int component_info_find(int ic, struct component_info *info)
{
  struct component_info_sort_compn_data sd = {
    .ic = ic,
  };
  return component_info_find_base(info, component_info_sort_compn, &sd);
}

/**
 * @brief Checks whether a given component index is in component_info array.
 * @param ic ID to look for.
 * @param info Array to find
 * @retval 0 not found
 * @retval 1 found
 *
 * @note Array **must** be sorted. Otherwise this function can return
 * invalid result.
 */
static inline int component_info_any_sorted(int ic, struct component_info *info)
{
  return component_info_find_sorted(ic, info) != -1;
}

/**
 * @brief Checks whether a given component index is in component_info array.
 * @param ic ID to look for.
 * @param info Array to find
 * @retval 0 not found
 * @retval 1 found
 *
 * This function performs linear search.
 */
static inline int component_info_any(int ic, struct component_info *info)
{
  return component_info_find(ic, info) != -1;
}

/**
 * @brief Merge component info arrays.
 */
static inline int component_info_merge_base(struct component_info *dest,
                                            struct component_info *src1,
                                            struct component_info *src2,
                                            component_info_comp1 *cmp,
                                            component_info_merge1 *merge,
                                            void *arg)
{
  int r;
  component_info tmp;
  struct component_info_comp1_data d = {
    .cmp = cmp,
    .merge = merge,
    .arg = arg,
  };

  component_info_init(&tmp);

  r = general_vector_node_merge(&tmp.node, &src1->node, &src2->node,
                                component_info_funcs(), &d);
  if (!r) {
    component_info_clear(&tmp);
    return r;
  }

  component_info_share(dest, &tmp);
  component_info_clear(&tmp);
  return r;
}

/**
 * @brief Merges ids from two arrays (without duplication)
 * @param dest Resulting array
 * @param src1 One array to merge
 * @param src2 Another array to merge
 * @return @p dest, or NULL if allocation has been failed.
 *
 * @p dest == @p src1 or @p dest == @p src2 is allowed.
 *
 * The prior content in @p dest will be discarded. The content of @p
 * src1 and @p src2 will not be changed (unless they are the same as @p
 * dest).
 *
 * This function returns @p dest or NULL rather than integer value
 * of general_vector_node_merge() because of compatible with older API.
 */
static inline struct component_info *
component_info_merge(struct component_info *dest, struct component_info *src1,
                     struct component_info *src2)
{
  if (!component_info_merge_base(dest, src1, src2, component_info_merge_comp1,
                                 component_info_merge_merge1, NULL))
    return NULL;
  return dest;
}

typedef int set_component_info_default_id(int index, void *arg);
typedef int set_component_info_is_required(int index, void *arg);
typedef int set_component_info_validator(char **msgout, const char **cmsgout,
                                         const struct component_info_data *d,
                                         int index, void *arg);
typedef int
set_component_info_extra_process(const struct component_info_data *d, int index,
                                 void *arg);

struct set_component_info_funcs_data
{
  int default_id;
  set_component_info_default_id *default_id_func;
  int is_required;
  set_component_info_is_required *is_required_func;
  set_component_info_validator *validator;
  set_component_info_extra_process *extra_proc;
  void *arg;
};

static inline struct set_component_info_funcs_data set_component_info_funcs_all(
  int default_id, set_component_info_default_id *default_id_func,
  int is_required, set_component_info_is_required *is_required_func,
  set_component_info_validator *validator,
  set_component_info_extra_process *extra_proc, void *arg)
{
  struct set_component_info_funcs_data d = {
    .default_id = default_id,
    .default_id_func = default_id_func,
    .is_required = is_required,
    .is_required_func = is_required_func,
    .validator = validator,
    .extra_proc = extra_proc,
    .arg = arg,
  };
  return d;
}

/**
 * Assumes that default_id_func and is_required_func is not NULL. If not,
 * default ID will be 0 and IDs will be marked as required.
 */
static inline struct set_component_info_funcs_data
set_component_info_funcs_ff(set_component_info_default_id *default_id_func,
                            set_component_info_is_required *is_required_func,
                            set_component_info_validator *validator,
                            set_component_info_extra_process *extra_proc,
                            void *arg)
{
  return set_component_info_funcs_all(0, default_id_func, 1, is_required_func,
                                      validator, extra_proc, arg);
}

/**
 * Assumes that is_required_func is not NULL. If not, IDs will be marked as
 * required.
 */
static inline struct set_component_info_funcs_data set_component_info_funcs_vf(
  int default_id, set_component_info_is_required *is_required_func,
  set_component_info_validator *validator,
  set_component_info_extra_process *extra_proc, void *arg)
{
  return set_component_info_funcs_all(default_id, NULL, 1, is_required_func,
                                      validator, extra_proc, arg);
}

/**
 * Assumes that default_id_func is not NULL. If not, default ID will be 0.
 */
static inline struct set_component_info_funcs_data set_component_info_funcs_fv(
  set_component_info_default_id *default_id_func, int is_required,
  set_component_info_validator *validator,
  set_component_info_extra_process *extra_proc, void *arg)
{
  return set_component_info_funcs_all(0, default_id_func, is_required, NULL,
                                      validator, extra_proc, arg);
}

static inline struct set_component_info_funcs_data set_component_info_funcs_vv(
  int default_id, int is_required, set_component_info_validator *validator,
  set_component_info_extra_process *extra_proc, void *arg)
{
  return set_component_info_funcs_all(default_id, NULL, is_required, NULL,
                                      validator, extra_proc, arg);
}

/**
 * Shortcut when IDs are all required
 */
static inline struct set_component_info_funcs_data
set_component_info_funcs_required(set_component_info_validator *validator,
                                  set_component_info_extra_process *extra_proc,
                                  void *arg)
{
  return set_component_info_funcs_ff(NULL, NULL, validator, extra_proc, arg);
}

/**
 * @brief Read CSV data and set component_info array.
 * @param info component_info array to set to.
 * @param csv CSV data to read from
 * @param fname Source file name of @p csv
 * @param row CSV row to read from
 * @param column CSV column to read from and returns final position
 * @param comp_data_head Link list head for component data info
 * @param number_of_comp Number of components to be read
 * @param append Append Ids or replace original content
 * @param allow_dups If non-0 value given, allows ID duplication
 * @param need_sorted Order value
 * @param funcs callback functions data
 * @param stat If not NULL, sets `ON` if error occured.
 * @return 1 if fatal error occured (e.g. memory allocation failed), 0
 *         otherwise. Note that simple read errors will return 0, you
 *         have to check the value of @p stat.
 *
 * If @p append is 0, replaces the original content in @p info, otherwise
 * append ids in tail.
 *
 * This function requires input order of IDs by the value of @p need_sorted
 * as following:
 *
 * - It positive value given, requests the user to input IDs in
 *   ascending order.
 * - If negative value given, requests the user to input IDs in
 *   descending order.
 * - Otherwise (i.e., 0 given), the user can input IDs in random order.
 *
 * If @p validator is NULL, any IDs will be accepted (including out-of-range).
 *
 * @p msgout must be allocated with any function that must be freed
 * with `free()` (ex. `malloc()`, `jupiter_asprintf()` etc). You can
 * set to NULL (to use @p cmsgout, or default message). If the message
 * does not need to be formatted, you can return constant string via
 * @p cmsgout.
 *
 * @note You can read CSV data in @p extra_process function, but
 *       required parameter will not be passed. Include them in @p
 *       arg. Ensure that @p column pointer is shared when reading from
 *       same row using SET_P_NEXT().
 *
 * `SET_P_NEXT()` will be used for reading ID values if @p
 * is_required_func is NULL and @p is_required is 0, or @p
 * is_required_func returns 0. Otherwise, `SET_P_NEXT_REQUIRED()` will
 * be used.
 */
JUPITER_DECL
int set_component_info(struct component_info *info, csv_data *csv,
                       const char *fname, csv_row **row, csv_column **column,
                       struct component_data *comp_data_head,
                       int number_of_comp, int append, int allow_dups,
                       int need_sorted,
                       const struct set_component_info_funcs_data *funcs,
                       int *stat);

/**
 * @brief Data to use set_component_info_default_validator()
 *
 * Generates error message like `Invalid component ID for
 * %(model_name). (It must be %(...).)`
 */
struct set_component_info_default_validator_data
{
  int accept_n1_gas;       /*!< Whether accepts -1 gas */
  component_phases phases; /*!< Whether accepting phases */
  const char *model_name;  /*!< Descriptive message (typ. model name) */
};

/**
 * @brief Process for usual ID validation
 *
 * This function is for wrapping by your custom function (if you also
 * need to do this if you want to specify extra_process, because @p
 * arg is shared)
 */
JUPITER_DECL
int set_component_info_default_validator(
  char **msgout, const char **cmsgout, const struct component_info_data *d,
  int index, struct set_component_info_default_validator_data *arg);

/**
 * @brief Process for usual ID validation
 *
 * This function can be directly specified in set_component_info() or
 * set_component_info_required(). @p arg must be a pointer to `struct
 * component_info_default_validator_data`.
 */
static inline int
set_component_info_default_validator_func(char **msgout, const char **cmsgout,
                                          const struct component_info_data *d,
                                          int index, void *arg)
{
  struct set_component_info_default_validator_data *p =
    (struct set_component_info_default_validator_data *)arg;
  return set_component_info_default_validator(msgout, cmsgout, d, index, p);
}

#ifdef __cplusplus
}
#endif

#endif
