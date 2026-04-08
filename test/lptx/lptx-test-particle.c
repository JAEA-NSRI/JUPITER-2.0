#include "jupiter/lptx/defs.h"
#include "jupiter/lptx/param.h"
#include "jupiter/lptx/particle.h"
#include "jupiter/lptx/priv_struct_defs.h"
#include "jupiter/lptx/ptflags.h"
#include "jupiter/lptx/util.h"
#include "jupiter/lptx/vector.h"
#include "jupiter/random/random.h"
#include "lptx-test.h"
#include "test-util.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define LPTX_test_res_comp_C(p1, p2, p3, p4, p5, p6, p7, p8, p9, n, ...) n
#define LPTX_test_res_comp_N(...) \
  LPTX_test_res_comp_C(__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define LPTX_test_X(x) x
#define LPTX_test_Y(...) __VA_ARGS__
#define LPTX_test_Y_scl(...) __VA_ARGS__

#define LPTX_test_Y_cmp_E1(p0) {LPTX_test_Y_scl p0}
#define LPTX_test_Y_cmp_E2(p0, ...) \
  {LPTX_test_Y_scl p0}, LPTX_test_Y_cmp_E1(__VA_ARGS__)
#define LPTX_test_Y_cmp_E3(p0, ...) \
  {LPTX_test_Y_scl p0}, LPTX_test_Y_cmp_E2(__VA_ARGS__)
#define LPTX_test_Y_cmp_E4(p0, ...) \
  {LPTX_test_Y_scl p0}, LPTX_test_Y_cmp_E3(__VA_ARGS__)
#define LPTX_test_Y_cmp_E5(p0, ...) \
  {LPTX_test_Y_scl p0}, LPTX_test_Y_cmp_E4(__VA_ARGS__)
#define LPTX_test_Y_cmp_E6(p0, ...) \
  {LPTX_test_Y_scl p0}, LPTX_test_Y_cmp_E5(__VA_ARGS__)
#define LPTX_test_Y_cmp_E7(p0, ...) \
  {LPTX_test_Y_scl p0}, LPTX_test_Y_cmp_E6(__VA_ARGS__)
#define LPTX_test_Y_cmp_E8(p0, ...) \
  {LPTX_test_Y_scl p0}, LPTX_test_Y_cmp_E7(__VA_ARGS__)
#define LPTX_test_Y_cmp_E9(p0, ...) \
  {LPTX_test_Y_scl p0}, LPTX_test_Y_cmp_E8(__VA_ARGS__)

#define LPTX_test_Y_cmp_E(n, ...) LPTX_test_Y_cmp_E##n(__VA_ARGS__)
#define LPTX_test_Y_cmp_X(n, ...) LPTX_test_Y_cmp_E(n, __VA_ARGS__)
#define LPTX_test_Y_cmp(...) \
  LPTX_test_Y_cmp_X(LPTX_test_res_comp_N(__VA_ARGS__), __VA_ARGS__)

#define LPTX_test_Y_LPTX_idtype LPTX_test_Y_scl
#define LPTX_test_Y_int LPTX_test_Y_scl
#define LPTX_test_Y_LPTX_type LPTX_test_Y_scl
#define LPTX_test_Y_float LPTX_test_Y_scl
#define LPTX_test_Y_double LPTX_test_Y_scl
#define LPTX_test_Y_LPTX_vector LPTX_test_Y_cmp

#define LPTX_test_A(t, s, m) \
  (t[s]) { LPTX_test_Y_##t m }

#define LPTX_test_scl_set_p(setter, p, start, ary, size, rsize) \
  (!test_compare_ii(setter(p, start, ary, size), ==, rsize))

#define LPTX_test_scl_set(setter, p, start, type, size, rsize, s) \
  LPTX_test_scl_set_p(setter, p, start, LPTX_test_A(type, size, s), size, rsize)

#define LPTX_test_scl_get(getter, p, start, type, size, rsize, getvn) \
  (!test_compare_ii(getter(p, start, getvn, size), ==, rsize))

#define LPTX_test_vecsoa_set_p(setter, p, start, aryx, aryy, aryz, size, \
                               rsize)                                    \
  (!test_compare_ii(setter(p, start, aryx, aryy, aryz, size), ==, rsize))

#define LPTX_test_vecsoa_set(setter, p, start, type, size, rsize, sx, sy, sz) \
  LPTX_test_vecsoa_set_p(setter, p, start, LPTX_test_A(type, size, sx),       \
                         LPTX_test_A(type, size, sy),                         \
                         LPTX_test_A(type, size, sz), size, rsize)

#define LPTX_test_vecsoa_get(getter, p, start, type, size, rsize, getvnx, \
                             getvny, getvnz)                              \
  (!test_compare_ii(getter(p, start, getvnx, getvny, getvnz, size), ==, rsize))

#define LPTX_test_vcnsoa_arys_e1(t, s, m0) LPTX_test_A(t, s, m0)
#define LPTX_test_vcnsoa_arys_e2(t, s, m0, ...) \
  LPTX_test_A(t, s, m0), LPTX_test_vcnsoa_arys_e1(t, s, __VA_ARGS__)
#define LPTX_test_vcnsoa_arys_e3(t, s, m0, ...) \
  LPTX_test_A(t, s, m0), LPTX_test_vcnsoa_arys_e2(t, s, __VA_ARGS__)
#define LPTX_test_vcnsoa_arys_e4(t, s, m0, ...) \
  LPTX_test_A(t, s, m0), LPTX_test_vcnsoa_arys_e3(t, s, __VA_ARGS__)
#define LPTX_test_vcnsoa_arys_e5(t, s, m0, ...) \
  LPTX_test_A(t, s, m0), LPTX_test_vcnsoa_arys_e4(t, s, __VA_ARGS__)
#define LPTX_test_vcnsoa_arys_e6(t, s, m0, ...) \
  LPTX_test_A(t, s, m0), LPTX_test_vcnsoa_arys_e5(t, s, __VA_ARGS__)
#define LPTX_test_vcnsoa_arys_e7(t, s, m0, ...) \
  LPTX_test_A(t, s, m0), LPTX_test_vcnsoa_arys_e6(t, s, __VA_ARGS__)
#define LPTX_test_vcnsoa_arys_e8(t, s, m0, ...) \
  LPTX_test_A(t, s, m0), LPTX_test_vcnsoa_arys_e7(t, s, __VA_ARGS__)
#define LPTX_test_vcnsoa_arys_e9(t, s, m0, ...) \
  LPTX_test_A(t, s, m0), LPTX_test_vcnsoa_arys_e8(t, s, __VA_ARGS__)

#define LPTX_test_vcnsoa_arys_e(t, s, n, ...) \
  LPTX_test_vcnsoa_arys_e##n(t, s, __VA_ARGS__)

#define LPTX_test_vcnsoa_arys_x(t, s, n, ...) \
  LPTX_test_vcnsoa_arys_e(t, s, n, __VA_ARGS__)

#define LPTX_test_vcnsoa_arys(cnst, t, s, nc, sets)                        \
  ((cnst t *[nc]){LPTX_test_vcnsoa_arys_x(t, s, LPTX_test_res_comp_N sets, \
                                          LPTX_test_Y sets)})

#define LPTX_test_vcnsoa_set_p(setter, p, start, vector_index, type, size,  \
                               rsize, ncompo, arys)                         \
  (!test_compare_ii(setter(p, start, vector_index, arys, ncompo, size), ==, \
                    rsize))

#define LPTX_test_vcnsoa_set_e(setter, p, start, vector_index, type, size,  \
                               rsize, ncompo, arys)                         \
  LPTX_test_vcnsoa_set_p(setter, p, start, vector_index, type, size, rsize, \
                         ncompo, arys)

#define LPTX_test_vcnsoa_set(setter, p, start, vector_index, type, size,    \
                             rsize, ncompo, sets)                           \
  LPTX_test_vcnsoa_set_e(setter, p, start, vector_index, type, size, rsize, \
                         ncompo,                                            \
                         LPTX_test_vcnsoa_arys(const, type, size, ncompo,   \
                                               sets))

#define LPTX_test_vcnsoa_get(getter, p, start, vector_index, type, size,     \
                             rsize, ncompo, getvn)                           \
  (!test_compare_ii(getter(p, start, vector_index, getvn, ncompo, size), ==, \
                    rsize))

#define LPTX_test_res_comp_A0(p0, ...) p0
#define LPTX_test_res_comp_A1(p0, ...) LPTX_test_res_comp_A0(__VA_ARGS__)
#define LPTX_test_res_comp_A2(p0, ...) LPTX_test_res_comp_A1(__VA_ARGS__)
#define LPTX_test_res_comp_A3(p0, ...) LPTX_test_res_comp_A2(__VA_ARGS__)
#define LPTX_test_res_comp_A4(p0, ...) LPTX_test_res_comp_A3(__VA_ARGS__)
#define LPTX_test_res_comp_A5(p0, ...) LPTX_test_res_comp_A4(__VA_ARGS__)
#define LPTX_test_res_comp_A6(p0, ...) LPTX_test_res_comp_A5(__VA_ARGS__)
#define LPTX_test_res_comp_A7(p0, ...) LPTX_test_res_comp_A6(__VA_ARGS__)
#define LPTX_test_res_comp_A8(p0, ...) LPTX_test_res_comp_A7(__VA_ARGS__)

#define LPTX_test_res_comp_P(i, ...) \
  LPTX_test_res_comp_A##i(__VA_ARGS__, LPTX_empty)
#define LPTX_test_res_comp_PP(i, e) LPTX_test_res_comp_P(i, e)
#define LPTX_test_res_comp_PE(i, e) LPTX_test_res_comp_PP(i, LPTX_test_Y e)

#define LPTX_test_res_comp_R(v, i, c, e, ...) (!c(v, i, e, __VA_ARGS__))

#define LPTX_test_res_comp_E1(v, c, e, ...) \
  LPTX_test_res_comp_R(v, 0, c, LPTX_test_res_comp_PE(0, e), __VA_ARGS__)

