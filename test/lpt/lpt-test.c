#include <stdio.h>
#include <stdlib.h>

#include <jupiter/lpt/LPTbnd.h>

#include "lpt-test-lib.h"

int main(int argc, char **argv)
{
  xLPTtest_run("mxpset");
  xLPTtest_run("set_get_npset");
  xLPTtest_run("set_get_pset");
  xLPTtest_run("set_get_ipttim");
  xLPTtest_run("open_log_file");
  xLPTtest_run("allocate");
  xLPTtest_run("set_get_pts");
  return xLPTtest_istat();
}
