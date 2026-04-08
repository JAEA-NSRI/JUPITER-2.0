
#include <stdlib.h>
#include <math.h>

#include "mat22.h"
#include "shape.h"
#include "defs.h"
#include "geom_assert.h"
#include "list.h"
#include "data.h"
#include "abuilder.h"
#include "abuilder-priv.h"
#include "global.h"
#include "struct_data.h"
#include "func_defs.h"
#include "func_data.h"
#include "funcs_common.h"
#include "shape_transform.h"
#include "variant.h"
#include "svector.h"
#include "vector.h"
#include "infomap.h"
#include "mat43.h"
#include "mat44.h"
#include "udata.h"
#include "udata-priv.h"
#include "shp_specials.h"

#define geom_shape_element_entry(ptr) \
  geom_list_entry(ptr, struct geom_shape_element, list)

/**
 * @ingroup Geometry
 * @brief Stores shape stack for interactive geometry building
 *
 * This struct is internal use. You can not use directly.
 */
struct geom_shape_stack
{
  geom_shape_element *start_shape[GEOM_SHAPE_STACK_SIZE];
  ///< Pointer to start shape for each stack level.
  int current_stack_p; ///< Current stack level
};

/**
 * @memberof geom_shape_stack
 * @brief Initialize builder shape stack
 * @param stk Stack data to initialize
 */
static void
geom_shape_stack_init(struct geom_shape_stack *stk)
{
  int i;

  GEOM_ASSERT(stk);

  for (i = 0; i < GEOM_SHAPE_STACK_SIZE; ++i) {
    stk->start_shape[i] = NULL;
  }
  stk->current_stack_p = -1;
}

/**
 * @memberof geom_shape_stack
 * @brief Push shape element to stack
 * @param stk Stack to push for
 * @param el Element to push
 * @param current_stack_level Stack level of el, in another stack (if
 *                            `stk` is for PUSH/COMB pair, give stack
 *                            level on GST/GED pair, and vice varsa).
 * @retval ::GEOM_SUCCESS No error
 * @retval ::GEOM_ERR_SHAPE_STACK_OVERFLOW Stack overflowed.
 *
 * Here, ::GEOM_ERR_GROUP_STACK_OVERFLOW is not used because we does
 * not distingish what `stk` does mean. Replace with it if required.
 *
 * Stack pointer will increment even if the stack will overflow.
 *
 * If you give -2 or less for \p current_stack_level, the the stack
 * level match will not be checked.
 */
static geom_error
geom_shape_stack_push(struct geom_shape_stack *stk, geom_shape_element *el)
{
  GEOM_ASSERT(stk);
  GEOM_ASSERT(el);

  stk->current_stack_p++;
  if (stk->current_stack_p >= GEOM_SHAPE_STACK_SIZE) {
    return GEOM_ERR_SHAPE_STACK_OVERFLOW;
  }
  stk->start_shape[stk->current_stack_p] = el;
  return GEOM_SUCCESS;
}

/**
 * @memberof geom_shape_stack
 * @brief Pop shape element from stack
 * @param stk Stack to pop from
 * @param current_stack_level Current stack level in another stack (if
 *                            `stk` is for PUSH/COMB pair, give stack
 *                            level on GST/GED pair, and vice varsa).
 * @param e   If non-NULL value is given, sets errors there.
 *
 * @return Poped element if successfully popped, NULL if underflow or
 *         stack is overflowing (i.e. stack pointer pointer is not
 *         valid.)
 *
 * @retval ::GEOM_SUCCESS No error
 * @retval ::GEOM_ERR_SHAPE_STACK_UNDERFLOW Stack underflowed.
 *
 * This function explicitly sets ::GEOM_SUCCESS if no errors occured.
 *
 * The value of \p current_stack_level is used to check mismatch.  If
 * given value is less than -1 (note: -1 is a valid level), mismatch
 * checking will not be done.
 *
 * Here, ::GEOM_ERR_GROUP_STACK_UNDERFLOW is not used because we does
 * not distingish what `stk` does mean.  Replace with it if required.
 *
 * Popping from which the stack is overflowed will return NULL and
 * sets ::GEOM_SUCCESS (No error), until stack level gets valid range.
 */
static geom_shape_element *
geom_shape_stack_pop(struct geom_shape_stack *stk, geom_error *e)
{
  geom_shape_element *el;

  GEOM_ASSERT(stk);

  if (e) *e = GEOM_SUCCESS;
  if (stk->current_stack_p - 1 < -1) {
    if (e) *e = GEOM_ERR_SHAPE_STACK_UNDERFLOW;
    return NULL;
  }
  el = NULL;
  if (stk->current_stack_p < GEOM_SHAPE_STACK_SIZE) {
    el = stk->start_shape[stk->current_stack_p];
  }
  stk->current_stack_p--;
  return el;
}

/**
 * @memberof geom_shape_stack
 * @brief Get current level of stack
 * @param stk Stack to get from.
 * @return Current level (values which -1 (empty) <= x <
 *                        ::GEOM_SHAPE_STACK_SIZE are valid).
 */
static int
geom_shape_stack_level(struct geom_shape_stack *stk)
{
  GEOM_ASSERT(stk);
  return stk->current_stack_p;
}

struct geom_shape_args_builder
{
  const geom_shape_funcs *funcs;
  geom_args_builder *ab;
  geom_shape shape;
};

static void
geom_shape_element_init(geom_shape_element *el, geom_shape_data *parent)
{
  GEOM_ASSERT(el);
  GEOM_ASSERT(parent);

  el->data = NULL;
  el->funcs = NULL;
  el->parent = parent;
  el->op_orig  = GEOM_SOP_INVALID;
  el->op_effec = GEOM_SOP_INVALID;
  el->transform = geom_mat43_E();
  el->copied = 0;
  geom_user_defined_data_init(&el->extra_data);
  geom_list_init(&el->list);
}

geom_shape_data *geom_shape_data_new(geom_data_element *parent, geom_error *e)
{
  geom_error er;
  geom_data *master;
  geom_shape_data *d;

  GEOM_ASSERT(parent);

  master = geom_data_element_parent(parent);
  GEOM_ASSERT(master);

  d = (geom_shape_data *)malloc(sizeof(geom_shape_data));
  if (!d) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  er = geom_data_add_pointer(master, d, free);
  if (e) *e = er;
  if (er != GEOM_SUCCESS) {
    free(d);
    return NULL;
  }

  parent->shape = d;
  d->parent = parent;
  d->nsub_cell = 1;
  d->offset = geom_vec3_c(0.0, 0.0, 0.0);
  d->origin = geom_vec3_c(0.0, 0.0, 0.0);
  d->repeat = geom_svec3_c(1, 1, 1);
  geom_shape_element_init(&d->element, d);
  geom_user_defined_data_init(&d->extra_data);
  return d;
}

void geom_shape_data_delete(geom_shape_data *data)
{
  struct geom_list *n, *p;
  geom_shape_element *el;
  geom_data *master;

  if (!data) return;

  master = geom_shape_data_master(data);
  GEOM_ASSERT(master);

  geom_list_foreach_safe(p, n, &data->element.list) {
    el = geom_shape_element_entry(p);
    geom_shape_element_delete(el);
  }

  geom_user_defined_data_free(master, &data->extra_data);
  geom_data_del_pointer(master, data);
}

geom_data_element *geom_shape_data_parent(geom_shape_data *data)
{
  GEOM_ASSERT(data);

  return data->parent;
}

geom_data *geom_shape_data_master(geom_shape_data *data)
{
  GEOM_ASSERT(data);

  return geom_data_element_master(data->parent);
}

/**
 * @memberof geom_shape_data
 * @brief Copy shape elements, if condition is matched
 *
 * @param data Shape data to operate
 * @param start First element to be copied
 * @param last  Last element to be copied (inclusive)
 * @param dest  Destination to insert copied elements.
 * @param ext   Extra data to pass to \p cond function.
 * @param cond  Condition function
 * @param copied_start Sets first copied elements, if non-NULL value is given.
 * @param copied_last  Sets last copied elements, if non-NULL value is given.
 *
 * @retval ::GEOM_SUCCESS Successfully copied
 * @retval ::GEOM_ERR_NOMEM Cannot allocate memory
 * @retval ::GEOM_ERR_RANGE Invalid copy range (i.e. last is before than start)
 *
 * All of \p start, \p last and \p dest must be added item on the \p data.
 *
 * If \p cond is NULL, all element will be copied. If \p cond given
 * and the elements for \p cond returned 0, such elements will not be
 * copied.
 *
 * This function can safely copied even if \p dest is inside of
 * the chain of \p start to \p last.
 *
 * \p copied_start will be next of the \p dest.
 */