#define LPTX_test_res_comp_E2(v, c, e, ...)      \
  LPTX_test_res_comp_E1(v, c, e, __VA_ARGS__) || \
    LPTX_test_res_comp_R(v, 1, c, LPTX_test_res_comp_PE(1, e), __VA_ARGS__)

#define LPTX_test_res_comp_E3(v, c, e, ...)      \
  LPTX_test_res_comp_E2(v, c, e, __VA_ARGS__) || \
    LPTX_test_res_comp_R(v, 2, c, LPTX_test_res_comp_PE(2, e), __VA_ARGS__)

#define LPTX_test_res_comp_E4(v, c, e, ...)      \
  LPTX_test_res_comp_E3(v, c, e, __VA_ARGS__) || \
    LPTX_test_res_comp_R(v, 3, c, LPTX_test_res_comp_PE(3, e), __VA_ARGS__)

#define LPTX_test_res_comp_E5(v, c, e, ...)      \
  LPTX_test_res_comp_E4(v, c, e, __VA_ARGS__) || \
    LPTX_test_res_comp_R(v, 4, c, LPTX_test_res_comp_PE(4, e), __VA_ARGS__)

#define LPTX_test_res_comp_E6(v, c, e, ...)      \
  LPTX_test_res_comp_E5(v, c, e, __VA_ARGS__) || \
    LPTX_test_res_comp_R(v, 5, c, LPTX_test_res_comp_PE(5, e), __VA_ARGS__)

#define LPTX_test_res_comp_E7(v, c, e, ...)      \
  LPTX_test_res_comp_E6(v, c, e, __VA_ARGS__) || \
    LPTX_test_res_comp_R(v, 6, c, LPTX_test_res_comp_PE(6, e), __VA_ARGS__)

#define LPTX_test_res_comp_E8(v, c, e, ...)      \
  LPTX_test_res_comp_E7(v, c, e, __VA_ARGS__) || \
    LPTX_test_res_comp_R(v, 7, c, LPTX_test_res_comp_PE(7, e), __VA_ARGS__)

#define LPTX_test_res_comp_E9(v, c, e, ...)      \
  LPTX_test_res_comp_E8(v, c, e, __VA_ARGS__) || \
    LPTX_test_res_comp_R(v, 8, c, LPTX_test_res_comp_PE(8, e), __VA_ARGS__)

#define LPTX_test_res_comp_E(v, c, n, e, ...) \
  LPTX_test_res_comp_E##n(v, c, e, __VA_ARGS__)

#define LPTX_test_res_comp_X(v, c, n, e, ...) \
  LPTX_test_res_comp_E(v, c, n, e, __VA_ARGS__)

#define LPTX_test_res_comp_T(v, c, e, ...) \
  LPTX_test_res_comp_X(v, c, LPTX_test_res_comp_N e, e, __VA_ARGS__)

#define LPTX_test_res_comp(v, c, ...) \
  LPTX_test_res_comp_T(v, c, __VA_ARGS__, LPTX_empty)

#define LPTX_test_scl_setget(setter, getter, p, start, type, size, rsize, \
                             getvn, setcomp, getcomp, sets, set_exps,     \
                             get_inits, get_exps)                         \
  (LPTX_test_scl_set(setter, p, start, type, size, rsize, sets) ||        \
   LPTX_test_res_comp(p, setcomp, set_exps, start, size) ||               \
   (getvn = LPTX_test_A(type, size, get_inits),                           \
    LPTX_test_res_comp(getvn, getcomp, get_inits, start, size) ||         \
      LPTX_test_scl_get(getter, p, start, type, size, rsize, getvn) ||    \
      LPTX_test_res_comp(getvn, getcomp, get_exps, start, size)))

#define LPTX_test_vecsoa_setget(setter, getter, p, start, type, size, rsize,  \
                                getvnx, getvny, getvnz, setcomp, getcomp,     \
                                setsx, setsy, setsz, set_exps, get_initsx,    \
                                get_initsy, get_initsz, get_expsx, get_expsy, \
                                get_expsz)                                    \
  (LPTX_test_vecsoa_set(setter, p, start, type, size, rsize, setsx, setsy,    \
                        setsz) ||                                             \
   LPTX_test_res_comp(p, setcomp, set_exps, start, size) ||                   \
   (getvnx = LPTX_test_A(type, size, get_initsx),                             \
    getvny = LPTX_test_A(type, size, get_initsy),                             \
    getvnz = LPTX_test_A(type, size, get_initsz),                             \
    LPTX_test_res_comp(getvnx, getcomp, get_initsx, start, size) ||           \
      LPTX_test_res_comp(getvny, getcomp, get_initsy, start, size) ||         \
      LPTX_test_res_comp(getvnz, getcomp, get_initsz, start, size) ||         \
      LPTX_test_vecsoa_get(getter, p, start, type, size, rsize, getvnx,       \
                           getvny, getvnz) ||                                 \
      LPTX_test_res_comp(getvnx, getcomp, get_expsx, start, size) ||          \
      LPTX_test_res_comp(getvny, getcomp, get_expsy, start, size) ||          \
      LPTX_test_res_comp(getvnz, getcomp, get_expsz, start, size)))

#define LPTX_test_vcnsoa_setget(setter, getter, p, start, vector_index, type, \
                                size, rsize, ncompo, getvn, setcomp, getcomp, \
                                sets, set_exps, get_inits, get_exps)          \
  (LPTX_test_vcnsoa_set(setter, p, start, vector_index, type, size, rsize,    \
                        ncompo, sets) ||                                      \
   LPTX_test_res_comp(p, setcomp, set_exps, start, size, vector_index) ||     \
   (getvn = LPTX_test_vcnsoa_arys(LPTX_empty, type, size, ncompo, get_inits), \
    LPTX_test_res_comp(getvn, getcomp, get_inits, start, size) ||             \
      LPTX_test_vcnsoa_get(getter, p, start, vector_index, type, size, rsize, \
                           ncompo, getvn) ||                                  \
      LPTX_test_res_comp(getvn, getcomp, get_exps, start, size)))

static const LPTX_particle_data *LPTX_particle_set_p(LPTX_particle_set *p,
                                                     LPTX_idtype idx)
{
  if (idx < 0 || idx >= LPTX_particle_set_number_of_particles(p)) {
    fprintf(stderr,
            "..... Index %" PRIdMAX " is out of range in a particle set.\n",
            (intmax_t)idx);
    return NULL;
  }

  return &p->particles[idx];
}

static const LPTX_type *LPTX_particle_set_v(LPTX_particle_set *p,
                                            LPTX_idtype idx,
                                            LPTX_idtype vector_index,
                                            LPTX_idtype component_index)
{
  const LPTX_particle_data *pt;
  const LPTX_particle_vector *v;

  pt = LPTX_particle_set_p(p, idx);
  if (!pt)
    return NULL;

  if (vector_index < 0 || vector_index >= pt->number_of_vectors) {
    fprintf(stderr,
            "..... Index %" PRIdMAX " is out of range of vector indices.\n",
            (intmax_t)vector_index);
    return NULL;
  }

  v = &pt->vectors[vector_index];
  if (component_index < 0 || component_index >= v->length) {
    fprintf(stderr,
            "..... Index %" PRIdMAX " is out of range of vector %" PRIdMAX
            ".\n",
            (intmax_t)component_index, (intmax_t)vector_index);
    return NULL;
  }
  return &v->v[component_index];
}

static const LPTX_particle_data *LPTX_param_pt_p(LPTX_param *p, LPTX_idtype idx)
{
  LPTX_idtype i;
  LPTX_particle_set *set;

  if (idx < 0) {
    fprintf(stderr,
            "..... Index %" PRIdMAX " is out of range in particle sets\n",
            (intmax_t)idx);
    return NULL;
  }

  i = idx;
  LPTX_foreach_particle_sets (set, p) {
    LPTX_idtype n;
    n = LPTX_particle_set_number_of_particles(set);
    if (i < n)
      return &set->particles[i];
    i -= n;
  }

  fprintf(stderr, "..... Index %" PRIdMAX " is out of range in particle sets\n",
          (intmax_t)idx);
  return NULL;
}

static const LPTX_type *LPTX_param_pt_v(LPTX_param *p, LPTX_idtype idx,
                                        LPTX_idtype vector_index,
                                        LPTX_idtype component_index)
{
  const LPTX_particle_data *pt;
  const LPTX_particle_vector *v;

  pt = LPTX_param_pt_p(p, idx);
  if (!pt)
    return NULL;

  if (vector_index < 0 || vector_index >= pt->number_of_vectors) {
    fprintf(stderr,
            "..... Index %" PRIdMAX " is out of range of vector indices.\n",
            (intmax_t)vector_index);
    return NULL;
  }

  v = &pt->vectors[vector_index];
  if (component_index < 0 || component_index >= v->length) {
    fprintf(stderr,
            "..... Index %" PRIdMAX " is out of range of vector %" PRIdMAX
            ".\n",
            (intmax_t)component_index, (intmax_t)vector_index);
    return NULL;
  }
  return &v->v[component_index];
}

static LPTX_bool filter_odd(const LPTX_particle_data *pt, void *arg)
{
  if (pt->base.particle_id % 2 == 0)
    return LPTX_false;
  return LPTX_true;
}

static int simple_id_ascsort(const LPTX_particle_data *a,
                             const LPTX_particle_data *b, void *arg)
{
  /* return a->base.particle_id - b->base.particle_id */
  if (a->base.particle_id < b->base.particle_id)
    return -1;
  if (a->base.particle_id > b->base.particle_id)
    return 1;
  return 0;
}

static int simple_id_decsort(const LPTX_particle_data *a,
                             const LPTX_particle_data *b, void *arg)
{
  /* return b->base.particle_id - a->base.particle_id */
  if (a->base.particle_id < b->base.particle_id)
    return 1;
  if (a->base.particle_id > b->base.particle_id)
    return -1;
  return 0;
}

