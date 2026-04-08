
#include "jupiter/control/defs.h"
#include "jupiter/control/error.h"
#include "jupiter/control/shared_object.h"
#include "jupiter/control/shared_object_priv.h"
#include "jupiter/geometry/util.h"
#include "test-util.h"
#include "test/control/control_test.h"
#include "test/control/control_test_expect_raise.h"

struct b1
{
  void *trap[1];
  jcntrl_shared_object obj;
};
#define b1__ancestor jcntrl_shared_object
#define b1__dnmem obj
enum b1_vtable_names
{
  b1_f1_id = JCNTRL_VTABLE_START(b1),
  b1_f2_id,
  b1_f3_id,
  JCNTRL_VTABLE_SIZE(b1)
};

struct b2
{
  void *trap[2];
  struct b1 obj;
};
#define b2__ancestor b1
#define b2__dnmem obj.b1__dnmem
JCNTRL_VTABLE_NONE(b2);

struct c1
{
  void *trap[3];
  jcntrl_shared_object obj;
};
#define c1__ancestor jcntrl_shared_object
#define c1__dnmem obj
JCNTRL_VTABLE_NONE(c1);

struct d1
{
  void *trap[4];
  jcntrl_shared_object obj;
};
#define d1__ancestor jcntrl_shared_object
#define d1__dnmem obj
enum d1_vtable_names
{
  d1_f4_id = JCNTRL_VTABLE_START(d1),
  JCNTRL_VTABLE_SIZE(d1)
};

struct base_call_counter
{
  int downcast, initializer, destructor, allocator, deleter;
};

struct call_counter
{
  struct base_call_counter b1, b2, c1, d1;
  int b1_f1, b1_f2, b1_f3;
  int b2_f1, b2_f2, b2_f3;
  int d1_f4;
};

static struct call_counter counter = {0};

static void call_counter_reset(void) { counter = (struct call_counter){0}; }
static void called_downcast(struct base_call_counter *p) { ++p->downcast; }
static void called_init(struct base_call_counter *p) { ++p->initializer; }
static void called_destr(struct base_call_counter *p) { ++p->destructor; }
static void called_alloc(struct base_call_counter *p) { ++p->allocator; }
static void called_delete(struct base_call_counter *p) { ++p->deleter; }

static JCNTRL_SHARED_METADATA_INIT_DECL(b1);
static JCNTRL_SHARED_METADATA_INIT_DECL(b2);
static JCNTRL_SHARED_METADATA_INIT_DECL(c1);
static JCNTRL_SHARED_METADATA_INIT_DECL(d1);

static jcntrl_shared_object *b1_object(struct b1 *p) { return &p->obj; }
static jcntrl_shared_object *b2_object(struct b2 *p)
{
  return b1_object(&p->obj);
}

static jcntrl_shared_object *c1_object(struct c1 *p) { return &p->obj; }
static jcntrl_shared_object *d1_object(struct d1 *p) { return &p->obj; }

static struct b1 *b1_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL_T(b1, struct b1, obj);
}

static struct b2 *b2_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL_T(b2, struct b2, obj);
}

static struct c1 *c1_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL_T(c1, struct c1, obj);
}

static struct d1 *d1_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL_T(d1, struct d1, obj);
}

static void *b1_downcast_v(jcntrl_shared_object *obj)
{
  called_downcast(&counter.b1);
  return b1_downcast_impl(obj);
}

static void *b2_downcast_v(jcntrl_shared_object *obj)
{
  called_downcast(&counter.b2);
  return b2_downcast_impl(obj);
}

static void *c1_downcast_v(jcntrl_shared_object *obj)
{
  called_downcast(&counter.c1);
  return c1_downcast_impl(obj);
}

static void *d1_downcast_v(jcntrl_shared_object *obj)
{
  called_downcast(&counter.d1);
  return d1_downcast_impl(obj);
}

static int b1_initializer(jcntrl_shared_object *obj)
{
  called_init(&counter.b1);
  return 1;
}