static geom_error
geom_shape_data_copy_elements(geom_shape_data *data,
                              geom_shape_element *start,
                              geom_shape_element *last,
                              geom_shape_element *dest,
                              void *ext,
                              int (*cond)(geom_shape_element *el, void *ext),
                              geom_shape_element **copied_start,
                              geom_shape_element **copied_last)
{
  geom_data *master;
  geom_shape_element *copy_head;
  geom_shape_element *p;
  geom_shape_element *n;
  geom_error e;

  GEOM_ASSERT(data);
  GEOM_ASSERT(start);
  GEOM_ASSERT(last);
  GEOM_ASSERT(dest);
  GEOM_ASSERT(data == geom_shape_element_parent(start));
  GEOM_ASSERT(data == geom_shape_element_parent(last));
  GEOM_ASSERT(data == geom_shape_element_parent(dest));
  GEOM_ASSERT(!geom_shape_element_is_head(start));
  GEOM_ASSERT(!geom_shape_element_is_head(last));

  master = geom_shape_data_master(data);
  GEOM_ASSERT(master);

  e = GEOM_SUCCESS;
  copy_head = NULL;
  last = geom_shape_element_next(last);
  for (p = start; p && p != last; p = geom_shape_element_next(p)) {
    if (cond) {
      if (cond(p, ext) == 0) continue;
    }
    n = geom_shape_element_new(data, &e);
    if (!n) goto error;
    if (!copy_head) {
      copy_head = n;
    } else {
      geom_list_insert_prev(&copy_head->list, &n->list);
    }
    n->op_orig  = p->op_orig;
    n->op_effec = p->op_effec;
    n->transform = p->transform;
    n->funcs = p->funcs;
    n->data = NULL;
    if (p->funcs) {
      n->data = geom_funcs_common_copy_data(&p->funcs->c, p->data, master, &e);
      if (p->data && !n->data) goto error;
    }
    n->copied = 1;
    geom_user_defined_data_init(&n->extra_data);
  }
  if (p != last) {
    e = GEOM_ERR_RANGE;
    goto error;
  }

  if (copy_head) {
    if (copied_start) *copied_start = copy_head;
    if (copied_last)  *copied_last  = n;
    geom_list_insert_list_next(&dest->list, &copy_head->list);
  }
  return GEOM_SUCCESS;

error:
  if (copy_head) {
    struct geom_list *lp, *ln;
    geom_list_foreach_safe(lp, ln, &copy_head->list) {
      p = geom_shape_element_entry(lp);
      geom_shape_element_delete(p);
    }
    geom_shape_element_delete(p);
  }
  return e;
}

static void
geom_shape_data_delete_copied_elements_in_region(geom_shape_data *data,
                                                 geom_shape_element *s,
                                                 geom_shape_element *e)
{
  geom_shape_element *el;
  struct geom_list *lp, *ln, *ls, *le;

  if (!s) {
    s = geom_shape_element_next(&data->element);
  }
  if (e) {
    e = geom_shape_element_next(e);
  }
  if (!e) {
    e = &data->element;
  }
  ls = &s->list;
  le = &e->list;

  geom_list_foreach_range_safe(lp, ln, ls, le) {
    el = geom_shape_element_entry(lp);
    if (el->copied) {
      geom_shape_element_delete(el);
    }
  }
}


void geom_shape_data_delete_copied_elements(geom_shape_data *data)
{
  geom_shape_element *el;
  struct geom_list *lp, *ln;

  geom_list_foreach_safe(lp, ln, &data->element.list) {
    el = geom_shape_element_entry(lp);
    if (el->copied) {
      geom_shape_element_delete(el);
    }
  }
}

static int
geom_shape_data_filter_body(geom_shape_element *el, void *ext)
{
  if (!el->funcs) return 0;
  if (el->funcs->shape_type == GEOM_SHPT_BODY) return 1;
  if (el->funcs->enum_val == GEOM_SHAPE_COMB) return 1;
  return 0;
}

static geom_shape_element *
geom_shape_data_create_copied_comb(geom_shape_data *data,
                                   geom_shape_element *dest,
                                   geom_shape_operator op, geom_error *e)
{
  geom_shape_element *el;

  el = geom_shape_element_new(data, e);
  if (!el) return NULL;

  el->funcs = geom_shape_comb;
  el->copied = 1;
  el->data = NULL;
  el->op_orig = op;
  el->op_effec = op;

  /* We assume COMB does not define allocator. see shp_specials.c */
  GEOM_ASSERT(!el->funcs->c.allocator);

  geom_list_insert_next(&dest->list, &el->list);

  return el;
}

static geom_error
geom_shape_data_copy_by_tf_simple(geom_shape_data *data,
                                  geom_shape_element *transform_el,
                                  geom_shape_element *target,
                                  int ncopy)
{
  geom_error e;
  geom_shape_element *cps;
  geom_shape_element *cpe;
  geom_shape_element *el;
  geom_shape_operator sop;
  geom_mat43 mat;
  int icopy;

  GEOM_ASSERT(data);
  GEOM_ASSERT(target);
  GEOM_ASSERT(transform_el);
  GEOM_ASSERT(transform_el->funcs);
  GEOM_ASSERT(transform_el->funcs->shape_type == GEOM_SHPT_TRANS);

  GEOM_ASSERT(ncopy > 0);
  GEOM_ASSERT(target->parent == data);
  GEOM_ASSERT(target->funcs);
  GEOM_ASSERT(target->funcs->shape_type == GEOM_SHPT_BODY);

  e = GEOM_SUCCESS;

  cps = NULL;
  cpe = target;
  mat = transform_el->transform;
  for (icopy = 0; icopy < ncopy; ++icopy) {
    e = geom_shape_data_copy_elements(data, target, target, cpe, NULL,
                                      NULL, NULL, &cpe);
    if (e != GEOM_SUCCESS) break;
    if (!cps) cps = cpe;
    cpe->op_effec  = transform_el->op_effec;
    cpe->op_orig   = transform_el->op_effec;
    cpe->transform = geom_mat43_mul(mat, cpe->transform);
    mat = geom_mat43_mul(mat, transform_el->transform);
  }
  do {
    if (e != GEOM_SUCCESS) break;
    sop = target->op_effec;
    if (sop != GEOM_SOP_PUSH && sop != GEOM_SOP_SET) {
      target->op_effec = GEOM_SOP_PUSH;
      el = geom_shape_data_create_copied_comb(data, cpe, sop, &e);
      if (e != GEOM_SUCCESS) break;
      cpe = el;
    }
  } while(0);
  if (e != GEOM_SUCCESS) {
    /* To achive to be an atomic operation, revert the modification. */
    geom_shape_data_delete_copied_elements_in_region(data, cps, cpe);
    return e;
  }
  return e;
}

static geom_error
geom_shape_data_copy_by_tf_combined(geom_shape_data *data,
                                    geom_shape_element *transform_el,
                                    geom_shape_element *target_start,
                                    geom_shape_element *target_end,
                                    int ncopy, int isclosed)
{
  geom_error e;
  geom_shape_element *cpos;
  geom_shape_element *cps;
  geom_shape_element *cpe;
  geom_shape_element *el;
  int icopy;
  geom_shape_operator sop;
  geom_mat43 mat;

  GEOM_ASSERT(data);
  GEOM_ASSERT(target_start);
  GEOM_ASSERT(target_end);
  GEOM_ASSERT(ncopy > 0);

  GEOM_ASSERT(target_start->funcs);
  GEOM_ASSERT(target_end->funcs);

  /* If a combined body, the first element must be op of SET/PUSH */
  GEOM_ASSERT(target_start->op_effec == GEOM_SOP_SET ||
              target_start->op_effec == GEOM_SOP_PUSH);

  e = GEOM_SUCCESS;

  cpos = NULL;
  cpe = target_end;
  if (isclosed && cpe->funcs->enum_val == GEOM_SHAPE_COMB) {
    /* insert point and copy end set to before COMB */
    target_end = geom_shape_element_prev(target_end);
    GEOM_ASSERT(target_end);
    cpe = target_end;
  }

  mat = transform_el->transform;
  sop = transform_el->op_effec;

  for (icopy = 0; icopy < ncopy; ++icopy) {
    e = geom_shape_data_copy_elements(data, target_start, target_end, cpe,
                                      NULL, geom_shape_data_filter_body,
                                      &cps, &cpe);
    if (e != GEOM_SUCCESS) break;
    if (!cpos) cpos = cps;

    el = geom_shape_data_create_copied_comb(data, cpe, sop, &e);
    if (e != GEOM_SUCCESS) break;

    cpe = el;
    for (el = cps; el && el != cpe; el = geom_shape_element_next(el)) {
      if (el->funcs->shape_type == GEOM_SHPT_BODY) {
        el->transform = geom_mat43_mul(mat, el->transform);
      }
    }
    mat = geom_mat43_mul(mat, transform_el->transform);
  }
  if (e != GEOM_SUCCESS) {
    /* To achive to be an atomic operation, revert the modification. */
    geom_shape_data_delete_copied_elements_in_region(data, cpos, cpe);
    return e;
  }
  return e;
}

/**
 * @memberof geom_shape_data
 * @private
 * @brief Calls error callback function if necessary.
 *
 * @param error Error value.
 * @param data  Shape data which errors occured
 * @param el    Shape element which errors occured, or NULL if not applicable.
 * @param callback call back function.
 * @param ext_data External data to pass call back function
 * @param set_err  Set error if given
 *
 * @return return value of callback function, 0 if callback function
 *         is not called (no callback function given, error is GEOM_SUCCESS)
 */