/* pass @p arg with ID to find in (const LPTX_idtype *) */
static int simple_id_ascfind(const LPTX_particle_data *p, void *arg)
{
  const LPTX_idtype *id = (const LPTX_idtype *)arg;
  if (p->base.particle_id < *id)
    return -1;
  if (p->base.particle_id > *id)
    return 1;
  return 0;
}

static int simple_id_decfind(const LPTX_particle_data *p, void *arg)
{
  const LPTX_idtype *id = (const LPTX_idtype *)arg;
  if (p->base.particle_id < *id)
    return 1;
  if (p->base.particle_id > *id)
    return -1;
  return 0;
}

static void merge_left(LPTX_particle_data *outp, const LPTX_particle_data *pa,
                       const LPTX_particle_data *pb, void *arg)
{
  if (outp != pa)
    LPTX_particle_copy(outp, pa);
}

static void merge_right(LPTX_particle_data *outp, const LPTX_particle_data *pa,
                        const LPTX_particle_data *pb, void *arg)
{
  if (outp != pb)
    LPTX_particle_copy(outp, pb);
}

static void merge_sum(LPTX_particle_data *outp, const LPTX_particle_data *pa,
                      const LPTX_particle_data *pb, void *arg)
{
  if (outp != pa)
    LPTX_particle_copy(outp, pa);
  if (outp != pb)
    outp->base.origin_id += pb->base.origin_id;
}