static int b2_initializer(jcntrl_shared_object *obj)
{
  called_init(&counter.b2);
  return 1;
}

static int d1_initializer(jcntrl_shared_object *obj)
{
  called_init(&counter.d1);
  return 1;
}

static void b1_destructor(jcntrl_shared_object *obj)
{
  called_destr(&counter.b1);
}

static void b2_destructor(jcntrl_shared_object *obj)
{
  called_destr(&counter.b2);
}

static void d1_destructor(jcntrl_shared_object *obj)
{
  called_destr(&counter.d1);
}

static jcntrl_shared_object *b2_alloc(void)
{
  called_alloc(&counter.b2);
  struct b2 *p = jcntrl_shared_object_default_allocator(struct b2);
  return p ? b2_object(p) : NULL;
}

static jcntrl_shared_object *c1_alloc(void)
{
  called_alloc(&counter.c1);
  struct c1 *p = jcntrl_shared_object_default_allocator(struct c1);
  return p ? c1_object(p) : NULL;
}

static jcntrl_shared_object *d1_alloc(void)
{
  called_alloc(&counter.d1);
  return NULL;
}

static void b2_deleter(jcntrl_shared_object *obj)
{
  called_delete(&counter.b2);
  jcntrl_shared_object_default_deleter(obj);
}

static void c1_deleter(jcntrl_shared_object *obj)
{
  called_delete(&counter.c1);
  jcntrl_shared_object_default_deleter(obj);
}

static void d1_deleter(jcntrl_shared_object *obj)
{
  called_delete(&counter.d1);
}

struct b1_f1_args
{
  int ret;
};

static void b1_f1__wrapper(jcntrl_shared_object *obj, void *args,
                           int (*f)(jcntrl_shared_object *obj))
{
  struct b1_f1_args *p = args;
  p->ret = f(obj);
}

static int b1_f1_impl(jcntrl_shared_object *obj)
{
  ++counter.b1_f1;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(b1, b1, f1)

static int b2_f1_impl(jcntrl_shared_object *obj)
{
  ++counter.b2_f1;
  return 11;
}

JCNTRL_VIRTUAL_WRAP(b2, b1, f1)

static int b1_f1(struct b1 *obj)
{
  struct b1_f1_args p = {0};
  jcntrl_shared_object_call_virtual(b1_object(obj), b1, f1, &p);
  return p.ret;
}

static void b1_f2__wrapper(jcntrl_shared_object *obj, void *args,
                           void (*f)(jcntrl_shared_object *obj))
{
  f(obj);
}

static void b1_f2_impl(jcntrl_shared_object *obj) { ++counter.b1_f2; }

JCNTRL_VIRTUAL_WRAP(b1, b1, f2)

static void b2_f2_impl(jcntrl_shared_object *obj)
{
  ++counter.b2_f2;
  jcntrl_shared_object_call_super(b1_metadata_init(), b1, f2, NULL);
}

JCNTRL_VIRTUAL_WRAP(b2, b1, f2)

static void b1_f2(struct b1 *obj)
{
  jcntrl_shared_object_call_virtual(b1_object(obj), b1, f1, NULL);
}

static void b1_f3_impl(jcntrl_shared_object *obj, void *p) { ++counter.b1_f3; }
static void d1_f4_impl(jcntrl_shared_object *obj, void *p) { ++counter.d1_f4; }

static void b1_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = b1_downcast_v;
  p->initializer = b1_initializer;
  p->destructor = b1_destructor;
  p->allocator = NULL;
  p->deleter = NULL;
  JCNTRL_VIRTUAL_WRAP_SET(p, b1, b1, f1);
  JCNTRL_VIRTUAL_WRAP_SET(p, b1, b1, f2);
  p->vtable[b1_f3_id].func = b1_f3_impl;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(b1, b1_init_func)

static void b2_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = b2_downcast_v;
  p->initializer = b2_initializer;
  p->destructor = b2_destructor;
  p->allocator = b2_alloc;
  p->deleter = b2_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, b2, b1, f1);
  JCNTRL_VIRTUAL_WRAP_SET(p, b2, b1, f2);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(b2, b2_init_func)