static int
geom_shape_data_call_error(geom_error error,
                           geom_shape_data *data, geom_shape_element *el,
                           geom_shape_data_error_callback *callback,
                           void *ext_data, geom_error *set_err,
                           geom_shape_element **error_element)
{
  GEOM_ASSERT(data);
  if (error == GEOM_SUCCESS) return 0;
  if (set_err) *set_err = error;
  if (error_element) *error_element = el;
  if (!callback) return 0;
  return callback(error, data, el, ext_data);
}

geom_error
geom_shape_data_update_all_transform(geom_shape_data *data,
                                     geom_shape_data_error_callback *callback,
                                     void *ext_data,
                                     geom_shape_element **error_element)
{
  geom_shape_element *el;
  geom_shape_element *first_body;
  geom_shape_element *tf_start_el;
  geom_shape_element *tf_end_el;
  geom_error last_error;
  geom_error curr_error;
  geom_error stack_error;
  geom_error e;
  struct geom_shape_stack cmb_stack;
  struct geom_shape_stack gst_stack;

  GEOM_ASSERT(data);

  last_error = GEOM_SUCCESS;
  curr_error = GEOM_SUCCESS;
  stack_error = GEOM_SUCCESS;

  el = geom_shape_data_get_element(data);
  if (!el) return last_error;

  geom_shape_stack_init(&cmb_stack);
  geom_shape_stack_init(&gst_stack);

  geom_shape_data_delete_copied_elements(data);

  first_body = NULL;
  tf_start_el = NULL;
  tf_end_el = NULL;

  /* Reset effective operator */
  el = geom_shape_data_get_element(data);
  for (; el; el = geom_shape_element_next(el)) {
    el->op_effec = el->op_orig;
  }

  el = geom_shape_data_get_element(data);
  for (; el; el = geom_shape_element_next(el)) {
    if (!el->funcs) {
      if (geom_shape_data_call_error(GEOM_ERR_SHAPE_NOT_SET, data, el, callback,
                                     ext_data, &curr_error, error_element)) {
        break;
      }
      continue;
    }
    if (el->funcs->shape_type == GEOM_SHPT_BODY) {
      e = GEOM_SUCCESS;
      if (!first_body) {
        first_body = el;
        if (el->op_orig != GEOM_SOP_SET && el->op_orig != GEOM_SOP_PUSH) {
          if (geom_shape_data_call_error(GEOM_ERR_SHAPE_OP_SHOULD_SET, data, el,
                                         callback, ext_data, &curr_error,
                                         error_element))
            break;
          first_body->op_effec = GEOM_SOP_SET;
        }
      }
      if (el->op_effec == GEOM_SOP_PUSH ||
          el->op_effec == GEOM_SOP_SET) {
        e = geom_shape_stack_push(&cmb_stack, el);
      }
      if (geom_shape_data_call_error(e, data, el, callback, ext_data,
                                     &curr_error, error_element))
        break;
      if (e == GEOM_ERR_SHAPE_STACK_OVERFLOW) {
        stack_error = e;
      }

      /* transformations always applied later */
      el->transform = geom_mat43_E();
      tf_start_el = el;
      tf_end_el = el;

    } else if (el->funcs->shape_type == GEOM_SHPT_TRANS) {
      double det;

      /* iterator */
      geom_shape_element *elcur;

      /* First and Last body (or COMB) inside side the copy region */
      geom_shape_element *start_body, *end_body;

      /* First and Last body (or COMB) currently processing group */
      geom_shape_element *tfs, *tfe;

      /* To insert copies after tfe, keep next of the end body */
      struct geom_list *tf_end_el_next; /* Next beyond the tf_end_el */
      struct geom_list *end_body_next;  /* Next beyond the end_body */
      struct geom_list *tfen;           /* Next beyond the tfe */

      start_body = NULL;
      end_body = NULL;
      tf_end_el_next = NULL;
      end_body_next = NULL;
      tfen = NULL;

      GEOM_ASSERT(el->funcs->transform_func);
      GEOM_ASSERT(el->data);

      el->transform = el->funcs->transform_func(el->data);
      det = geom_mat43_det(el->transform);
      if (fabs(det) <= 1.0e-10) {
        if (geom_shape_data_call_error(GEOM_ERR_SINGULAR_TRANSFORM, data, el,
                                       callback, ext_data, &curr_error,
                                       error_element))
          break;
      }

      if (stack_error != GEOM_SUCCESS) continue;
      if (last_error  == GEOM_ERR_NOMEM) continue;

      /* Find body elements. */
      if (tf_start_el && tf_end_el) {
        tfe = geom_shape_element_next(tf_end_el);
        for (elcur = tf_start_el; elcur != tfe;
             elcur = geom_shape_element_next(elcur)) {
          GEOM_ASSERT(elcur);
          /* Checked by outer loop. */
          GEOM_ASSERT(elcur->funcs);
          if (elcur->funcs->shape_type == GEOM_SHPT_BODY ||
              elcur->funcs->enum_val == GEOM_SHAPE_COMB) {
            if (!start_body) start_body = elcur;
            end_body = elcur;
          }
        }
        /* These locations will not change */
        tf_end_el_next = geom_list_next(&tf_end_el->list);
        if (end_body) {
          end_body_next  = geom_list_next(&end_body->list);
        }
      }
      if (start_body && end_body) {
        int ncopy;
        int breakflg;
        ncopy = geom_shape_element_get_transformation_copy_n(el);
        GEOM_ASSERT(ncopy >= 0);

        breakflg = 0;

        tfs = start_body;
        for(;;) {
          /* Stack count (used to check COMB) */
          int stkcnt;

          /* Skip to next body shape */
          while (tfs != end_body) {
            GEOM_ASSERT(tfs);
            if (tfs->funcs->shape_type == GEOM_SHPT_BODY ||
                tfs->funcs->enum_val == GEOM_SHAPE_COMB) break;
            tfs = geom_shape_element_next(tfs);
          }
          GEOM_ASSERT(tfs);

          /* Found COMB without PUSH */
          if (tfs->funcs->enum_val == GEOM_SHAPE_COMB) {
            breakflg = geom_shape_data_call_error(GEOM_ERR_COMB_WITHOUT_PUSH,
                                                  data, tfs, callback, ext_data,
                                                  &curr_error, error_element);
            stack_error = curr_error;
            break;
          }

          stkcnt = 0;
          /* If PUSH/SET is used, find corresponding COMB. */
          tfe = tfs;
          if (tfs->op_effec == GEOM_SOP_SET ||
              tfs->op_effec == GEOM_SOP_PUSH) {
            stkcnt = 0;
            while (1) {
              if (tfe->funcs->shape_type == GEOM_SHPT_BODY) {
                if (tfe->op_effec == GEOM_SOP_SET ||
                    tfe->op_effec == GEOM_SOP_PUSH) {
                  stkcnt++;
                }
              } else {
                if (tfe->funcs->enum_val == GEOM_SHAPE_COMB) {
                  stkcnt--;
                  /* Corresponding COMB is present.*/
                  if (stkcnt <= 0) break;
                }
              }
              if (tfe == end_body) break;
              tfe = geom_shape_element_next(tfe);
            }

            if (stkcnt != 0 && stkcnt != 1) {
              breakflg =
                geom_shape_data_call_error(GEOM_ERR_INVALID_STRUCTURE, data,
                                           tf_end_el, callback, ext_data,
                                           &curr_error, error_element);
              stack_error = curr_error;
              break;
            }
          }

          tfen = geom_list_next(&tfe->list);

          if (ncopy == 0) {
            geom_shape_element *ep;
            if (el->op_orig != GEOM_SOP_SET) {
              breakflg =
                geom_shape_data_call_error(GEOM_ERR_SHAPE_OP_SHOULD_SET, data,
                                           el, callback, ext_data, &curr_error,
                                           error_element);
            }

            ep = geom_shape_element_entry(tfen);
            for (elcur = tfs; elcur != ep;
                 elcur = geom_shape_element_next(elcur)) {
              GEOM_ASSERT(elcur);
              if (elcur->funcs->shape_type == GEOM_SHPT_BODY) {
                elcur->transform = geom_mat43_mul(el->transform,
                                                  elcur->transform);
              }
            }
          } else {
            if (el->op_orig == GEOM_SOP_SET || el->op_orig == GEOM_SOP_PUSH) {
              breakflg =
                geom_shape_data_call_error(GEOM_ERR_INVALID_SHAPE_OP, data, el,
                                           callback, ext_data, &curr_error,
                                           error_element);
            }

            if (tfs == tfe) {
              e = geom_shape_data_copy_by_tf_simple(data, el, tfs, ncopy);
            } else {
              e = geom_shape_data_copy_by_tf_combined(data, el, tfs, tfe,
                                                      ncopy, (stkcnt == 0));
            }
            if (e != GEOM_SUCCESS) {
              breakflg =
                geom_shape_data_call_error(e, data, el, callback, ext_data,
                                           &curr_error, error_element);
              break;
            }
          }

          /* This is the last body */
          if (tfen == end_body_next) break;
          tfe = geom_shape_element_entry(tfen);
          tfs = tfe;
        }
        if (breakflg) break;

        if (tf_end_el_next) {
          tf_end_el = geom_shape_element_entry(geom_list_prev(tf_end_el_next));
        }

      } else {
        if (geom_shape_data_call_error(GEOM_ERR_NO_SHAPES_TO_TRANSFORM, data,
                                       el, callback, ext_data, &curr_error,
                                       error_element))
          break;
      }

    } else {
      e = GEOM_ERR_INVALID_SHAPE;
      tf_start_el = NULL;
      tf_end_el = NULL;

      if (el->funcs->enum_val == GEOM_SHAPE_COMB) {
        if (el->op_orig == GEOM_SOP_SET || el->op_orig == GEOM_SOP_PUSH) {
          if (geom_shape_data_call_error(GEOM_ERR_INVALID_SHAPE_OP, data, el,
                                         callback, ext_data, &curr_error,
                                         error_element))
            break;
        }

        tf_start_el = geom_shape_stack_pop(&cmb_stack, &e);
        tf_end_el = el;

        if (geom_shape_stack_level(&cmb_stack) == -1) {
          e = GEOM_ERR_SHAPE_STACK_UNDERFLOW;
          tf_start_el = NULL;
          tf_end_el  = NULL;
        }

      } else if (el->funcs->enum_val == GEOM_SHAPE_GST) {
        if (el->op_orig != GEOM_SOP_SET) {
          if (geom_shape_data_call_error(GEOM_ERR_SHAPE_OP_SHOULD_SET, data, el,
                                         callback, ext_data, &curr_error,
                                         error_element))
            break;
        }
        e = geom_shape_stack_push(&gst_stack, el);
        if (e == GEOM_ERR_SHAPE_STACK_OVERFLOW) {
          e = GEOM_ERR_GROUP_STACK_OVERFLOW;
        }

      } else if (el->funcs->enum_val == GEOM_SHAPE_GED) {
        if (el->op_orig != GEOM_SOP_SET) {
          if (geom_shape_data_call_error(GEOM_ERR_SHAPE_OP_SHOULD_SET, data, el,
                                         callback, ext_data, &curr_error,
                                         error_element))
            break;
        }

        tf_start_el = geom_shape_stack_pop(&gst_stack, &e);
        tf_end_el = el;

        if (e == GEOM_ERR_SHAPE_STACK_UNDERFLOW) {
          e = GEOM_ERR_GROUP_STACK_UNDERFLOW;
        }
      }
      if (e != GEOM_SUCCESS) {
        if (geom_shape_data_call_error(e, data, el, callback, ext_data,
                                       &curr_error, error_element))
          break;
        switch (e) {
        case GEOM_ERR_SHAPE_STACK_OVERFLOW:
        case GEOM_ERR_SHAPE_STACK_UNDERFLOW:
          stack_error = e;
          break;
        default:
          /* nop */
          break;
        }
      }
    }
    if (last_error != GEOM_ERR_NOMEM) last_error = curr_error;
  }
  if (!first_body) {
    if (geom_shape_data_call_error(GEOM_ERR_NO_BODY_SHAPES, data, NULL,
                                   callback, ext_data, &curr_error,
                                   error_element)) {
      return last_error;
    }
  }

  if (last_error != GEOM_ERR_NOMEM) last_error = curr_error;

  if (curr_error == GEOM_SUCCESS && stack_error == GEOM_SUCCESS) {
    int f;
    f = 0;
    while (geom_shape_stack_level(&cmb_stack) >= 1) {
      el = geom_shape_stack_pop(&cmb_stack, NULL);
      f = geom_shape_data_call_error(GEOM_ERR_SHAPE_STACK_UNCLOSED, data, el,
                                     callback, ext_data, &curr_error,
                                     error_element);
      if (stack_error == GEOM_SUCCESS)
        last_error = curr_error;
      if (f)
        break;
    }
    while (!f && geom_shape_stack_level(&gst_stack) >= 0) {
      el = geom_shape_stack_pop(&gst_stack, NULL);
      f = geom_shape_data_call_error(GEOM_ERR_GROUP_STACK_UNCLOSED, data, el,
                                     callback, ext_data, &curr_error,
                                     error_element);
      if (stack_error == GEOM_SUCCESS)
        last_error = curr_error;
      if (f)
        break;
    }
    if (stack_error != GEOM_SUCCESS && last_error != GEOM_ERR_NOMEM) {
      last_error = stack_error;
    }
  }

  if (last_error == GEOM_SUCCESS) {
    geom_shape_stack_init(&cmb_stack);

    el = geom_shape_data_get_element(data);
    for (; el; el = geom_shape_element_next(el)) {
      if (el->funcs->enum_val == GEOM_SHAPE_COMB) {
        geom_shape_stack_pop(&cmb_stack, NULL);
      } else if (el->funcs->shape_type == GEOM_SHPT_BODY) {
        if (el->op_effec == GEOM_SOP_SET ||
            el->op_effec == GEOM_SOP_PUSH) {
          last_error = geom_shape_stack_push(&cmb_stack, el);
          if (last_error != GEOM_SUCCESS) {
            geom_shape_data_call_error(last_error, data, el, callback, ext_data,
                                       &last_error, error_element);
            break;
          }
        }
      }
    }
  }

  return last_error;
}