int test_lptx_particle(void)
{
  int r = 0;
  LPTX_param *p = NULL;
  LPTX_particle_set *s1 = NULL;
  LPTX_particle_set *s2 = NULL;
  LPTX_particle_set *s3 = NULL;

  do {
    if (!test_compare_pp((s1 =
                            LPTX_particle_set_new(5, 2, (LPTX_idtype[]){1, 2})),
                         !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_pp((s2 =
                            LPTX_particle_set_new(3, 2, (LPTX_idtype[]){1, 2})),
                         !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(LPTX_particle_set_are_mergeable(NULL, NULL), ==,
                         LPTX_false))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_are_mergeable(s1, NULL), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_are_mergeable(NULL, s2), ==,
                         LPTX_true))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_are_mergeable(s1, s2), ==,
                         LPTX_true))
      r = 1;

    if (!test_compare_pp((s3 = LPTX_particle_set_new(4, 3,
                                                     (LPTX_idtype[]){1, 2, 3})),
                         !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(LPTX_particle_set_are_mergeable(s1, s3), ==,
                         LPTX_false))
      r = 1;

    LPTX_particle_set_delete(s3);
    if (!test_compare_pp((s3 =
                            LPTX_particle_set_new(1, 2, (LPTX_idtype[]){2, 1})),
                         !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(LPTX_particle_set_are_mergeable(s1, s3), ==,
                         LPTX_false))
      r = 1;

    LPTX_particle_set_delete(s3);
    if (!test_compare_pp((s3 = LPTX_particle_set_new(4, 0, NULL)), !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(LPTX_particle_set_are_mergeable(s1, s3), ==,
                         LPTX_false))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_are_mergeable(s3, s1), ==,
                         LPTX_false))
      r = 1;
  } while (0);

  do {
    if (s1)
      LPTX_particle_set_delete(s1);
    if (s2)
      LPTX_particle_set_delete(s2);
    if (s3)
      LPTX_particle_set_delete(s3);
    s1 = NULL;
    s2 = NULL;
    s3 = NULL;

    if (!test_compare_pp((s1 = LPTX_particle_set_new(5, 3,
                                                     (LPTX_idtype[]){2, 1, 0})),
                         !=, NULL))
      r = 1;
    if (r)
      break;

    s1->particles[0].base.particle_id = 0;
    s1->particles[1].base.particle_id = 5;
    s1->particles[2].base.particle_id = 7;
    s1->particles[3].base.particle_id = 2;
    s1->particles[4].base.particle_id = 3;
    LPTX_particle_set_used(&s1->particles[0].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[1].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[2].base, LPTX_false);
    LPTX_particle_set_used(&s1->particles[3].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[4].base, LPTX_true);

    if (!test_compare_pp(s1->sorted[0], ==, &s1->particles[0]))
      r = 1;
    if (!test_compare_pp(s1->sorted[1], ==, &s1->particles[1]))
      r = 1;
    if (!test_compare_pp(s1->sorted[2], ==, &s1->particles[2]))
      r = 1;
    if (!test_compare_pp(s1->sorted[3], ==, &s1->particles[3]))
      r = 1;
    if (!test_compare_pp(s1->sorted[4], ==, &s1->particles[4]))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[0], ==, 0))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[1], ==, 1))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[2], ==, 2))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[3], ==, 3))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[4], ==, 4))
      r = 1;

    LPTX_particle_set_sort_particles(s1, simple_id_ascsort, NULL);

    if (!test_compare_pp(s1->sorted[0], ==, &s1->particles[0]))
      r = 1;
    if (!test_compare_pp(s1->sorted[1], ==, &s1->particles[3]))
      r = 1;
    if (!test_compare_pp(s1->sorted[2], ==, &s1->particles[4]))
      r = 1;
    if (!test_compare_pp(s1->sorted[3], ==, &s1->particles[1]))
      r = 1;
    if (!test_compare_pp(s1->sorted[4], ==, &s1->particles[2]))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[0], ==, 0))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[1], ==, 3))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[2], ==, 4))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[3], ==, 1))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[4], ==, 2))
      r = 1;

    LPTX_particle_set_sort_particles(s1, simple_id_decsort, NULL);

    if (!test_compare_pp(s1->sorted[0], ==, &s1->particles[2]))
      r = 1;
    if (!test_compare_pp(s1->sorted[1], ==, &s1->particles[1]))
      r = 1;
    if (!test_compare_pp(s1->sorted[2], ==, &s1->particles[4]))
      r = 1;
    if (!test_compare_pp(s1->sorted[3], ==, &s1->particles[3]))
      r = 1;
    if (!test_compare_pp(s1->sorted[4], ==, &s1->particles[0]))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[0], ==, 4))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[1], ==, 1))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[2], ==, 0))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[3], ==, 3))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[4], ==, 2))
      r = 1;

    s1->particles[0].base.particle_id = 0;
    s1->particles[1].base.particle_id = 9;
    s1->particles[2].base.particle_id = 8;
    s1->particles[3].base.particle_id = 5;
    s1->particles[4].base.particle_id = 4;
    LPTX_particle_set_isort_particles(s1, simple_id_ascsort, NULL);

    if (!test_compare_pp(s1->sorted[0], ==, &s1->particles[0]))
      r = 1;
    if (!test_compare_pp(s1->sorted[1], ==, &s1->particles[4]))
      r = 1;
    if (!test_compare_pp(s1->sorted[2], ==, &s1->particles[3]))
      r = 1;
    if (!test_compare_pp(s1->sorted[3], ==, &s1->particles[2]))
      r = 1;
    if (!test_compare_pp(s1->sorted[4], ==, &s1->particles[1]))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[0], ==, 0))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[1], ==, 4))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[2], ==, 3))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[3], ==, 2))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[4], ==, 1))
      r = 1;

    s1->particles[0].base.particle_id = 9;
    s1->particles[1].base.particle_id = 1;
    s1->particles[2].base.particle_id = 5;
    s1->particles[3].base.particle_id = 1;
    s1->particles[4].base.particle_id = 4;
    LPTX_particle_set_isort_particles(s1, simple_id_ascsort, NULL);

    /* insertion sort is stable */
    if (!test_compare_pp(s1->sorted[0], ==, &s1->particles[3]))
      r = 1;
    if (!test_compare_pp(s1->sorted[1], ==, &s1->particles[1]))
      r = 1;
    if (!test_compare_pp(s1->sorted[2], ==, &s1->particles[4]))
      r = 1;
    if (!test_compare_pp(s1->sorted[3], ==, &s1->particles[2]))
      r = 1;
    if (!test_compare_pp(s1->sorted[4], ==, &s1->particles[0]))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[0], ==, 4))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[1], ==, 1))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[2], ==, 3))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[3], ==, 0))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[4], ==, 2))
      r = 1;

    LPTX_particle_set_isort_particles(s1, simple_id_decsort, NULL);

    if (!test_compare_pp(s1->sorted[0], ==, &s1->particles[0]))
      r = 1;
    if (!test_compare_pp(s1->sorted[1], ==, &s1->particles[2]))
      r = 1;
    if (!test_compare_pp(s1->sorted[2], ==, &s1->particles[4]))
      r = 1;
    if (!test_compare_pp(s1->sorted[3], ==, &s1->particles[3]))
      r = 1;
    if (!test_compare_pp(s1->sorted[4], ==, &s1->particles[1]))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[0], ==, 0))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[1], ==, 4))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[2], ==, 1))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[3], ==, 3))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[4], ==, 2))
      r = 1;

    s1->particles[0].base.particle_id = 15;
    s1->particles[1].base.particle_id = 6;
    s1->particles[2].base.particle_id = 13;
    s1->particles[3].base.particle_id = 1;
    s1->particles[4].base.particle_id = 17;
    LPTX_particle_set_qsort_particles(s1, simple_id_ascsort, NULL);

    if (!test_compare_pp(s1->sorted[0], ==, &s1->particles[3]))
      r = 1;
    if (!test_compare_pp(s1->sorted[1], ==, &s1->particles[1]))
      r = 1;
    if (!test_compare_pp(s1->sorted[2], ==, &s1->particles[2]))
      r = 1;
    if (!test_compare_pp(s1->sorted[3], ==, &s1->particles[0]))
      r = 1;
    if (!test_compare_pp(s1->sorted[4], ==, &s1->particles[4]))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[0], ==, 3))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[1], ==, 1))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[2], ==, 2))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[3], ==, 0))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[4], ==, 4))
      r = 1;

    s1->particles[0].base.particle_id = 8;
    s1->particles[1].base.particle_id = 5;
    s1->particles[2].base.particle_id = 14;
    s1->particles[3].base.particle_id = 13;
    s1->particles[4].base.particle_id = 4;
    LPTX_particle_set_qsort_particles(s1, simple_id_ascsort, NULL);

    if (!test_compare_pp(s1->sorted[0], ==, &s1->particles[4]))
      r = 1;
    if (!test_compare_pp(s1->sorted[1], ==, &s1->particles[1]))
      r = 1;
    if (!test_compare_pp(s1->sorted[2], ==, &s1->particles[0]))
      r = 1;
    if (!test_compare_pp(s1->sorted[3], ==, &s1->particles[3]))
      r = 1;
    if (!test_compare_pp(s1->sorted[4], ==, &s1->particles[2]))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[0], ==, 2))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[1], ==, 1))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[2], ==, 4))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[3], ==, 3))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[4], ==, 0))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_sorted_index_from_array(s1, 0), ==,
                         2))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_sorted_index_from_array(s1, 1), ==,
                         1))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_sorted_index_from_array(s1, 2), ==,
                         4))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_sorted_index_from_array(s1, 3), ==,
                         3))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_sorted_index_from_array(s1, 4), ==,
                         0))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_sorted_index_from_array(s1, 5), ==,
                         -1))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_sorted_index_from_array(s1, -1), ==,
                         -1))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_array_index_from_sorted(s1, 0), ==,
                         4))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_array_index_from_sorted(s1, 1), ==,
                         1))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_array_index_from_sorted(s1, 2), ==,
                         0))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_array_index_from_sorted(s1, 3), ==,
                         3))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_array_index_from_sorted(s1, 4), ==,
                         2))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_array_index_from_sorted(s1, 5), ==,
                         -1))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_array_index_from_sorted(s1, -1), ==,
                         -1))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_search(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){0}),
                         ==, -1))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_binary_search(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){4}),
                         ==, 0))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_binary_search(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){5}),
                         ==, 1))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_binary_search(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){8}),
                         ==, 2))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_binary_search(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){13}),
                         ==, 3))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_binary_search(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){14}),
                         ==, 4))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_binary_search(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){10}),
                         ==, -1))
      r = 1;

    /* Find first index that is greater than or equal to given value */
    if (!test_compare_ii(LPTX_particle_set_binary_lbound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){0}),
                         ==, 0))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_lbound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){4}),
                         ==, 0))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_lbound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){5}),
                         ==, 1))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_lbound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){7}),
                         ==, 2))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_lbound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){8}),
                         ==, 2))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_lbound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){9}),
                         ==, 3))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_lbound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){14}),
                         ==, 4))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_lbound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){15}),
                         ==, 5))
      r = 1;

    /* Find first index that is greater than given value */
    if (!test_compare_ii(LPTX_particle_set_binary_ubound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){0}),
                         ==, 0))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_ubound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){4}),
                         ==, 1))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_ubound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){5}),
                         ==, 2))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_ubound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){7}),
                         ==, 2))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_ubound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){8}),
                         ==, 3))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_ubound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){9}),
                         ==, 3))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_ubound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){14}),
                         ==, 5))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_binary_ubound(s1, simple_id_ascfind,
                                                         (LPTX_idtype[]){15}),
                         ==, 5))
      r = 1;

    {
      LPTX_idtype first, last;

      first = -1;
      last = -1;
      LPTX_particle_set_binary_partition(s1, &first, &last, simple_id_ascfind,
                                         (LPTX_idtype[]){0});
      if (!test_compare_ii(first, ==, 0))
        r = 1;
      if (!test_compare_ii(last, ==, 0))
        r = 1;

      first = -1;
      last = -1;
      LPTX_particle_set_binary_partition(s1, &first, &last, simple_id_ascfind,
                                         (LPTX_idtype[]){4});
      if (!test_compare_ii(first, ==, 0))
        r = 1;
      if (!test_compare_ii(last, ==, 1))
        r = 1;

      first = 0;
      last = 0;
      LPTX_particle_set_binary_partition(s1, &first, &last, simple_id_ascfind,
                                         (LPTX_idtype[]){8});
      if (!test_compare_ii(first, ==, 2))
        r = 1;
      if (!test_compare_ii(last, ==, 3))
        r = 1;

      first = 0;
      last = 0;
      LPTX_particle_set_binary_partition(s1, &first, &last, simple_id_ascfind,
                                         (LPTX_idtype[]){13});
      if (!test_compare_ii(first, ==, 3))
        r = 1;
      if (!test_compare_ii(last, ==, 4))
        r = 1;

      first = 0;
      last = 0;
      LPTX_particle_set_binary_partition(s1, &first, &last, simple_id_ascfind,
                                         (LPTX_idtype[]){14});
      if (!test_compare_ii(first, ==, 4))
        r = 1;
      if (!test_compare_ii(last, ==, 5))
        r = 1;

      first = 0;
      last = 0;
      LPTX_particle_set_binary_partition(s1, &first, &last, simple_id_ascfind,
                                         (LPTX_idtype[]){17});
      if (!test_compare_ii(first, ==, 5))
        r = 1;
      if (!test_compare_ii(last, ==, 5))
        r = 1;

      first = 0;
      last = 0;
      LPTX_particle_set_binary_partition(s1, &first, &last, simple_id_ascfind,
                                         (LPTX_idtype[]){9});
      if (!test_compare_ii(first, ==, 3))
        r = 1;
      if (!test_compare_ii(last, ==, 3))
        r = 1;
    }

    s1->particles[0].base.particle_id = 0;
    s1->particles[1].base.particle_id = 5;
    s1->particles[2].base.particle_id = 7;
    s1->particles[3].base.particle_id = 2;
    s1->particles[4].base.particle_id = 3;
    LPTX_particle_set_used(&s1->particles[0].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[1].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[2].base, LPTX_false);
    LPTX_particle_set_used(&s1->particles[3].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[4].base, LPTX_true);

    LPTX_particle_set_sort_particles_id(s1);

    if (!test_compare_pp(s1->sorted[0], ==, &s1->particles[0]))
      r = 1;
    if (!test_compare_pp(s1->sorted[1], ==, &s1->particles[3]))
      r = 1;
    if (!test_compare_pp(s1->sorted[2], ==, &s1->particles[4]))
      r = 1;
    if (!test_compare_pp(s1->sorted[3], ==, &s1->particles[1]))
      r = 1;
    if (!test_compare_pp(s1->sorted[4], ==, &s1->particles[2]))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[0], ==, 0))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[1], ==, 3))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[2], ==, 4))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[3], ==, 1))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[4], ==, 2))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_used_particles_sorted(s1), ==, 4))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 0), ==, 0))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 1), ==, -1))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 2), ==, 1))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 3), ==, 2))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 5), ==, 3))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 7), ==, -1))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 11), ==, -1))
      r = 1;

    s1->particles[0].base.particle_id = 15;
    s1->particles[1].base.particle_id = 7;
    s1->particles[2].base.particle_id = 6;
    s1->particles[3].base.particle_id = 11;
    s1->particles[4].base.particle_id = 9;
    LPTX_particle_set_used(&s1->particles[0].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[1].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[2].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[3].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[4].base, LPTX_true);

    LPTX_particle_set_sort_particles_id(s1);

    if (!test_compare_pp(s1->sorted[0], ==, &s1->particles[2]))
      r = 1;
    if (!test_compare_pp(s1->sorted[1], ==, &s1->particles[1]))
      r = 1;
    if (!test_compare_pp(s1->sorted[2], ==, &s1->particles[4]))
      r = 1;
    if (!test_compare_pp(s1->sorted[3], ==, &s1->particles[3]))
      r = 1;
    if (!test_compare_pp(s1->sorted[4], ==, &s1->particles[0]))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[0], ==, 4))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[1], ==, 1))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[2], ==, 0))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[3], ==, 3))
      r = 1;
    if (!test_compare_ii(s1->sorted_indices[4], ==, 2))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_used_particles_sorted(s1), ==, 5))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 0), ==, -1))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 6), ==, 0))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 7), ==, 1))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 9), ==, 2))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 11), ==, 3))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 15), ==, 4))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_find_index(s1, 10), ==, -1))
      r = 1;

    if (!test_compare_pp((s2 = LPTX_particle_set_new(7, 3,
                                                     (LPTX_idtype[]){2, 1, 0})),
                         !=, NULL))
      r = 1;
    if (r)
      break;

    for (int i = 0; i < 5; ++i)
      s1->particles[i].base.origin_id = 1;
    for (int i = 0; i < 7; ++i)
      s2->particles[i].base.origin_id = 2;

    s2->particles[0].base.particle_id = 16;
    s2->particles[1].base.particle_id = 11;
    s2->particles[2].base.particle_id = 17;
    s2->particles[3].base.particle_id = 1;
    s2->particles[4].base.particle_id = 4;
    s2->particles[5].base.particle_id = 2;
    s2->particles[6].base.particle_id = 7;
    LPTX_particle_set_used(&s2->particles[0].base, LPTX_true);
    LPTX_particle_set_used(&s2->particles[1].base, LPTX_false);
    LPTX_particle_set_used(&s2->particles[2].base, LPTX_true);
    LPTX_particle_set_used(&s2->particles[3].base, LPTX_true);
    LPTX_particle_set_used(&s2->particles[4].base, LPTX_true);
    LPTX_particle_set_used(&s2->particles[5].base, LPTX_false);
    LPTX_particle_set_used(&s2->particles[6].base, LPTX_true);

    if (!test_compare_ii(LPTX_particle_set_merge(&s3, s1, s2, merge_left, NULL),
                         ==, LPTX_true))
      r = 1;
    if (!test_compare_pp(s3, !=, NULL))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(s3->number_particles, ==, 10))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[0].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[1].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[2].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[3].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[4].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[5].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[6].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[7].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[8].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[9].base), ==,
                         LPTX_false))
      r = 1;

    if (!test_compare_ii(s3->particles[0].base.particle_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[1].base.particle_id, ==, 4))
      r = 1;
    if (!test_compare_ii(s3->particles[2].base.particle_id, ==, 6))
      r = 1;
    if (!test_compare_ii(s3->particles[3].base.particle_id, ==, 7))
      r = 1;
    if (!test_compare_ii(s3->particles[4].base.particle_id, ==, 9))
      r = 1;
    if (!test_compare_ii(s3->particles[5].base.particle_id, ==, 11))
      r = 1;
    if (!test_compare_ii(s3->particles[6].base.particle_id, ==, 15))
      r = 1;
    if (!test_compare_ii(s3->particles[7].base.particle_id, ==, 16))
      r = 1;
    if (!test_compare_ii(s3->particles[8].base.particle_id, ==, 17))
      r = 1;

    if (!test_compare_ii(s3->particles[0].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[1].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[2].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[3].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[4].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[5].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[6].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[7].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[8].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[9].base.origin_id, ==,
                         LPTX_ORIGIN_UNDEFINED))
      r = 1;

    LPTX_particle_set_delete(s3);
    s3 = NULL;

    if (!test_compare_ii(LPTX_particle_set_merge(&s3, s1, s2, merge_right,
                                                 NULL),
                         ==, LPTX_true))
      r = 1;
    if (!test_compare_pp(s3, !=, NULL))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(s3->number_particles, ==, 10))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[0].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[1].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[2].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[3].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[4].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[5].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[6].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[7].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[8].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[9].base), ==,
                         LPTX_false))
      r = 1;

    if (!test_compare_ii(s3->particles[0].base.particle_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[1].base.particle_id, ==, 4))
      r = 1;
    if (!test_compare_ii(s3->particles[2].base.particle_id, ==, 6))
      r = 1;
    if (!test_compare_ii(s3->particles[3].base.particle_id, ==, 7))
      r = 1;
    if (!test_compare_ii(s3->particles[4].base.particle_id, ==, 9))
      r = 1;
    if (!test_compare_ii(s3->particles[5].base.particle_id, ==, 11))
      r = 1;
    if (!test_compare_ii(s3->particles[6].base.particle_id, ==, 15))
      r = 1;
    if (!test_compare_ii(s3->particles[7].base.particle_id, ==, 16))
      r = 1;
    if (!test_compare_ii(s3->particles[8].base.particle_id, ==, 17))
      r = 1;

    if (!test_compare_ii(s3->particles[0].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[1].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[2].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[3].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[4].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[5].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[6].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[7].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[8].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[9].base.origin_id, ==,
                         LPTX_ORIGIN_UNDEFINED))
      r = 1;

    LPTX_particle_set_delete(s3);
    s3 = NULL;

    if (!test_compare_ii(LPTX_particle_set_merge(&s3, s1, s2, merge_sum, NULL),
                         ==, LPTX_true))
      r = 1;
    if (!test_compare_pp(s3, !=, NULL))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(s3->number_particles, ==, 10))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[0].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[1].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[2].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[3].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[4].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[5].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[6].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[7].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[8].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[9].base), ==,
                         LPTX_false))
      r = 1;

    if (!test_compare_ii(s3->particles[0].base.particle_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[1].base.particle_id, ==, 4))
      r = 1;
    if (!test_compare_ii(s3->particles[2].base.particle_id, ==, 6))
      r = 1;
    if (!test_compare_ii(s3->particles[3].base.particle_id, ==, 7))
      r = 1;
    if (!test_compare_ii(s3->particles[4].base.particle_id, ==, 9))
      r = 1;
    if (!test_compare_ii(s3->particles[5].base.particle_id, ==, 11))
      r = 1;
    if (!test_compare_ii(s3->particles[6].base.particle_id, ==, 15))
      r = 1;
    if (!test_compare_ii(s3->particles[7].base.particle_id, ==, 16))
      r = 1;
    if (!test_compare_ii(s3->particles[8].base.particle_id, ==, 17))
      r = 1;

    if (!test_compare_ii(s3->particles[0].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[1].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[2].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[3].base.origin_id, ==, 3))
      r = 1;
    if (!test_compare_ii(s3->particles[4].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[5].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[6].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[7].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[8].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[9].base.origin_id, ==,
                         LPTX_ORIGIN_UNDEFINED))
      r = 1;

    LPTX_particle_set_delete(s3);
    s3 = NULL;

    if (!test_compare_ii(LPTX_particle_set_merge(&s3, s1, NULL, merge_right,
                                                 NULL),
                         ==, LPTX_true))
      r = 1;
    if (!test_compare_pp(s3, !=, NULL))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(s3->number_particles, ==, 5))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[0].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[1].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[2].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[3].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[4].base), ==,
                         LPTX_true))
      r = 1;

    if (!test_compare_ii(s3->particles[0].base.particle_id, ==, 6))
      r = 1;
    if (!test_compare_ii(s3->particles[1].base.particle_id, ==, 7))
      r = 1;
    if (!test_compare_ii(s3->particles[2].base.particle_id, ==, 9))
      r = 1;
    if (!test_compare_ii(s3->particles[3].base.particle_id, ==, 11))
      r = 1;
    if (!test_compare_ii(s3->particles[4].base.particle_id, ==, 15))
      r = 1;

    if (!test_compare_ii(s3->particles[0].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[1].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[2].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[3].base.origin_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[4].base.origin_id, ==, 1))
      r = 1;

    LPTX_particle_set_delete(s3);
    s3 = NULL;

    if (!test_compare_ii(LPTX_particle_set_merge(&s3, NULL, s2, merge_left,
                                                 NULL),
                         ==, LPTX_true))
      r = 1;
    if (!test_compare_pp(s3, !=, NULL))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(s3->number_particles, ==, 5))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[0].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[1].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[2].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[3].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[4].base), ==,
                         LPTX_true))
      r = 1;

    if (!test_compare_ii(s3->particles[0].base.particle_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[1].base.particle_id, ==, 4))
      r = 1;
    if (!test_compare_ii(s3->particles[2].base.particle_id, ==, 7))
      r = 1;
    if (!test_compare_ii(s3->particles[3].base.particle_id, ==, 16))
      r = 1;
    if (!test_compare_ii(s3->particles[4].base.particle_id, ==, 17))
      r = 1;

    if (!test_compare_ii(s3->particles[0].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[1].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[2].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[3].base.origin_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[4].base.origin_id, ==, 2))
      r = 1;

    LPTX_particle_set_delete(s3);
    s3 = NULL;

    s1->particles[0].base.particle_id = 6;
    s1->particles[1].base.particle_id = 2;
    s1->particles[2].base.particle_id = 1;
    s1->particles[3].base.particle_id = 6;
    s1->particles[4].base.particle_id = 1;
    s1->particles[0].base.origin_id = 10;
    s1->particles[1].base.origin_id = 11;
    s1->particles[2].base.origin_id = 12;
    s1->particles[3].base.origin_id = 13;
    s1->particles[4].base.origin_id = 14;
    LPTX_particle_set_used(&s1->particles[0].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[1].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[2].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[3].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[4].base, LPTX_true);

    s2->particles[0].base.particle_id = 5;
    s2->particles[1].base.particle_id = 0;
    s2->particles[2].base.particle_id = 1;
    s2->particles[3].base.particle_id = 6;
    s2->particles[4].base.particle_id = 1;
    s2->particles[5].base.particle_id = 4;
    s2->particles[6].base.particle_id = 2;
    s2->particles[0].base.origin_id = 100;
    s2->particles[1].base.origin_id = 200;
    s2->particles[2].base.origin_id = 300;
    s2->particles[3].base.origin_id = 400;
    s2->particles[4].base.origin_id = 500;
    s2->particles[5].base.origin_id = 600;
    s2->particles[6].base.origin_id = 700;
    LPTX_particle_set_used(&s2->particles[0].base, LPTX_true);
    LPTX_particle_set_used(&s2->particles[1].base, LPTX_true);
    LPTX_particle_set_used(&s2->particles[2].base, LPTX_true);
    LPTX_particle_set_used(&s2->particles[3].base, LPTX_true);
    LPTX_particle_set_used(&s2->particles[4].base, LPTX_true);
    LPTX_particle_set_used(&s2->particles[5].base, LPTX_true);
    LPTX_particle_set_used(&s2->particles[6].base, LPTX_true);

    if (!test_compare_ii(LPTX_particle_set_merge(&s3, s1, s2, merge_sum, NULL),
                         ==, LPTX_true))
      r = 1;
    if (!test_compare_pp(s3, !=, NULL))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(s3->number_particles, ==, 12))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[0].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[1].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[2].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[3].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[4].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[5].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[6].base), ==,
                         LPTX_false))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[7].base), ==,
                         LPTX_false))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[8].base), ==,
                         LPTX_false))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[9].base), ==,
                         LPTX_false))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[10].base), ==,
                         LPTX_false))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[11].base), ==,
                         LPTX_false))
      r = 1;

    if (!test_compare_ii(s3->particles[0].base.particle_id, ==, 0))
      r = 1;
    if (!test_compare_ii(s3->particles[1].base.particle_id, ==, 1))
      r = 1;
    if (!test_compare_ii(s3->particles[2].base.particle_id, ==, 2))
      r = 1;
    if (!test_compare_ii(s3->particles[3].base.particle_id, ==, 4))
      r = 1;
    if (!test_compare_ii(s3->particles[4].base.particle_id, ==, 5))
      r = 1;
    if (!test_compare_ii(s3->particles[5].base.particle_id, ==, 6))
      r = 1;

    if (!test_compare_ii(s3->particles[0].base.origin_id, ==, 200))
      r = 1;
    if (!test_compare_ii(s3->particles[1].base.origin_id, ==, 826))
      r = 1;
    if (!test_compare_ii(s3->particles[2].base.origin_id, ==, 711))
      r = 1;
    if (!test_compare_ii(s3->particles[3].base.origin_id, ==, 600))
      r = 1;
    if (!test_compare_ii(s3->particles[4].base.origin_id, ==, 100))
      r = 1;
    if (!test_compare_ii(s3->particles[5].base.origin_id, ==, 423))
      r = 1;

  } while (0);

  do {
    if (s1)
      LPTX_particle_set_delete(s1);
    if (s2)
      LPTX_particle_set_delete(s2);
    if (s3)
      LPTX_particle_set_delete(s3);
    s1 = NULL;
    s2 = NULL;
    s3 = NULL;

    if (!test_compare_pp((s1 = LPTX_particle_set_new(5, 3,
                                                     (LPTX_idtype[]){2, 1, 0})),
                         !=, NULL))
      r = 1;
    if (r)
      break;

    s1->particles[0].base.particle_id = 0;
    s1->particles[1].base.particle_id = 5;
    s1->particles[2].base.particle_id = 7;
    s1->particles[3].base.particle_id = 2;
    s1->particles[4].base.particle_id = 3;
    LPTX_particle_set_used(&s1->particles[0].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[1].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[2].base, LPTX_false);
    LPTX_particle_set_used(&s1->particles[3].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[4].base, LPTX_true);
    s1->particles[0].base.velocity = LPTX_vector_c(0., 1., 2.);
    s1->particles[1].base.velocity = LPTX_vector_c(3., 4., 5.);
    s1->particles[2].base.velocity = LPTX_vector_c(6., 7., 8.);
    s1->particles[3].base.velocity = LPTX_vector_c(9., 10., 11.);
    s1->particles[4].base.velocity = LPTX_vector_c(12., 13., 14.);
    *(LPTX_type *)&s1->particles[0].vectors[0].v[0] = LPTX_C(15.);
    *(LPTX_type *)&s1->particles[0].vectors[0].v[1] = LPTX_C(16.);
    *(LPTX_type *)&s1->particles[1].vectors[0].v[0] = LPTX_C(17.);
    *(LPTX_type *)&s1->particles[1].vectors[0].v[1] = LPTX_C(18.);
    *(LPTX_type *)&s1->particles[2].vectors[0].v[0] = LPTX_C(19.);
    *(LPTX_type *)&s1->particles[2].vectors[0].v[1] = LPTX_C(20.);
    *(LPTX_type *)&s1->particles[3].vectors[0].v[0] = LPTX_C(21.);
    *(LPTX_type *)&s1->particles[3].vectors[0].v[1] = LPTX_C(22.);
    *(LPTX_type *)&s1->particles[4].vectors[0].v[0] = LPTX_C(23.);
    *(LPTX_type *)&s1->particles[4].vectors[0].v[1] = LPTX_C(24.);

    if (!test_compare_ii(LPTX_particle_set_filter(NULL, NULL, s1, filter_odd,
                                                  NULL),
                         ==, LPTX_true))
      r = 1;

    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[0].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[1].base), ==,
                         LPTX_false))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[2].base), ==,
                         LPTX_false))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[3].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[4].base), ==,
                         LPTX_false))
      r = 1;

    LPTX_particle_set_used(&s1->particles[0].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[1].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[2].base, LPTX_false);
    LPTX_particle_set_used(&s1->particles[3].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[4].base, LPTX_true);

    if (!test_compare_ii(LPTX_particle_set_filter(&s2, NULL, s1, filter_odd,
                                                  NULL),
                         ==, LPTX_true))
      r = 1;

    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[0].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[1].base), ==,
                         LPTX_false))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[2].base), ==,
                         LPTX_false))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[3].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[4].base), ==,
                         LPTX_false))
      r = 1;

    if (!test_compare_pp(s2, !=, NULL))
      r = 1;
    do {
      if (!s2)
        break;

      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s2), ==, 2))
        r = 1;

      if (!test_compare_ii(s2->particles[0].base.particle_id, ==, 5))
        r = 1;
      if (!test_compare_ii(s2->particles[1].base.particle_id, ==, 3))
        r = 1;

      if (!test_compare_ii(LPTX_particle_is_used(&s2->particles[0].base), ==,
                           LPTX_true))
        r = 1;
      if (!test_compare_ii(LPTX_particle_is_used(&s2->particles[1].base), ==,
                           LPTX_true))
        r = 1;

      if (!test_compare_lptxvecl(s2->particles[0].base.velocity, ==,
                                 (3., 4., 5.)))
        r = 1;
      if (!test_compare_lptxvecl(s2->particles[1].base.velocity, ==,
                                 (12., 13., 14.)))
        r = 1;

      if (!test_compare_dd(s2->particles[0].vectors[0].v[0], ==, 17.))
        r = 1;
      if (!test_compare_dd(s2->particles[0].vectors[0].v[1], ==, 18.))
        r = 1;
      if (!test_compare_dd(s2->particles[1].vectors[0].v[0], ==, 23.))
        r = 1;
      if (!test_compare_dd(s2->particles[1].vectors[0].v[1], ==, 24.))
        r = 1;

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);

    LPTX_particle_set_used(&s1->particles[0].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[1].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[2].base, LPTX_false);
    LPTX_particle_set_used(&s1->particles[3].base, LPTX_true);
    LPTX_particle_set_used(&s1->particles[4].base, LPTX_true);

    if (!test_compare_ii(LPTX_particle_set_filter(&s2, &s3, s1, filter_odd,
                                                  NULL),
                         ==, LPTX_true))
      r = 1;

    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[0].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[1].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[2].base), ==,
                         LPTX_false))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[3].base), ==,
                         LPTX_true))
      r = 1;
    if (!test_compare_ii(LPTX_particle_is_used(&s1->particles[4].base), ==,
                         LPTX_true))
      r = 1;

    if (!test_compare_pp(s2, !=, NULL))
      r = 1;
    do {
      if (!s2)
        break;

      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s2), ==, 2))
        r = 1;

      if (!test_compare_ii(s2->particles[0].base.particle_id, ==, 5))
        r = 1;
      if (!test_compare_ii(s2->particles[1].base.particle_id, ==, 3))
        r = 1;

      if (!test_compare_ii(LPTX_particle_is_used(&s2->particles[0].base), ==,
                           LPTX_true))
        r = 1;
      if (!test_compare_ii(LPTX_particle_is_used(&s2->particles[1].base), ==,
                           LPTX_true))
        r = 1;

      if (!test_compare_lptxvecl(s2->particles[0].base.velocity, ==,
                                 (3., 4., 5.)))
        r = 1;
      if (!test_compare_lptxvecl(s2->particles[1].base.velocity, ==,
                                 (12., 13., 14.)))
        r = 1;

      if (!test_compare_dd(s2->particles[0].vectors[0].v[0], ==, 17.))
        r = 1;
      if (!test_compare_dd(s2->particles[0].vectors[0].v[1], ==, 18.))
        r = 1;
      if (!test_compare_dd(s2->particles[1].vectors[0].v[0], ==, 23.))
        r = 1;
      if (!test_compare_dd(s2->particles[1].vectors[0].v[1], ==, 24.))
        r = 1;

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);

    if (!test_compare_pp(s3, !=, NULL))
      r = 1;
    do {
      if (!s3)
        break;

      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s3), ==, 2))
        r = 1;

      if (!test_compare_ii(s3->particles[0].base.particle_id, ==, 0))
        r = 1;
      if (!test_compare_ii(s3->particles[1].base.particle_id, ==, 2))
        r = 1;

      if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[0].base), ==,
                           LPTX_true))
        r = 1;
      if (!test_compare_ii(LPTX_particle_is_used(&s3->particles[1].base), ==,
                           LPTX_true))
        r = 1;

      if (!test_compare_lptxvecl(s3->particles[0].base.velocity, ==,
                                 (0., 1., 2.)))
        r = 1;
      if (!test_compare_lptxvecl(s3->particles[1].base.velocity, ==,
                                 (9., 10., 11.)))
        r = 1;

      if (!test_compare_dd(s3->particles[0].vectors[0].v[0], ==, 15.))
        r = 1;
      if (!test_compare_dd(s3->particles[0].vectors[0].v[1], ==, 16.))
        r = 1;
      if (!test_compare_dd(s3->particles[1].vectors[0].v[0], ==, 21.))
        r = 1;
      if (!test_compare_dd(s3->particles[1].vectors[0].v[1], ==, 22.))
        r = 1;

      LPTX_particle_set_delete(s3);
      s3 = NULL;
    } while (0);
  } while (0);

  do {
    if (s1)
      LPTX_particle_set_delete(s1);
    if (s2)
      LPTX_particle_set_delete(s2);
    if (s3)
      LPTX_particle_set_delete(s3);
    s1 = NULL;
    s2 = NULL;
    s3 = NULL;

    if (!test_compare_pp((s1 = LPTX_particle_set_new(5, 3,
                                                     (LPTX_idtype[]){2, 1, 0})),
                         !=, NULL))
      r = 1;
    if (!test_compare_pp((s2 = LPTX_particle_set_new(3, 3,
                                                     (LPTX_idtype[]){2, 1, 0})),
                         !=, NULL))
      r = 1;
    if (!test_compare_pp((s3 =
                            LPTX_particle_set_new(3, 2, (LPTX_idtype[]){1, 3})),
                         !=, NULL))
      r = 1;
    if (!test_compare_pp((p = LPTX_param_new()), !=, NULL))
      r = 1;
    if (r)
      break;

    if (!test_compare_pp(LPTX_particle_set_append(p, s2), ==, s2))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_append(p, s3), ==, s3))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s1), ==, 3))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_set_vector_sizes(s1)[0], ==, 2))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_vector_sizes(s1)[1], ==, 1))
      r = 1;
    if (!test_compare_ii(LPTX_particle_set_vector_sizes(s1)[2], ==, 0))
      r = 1;

    if (!test_compare_ii(LPTX_param_get_allocated_particle_vector_size(p, 0),
                         ==, 2))
      r = 1;
    if (!test_compare_ii(LPTX_param_get_allocated_particle_vector_size(p, 1),
                         ==, 3))
      r = 1;
    if (!test_compare_ii(LPTX_param_get_allocated_particle_vector_size(p, 2),
                         ==, 0))
      r = 1;

    if (!test_compare_pp(LPTX_particle_set_p(s1, 0), ==, &s1->particles[0]))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_p(s1, 1), ==, &s1->particles[1]))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_p(s1, -1), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_p(s1, 6), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_p(p, 0), ==, &s2->particles[0]))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_p(p, 1), ==, &s2->particles[1]))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_p(p, 5), ==, &s3->particles[2]))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_p(p, -1), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_p(p, 6), ==, NULL))
      r = 1;

    if (!test_compare_pp(LPTX_particle_set_v(s1, 0, 0, 0), ==,
                         &s1->particles[0].vectors[0].v[0]))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_v(s1, 0, 0, 1), ==,
                         &s1->particles[0].vectors[0].v[1]))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_v(s1, 0, 0, 2), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_v(s1, 0, 0, -1), ==, NULL))
      r = 1;

    if (!test_compare_pp(LPTX_particle_set_v(s1, 0, 1, 0), ==,
                         &s1->particles[0].vectors[1].v[0]))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_v(s1, 0, 1, 1), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_v(s1, 0, 1, -1), ==, NULL))
      r = 1;

    if (!test_compare_pp(LPTX_particle_set_v(s1, 0, 2, 0), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_v(s1, 0, 3, 0), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_v(s1, 0, -1, 0), ==, NULL))
      r = 1;

    if (!test_compare_pp(LPTX_particle_set_v(s1, 1, 0, 0), ==,
                         &s1->particles[1].vectors[0].v[0]))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_v(s1, -1, 0, 0), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_particle_set_v(s1, 6, 0, 0), ==, NULL))
      r = 1;

    if (!test_compare_pp(LPTX_param_pt_v(p, 0, 0, 0), ==,
                         &s2->particles[0].vectors[0].v[0]))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 0, 0, 1), ==,
                         &s2->particles[0].vectors[0].v[1]))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 0, 0, 2), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 0, 0, -1), ==, NULL))
      r = 1;

    if (!test_compare_pp(LPTX_param_pt_v(p, 0, 1, 0), ==,
                         &s2->particles[0].vectors[1].v[0]))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 0, 1, 1), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 0, 1, -1), ==, NULL))
      r = 1;

    if (!test_compare_pp(LPTX_param_pt_v(p, 0, 2, 0), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 0, 3, 0), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 0, -1, 0), ==, NULL))
      r = 1;

    if (!test_compare_pp(LPTX_param_pt_v(p, 1, 0, 0), ==,
                         &s2->particles[1].vectors[0].v[0]))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 5, 0, 0), ==,
                         &s3->particles[2].vectors[0].v[0]))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 5, 0, 1), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 5, 0, 2), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 5, 0, -1), ==, NULL))
      r = 1;

    if (!test_compare_pp(LPTX_param_pt_v(p, 5, 1, 0), ==,
                         &s3->particles[2].vectors[1].v[0]))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 5, 1, 1), ==,
                         &s3->particles[2].vectors[1].v[1]))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 5, 1, 4), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 5, 1, -1), ==, NULL))
      r = 1;

    if (!test_compare_pp(LPTX_param_pt_v(p, 5, 2, 0), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 5, 3, 0), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 5, -1, 0), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, -1, 0, 0), ==, NULL))
      r = 1;
    if (!test_compare_pp(LPTX_param_pt_v(p, 6, 0, 0), ==, NULL))
      r = 1;

