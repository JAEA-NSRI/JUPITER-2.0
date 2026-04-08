#include <string.h>
#include <inttypes.h>

#include "control_test.h"

#include <jupiter/control/error.h>
#include <jupiter/control/information.h>
#include <jupiter/control/executive.h>
#include <jupiter/control/input.h>
#include <jupiter/control/output.h>
#include <jupiter/control/connection.h>
#include <jupiter/control/executive_data.h>

#include "jupiter/control/defs.h"
#include "jupiter/control/shared_object.h"
#include "jupiter/control/shared_object_priv.h"
#include "jupiter/geometry/util.h"
#include "test-util.h"
#include "test/control/control_test_empty_exec.h"

static int excdn_comp(void *g, void *e, void *a) { return g == e; }
static int excdn_prnt(char **b, void *d, void *a)
{
  jcntrl_executive *p = d;
  jcntrl_shared_object *o;
  if (p) {
    o = jcntrl_executive_object(p);
    return test_compare_asprintf(b, "%#" PRIxPTR " {name=\"%s\"} (%s)",
                                 (uintptr_t)p, jcntrl_executive_get_name(p),
                                 jcntrl_shared_object_class_name(o));
  } else {
    return test_compare_asprintf(b, "%#" PRIxPTR "", (uintptr_t)p);
  }
}

static int excdn_prntprt(char **b, void *g, void *e, void *a)
{
  if (!a)
    return -1;
  return test_compare_asprintf(b, "     Port Index: %d\n", *(int *)a);
}

static int test_excdnf(jcntrl_executive *got, jcntrl_executive *exp,
                       const char *fn, long ln, int *ip,
                       const char *description)
{
  return test_compare_typed(got, exp, ip, description, fn, ln, excdn_comp,
                            excdn_prnt, excdn_prnt, excdn_prntprt);
}

#define test_compare_l_excdn(got, exp, file, line) \
  test_excdnf(got, exp, file, line, NULL, #got " == " #exp)

#define test_compare_excdn(got, exp) \
  test_compare_l_excdn(got, exp, __FILE__, __LINE__)

static int test_downstreams(jcntrl_output *output, const char *file, long line,
                            int num_expect, struct empty_exec **p)
{
  va_list ap;
  jcntrl_connection *conn;
  int in_list;
  int ret;

  ret = 1;
  conn = jcntrl_output_downstreams(output);

  for (in_list = 0, conn = jcntrl_connection_next(conn); conn;
       in_list++, conn = jcntrl_connection_next(conn)) {
    jcntrl_input *down;
    jcntrl_executive *executive_got;
    jcntrl_executive *expected_executive;

    down = jcntrl_connection_get_downstream_port(conn);
    executive_got = jcntrl_input_owner(down);
    if (in_list < num_expect) {
      expected_executive = &p[in_list]->executive;
    } else {
      expected_executive = NULL;
    }
    if (!test_excdnf(executive_got, expected_executive, file, line, &in_list,
                     "executive_got == expected_executive"))
      ret = 0;
  }

  if (!test_compare_l_ii(in_list, ==, num_expect, file, line))
    ret = 0;
  return ret;
}

#define test_downstreams_n(output, ...)                           \
  test_downstreams(output, __FILE__, __LINE__,                    \
                   sizeof((struct empty_exec *[]){__VA_ARGS__}) / \
                     sizeof(struct empty_exec *),                 \
                   (struct empty_exec *[]){__VA_ARGS__})

#define test_downstreams_0(output) \
  test_downstreams(output, __FILE__, __LINE__, 0, NULL)

int test_control_connection(void)
{
  struct empty_exec test_executive[5];
  jcntrl_input *input, *ifp;
  jcntrl_output *output, *ofp;
  jcntrl_connection *conn, *cfp;
  int ret;
  int i;
  ret = 0;

  for (i = 0; i < 5; ++i) {
    empty_exec_init(&test_executive[i]);
  }
  output = jcntrl_executive_get_output(&test_executive[0].executive);
  input = jcntrl_executive_get_input(&test_executive[1].executive);

  JCNTRL_ASSERT(output);
  JCNTRL_ASSERT(input);

  ifp = jcntrl_input_add(input);
  if (!test_compare_pp(ifp, !=, NULL)) {
    ret = 1;
    goto cleanup;
  }

  ofp = jcntrl_output_add(output);
  if (!test_compare_pp(ofp, !=, NULL)) {
    ret = 1;
    goto cleanup;
  }

  if (!test_compare_ii(jcntrl_input_connect(ifp, ofp), ==, 1)) {
    ret = 1;
    goto cleanup;
  }

  if (!test_compare_excdn(jcntrl_input_upstream_executive(ifp),
                          &test_executive[0].executive)) {
    ret = 1;
  }

  fprintf(stderr, "***** Test for test_downstreams()\n");
  if (!test_compare_ii(test_downstreams_0(ofp), ==, 0))
    ret = 1;

  if (!test_compare_ii(test_downstreams_n(ofp, &test_executive[0]), ==, 0))
    ret = 1;

  if (!test_compare_ii(test_downstreams_n(ofp, &test_executive[1]), ==, 1))
    ret = 1;

  if (!test_compare_ii(test_downstreams_n(ofp, &test_executive[1],
                                          &test_executive[2]), ==, 0))
    ret = 1;
  fprintf(stderr, "***** End test for test_downstreams()\n");

  if (!test_downstreams_n(ofp, &test_executive[1]))
    ret = 1;

  if (!test_compare_ii(jcntrl_input_is_connected(ifp), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_input_disconnect(ifp), ==, 1))
    ret = 1;

  if (!test_downstreams_0(ofp))
    ret = 1;

  if (!test_compare_ii(jcntrl_input_is_connected(ifp), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_input_disconnect(ifp), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_input_is_connected(ifp), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_input_connect(ifp, ofp), ==, 1))
    ret = 1;

  input = jcntrl_executive_get_input(&test_executive[2].executive);
  ifp = jcntrl_input_add(input);
  if (!test_compare_pp(ifp, !=, NULL)) {
    ret = 1;
    goto cleanup;
  }

  if (!test_compare_ii(jcntrl_input_connect(ifp, ofp), ==, 1))
    ret = 1;

  if (!test_downstreams_n(ofp, &test_executive[1], &test_executive[2]))
    ret = 1;

  input = jcntrl_executive_get_input(&test_executive[1].executive);
  if (!test_compare_pp((ifp = jcntrl_input_at(input, 0)), !=, NULL)) {
    ret = 1;
  } else {
    if (!test_compare_excdn(jcntrl_input_upstream_executive(ifp),
                            &test_executive[0].executive))
      ret = 1;
  }

  conn = jcntrl_output_downstreams(ofp);
  for (i = 0, conn = jcntrl_connection_next(conn); conn;
       ++i, conn = jcntrl_connection_next(conn)) {
    if (!test_compare_pp(jcntrl_connection_get_upstream_port(conn), ==, ofp))
      ret = 1;
  }
  if (test_compare(i, 2))
    ret = 1;

cleanup:
  for (i = 0; i < 5; ++i) {
    jcntrl_executive_delete(&test_executive[i].executive);
  }
  return ret;
}