void geom_shape_data_set_offsetv(geom_shape_data *data, geom_vec3 offset)
{
  GEOM_ASSERT(data);

  data->offset = offset;
}

void geom_shape_data_set_originv(geom_shape_data *data, geom_vec3 origin)
{
  GEOM_ASSERT(data);

  data->origin = origin;
}

void geom_shape_data_set_repeatv(geom_shape_data *data, geom_svec3 repeat)
{
  GEOM_ASSERT(data);

  data->repeat = repeat;
}

void geom_shape_data_set_nsub_cell(geom_shape_data *data, int nsub_cell)
{
  GEOM_ASSERT(data);

  data->nsub_cell = nsub_cell;
}

geom_vec3 geom_shape_data_get_offset(geom_shape_data *data)
{
  GEOM_ASSERT(data);

  return data->offset;
}

geom_vec3 geom_shape_data_get_origin(geom_shape_data *data)
{
  GEOM_ASSERT(data);

  return data->origin;
}

geom_svec3 geom_shape_data_get_repeat(geom_shape_data *data)
{
  GEOM_ASSERT(data);

  return data->repeat;
}

int geom_shape_data_get_nsub_cell(geom_shape_data *data)
{
  GEOM_ASSERT(data);

  return data->nsub_cell;
}

static int
geom_shape_data_boolean_calc(geom_shape_operator op, int a, int b)
{
  switch(op) {
  case GEOM_SOP_OR:
  case GEOM_SOP_ADD:
    if (a || b) return 1;
    return 0;
  case GEOM_SOP_AND:
  case GEOM_SOP_MUL:
    if (a && b) return 1;
    return 0;
  case GEOM_SOP_SUB:
    if (a && b) return 0;
    return a;
  case GEOM_SOP_XOR:
    /* !!a may not equal to a: ensures 0 or 1 */
    if ((!!a) != (!!b)) return 1;
    return 0;
  case GEOM_SOP_SET:
  case GEOM_SOP_PUSH:
    return b;
  case GEOM_SOP_INVALID:
    GEOM_UNREACHABLE();
    return 0;
  }
}

static geom_vec3 geom_shape_data_get_element_world_offset(geom_shape_data *data,
                                                          geom_svec3 repeat_idx)
{
  geom_vec3 vv;

  GEOM_ASSERT(data);

  vv = geom_vec3_c(geom_svec3_x(repeat_idx), geom_svec3_y(repeat_idx),
                   geom_svec3_z(repeat_idx));
  return geom_vec3_mul_each_element(vv, data->offset);
}

/**
 * Calculate the point in the world of each shape element.
 */
static geom_vec3 geom_shape_data_get_element_world_p(geom_shape_data *data,
                                                     geom_svec3 repeat_idx,
                                                     geom_vec3 pnt)
{
  geom_vec3 vv;

  vv = geom_shape_data_get_element_world_offset(data, repeat_idx);
  return geom_vec3_sub(geom_vec3_sub(pnt, data->origin), vv);
}

static int geom_shape_element_inbody_test_at(geom_shape_element *element,
                                             geom_vec3 pnt)
{
  geom_mat43 mat;
  geom_vec3 start, end;
  double xx, yy, zz;

  if (geom_shape_element_get_shape_type(element) != GEOM_SHPT_BODY)
    return 0;

  GEOM_ASSERT(element);
  GEOM_ASSERT(element->funcs);
  GEOM_ASSERT(element->funcs->body_testf);

  mat = geom_mat43_inv(element->transform);
  pnt = geom_mat43_mul_column_vec3(mat, pnt);
  xx = geom_vec3_x(pnt);
  yy = geom_vec3_y(pnt);
  zz = geom_vec3_z(pnt);

  if (element->funcs->body_bboxf) {
    element->funcs->body_bboxf(element->data, &start, &end);
    if (xx < geom_vec3_x(start) || xx > geom_vec3_x(end))
      return 0;
    if (yy < geom_vec3_y(start) || yy > geom_vec3_y(end))
      return 0;
    if (zz < geom_vec3_z(start) || zz > geom_vec3_z(end))
      return 0;
  }
  return element->funcs->body_testf(element->data, xx, yy, zz);
}