#define LPTX_comp_iieq(v, i, e, ...) test_compare_ii(v[i], ==, e)
#define LPTX_comp_ddeq(v, i, e, ...) test_compare_dd(v[i], ==, e)
#define LPTX_comp_vveq(v, i, e, ...) test_compare_lptxvecl(v[i], ==, e)

#define LPTX_compv_fP(f, v, i, e, ...) f(v, i, e, __VA_ARGS__)
#define LPTX_compv_fP0(p0, ...) p0
#define LPTX_compv_fP1(p0, ...) LPTX_compv_fP0(__VA_ARGS__)
#define LPTX_compv_fP2(p0, ...) LPTX_compv_fP1(__VA_ARGS__)
#define LPTX_compv_fP3(p0, ...) LPTX_compv_fP2(__VA_ARGS__)
#define LPTX_compv_fP4(p0, ...) LPTX_compv_fP3(__VA_ARGS__)
#define LPTX_compv_fP5(p0, ...) LPTX_compv_fP4(__VA_ARGS__)
#define LPTX_compv_fP6(p0, ...) LPTX_compv_fP5(__VA_ARGS__)
#define LPTX_compv_fP7(p0, ...) LPTX_compv_fP6(__VA_ARGS__)
#define LPTX_compv_fP8(p0, ...) LPTX_compv_fP7(__VA_ARGS__)

