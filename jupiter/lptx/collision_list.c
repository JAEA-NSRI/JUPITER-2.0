#include "collision_list.h"
#include "defs.h"
#include "jupiter/geometry/list.h"
#include "overflow.h"
#include "param.h"
#include "particle.h"
#include "priv_struct_defs.h"
#include "priv_util.h"
#include "ptflags.h"
#include <stdlib.h>

static void LPTX_collision_list_append(LPTX_collision_list *l,
                                       LPTX_collision_list_set *s)
{
  geom_list_insert_prev(&l->list, &s->list.list);
}

static void LPTX_collision_list_prepend(LPTX_collision_list *l,
                                        LPTX_collision_list_set *s)
{
  geom_list_insert_next(&l->list, &s->list.list);
}

static void LPTX_collision_list_set_init(LPTX_collision_list_set *s,
                                         LPTX_idtype number_of_entries)
{
  LPTX_collision_list_init(&s->list);
  s->number_of_entries = number_of_entries;

#ifdef _OPENMP
#pragma omp parallel for
#endif
  for (LPTX_idtype jj = 0; jj < number_of_entries; ++jj)
    LPTX_collision_list_data_init(&s->entries[jj]);
}

LPTX_collision_list_set *LPTX_collision_list_set_new(LPTX_idtype num_entry)
{
  LPTX_idtype sz, nbytes;
  LPTX_collision_list_set *l;

  sz = sizeof(LPTX_collision_list_data);
  if (LPTX_s_mul_overflow(sz, num_entry, &nbytes))
    return NULL;

  sz = sizeof(LPTX_collision_list_set);
  if (LPTX_s_add_overflow(sz, nbytes, &nbytes))
    return NULL;

  l = (LPTX_collision_list_set *)malloc(nbytes);
  if (!l)
    return NULL;

  LPTX_collision_list_set_init(l, num_entry);
  return l;
}

static void LPTX_collision_list_delete(LPTX_collision_list *list)
{
  geom_list_delete(&list->list);
}

void LPTX_collision_list_set_delete(LPTX_collision_list_set *list)
{
  LPTX_collision_list_delete(&list->list);
  free(list);
}

static void LPTX_collision_list_delete_all(LPTX_collision_list *list)
{
  struct geom_list *lp, *ln, *lh;
  lh = &list->list;
  geom_list_foreach_safe (lp, ln, lh) {
    LPTX_collision_list_set *s;
    s = LPTX_collision_list_set_entry(lp);
    LPTX_collision_list_set_delete(s);
  }
}

void LPTX_collision_list_set_delete_all(LPTX_param *param)
{
  LPTX_collision_list_delete_all(&param->collision_list_head);
}

struct LPTX_fill_particle_ptr_list_data
{
  LPTX_particle_data **p;
};

static LPTX_bool LPTX_fill_particle_ptr(LPTX_particle_set *set, void *arg)
{
  LPTX_idtype np;
  struct LPTX_fill_particle_ptr_list_data *p = arg;

  np = LPTX_particle_set_number_of_particles(set);

#ifdef _OPENMP
#pragma omp for
#endif
  for (LPTX_idtype jj = 0; jj < np; ++jj)
    p->p[jj] = &set->particles[jj];

  p->p += np;
  return LPTX_false;
}

static int LPTX_collision_list_ptr_sort(const void *a, const void *b)
{
  LPTX_particle_data *pa = *(LPTX_particle_data **)a;
  LPTX_particle_data *pb = *(LPTX_particle_data **)b;
  const LPTX_particle *pba, *pbb;
  LPTX_bool ua, ub;

  pba = &pa->base;
  pbb = &pb->base;

  ua = LPTX_particle_is_used(pba) && LPTX_particle_can_collide(pba);
  ub = LPTX_particle_is_used(pbb) && LPTX_particle_can_collide(pbb);
  if (!!ua != !!ub) {
    if (ua)
      return -1;
    if (ub)
      return 1;
  }
  if (!ua || !ub)
    return 0; /* not interested */

  if (pba->kcfpt != pbb->kcfpt)
    return pba->kcfpt - pbb->kcfpt;
  if (pba->jcfpt != pbb->jcfpt)
    return pba->jcfpt - pbb->jcfpt;
  return pba->icfpt - pbb->icfpt;
}

static LPTX_bool
LPTX_collision_list_set_check_linked(LPTX_collision_list_data *a,
                                     LPTX_collision_list_data *s)
{
  struct geom_list *lp, *lh;

  if (a == s)
    return LPTX_true;

  lh = &a->list;
  geom_list_foreach (lp, lh) {
    if (lp == &s->list)
      return LPTX_true;
  }
  return LPTX_false;
}