/*
 * Skip until body shape or COMB
 */
static geom_shape_element *
geom_shape_data_ff_for_body(geom_shape_element *element)
{
  for (; element; element = geom_shape_element_next(element)) {
    if (geom_shape_element_get_shape_type(element) == GEOM_SHPT_BODY)
      break;
    if (geom_shape_element_get_shape(element) == GEOM_SHAPE_COMB)
      break;
  }
  return element;
}

static geom_shape_element *
geom_shape_data_find_matching_comb(geom_shape_element *element)
{
  int first;
  first = 1;

  element = geom_shape_data_ff_for_body(element);
  if (!element)
    return element;

  for (; element; element = geom_shape_element_next(element)) {
    geom_shape shape;
    geom_shape_type type;
    geom_shape_operator op;

    op = geom_shape_element_effective_operator(element);
    type = geom_shape_element_get_shape_type(element);
    shape = geom_shape_element_get_shape(element);

    if (type == GEOM_SHPT_BODY) {
      if (!first && (op == GEOM_SOP_SET || op == GEOM_SOP_PUSH)) {
        element = geom_shape_data_find_matching_comb(element);
        if (!element)
          break;
      } else {
        first = 0;
      }
    }
    if (shape == GEOM_SHAPE_COMB) {
      break;
    }
  }
  return element;
}

/**
 * @brief Do inbody test in a branch (PUSH to corresponding COMB) or given end.
 * @param element Start element
 * @param end_element End element (corresponding COMB if NULL)
 * @param pnt Point to calculate
 * @param comb_element Sets matched COMB element if non-NULL has given
 * @return result
 *
 * This function does short-circuit evaluation.
 *
 * @warning For passing @p end_element, it must be in the same branch in
 *          @p element. Otherwise it will never hit.
 */
static int geom_shape_data_inbody_test_in_branch(
  geom_shape_element *element, geom_shape_element *end_element, geom_vec3 pnt,
  geom_shape_element **comb_element)
{
  int first;
  int result;
  geom_shape_element *c_element;
  geom_shape_element *n_end;

  if (end_element)
    n_end = geom_shape_element_next(end_element);

  first = 1;
  result = 0;
  for (; element && element != n_end;
       element = geom_shape_element_next(element)) {
    geom_shape shape;
    geom_shape_type type;
    geom_shape_operator op;
    int test;

    shape = geom_shape_element_get_shape(element);
    type = geom_shape_element_get_shape_type(element);
    op = geom_shape_element_effective_operator(element);

    if (type == GEOM_SHPT_BODY) {
      if (!first && (op == GEOM_SOP_SET || op == GEOM_SOP_PUSH)) {
        geom_shape_operator comb_op;
        int need_to_dive;

        c_element = geom_shape_data_find_matching_comb(element);
        if (!c_element) {
          element = c_element;
          break;
        }

        comb_op = geom_shape_element_effective_operator(c_element);
        need_to_dive = 0;
        switch (comb_op) {
        case GEOM_SOP_AND:
        case GEOM_SOP_MUL:
        case GEOM_SOP_SUB:
          if (result)
            need_to_dive = 1;
          break;
        case GEOM_SOP_XOR:
          need_to_dive = 1;
          break;
        case GEOM_SOP_OR:
        case GEOM_SOP_ADD:
          if (!result)
            need_to_dive = 1;
          break;
        case GEOM_SOP_SET:
        case GEOM_SOP_PUSH:
        case GEOM_SOP_INVALID:
          GEOM_UNREACHABLE();
          break;
        }
        if (need_to_dive) {
          test =
            geom_shape_data_inbody_test_in_branch(element, NULL, pnt, NULL);
          result = geom_shape_data_boolean_calc(comb_op, result, test);
        }

        element = c_element;
      } else {
        test = geom_shape_element_inbody_test_at(element, pnt);
        if (first) {
          result = test;
          first = 0;
        } else {
          result = geom_shape_data_boolean_calc(op, result, test);
        }
      }
    }
    if (shape == GEOM_SHAPE_COMB)
      break;
  }
  if (comb_element)
    *comb_element = element;
  return result;
}

int geom_shape_data_inbody_test_at(geom_shape_data *data,
                                   double x, double y, double z)
{
  int ret;
  int sstack[GEOM_SHAPE_STACK_SIZE];
  int scur;
  int test;
  geom_range3 rnge;
  geom_svec3 it;
  geom_shape_element *el;
  geom_vec3 vo, vv;

  GEOM_ASSERT(data);

  scur = 0;
  vo = geom_vec3_c(x, y, z);
  rnge = geom_range3_c_vec(geom_svec3_c(0, 0, 0), data->repeat, 0);
  ret = 0;

  /* We do not OpenMP parallelization here. May be done at higher level. */
  geom_range3_foreach(it, rnge, geom_svec3_c(1, 1, 1)) {
    vv = geom_shape_data_get_element_world_p(data, it, vo);
    el = geom_shape_data_get_element(data);
    for (scur = 0; scur < GEOM_SHAPE_STACK_SIZE; ++scur) {
      sstack[scur] = 0;
    }

    scur = -1;

    for (; el; el = geom_shape_element_next(el)) {
      geom_shape_type t;
      geom_shape shape;
      geom_shape_operator sop;

      sop = geom_shape_element_effective_operator(el);
      shape = geom_shape_element_get_shape(el);
      t = geom_shape_element_get_shape_type(el);

      if (t == GEOM_SHPT_BODY) {
        test = geom_shape_element_inbody_test_at(el, vv);

        if (scur < 0 || sop == GEOM_SOP_SET || sop == GEOM_SOP_PUSH) {
          sstack[++scur] = test;
        } else {
          sstack[scur] = geom_shape_data_boolean_calc(sop, sstack[scur], test);
        }
      } else if (shape == GEOM_SHAPE_COMB) {
        if (scur > 0) {
          int a, b;
          b = sstack[scur];
          a = sstack[--scur];
          sstack[scur] = geom_shape_data_boolean_calc(sop, a, b);
        }
      }
    }

    if (sstack[0] > 0) {
      ret = 1;
      break;
    }
  }
  return ret;
}

int geom_shape_data_has_surface(geom_shape_data *data)
{
  geom_shape_element *el;

  GEOM_ASSERT(data);

  el = geom_shape_data_get_element(data);
  for (; el; el = geom_shape_element_next(el)) {
    if (geom_shape_element_n_surface(el) > 0)
      return 1;
  }
  return 0;
}

int geom_shape_data_has_enabled_surface(geom_shape_data *data)
{
  geom_shape_element *el;

  GEOM_ASSERT(data);

  el = geom_shape_data_get_element(data);
  for (; el; el = geom_shape_element_next(el)) {
    if (geom_shape_element_n_enabled_surface(el) > 0)
      return 1;
  }
  return 0;
}

static geom_error
geom_shape_element_nearest_surface(geom_shape_element *element, geom_vec3 xyz,
                                   int *surfid, geom_vec2 *uv, geom_vec3 *surfp,
                                   geom_vec3 *normv)
{
  int nsurf, isurf, first, min_surf;
  geom_mat43 mat;
  geom_vec3 pnt, msurfp, mnormv;
  geom_vec2 muv;
  double lmin;

  GEOM_ASSERT(element);
  GEOM_ASSERT(element->funcs);

  nsurf = geom_shape_element_n_surface(element);
  if (nsurf < 0) {
    return GEOM_ERR_NO_SURFACE_IN_SHAPE;
  }

  GEOM_ASSERT(element->funcs->body_wrapf);
  GEOM_ASSERT(element->funcs->body_unwrapf);

  mat = geom_mat43_inv(element->transform);
  pnt = geom_mat43_mul_column_vec3(mat, xyz);

  first = 1;
  for (isurf = 0; isurf < nsurf; ++isurf) {
    geom_vec3 s, n;
    geom_vec2 u;
    double l;

    if (!geom_shape_element_is_surface_enabled(element, isurf))
      continue;

    if (element->funcs->body_unwrapf(element->data, pnt, isurf, &u))
      continue;

    if (element->funcs->body_wrapf(element->data, u, isurf, &s, &n))
      continue;

    l = geom_vec3_length(geom_vec3_sub(pnt, s));
    if (first || lmin > l) {
      lmin = l;
      muv = u;
      msurfp = s;
      mnormv = n;
      min_surf = isurf;
    }

    first = 0;
  }

  if (first)
    return GEOM_ERR_NO_ENABLED_SURFACE;

  if (surfid)
    *surfid = min_surf;
  if (uv)
    *uv = muv;
  if (surfp)
    *surfp = geom_mat43_mul_column_vec3(element->transform, msurfp);
  if (normv) {
    /**
     * References:
     *  - http://www.songho.ca/opengl/gl_normaltransform.html
     *  - https://www.cs.upc.edu/~robert/teaching/idi/normalsOpenGL.pdf
     */
    geom_mat43 normm;
    double l;
    /* mat is (element->transform)^(-1), and discards translation part */
    normm = geom_mat44_submat43(geom_mat43_transpose(mat));
    mnormv = geom_mat43_mul_column_vec3(normm, mnormv);
    l = geom_vec3_length(mnormv);
    *normv = geom_vec3_factor(mnormv, 1.0 / l);
  }

  return GEOM_SUCCESS;
}