#define LPTX_compv_fE1(f, v, a, ...) \
  LPTX_compv_fP(f, v, 0, LPTX_compv_fP0(__VA_ARGS__), LPTX_test_Y a)
#define LPTX_compv_fE2(f, v, a, ...)      \
  LPTX_compv_fE1(f, v, a, __VA_ARGS__) && \
    LPTX_compv_fP(f, v, 1, LPTX_compv_fP1(__VA_ARGS__), LPTX_testv_Y a)
#define LPTX_compv_fE3(f, v, a, ...)      \
  LPTX_compv_fE2(f, v, a, __VA_ARGS__) && \
    LPTX_compv_fP(f, v, 2, LPTX_compv_fP2(__VA_ARGS__), LPTX_testv_Y a)
#define LPTX_compv_fE4(f, v, a, ...)      \
  LPTX_compv_fE3(f, v, a, __VA_ARGS__) && \
    LPTX_compv_fP(f, v, 3, LPTX_compv_fP3(__VA_ARGS__), LPTX_testv_Y a)

#define LPTX_compv_fE(f, v, n, a, ...) (LPTX_compv_fE##n(f, v, a, __VA_ARGS__))

#define LPTX_compv_fN(...) LPTX_test_res_comp_N(__VA_ARGS__)
#define LPTX_compv_fX(f, v, n, a, ...) LPTX_compv_fE(f, v, n, a, __VA_ARGS__)