static void LPTX_collision_list_make_multilink(LPTX_collision_list *root,
                                               LPTX_collision_list_set *set,
                                               LPTX_collision_list_data *start)
{
  LPTX_particle_data *sa, *sb;
  LPTX_collision_list_data *p;
  LPTX_bool la, lb;
  struct geom_list *lp, *lh;
  lp = &set->list.list;
  lh = &root->list;

  p = start - 1;
  sa = start->a;
  sb = start->b;
  la = LPTX_false;
  lb = LPTX_false;

  for (; lp != lh; lp = geom_list_prev(lp)) {
    LPTX_collision_list_set *cs;
    cs = LPTX_collision_list_set_entry(lp);
    if (cs != set)
      p = &cs->entries[cs->number_of_entries - 1];

    for (; p >= cs->entries; --p) {
      LPTX_particle_data *pa, *pb;
      pa = p->a;
      pb = p->b;

      if (!la && (pa == sa || pb == sa)) {
        if (!LPTX_collision_list_set_check_linked(p, start))
          geom_list_insert_list_prev(&p->list, &start->list);
        la = LPTX_true;
      }

      if (!lb && (pa == sb || pb == sb)) {
        if (!LPTX_collision_list_set_check_linked(p, start))
          geom_list_insert_list_prev(&p->list, &start->list);
        lb = LPTX_true;
      }

      if (la && lb)
        return;
    }
  }
}

static LPTX_idtype LPTX_collision_list_build(
  LPTX_collision_list *root, LPTX_idtype npt, LPTX_particle_data **sorted_list,
  LPTX_bool binary_only, LPTX_cb_collision_if *cond, void *arg)
{
  LPTX_collision_list_set *set;
  LPTX_idtype usep = 0;
  LPTX_idtype nent = 0;

  set = LPTX_collision_list_set_new(npt);
  if (!set)
    return -1;

  LPTX_collision_list_append(root, set);

  for (LPTX_idtype js = 0, je = 0; js < npt; js = je, je = js + 1) {
    LPTX_particle_data *ps;
    LPTX_particle *pbs;

    ps = sorted_list[js];
    pbs = &ps->base;

    if (!LPTX_particle_is_used(pbs) || !LPTX_particle_can_collide(pbs))
      break;

    for (; je < npt; ++je) {
      LPTX_particle_data *pe;
      LPTX_particle *pbe;

      pe = sorted_list[je];
      pbe = &pe->base;
      if (!LPTX_particle_is_used(pbe) || !LPTX_particle_can_collide(pbe))
        break;

      if (pbs->icfpt != pbe->icfpt)
        break;
      if (pbs->jcfpt != pbe->jcfpt)
        break;
      if (pbs->kcfpt != pbe->kcfpt)
        break;
    }
    if (je == js + 1)
      continue; /* nothing can be collided */

    for (LPTX_idtype ja = js; ja < je - 1; ++ja) {
      for (LPTX_idtype jb = js + 1; jb < je; ++jb) {
        LPTX_idtype nnewp;
        LPTX_bool f;
        LPTX_particle_data *pa, *pb;
        LPTX_collision_list_data *dn;

        pa = sorted_list[ja];
        pb = sorted_list[jb];
        nnewp = 0;
        f = cond(&nnewp, pa, pb, arg);
        if (!f)
          continue;

        ++nent;

        if (usep == npt) {
          set = LPTX_collision_list_set_new(npt);
          if (!set)
            return -1;

          LPTX_collision_list_append(root, set);
          usep = 0;
        }

        dn = &set->entries[usep++];
        dn->a = pa;
        dn->b = pb;
        dn->new_p = nnewp;

        if (binary_only)
          continue;

        LPTX_collision_list_make_multilink(root, set, dn);
      }
    }
  }

  return nent;
}

LPTX_idtype LPTX_collision_list_update(LPTX_param *param, LPTX_bool binary_only,
                                       LPTX_cb_collision_if *cond, void *arg)
{
  LPTX_idtype nent;
  LPTX_idtype npt;
  LPTX_particle_data **ptrs;
  LPTX_collision_list root;

  LPTX_collision_list_init(&root);
  nent = -1;

  ptrs = NULL;
  npt = LPTX_param_get_total_particles(param);
  ptrs = (LPTX_particle_data **)malloc(sizeof(LPTX_particle_data *) * npt);
  if (!ptrs)
    return -1;

  {
    struct LPTX_fill_particle_ptr_list_data a = {.p = ptrs};
    LPTX_param_foreach_particle_sets_i(param, LPTX_fill_particle_ptr, &a);
  }

  /**
   * @todo we may be wanted to do parallel sorting
   *
   * Most part of this function is algorithmically hard to parallelize,
   * and thus completely not parallelized.
   */
  qsort(ptrs, npt, sizeof(LPTX_particle_data *), LPTX_collision_list_ptr_sort);

  nent = LPTX_collision_list_build(&root, npt, ptrs, binary_only, cond, arg);

  if (nent >= 0) {
    LPTX_collision_list_set_delete_all(param);

    if (nent > 0) {
      geom_list_insert_list_prev(&param->collision_list_head.list, &root.list);
      geom_list_delete(&root.list);
    }

    param->number_of_collisions = nent;
  }

  free(ptrs);
  LPTX_collision_list_delete_all(&root);
  return nent;
}