int geom_shape_data_nearest_surface_default_iter_count(void)
{
  return 5;
}

double geom_shape_data_nearest_surface_default_iter_criterion(void)
{
  return 1.0e-6;
}

struct geom_shape_data_nearest_surface_data
{
  geom_shape_element *element;
  geom_vec2 uv;
  geom_vec3 surfp;
  geom_vec3 normv;
  double lp;
};
typedef struct geom_shape_data_nearest_surface_data
  geom_shape_data_nearest_surface_data;

/**
 * @note @p a_surfp must be in the coordinate of @p b_element.
 * @note @p b_surfp must be in the coordinate of @p a_element.
 */
static void geom_shape_data_nearest_surface_get_merge_use(
  geom_shape_element *a_element, geom_shape_element *a_end_element,
  geom_shape_element *b_element, geom_shape_element *b_end_element,
  geom_vec3 a_surfp, geom_vec3 b_surfp, geom_shape_operator sop,
  int *use_a, int *use_b)
{
  int a_surfp_is_in_b, b_surfp_is_in_a;

  GEOM_ASSERT(use_a);
  GEOM_ASSERT(use_b);

  *use_a = 1;
  *use_b = 1;

  switch (sop) {
  case GEOM_SOP_PUSH:
  case GEOM_SOP_SET:
  case GEOM_SOP_INVALID:
    GEOM_UNREACHABLE();
    return;

  case GEOM_SOP_XOR:
    /* always both surfaces are valid */
    return;

  case GEOM_SOP_MUL:
  case GEOM_SOP_AND:
  case GEOM_SOP_OR:
  case GEOM_SOP_ADD:
  case GEOM_SOP_SUB:
    break;
  }

  a_surfp_is_in_b =
    geom_shape_data_inbody_test_in_branch(b_element, b_end_element,
                                          a_surfp, NULL);

  b_surfp_is_in_a =
    geom_shape_data_inbody_test_in_branch(a_element, a_end_element,
                                          b_surfp, NULL);

  if (sop == GEOM_SOP_ADD || sop == GEOM_SOP_OR) {
    /* Invalid if surface point is inside of opposite  shape(s) */
    if (a_surfp_is_in_b)
      *use_a = 0;
    if (b_surfp_is_in_a)
      *use_b = 0;
  } else if (sop == GEOM_SOP_MUL || sop == GEOM_SOP_AND) {
    /* Invalid if surface point is outside of opposite shape(s) */
    if (!a_surfp_is_in_b)
      *use_a = 0;
    if (!b_surfp_is_in_a)
      *use_b = 0;
  } else {
    GEOM_ASSERT(sop == GEOM_SOP_SUB);
    /*
     * - Invalid if a_surfp is inside of shapes in branch of b_element
     * - Invalid if b_surfp is outside of shapes in branch of a_element
     */
    if (a_surfp_is_in_b)
      *use_a = 0;
    if (!b_surfp_is_in_a)
      *use_b = 0;
  }
}

static geom_error geom_shape_data_nearest_surface_in_branch(
  geom_shape_element *element, geom_shape_element *end_element,
  geom_shape_element **comb_element, geom_vec3 xyz, int iter_count,
  double iter_crit, geom_shape_data_nearest_surface_data *d);

/**
 * @brief Merges computed nearest surface result of combination branches, and
 *        recalculate if necessary.
 * @param a_element Start element of branch A.
 * @param a_end_element End element of branch A (NULL if corresponding COMB)
 * @param b_element End element of branch B.
 * @param b_end_element End element of branch B (NULL if corresponding COMB)
 * @param a_pnt Point to calculate (in coordinate of branch A)
 * @param sop Shape operation that applied to branch A and B.
 * @param iter_count Number of maximum iteration
 * @param iter_crit Maximum allowable inaccuracy
 * @param transform_func_a_to_b function to translate point in A to B.
 * @param transform_func_b_to_a function to translate point in B to A.
 * @param tffunc_data Extra data that should be passed to transform funcs
 *
 * @retval GEOM_SUCCESS Returning accurate result (did not iterate)
 * @retval GEOM_ERR_INACCURATE_SURFACE Returning inaccurate result (iterated)
 * @retval ... Other errors
 *
 * Merged result will be stored in @p a_data.
 */
static geom_error geom_shape_data_nearest_surface_merge(
  geom_shape_element *a_element, geom_shape_element *a_end_element,
  geom_shape_element *b_element, geom_shape_element *b_end_element,
  geom_vec3 a_pnt, geom_shape_operator sop, int iter_count, double iter_crit,
  geom_vec3 (*transform_func_a_to_b)(void *data, geom_vec3 pnt),
  geom_vec3 (*transform_func_b_to_a)(void *data, geom_vec3 pnt),
  void *tffunc_data, geom_shape_data_nearest_surface_data *a_data,
  geom_shape_data_nearest_surface_data *b_data)
{
  geom_vec3 b_pnt;
  geom_vec3 a_surfp, b_surfp;
  geom_vec3 a_surfp_in_b, b_surfp_in_a;
  geom_error a_err, b_err;
  int use_a, use_b;

  GEOM_ASSERT(a_element);
  GEOM_ASSERT(b_element);
  GEOM_ASSERT(a_data);
  GEOM_ASSERT(b_data);

  use_a = 0;
  use_b = 0;
  if (a_data->element) {
    use_a = 1;

    a_surfp = a_data->surfp;
    a_surfp_in_b = a_surfp;
    if (transform_func_a_to_b)
      a_surfp_in_b = transform_func_a_to_b(tffunc_data, a_surfp_in_b);
  }

  if (b_data->element) {
    use_b = 1;

    b_surfp = b_data->surfp;
    b_surfp_in_a = b_surfp;
    if (transform_func_b_to_a)
      b_surfp_in_a = transform_func_b_to_a(tffunc_data, b_surfp_in_a);
  }

  if (use_a) {
    if (use_b) {
      geom_shape_data_nearest_surface_get_merge_use(a_element, a_end_element,
                                                    b_element, b_end_element,
                                                    a_surfp_in_b, b_surfp_in_a,
                                                    sop, &use_a, &use_b);
    }
  } else if (!use_b) {
    /* Both branches has no shapes which supports their surface */
    return GEOM_SUCCESS;
  }

  if (use_a || use_b) {
    if ((use_a && use_b && b_data->lp < a_data->lp) || (!use_a && use_b)) {
      b_data->surfp = b_surfp_in_a;
      *a_data = *b_data;
    }
    return GEOM_SUCCESS;
  }

  /* Start iteration method */
  if (iter_count < 0)
    iter_count = geom_shape_data_nearest_surface_default_iter_count();
  if (iter_crit <= 0.0)
    iter_crit = geom_shape_data_nearest_surface_default_iter_criterion();
  iter_crit = iter_crit * iter_crit;

  if (iter_count > 0) {
    geom_shape_data_nearest_surface_data a_it_data, b_it_data;
    geom_vec3 v, a_it_pnt, b_it_pnt;
    geom_error err;
    double crit, n_crit;
    int update_a;
    int modflag;

    v = geom_vec3_sub(a_surfp, b_surfp_in_a);
    crit = geom_vec3_inner_prod(v, v);
    a_it_pnt = a_pnt;
    for (int iter = 0; iter < iter_count && crit > iter_crit; ++iter) {
      update_a = (iter % 2 == 1);

      if (update_a) {
        a_it_pnt = b_surfp_in_a;

        err =
          geom_shape_data_nearest_surface_in_branch(a_element, a_end_element,
                                                    NULL, a_it_pnt, iter_count,
                                                    iter_crit, &b_it_data);
        if (err != GEOM_SUCCESS && err != GEOM_ERR_INACCURATE_SURFACE)
          return err;

        if (!a_it_data.element)
          break;

        a_surfp = a_it_data.surfp;

      } else {
        a_it_pnt = a_surfp;

        if (transform_func_a_to_b) {
          b_it_pnt = transform_func_a_to_b(tffunc_data, a_it_pnt);
        } else {
          b_it_pnt = a_it_pnt;
        }

        err =
          geom_shape_data_nearest_surface_in_branch(b_element, b_end_element,
                                                    NULL, b_it_pnt, iter_count,
                                                    iter_crit, &b_it_data);
        if (err != GEOM_SUCCESS && err != GEOM_ERR_INACCURATE_SURFACE)
          return err;

        if (!b_it_data.element)
          break;

        b_surfp_in_a = b_surfp;
        if (transform_func_b_to_a)
          b_surfp_in_a = transform_func_b_to_a(tffunc_data, b_surfp_in_a);
      }

      v = geom_vec3_sub(a_surfp, b_surfp_in_a);
      n_crit = geom_vec3_inner_prod(v, v);
    }

    if (a_it_data.element) {
      *a_data = a_it_data;
    } else if (b_it_data.element) {
      *a_data = b_it_data;
    }
  }
  return GEOM_ERR_INACCURATE_SURFACE;
}