#define LPTX_compv_f(f, v, e, ...) \
  LPTX_compv_fX(f, v, LPTX_compv_fN e, (__VA_ARGS__), LPTX_test_Y e, LPTX_empty)

#define LPTX_compv_ddeq(v, i, e, ...) \
  LPTX_compv_f(LPTX_comp_ddeq, v[i], e, __VA_ARGS__)

#define LPTX_comp_pt(getps, memf, cmp_x, v, i, st, sz, e)               \
  (getps(v, st + i)                                                     \
     ? cmp_x(memf(getps(v, st + i)), ==, e,                             \
             memf##_expr(#v "->particles[" #st " + " #i "]") " == " #e, \
             __FILE__, __LINE__)                                        \
     : 1)

#define LPTX_comp_set_pt(memf, cmp_x, v, i, st, sz, e) \
  LPTX_comp_pt(LPTX_particle_set_p, memf, cmp_x, v, i, st, sz, e)

#define LPTX_comp_prm_pt(memf, cmp_x, v, i, st, sz, e) \
  LPTX_comp_pt(LPTX_param_pt_p, memf, cmp_x, v, i, st, sz, e)

#define LPTX_comp_ptid(p) p->base.particle_id
#define LPTX_comp_ptid_expr(p) p ".base.particle_id"

#define LPTX_comp_sptid(v, i, e, st, sz, ...) \
  LPTX_comp_set_pt(LPTX_comp_ptid, test_compare_x_ii, v, i, st, sz, e)

#define LPTX_comp_pptid(v, i, e, st, sz, ...) \
  LPTX_comp_prm_pt(LPTX_comp_ptid, test_compare_x_ii, v, i, st, sz, e)

    {
      LPTX_idtype *ids;
      if (LPTX_test_scl_setget(LPTX_particle_set_set_particle_id,
                               LPTX_particle_set_get_particle_id, s1, 0,
                               LPTX_idtype, 3, 3, ids, LPTX_comp_sptid,
                               LPTX_comp_iieq, (1, 3, 5), (1, 3, 5), (0, 0, 0),
                               (1, 3, 5)))
        r = 1;

      if (LPTX_test_scl_setget(LPTX_particle_set_set_particle_id,
                               LPTX_particle_set_get_particle_id, s1, 3,
                               LPTX_idtype, 3, 2, ids, LPTX_comp_sptid,
                               LPTX_comp_iieq, (2, 4, 6), (2, 4), (0, 0, 0),
                               (2, 4, 0)))
        r = 1;

      if (LPTX_test_scl_setget(LPTX_set_particle_particle_id,
                               LPTX_get_particle_particle_id, p, 0, LPTX_idtype,
                               3, 3, ids, LPTX_comp_pptid, LPTX_comp_iieq,
                               (1, 3, 5), (1, 3, 5), (0, 0, 0), (1, 3, 5)))
        r = 1;

      if (LPTX_test_scl_setget(LPTX_set_particle_particle_id,
                               LPTX_get_particle_particle_id, p, 3, LPTX_idtype,
                               4, 3, ids, LPTX_comp_pptid, LPTX_comp_iieq,
                               (2, 4, 6, 8), (2, 4, 6), (0, 0, 0, 0),
                               (2, 4, 6, 0)))
        r = 1;
    }

#define LPTX_comp_oid(p) p->base.origin_id
#define LPTX_comp_oid_expr(p) p ".base.origin_id"

#define LPTX_comp_soid(v, i, e, st, sz, ...) \
  LPTX_comp_set_pt(LPTX_comp_oid, test_compare_x_ii, v, i, st, sz, e)

#define LPTX_comp_poid(v, i, e, st, sz, ...) \
  LPTX_comp_prm_pt(LPTX_comp_oid, test_compare_x_ii, v, i, st, sz, e)

    {
      LPTX_idtype *ids;
      if (LPTX_test_scl_setget(LPTX_particle_set_set_origin_id,
                               LPTX_particle_set_get_origin_id, s1, 0,
                               LPTX_idtype, 3, 3, ids, LPTX_comp_soid,
                               LPTX_comp_iieq, (2, 3, 7), (2, 3, 7), (0, 0, 0),
                               (2, 3, 7)))
        r = 1;

      if (LPTX_test_scl_setget(LPTX_particle_set_set_origin_id,
                               LPTX_particle_set_get_origin_id, s1, 3,
                               LPTX_idtype, 3, 2, ids, LPTX_comp_soid,
                               LPTX_comp_iieq, (2, 4, 6), (2, 4), (0, 0, 0),
                               (2, 4, 0)))
        r = 1;

      if (LPTX_test_scl_setget(LPTX_set_particle_origin_id,
                               LPTX_get_particle_origin_id, p, 0, LPTX_idtype,
                               3, 3, ids, LPTX_comp_poid, LPTX_comp_iieq,
                               (2, 3, 7), (2, 3, 7), (0, 0, 0), (2, 3, 7)))
        r = 1;

      if (LPTX_test_scl_setget(LPTX_set_particle_origin_id,
                               LPTX_get_particle_origin_id, p, 3, LPTX_idtype,
                               4, 3, ids, LPTX_comp_poid, LPTX_comp_iieq,
                               (2, 4, 6, 8), (2, 4, 6), (0, 0, 0, 0),
                               (2, 4, 6, 0)))
        r = 1;
    }