static void c1_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = c1_downcast_v;
  p->initializer = NULL;
  p->destructor = NULL;
  p->allocator = c1_alloc;
  p->deleter = c1_deleter;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(c1, c1_init_func)

static void d1_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = d1_downcast_v;
  p->initializer = d1_initializer;
  p->destructor = d1_destructor;
  p->allocator = d1_alloc;
  p->deleter = d1_deleter;
  p->vtable[d1_f4_id].func = d1_f4_impl;
  JCNTRL_ASSERT(d1_f4_id == b1_f1_id);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(d1, d1_init_func)

static int ints_cmp(int n, int **a, int **b)
{
  for (int i = 0; i < n; ++i)
    if (*(a[i]) != *(b[i]))
      return 0;
  return 1;
}

#define ints_cmp_1(x, m1) &x->m1
#define ints_cmp_2(x, m1, m2) &x->m1, &x->m2
#define ints_cmp_3(x, m1, ...) &x->m1, ints_cmp_2(x, __VA_ARGS__)
#define ints_cmp_4(x, m1, ...) &x->m1, ints_cmp_3(x, __VA_ARGS__)
#define ints_cmp_5(x, m1, ...) &x->m1, ints_cmp_4(x, __VA_ARGS__)
#define ints_cmp_6(x, m1, ...) &x->m1, ints_cmp_5(x, __VA_ARGS__)
#define ints_cmp_7(x, m1, ...) &x->m1, ints_cmp_6(x, __VA_ARGS__)
#define ints_cmp_8(x, m1, ...) &x->m1, ints_cmp_7(x, __VA_ARGS__)
#define ints_cmp_9(x, m1, ...) &x->m1, ints_cmp_8(x, __VA_ARGS__)
#define ints_cmp_10(x, m1, ...) &x->m1, ints_cmp_9(x, __VA_ARGS__)
#define ints_cmp_w(n, t, x, ...) \
  (t *[]) { ints_cmp_##n(x, __VA_ARGS__) }
#define ints_cmp_e(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, n, ...) n
#define ints_cmp_p(...) \
  ints_cmp_e(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define ints_cmp_n(...) ints_cmp_p(__VA_ARGS__)
#define ints_cmp_y(n, f, t, a, b, ...) \
  f(n, ints_cmp_w(n, t, a, __VA_ARGS__), ints_cmp_w(n, t, b, __VA_ARGS__))
#define ints_cmp_x(n, f, t, a, b, ...) ints_cmp_y(n, f, t, a, b, __VA_ARGS__)
#define ints_cmp_b(f, t, a, b, ...) \
  ints_cmp_x(ints_cmp_n(__VA_ARGS__), f, t, a, b, __VA_ARGS__)
#define ints_cmp(a, b, ...) ints_cmp_b(ints_cmp, int, a, b, __VA_ARGS__)
#define bcnt_cmp(a, b, ...) \
  ints_cmp_b(base_counters_cmp, struct base_call_counter, a, b, __VA_ARGS__)

static int base_counter_cmp(struct base_call_counter *a,
                            struct base_call_counter *b)
{
  return ints_cmp(a, b, downcast, initializer, destructor, allocator, deleter);
}

static int base_counters_cmp(int n, struct base_call_counter **a,
                             struct base_call_counter **b)
{
  for (int i = 0; i < n; ++i)
    if (!base_counter_cmp(a[i], b[i]))
      return 0;
  return 1;
}

static int counter_cmp(struct call_counter *a, struct call_counter *b)
{
  if (!bcnt_cmp(a, b, b1, b2, c1, d1))
    return 0;
  return ints_cmp(a, b, b1_f1, b1_f2, b1_f3, b2_f1, b2_f2, b2_f3, d1_f4);
}

static int test_counter_cmp(void *got, void *exp, void *arg)
{
  return counter_cmp((struct call_counter *)got, (struct call_counter *)exp);
}

static int test_counter_prn(char **buf, void *val, void *arg)
{
  const int p = 17;
  struct call_counter *c = (struct call_counter *)val;
  return test_compare_asprintf(
    buf,
    "%*s{b1 = {downcast = %d, initializer = %d, destructor = %d,\n"
    "%*s       allocator = %d, deleter = %d},\n"
    "%*s b2 = {downcast = %d, initializer = %d, destructor = %d,\n"
    "%*s       allocator = %d, deleter = %d},\n"
    "%*s c1 = {downcast = %d, initializer = %d, destructor = %d,\n"
    "%*s       allocator = %d, deleter = %d},\n"
    "%*s d1 = {downcast = %d, initializer = %d, destructor = %d,\n"
    "%*s       allocator = %d, deleter = %d},\n"
    "%*s b1_f1 = %d, b1_f2 = %d, b1_f3 = %d,\n"
    "%*s b2_f1 = %d, b2_f2 = %d, b2_f3 = %d, d1_f4 = %d}",      //
    0, "", c->b1.downcast, c->b1.initializer, c->b1.destructor, //
    p, "", c->b1.allocator, c->b1.deleter,                      //
    p, "", c->b2.downcast, c->b2.initializer, c->b2.destructor, //
    p, "", c->b2.allocator, c->b2.deleter,                      //
    p, "", c->c1.downcast, c->c1.initializer, c->c1.destructor, //
    p, "", c->c1.allocator, c->c1.deleter,                      //
    p, "", c->d1.downcast, c->d1.initializer, c->d1.destructor, //
    p, "", c->d1.allocator, c->d1.deleter,                      //
    p, "", c->b1_f1, c->b1_f2, c->b1_f3,                        //
    p, "", c->b2_f1, c->b2_f2, c->b2_f3, c->d1_f4);
}

#define test_compare_ccntv(got, exp)                                         \
  test_compare_typed(&got, &exp, NULL, #got " == " #exp, __FILE__, __LINE__, \
                     test_counter_cmp, test_counter_prn, test_counter_prn,   \
                     NULL)

#define test_compare_ccnt_w(...) __VA_ARGS__
#define test_compare_ccnt(got, exp)                                         \
  test_compare_typed(&got, &((struct call_counter)test_compare_ccnt_w exp), \
                     NULL, #got " == (struct call_counter)" #exp, __FILE__, \
                     __LINE__, test_counter_cmp, test_counter_prn,          \
                     test_counter_prn, NULL)

int test_control_object(void)
{
  struct b1 ob1_1, ob1_2, *pb1;
  struct b2 ob2_1, ob2_2, *pb2;
  struct c1 oc1;
  struct d1 od1;
  int ret = 0;

  pb1 = NULL;
  pb2 = NULL;

  control_test_use_expect_raise();
  begin_expected_raise();
  call_counter_reset();

  fprintf(stderr, "..... Test for test_compare_ccnt()\n");
  call_counter_reset();
  counter.b1.allocator = 1;
  if (!test_compare_ii(test_compare_ccnt(counter, ({0})), ==, 0))
    ret = 1;

  counter.c1.initializer = 99;
  if (!test_compare_ii(test_compare_ccnt(counter, ({.b1.allocator = 1,
                                                    .c1.initializer = 99})),
                       ==, 1))
    ret = 1;

  call_counter_reset();
  counter.b1_f1 = 224;
  if (!test_compare_ii(test_compare_ccnt(counter, ({.b1_f1 = 224})), ==, 1))
    ret = 1;

  counter.d1_f4 = 99;
  if (!test_compare_ii(test_compare_ccnt(counter, ({.b1_f1 = 224})), ==, 0))
    ret = 1;

  fprintf(stderr, "..... End test for test_compare_ccnt()\n");

  if (!test_compare_ii(jcntrl_shared_object_data_is_a(b1_metadata_init(),
                                                      b2_metadata_init()),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_shared_object_data_is_a(b2_metadata_init(),
                                                      b1_metadata_init()),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_shared_object_data_is_a(b1_metadata_init(),
                                                      c1_metadata_init()),
                       ==, 0))
    ret = 1;

  call_counter_reset();
  jcntrl_shared_object_static_init(b1_object(&ob1_1), b1_metadata_init());
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_ii(counter.b1.initializer, ==, 1))
    ret = 1;

  call_counter_reset();
  jcntrl_shared_object_static_init(b1_object(&ob1_2), b1_metadata_init());
  if (!test_compare_ccnt(counter, ({.b1 = {.initializer = 1}})))
    ret = 1;

  // statically initialized
  if (!test_compare_ii(b1_object(&ob1_2)->ref_count, ==, -1))
    ret = 1;

  // Asserted not to:
  // if (!test_compare_pp(jcntrl_shared_object_shallow_copy(b1_object(&ob1_2)),
  //                      ==, NULL))
  //   ret = 1;

  begin_expected_raise();
  call_counter_reset();
  if (!test_compare_pp(jcntrl_shared_object_new_by_meta(b1_metadata_init()), ==,
                       NULL))
    ret = 1;

  if (!test_compare_ccnt(counter, ({0})))
    ret = 1;
  if (test_expect_raise_one(JCNTRL_ERROR_ARGUMENT))
    ret = 1;

  begin_expected_raise();
  call_counter_reset();
  if (!test_compare_pp((pb2 =
                          jcntrl_shared_object_new_by_meta(b2_metadata_init())),
                       !=, NULL))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  // calls downcast for returning original pointer
  if (!test_compare_ccnt(counter, ({.b1 = {.initializer = 1},
                                    .b2 = {.downcast = 1,
                                           .initializer = 1,
                                           .allocator = 1}})))
    ret = 1;

  if (pb2) {
    if (!test_compare_ii(b2_object(pb2)->ref_count, ==, 1))
      ret = 1;

    call_counter_reset();
    if (!test_compare_pp(jcntrl_shared_object_take_ownership(b2_object(pb2)),
                         ==, b2_object(pb2)))
      ret = 1;

    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ccnt(counter, ({0})))
      ret = 1;
    if (!test_compare_ii(b2_object(pb2)->ref_count, ==, 2))
      ret = 1;
    if (!test_compare_ii(jcntrl_shared_object_refcount(b2_object(pb2)), ==, 2))
      ret = 1;
    if (!test_compare_ii(jcntrl_shared_object_is_shared(b2_object(pb2)), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_shared_object_is_static(b2_object(pb2)), ==, 0))
      ret = 1;

    call_counter_reset();
    if (!test_compare_pp(jcntrl_shared_object_delete(b2_object(pb2)), ==,
                         b2_object(pb2)))
      ret = 1;

    if (!test_compare_ccnt(counter, ({0})))
      ret = 1;
    if (!test_compare_ii(b2_object(pb2)->ref_count, ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_shared_object_refcount(b2_object(pb2)), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_shared_object_is_shared(b2_object(pb2)), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_shared_object_is_static(b2_object(pb2)), ==, 0))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    call_counter_reset();
    // jcntrl_shared_object_default_deleter calls downcast to obtain original
    jcntrl_shared_object_delete(b2_object(pb2));

    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ccnt(counter, ({.b1 = {.destructor = 1},
                                      .b2 = {.downcast = 1,
                                             .destructor = 1,
                                             .deleter = 1}})))
      ret = 1;
  }

  call_counter_reset();
  if (!test_compare_pp((pb2 = jcntrl_shared_object_new_by_name("b2")), !=,
                       NULL))
    ret = 1;

  if (test_expect_not_raised())
    ret = 1;
  if (!test_compare_ccnt(counter, ({.b1 = {.initializer = 1},
                                    .b2 = {.downcast = 1,
                                           .initializer = 1,
                                           .allocator = 1}})))
    ret = 1;

  if (pb2) {
    jcntrl_shared_object_delete(b2_object(pb2));

    if (test_expect_not_raised())
      ret = 1;
  }

  call_counter_reset();
  jcntrl_shared_object_static_init(b1_object(&ob1_1), b1_metadata_init());
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_ccnt(counter, ({.b1 = {.initializer = 1}})))
    ret = 1;

  if (!test_compare_ii(jcntrl_shared_object_refcount(b1_object(&ob1_1)), ==,
                       -1))
    ret = 1;
  if (!test_compare_ii(jcntrl_shared_object_is_shared(b1_object(&ob1_1)), ==,
                       0))
    ret = 1;
  if (!test_compare_ii(jcntrl_shared_object_is_static(b1_object(&ob1_1)), ==,
                       1))
    ret = 1;

  call_counter_reset();
  jcntrl_shared_object_static_init(b2_object(&ob2_1), b2_metadata_init());
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_ccnt(counter, ({.b1 = {.initializer = 1},
                                    .b2 = {.initializer = 1}})))
    ret = 1;

  call_counter_reset();
  jcntrl_shared_object_static_init(c1_object(&oc1), c1_metadata_init());
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_ccnt(counter, ({.c1 = {.initializer = 0}})))
    ret = 1;

  call_counter_reset();
  if (!test_compare_pp(jcntrl_shared_object_downcast_by_meta(b1_metadata_init(),
                                                             b2_object(&ob2_1)),
                       ==, &ob2_1.obj))
    ret = 1;
  if (!test_compare_ccnt(counter, ({.b1 = {.downcast = 1}})))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  call_counter_reset();
  if (!test_compare_pp(jcntrl_shared_object_downcast_by_meta(b1_metadata_init(),
                                                             b1_object(&ob1_1)),
                       ==, &ob1_1))
    ret = 1;
  if (!test_compare_ccnt(counter, ({.b1 = {.downcast = 1}})))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  call_counter_reset();
  if (!test_compare_pp(jcntrl_shared_object_downcast_by_meta(b2_metadata_init(),
                                                             b2_object(&ob2_1)),
                       ==, &ob2_1))
    ret = 1;
  if (!test_compare_ccnt(counter, ({.b2 = {.downcast = 1}})))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  call_counter_reset();
  if (!test_compare_pp(jcntrl_shared_object_downcast_by_meta(b2_metadata_init(),
                                                             b1_object(&ob1_1)),
                       ==, NULL))
    ret = 1;
  if (!test_compare_ccnt(counter, ({0})))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  call_counter_reset();
  if (!test_compare_pp(jcntrl_shared_object_downcast_by_meta(b2_metadata_init(),
                                                             c1_object(&oc1)),
                       ==, NULL))
    ret = 1;
  if (!test_compare_ccnt(counter, ({0})))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  call_counter_reset();
  if (!test_compare_pp(jcntrl_shared_object_downcast_by_name("b1",
                                                             b2_object(&ob2_1)),
                       ==, &ob2_1.obj))
    ret = 1;
  if (!test_compare_ccnt(counter, ({.b1 = {.downcast = 1}})))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  call_counter_reset();
  if (!test_compare_pp(jcntrl_shared_object_downcast_by_name("b1",
                                                             b1_object(&ob1_1)),
                       ==, &ob1_1))
    ret = 1;
  if (!test_compare_ccnt(counter, ({.b1 = {.downcast = 1}})))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  call_counter_reset();
  if (!test_compare_pp(jcntrl_shared_object_downcast_by_name("b2",
                                                             b2_object(&ob2_1)),
                       ==, &ob2_1))
    ret = 1;
  if (!test_compare_ccnt(counter, ({.b2 = {.downcast = 1}})))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  call_counter_reset();
  if (!test_compare_pp(jcntrl_shared_object_downcast_by_name("b2",
                                                             b1_object(&ob1_1)),
                       ==, NULL))
    ret = 1;
  if (!test_compare_ccnt(counter, ({0})))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  call_counter_reset();
  if (!test_compare_pp(jcntrl_shared_object_downcast_by_name("b2",
                                                             c1_object(&oc1)),
                       ==, NULL))
    ret = 1;
  if (!test_compare_ccnt(counter, ({0})))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  return ret;
}
