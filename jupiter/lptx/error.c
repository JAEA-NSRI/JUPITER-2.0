#include "error.h"
#include "priv_util.h"

#include <stdarg.h>
#include <stdio.h>

void LPTX__assert_impl(int cond, const char *cond_text, const char *file,
                       long line, const char *msg, ...)
{
  int rank = -1;
  const char *fmtp = NULL;
  const char *rnkp = NULL;
  const char *prep = "";
  const char *posp = "";
  char *p = NULL;
  char *r = NULL;

  if (msg) {
    int n;
    va_list ap;
    va_start(ap, msg);
    n = LPTX_vasprintf(&p, msg, ap);
    va_end(ap);
    if (n < 0)
      p = NULL;
    fmtp = p ? p : "[[failed to format message]]";
  }

#ifdef JUPITER_LPTX_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank >= 0) {
    int n;
    n = LPTX_asprintf(&r, " in MPI rank %d", rank);
    if (n < 0)
      r = NULL;
  }
  rnkp = r ? r : " in unknown MPI rank";
#else
  rnkp = "";
#endif

  if (fmtp) {
    prep = "*** ";
    posp = "\n";
  }

  fprintf(stderr,
          "*** LPTX assertion failure%s: %s\n"
          "*** from %s(%ld)\n%s%s%s",
          rnkp, cond_text, file, line, prep, fmtp, posp);

  if (p)
    free(p);
  if (r)
    free(r);
}