#define LPTX_comp_fvel(p) p->base.fluid_velocity
#define LPTX_comp_fvel_expr(p) p ".base.fluid_velocity"

#define LPTX_comp_sfvel(v, i, e, st, sz, ...) \
  LPTX_comp_set_pt(LPTX_comp_fvel, test_compare_x_lptxvecl, v, i, st, sz, e)

#define LPTX_comp_pfvel(v, i, e, st, sz, ...) \
  LPTX_comp_prm_pt(LPTX_comp_fvel, test_compare_x_lptxvecl, v, i, st, sz, e)
    {
      LPTX_vector *v;

      if (LPTX_test_scl_setget(LPTX_particle_set_set_fluid_velocity,
                               LPTX_particle_set_get_fluid_velocity, s1, 0,
                               LPTX_vector, 3, 3, v, LPTX_comp_sfvel,
                               LPTX_comp_vveq,
                               ((1., 3., 5.), (2., 4., 6.), (7., 8., 9.)),
                               ((1., 3., 5.), (2., 4., 6.), (7., 8., 9.)),
                               ((0., 0., 0.), (0., 0., 0.), (0., 0., 0.)),
                               ((1., 3., 5.), (2., 4., 6.), (7., 8., 9.))))
        r = 1;
    }

    {
      LPTX_type *v;

      if (LPTX_test_scl_setget(LPTX_particle_set_set_fluid_velocity_t_aos,
                               LPTX_particle_set_get_fluid_velocity_t_aos, s1,
                               0, LPTX_type, 9, 9, v, LPTX_comp_sfvel,
                               LPTX_comp_ddeq,
                               (1., 3., 5., 2., 4., 6., 7., 8., 9.),
                               ((1., 3., 5.), (2., 4., 6.), (7., 8., 9.)),
                               (0., 0., 0., 0., 0., 0., 0., 0., 0.),
                               (1., 3., 5., 2., 4., 6., 7., 8., 9.)))
        r = 1;

      if (LPTX_test_scl_setget(LPTX_particle_set_set_fluid_velocity_t_aos,
                               LPTX_particle_set_get_fluid_velocity_t_aos, s1,
                               0, LPTX_type, 8, 6, v, LPTX_comp_sfvel,
                               LPTX_comp_ddeq, (2., 4., 6., 7., 8., 9., 1., 3.),
                               ((2., 4., 6.), (7., 8., 9.), (7., 8., 9.)),
                               (0., 0., 0., 0., 0., 0., 0., 0.),
                               (2., 4., 6., 7., 8., 9., 0., 0.)))
        r = 1;
    }

    {
      LPTX_type *vx, *vy, *vz;

      if (LPTX_test_vecsoa_setget(LPTX_particle_set_set_fluid_velocity_t_soa,
                                  LPTX_particle_set_get_fluid_velocity_t_soa,
                                  s1, 0, LPTX_type, 3, 3, vx, vy, vz,
                                  LPTX_comp_sfvel, LPTX_comp_ddeq, (7., 8., 9.),
                                  (1., 3., 5.), (2., 4., 6.),
                                  ((7., 1., 2.), (8., 3., 4.), (9., 5., 6.)),
                                  (0., 0., 0.), (0., 0., 0.), (0., 0., 0.),
                                  (7., 8., 9.), (1., 3., 5.), (2., 4., 6.)))
        r = 1;
    }

#define LPTX_comp_exit(p) LPTX_particle_is_exited(&p->base)
#define LPTX_comp_exit_expr(p) "LPTX_particle_is_exited(&" p ".base)"

#define LPTX_comp_sexit(v, i, e, st, sz, ...) \
  LPTX_comp_set_pt(LPTX_comp_exit, test_compare_x_ii, v, i, st, sz, e)

#define LPTX_comp_pexit(v, i, e, st, sz, ...) \
  LPTX_comp_prm_pt(LPTX_comp_exit, test_compare_x_ii, v, i, st, sz, e)
    {
      int *exited;
      if (LPTX_test_scl_setget(LPTX_particle_set_set_exited_i,
                               LPTX_particle_set_get_exited_i, s1, 0, int, 3, 3,
                               exited, LPTX_comp_sexit, LPTX_comp_iieq,
                               (1, 0, 3), (LPTX_true, LPTX_false, LPTX_true),
                               (0, 1, 0), (1, 0, 1)))
        r = 1;
    }

#define LPTX_compv_pt(getps, memf, cmp_x, v, i, st, sz, vi, ic, e)       \
  (getps(v, st + i, vi, ic)                                              \
     ? cmp_x(memf(*getps(v, st + i, vi, ic)), ==, e,                     \
             memf##_expr(#v "->particles[" #st " + " #i "].vectors[" #vi \
                            "].v[" #ic "]") " == " #e,                   \
             __FILE__, __LINE__)                                         \
     : 1)

#define LPTX_compv_ptV(...) LPTX_compv_pt(__VA_ARGS__)

#define LPTX_compv_ptE1(e, ...) \
  LPTX_compv_ptV(__VA_ARGS__, 0, LPTX_test_res_comp_PE(0, e))

#define LPTX_compv_ptE2(e, ...)       \
  (LPTX_compv_ptE1(e, __VA_ARGS__) && \
   LPTX_compv_ptV(__VA_ARGS__, 1, LPTX_test_res_comp_PE(1, e)))

#define LPTX_compv_ptE3(e, ...)       \
  (LPTX_compv_ptE2(e, __VA_ARGS__) && \
   LPTX_compv_ptV(__VA_ARGS__, 1, LPTX_test_res_comp_PE(2, e)))

#define LPTX_compv_ptE4(e, ...)       \
  (LPTX_compv_ptE3(e, __VA_ARGS__) && \
   LPTX_compv_ptV(__VA_ARGS__, 1, LPTX_test_res_comp_PE(3, e)))

#define LPTX_compv_ptE5(e, ...)       \
  (LPTX_compv_ptE4(e, __VA_ARGS__) && \
   LPTX_compv_ptV(__VA_ARGS__, 1, LPTX_test_res_comp_PE(4, e)))

#define LPTX_compv_ptE6(e, ...)       \
  (LPTX_compv_ptE5(e, __VA_ARGS__) && \
   LPTX_compv_ptV(__VA_ARGS__, 1, LPTX_test_res_comp_PE(5, e)))

#define LPTX_compv_ptE7(e, ...)       \
  (LPTX_compv_ptE6(e, __VA_ARGS__) && \
   LPTX_compv_ptV(__VA_ARGS__, 1, LPTX_test_res_comp_PE(6, e)))

#define LPTX_compv_ptE8(e, ...)       \
  (LPTX_compv_ptE7(e, __VA_ARGS__) && \
   LPTX_compv_ptV(__VA_ARGS__, 1, LPTX_test_res_comp_PE(7, e)))

#define LPTX_compv_ptE9(e, ...)       \
  (LPTX_compv_ptE8(e, __VA_ARGS__) && \
   LPTX_compv_ptV(__VA_ARGS__, 1, LPTX_test_res_comp_PE(8, e)))

#define LPTX_compv_ptE(n, e, ...) LPTX_compv_ptE##n(e, __VA_ARGS__)

#define LPTX_compv_ptN(n, e, ...) LPTX_compv_ptE(n, e, __VA_ARGS__)

#define LPTX_compv_ptX(e, ...) \
  LPTX_compv_ptN(LPTX_test_res_comp_N e, e, __VA_ARGS__)

#define LPTX_compv_set_pt(memf, cmp_x, v, i, e, st, sz, vi) \
  LPTX_compv_ptX(e, LPTX_particle_set_v, memf, cmp_x, v, i, st, sz, vi)

#define LPTX_compv_prm_pt(memf, cmp_x, v, i, e, st, sz, vi) \
  LPTX_compv_ptX(e, LPTX_param_pt_v, memf, cmp_x, v, i, st, sz, vi)

#define LPTX_comp_vector(p) p
#define LPTX_comp_vector_expr(p) p

#define LPTX_comp_spvec(v, i, e, st, sz, vi, ...) \
  LPTX_compv_set_pt(LPTX_comp_vector, test_compare_x_dd, v, i, e, st, sz, vi)

#define LPTX_comp_ppvec(v, i, e, st, sz, vi, ...) \
  LPTX_compv_prm_pt(LPTX_comp_vector, test_compare_x_dd, v, i, e, st, sz, vi)

    {
      LPTX_type **v;
      if (LPTX_test_vcnsoa_setget(LPTX_particle_set_set_vector_t_soa,
                                  LPTX_particle_set_get_vector_t_soa, s1, 0, 0,
                                  LPTX_type, 3, 3, 2, v, LPTX_comp_spvec,
                                  LPTX_compv_ddeq, ((1, 2, 3), (4, 5, 6)),
                                  ((1, 4), (2, 5), (3, 6)),
                                  ((0, 0, 0), (0, 0, 0)),
                                  ((1, 2, 3), (4, 5, 6))))
        r = 1;
    }
  } while (0);

  if (s1)
    LPTX_particle_set_delete(s1);
  if (s2)
    LPTX_particle_set_delete(s2);
  if (s3)
    LPTX_particle_set_delete(s3);
  if (p)
    LPTX_param_delete(p);
  return r;
}