static geom_error geom_shape_data_nearest_surface_in_branch(
  geom_shape_element *element, geom_shape_element *end_element,
  geom_shape_element **comb_element, geom_vec3 xyz,
  int iter_count, double iter_crit, geom_shape_data_nearest_surface_data *d)
{
  int first;
  geom_shape shape;
  geom_shape_operator sop;
  geom_error err;
  geom_shape_element *first_element;
  geom_shape_element *prev_element;
  geom_shape_element *next_element;
  geom_shape_element *c_element;
  geom_shape_element *n_end;
  geom_shape_data_nearest_surface_data o_data;
  int iter, max_iter;

  GEOM_ASSERT(element);
  GEOM_ASSERT(element->funcs);
  GEOM_ASSERT(d);

  if (iter_count < 0)
    iter_count = geom_shape_data_nearest_surface_default_iter_count();

  if (iter_crit <= 0.0)
    iter_crit = geom_shape_data_nearest_surface_default_iter_criterion();

  n_end = NULL;
  if (end_element)
    n_end = geom_shape_element_next(end_element);

  d->element = NULL;

  prev_element = element;
  next_element = NULL;

  err = GEOM_SUCCESS;
  first = 1;
  for (; element && element != n_end;
       prev_element = element, element = next_element) {
    geom_shape_element *b_end;
    shape = geom_shape_element_get_shape(element);
    sop = geom_shape_element_effective_operator(element);
    next_element = geom_shape_element_next(element);

    if (shape == GEOM_SHAPE_COMB)
      break;

    if (geom_shape_element_get_shape_type(element) != GEOM_SHPT_BODY) {
      continue;
    }

    if (first)
      first_element = element;

    if (!first && (sop == GEOM_SOP_SET || sop == GEOM_SOP_PUSH)) {
      c_element = NULL;
      err = geom_shape_data_nearest_surface_in_branch(element, NULL, &c_element,
                                                      xyz, iter_count,
                                                      iter_crit, &o_data);
      if (err != GEOM_SUCCESS && err != GEOM_ERR_INACCURATE_SURFACE)
        return err;

      sop = geom_shape_element_effective_operator(c_element);
      b_end = NULL;
    } else {
      geom_shape_data_nearest_surface_data *outp;
      if (first) {
        outp = d;
      } else {
        outp = &o_data;
      }

      err = geom_shape_element_nearest_surface(element, xyz, NULL, &outp->uv,
                                               &outp->surfp, &outp->normv);
      if (err == GEOM_SUCCESS) {
        outp->element = element;
      } else if (err == GEOM_ERR_NO_SURFACE_IN_SHAPE ||
                 err == GEOM_ERR_NO_ENABLED_SURFACE) {
        err = GEOM_SUCCESS;
        outp->element = NULL;
      } else {
        return err;
      }

      b_end = element;
    }

    if (!first) {
      err = geom_shape_data_nearest_surface_merge(first_element, prev_element,
                                                  element, b_end, xyz, sop,
                                                  iter_count, iter_crit, NULL,
                                                  NULL, NULL, d, &o_data);
      if (err != GEOM_SUCCESS && err != GEOM_ERR_INACCURATE_SURFACE)
        return err;
    }
    first = 0;
  }

  if (first)
    err = GEOM_ERR_NO_BODY_SHAPES;
  if (comb_element)
    *comb_element = element;
  return err;
}

geom_error geom_shape_data_nearest_surface(geom_shape_data *data, geom_vec3 xyz,
                                           int iter_count, double iter_crit,
                                           geom_shape_element **surface_element,
                                           geom_vec2 *uv, geom_vec3 *surfp,
                                           geom_vec3 *normv)
{
  int ret;
  geom_vec3 min_surfp;
  geom_vec2 min_uv;
  geom_shape_data_nearest_surface_data d, o;
  geom_shape_element *min_surface;
  double min_l;
  int first;
  geom_error err;
  geom_range3 rnge;
  geom_svec3 surf_it, it;
  geom_shape_element *el;
  geom_vec3 vp;

  GEOM_ASSERT(data);

  err = GEOM_SUCCESS;
  min_surface = NULL;
  min_surfp = geom_vec3_c(0.0, 0.0, 0.0);
  min_uv = geom_vec2_c(0.0, 0.0);
  rnge = geom_range3_c_vec(geom_svec3_c(0, 0, 0), data->repeat, 0);
  el = geom_shape_data_get_element(data);
  first = 1;

  geom_range3_foreach(it, rnge, geom_svec3_c(1, 1, 1)) {
    geom_shape_data_nearest_surface_data *outp;
    geom_vec3 vv;
    vv = geom_shape_data_get_element_world_p(data, it, xyz);

    if (first) {
      outp = &d;
    } else {
      outp = &o;
    }
    err =
      geom_shape_data_nearest_surface_in_branch(el, NULL, NULL, vv, iter_count,
                                                iter_crit, outp);
    if (err != GEOM_SUCCESS && err != GEOM_ERR_INACCURATE_SURFACE)
      return err;

    if (!first) {
      /**
       * @todo @p a_element must include all history for iteration
       * method. So iteration method is currently disabled here.
       */
      err = geom_shape_data_nearest_surface_merge(el, NULL, el, NULL, vv,
                                                  GEOM_SOP_ADD, 0, 0.0, NULL,
                                                  NULL, NULL, &d, &o);
      if (err != GEOM_SUCCESS && err != GEOM_ERR_INACCURATE_SURFACE)
        return err;
    }
    first = 0;
  }

  if (surface_element)
    *surface_element = d.element;
  if (uv)
    *uv = d.uv;
  if (surfp)
    *surfp = d.surfp;
  if (normv)
    *normv = d.normv;
  return err;
}

void geom_shape_data_add_element(geom_shape_element *el)
{
  geom_shape_data *shp;

  GEOM_ASSERT(el);

  shp = geom_shape_element_parent(el);
  GEOM_ASSERT(shp);

  geom_list_insert_prev(&shp->element.list, &el->list);
}

geom_shape_element *
geom_shape_data_get_element(geom_shape_data *data)
{
  struct geom_list *l;

  GEOM_ASSERT(data);

  l = geom_list_next(&data->element.list);
  if (l == &data->element.list) return NULL;

  return geom_shape_element_entry(l);
}

geom_shape_element *
geom_shape_element_new(geom_shape_data *data, geom_error *e)
{
  geom_error err;
  geom_data *master;
  geom_shape_element *el;

  GEOM_ASSERT(data);

  master = geom_shape_data_master(data);
  GEOM_ASSERT(master);

  el = (geom_shape_element *)malloc(sizeof(geom_shape_element));
  if (!el) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  err = geom_data_add_pointer(master, el, free);
  if (err != GEOM_SUCCESS) {
    free(el);
    if (e) *e = err;
    return NULL;
  }

  geom_shape_element_init(el, data);
  if (e) *e = GEOM_SUCCESS;
  return el;
}

geom_shape_data *
geom_shape_element_parent(geom_shape_element *e)
{
  GEOM_ASSERT(e);

  return e->parent;
}

geom_data *geom_shape_element_master(geom_shape_element *e)
{
  GEOM_ASSERT(e);

  return geom_shape_data_master(geom_shape_element_parent(e));
}

void geom_shape_element_delete(geom_shape_element *e)
{
  geom_data *master;
  geom_error err;

  GEOM_ASSERT(e);

  master = geom_shape_element_master(e);
  GEOM_ASSERT(master);

  geom_list_delete(&e->list);

  if (e->funcs) {
    geom_funcs_common_deallocate(&e->funcs->c, e->data, master);
  }
  geom_user_defined_data_free(master, &e->extra_data);

  err = geom_data_del_pointer(master, e);
  GEOM_ASSERT(err == GEOM_SUCCESS);
}

geom_shape_element *geom_shape_element_next(geom_shape_element *e)
{
  geom_shape_element *el;
  GEOM_ASSERT(e);

  el = geom_shape_element_entry(geom_list_next(&e->list));
  if (geom_shape_element_is_head(el)) {
    return NULL;
  } else {
    return el;
  }
}

geom_shape_element *geom_shape_element_prev(geom_shape_element *e)
{
  geom_shape_element *el;
  GEOM_ASSERT(e);

  el = geom_shape_element_entry(geom_list_prev(&e->list));
  if (geom_shape_element_is_head(el)) {
    return NULL;
  } else {
    return el;
  }
}

int geom_shape_element_is_head(geom_shape_element *e)
{
  GEOM_ASSERT(e);
  GEOM_ASSERT(e->parent);

  if (e == &e->parent->element) return 1;
  return 0;
}

