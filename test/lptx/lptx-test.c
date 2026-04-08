#include "lptx-test.h"
#include "jupiter/lptx/vector.h"
#include "test-util.h"

#define add_test(case_name) {.name = #case_name, .func = test_lptx_##case_name}

int main(int argc, char **argv)
{
  return run_test_main(argc, argv, //
                       add_test(param), add_test(particle), add_test(overflow),
                       add_test(util), add_test(comm));
}

int test_compare_lptxvec_cmp(void *got, void *exp, void *arg)
{
  double gotx, goty, gotz, expx, expy, expz;
  gotx = LPTX_vector_x(*(LPTX_vector *)got);
  goty = LPTX_vector_y(*(LPTX_vector *)got);
  gotz = LPTX_vector_z(*(LPTX_vector *)got);
  expx = LPTX_vector_x(*(LPTX_vector *)exp);
  expy = LPTX_vector_y(*(LPTX_vector *)exp);
  expz = LPTX_vector_z(*(LPTX_vector *)exp);
  return test_compare_dd_cmp(&gotx, &expx, arg) &&
         test_compare_dd_cmp(&goty, &expy, arg) &&
         test_compare_dd_cmp(&gotz, &expz, arg);
}

int test_compare_lptxvec_prn(char **buf, void *d, void *a)
{
  double dx, dy, dz;
  int rx, ry, rz, r;
  char *bufx, *bufy, *bufz;
  dx = LPTX_vector_x(*(LPTX_vector *)d);
  dy = LPTX_vector_y(*(LPTX_vector *)d);
  dz = LPTX_vector_z(*(LPTX_vector *)d);
  rx = test_compare_d_prn(&bufx, &dx, NULL);
  ry = test_compare_d_prn(&bufy, &dy, NULL);
  rz = test_compare_d_prn(&bufz, &dz, NULL);

  r = -1;
  if (rx >= 0 && ry >= 0 && rz >= 0)
    r = test_compare_asprintf(buf, "(%s, %s, %s)", bufx, bufy, bufz);
  if (rx >= 0)
    free(bufx);
  if (ry >= 0)
    free(bufy);
  if (rz >= 0)
    free(bufz);
  return r;
}