geom_shape_args_builder *
geom_shape_args_builder_new(geom_shape shape, geom_error *e)
{
  const geom_shape_funcs *p;
  geom_args_builder *b;
  geom_shape_args_builder *sab;

  p = geom_get_shape_func(shape);
  if (!p) {
    if (e) *e = GEOM_ERR_INVALID_SHAPE;
    return NULL;
  }

  sab = (geom_shape_args_builder *)malloc(sizeof(geom_shape_args_builder));
  if (!sab) {
    if (e) *e = GEOM_ERR_NOMEM;
    return NULL;
  }

  if (p->c.args_next) {
    b = geom_args_builder_new(p->c.args_next, p->c.args_check, e);
    if (!b) {
      free(sab);
      return NULL;
    }
  } else {
    b = NULL;
  }

  sab->ab = b;
  sab->shape = shape;
  sab->funcs = p;
  if (e) *e = GEOM_SUCCESS;
  return sab;
}

void geom_shape_args_builder_delete(geom_shape_args_builder *b)
{
  if (!b) return;

  geom_args_builder_free(b->ab);
  free(b);
}

geom_args_builder *
geom_shape_args_get_builder(geom_shape_args_builder *b)
{
  GEOM_ASSERT(b);

  return b->ab;
}

geom_error geom_shape_element_set_shape(geom_shape_element *e,
                                        geom_shape_operator op,
                                        geom_shape_args_builder *shape)
{
  geom_error err;
  const geom_shape_funcs *fun;
  geom_data *master;
  geom_args_builder *ab;
  geom_shape_transform *m;
  void *data;

  GEOM_ASSERT(e);
  GEOM_ASSERT(e->parent);
  GEOM_ASSERT(shape);
  GEOM_ASSERT(!geom_shape_element_is_head(e));

  master = geom_shape_element_master(e);
  GEOM_ASSERT(master);

  fun = shape->funcs;
  GEOM_ASSERT(fun);

  err = GEOM_SUCCESS;
  data = geom_funcs_common_allocate(&fun->c, master, &err);
  if (err != GEOM_SUCCESS) {
    return err;
  }
  if (data) {
    ab = geom_shape_args_get_builder(shape);
    err = geom_funcs_common_set_data(&fun->c, data, ab);
  }

  if (data && fun->shape_type == GEOM_SHPT_TRANS) {
    const geom_shape_funcs *tfunc;
    tfunc = geom_shape_transform_func();
    m = (geom_shape_transform *)
      geom_funcs_common_allocate(&tfunc->c, master, &err);
    if (!m) {
      if (data) {
        geom_funcs_common_deallocate(&fun->c, data, master);
      }
      return err;
    }
    geom_shape_transform_set_data(m, master, data, fun);
    fun = tfunc;
    data = m;
  }

  if (e->data) {
    geom_data_del_pointer(master, e->data);
    geom_funcs_common_deallocate(&e->funcs->c, e->data, master);
  }

  e->funcs = fun;
  e->data = data;
  e->op_effec = op;
  e->op_orig  = op;
  return GEOM_SUCCESS;
}

geom_shape_operator
geom_shape_element_effective_operator(geom_shape_element *e)
{
  GEOM_ASSERT(e);

  return e->op_effec;
}

geom_shape_operator
geom_shape_element_original_operator(geom_shape_element *e)
{
  GEOM_ASSERT(e);

  return e->op_orig;
}

geom_shape geom_shape_element_get_shape(geom_shape_element *e)
{
  GEOM_ASSERT(e);

  if (e->funcs) {
    if (e->funcs == geom_shape_transform_func()) {
      return geom_shape_transform_get_shape((geom_shape_transform *)e->data);
    } else {
      return e->funcs->enum_val;
    }
  } else {
    return GEOM_SHAPE_INVALID;
  }
}

geom_shape_type geom_shape_element_get_shape_type(geom_shape_element *el)
{
  GEOM_ASSERT(el);

  if (!el->funcs) return GEOM_SHPT_INVALID;
  return el->funcs->shape_type;
}

const geom_mat43 *
geom_shape_element_get_transform(geom_shape_element *e)
{
  GEOM_ASSERT(e);

  return &e->transform;
}

geom_shape_type geom_shape_type_for_shape(geom_shape shape)
{
  const geom_shape_funcs *func;
  func = geom_get_shape_func(shape);
  if (!func) {
    return GEOM_SHPT_INVALID;
  }
  return func->shape_type;
}

int geom_shape_element_get_transformation_copy_n(geom_shape_element *e)
{
  geom_shape_transform *m;
  GEOM_ASSERT(e);

  if (e->funcs && e->funcs->shape_type == GEOM_SHPT_TRANS) {
    m = e->data;
    return geom_shape_transform_get_copy_num(m);
  } else {
    return -1;
  }
}

geom_error
geom_shape_element_set_transformation_copy_n(geom_shape_element *e,
                                             int ncopy)
{
  geom_shape_transform *m;
  geom_data *master;
  GEOM_ASSERT(e);
  GEOM_ASSERT(ncopy >= 0);
  GEOM_ASSERT(!geom_shape_element_is_head(e));

  master = geom_shape_element_master(e);
  GEOM_ASSERT(master);

  if (!e->funcs) return GEOM_ERR_SHAPE_NOT_SET;

  if (e->funcs->shape_type == GEOM_SHPT_TRANS) {
    m = e->data;
    GEOM_ASSERT(m);
    geom_shape_transform_set_copy_num(m, ncopy);
  } else {
    return GEOM_ERR_INVALID_SHAPE;
  }
  return GEOM_SUCCESS;
}

geom_info_map *geom_shape_element_shape_info(geom_shape_element *e)
{
  geom_info_map *m;
  geom_error err;

  GEOM_ASSERT(e);
  if (e->funcs && e->funcs->c.infomap_gen) {
    m = geom_info_map_new(&err);
    if (!m) return NULL;
    err = e->funcs->c.infomap_gen(e->data, m);
    if (err != GEOM_SUCCESS) {
      geom_info_map_delete_all(m);
      return NULL;
    }
    return m;
  } else {
    return NULL;
  }
}

int geom_shape_element_is_copied(geom_shape_element *e)
{
  GEOM_ASSERT(e);

  return e->copied;
}

geom_error geom_shape_element_set_parameter(geom_shape_element *e,
                                            geom_size_type index,
                                            const geom_variant *var,
                                            geom_variant *einfo)
{
  geom_error err;

  GEOM_ASSERT(e);
  GEOM_ASSERT(var);
  GEOM_ASSERT(e->funcs);

  if (e->funcs->c.args_check && e->funcs->c.set_value) {
    err = e->funcs->c.args_check(e->data, NULL, index, var, einfo);
    if (err != GEOM_SUCCESS) {
      return err;
    }

    err = e->funcs->c.set_value(e->data, index, var);
    return err;
  }
  return GEOM_ERR_RANGE;
}

geom_error geom_shape_element_get_parameter(geom_shape_element *e,
                                            geom_size_type index,
                                            geom_variant *out_variable)
{
  GEOM_ASSERT(e);
  GEOM_ASSERT(out_variable);
  GEOM_ASSERT(e->funcs);

  if (e->funcs->c.get_value) {
    return e->funcs->c.get_value(e->data, index, out_variable);
  }
  return GEOM_ERR_RANGE;
}

int geom_shape_element_n_surface(geom_shape_element *e)
{
  GEOM_ASSERT(e);
  GEOM_ASSERT(e->funcs);

  if (e->funcs->body_nsurff) {
    return e->funcs->body_nsurff(e->data);
  }
  return 0;
}

int geom_shape_element_n_enabled_surface(geom_shape_element *e)
{
  GEOM_ASSERT(e);
  GEOM_ASSERT(e->funcs);

  if (e->funcs->body_nsurff) {
    int n;
    n = e->funcs->body_nsurff(e->data);
    if (e->funcs->body_has_surff) {
      int c;
      c = 0;
      for (int i = 0; i < n; ++i) {
        if (e->funcs->body_has_surff(e->data, i))
          ++c;
      }
      return c;
    } else {
      /* Treat all surfaces are enabled */
      return n;
    }
  } else {
    return 0;
  }
}

int geom_shape_element_is_surface_enabled(geom_shape_element *e, int surfid)
{
  GEOM_ASSERT(e);
  GEOM_ASSERT(e->funcs);

  if (e->funcs->body_nsurff) {
    if (e->funcs->body_has_surff) {
      return e->funcs->body_has_surff(e->data, surfid);
    } else {
      /* returns true for any surfid */
      return 1;
    }
  } else {
    return 0;
  }
}

geom_error
geom_shape_element_set_extra_data(geom_shape_element *e,
                                  void *data, geom_deallocator *dealloc)
{
  GEOM_ASSERT(e);

  return geom_user_defined_data_set(geom_shape_element_master(e),
                                    &e->extra_data, data, dealloc);
}

const geom_user_defined_data *
geom_shape_element_get_extra_data(geom_shape_element *e)
{
  GEOM_ASSERT(e);

  return &e->extra_data;
}
