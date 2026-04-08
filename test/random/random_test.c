#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <jupiter/random/random.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

double maxd(double x, double y) { return (x < y) ? y : x; }

double reldiff(double x, double y)
{
  double d = fabs(x - y);
  if (d <= 0.0)
    return 0.0;
  return d / maxd(fabs(x), fabs(y));
}

double inverse1(uint64_t n)
{
  double d = n + 1;
  return 1.0 / d;
}

double factorial(uint64_t n)
{
  double d = 1.0;
  for (; n > 1; --n)
    d *= n;
  return d;
}

double double_factorial(uint64_t n)
{
  double d = 1.0;
  for (; n > 1; n -= 2)
    d *= n;
  return d;
}

double normal_dist_moments(uint64_t n)
{
  return (n % 2 == 1) ? 0.0 : double_factorial(n - 1);
}

double cdf_unit(double a, double b)
{
  if (b < 0.0)
    return 0.0;
  if (b < 1.0) {
    if (a < 0.0)
      a = 0.0;
    return b - a;
  }
  if (a < 0.0)
    return 1.0;
  b = 1.0;
  if (a < 1.0)
    return b - a;
  return 0.0;
}

double cdf_expr(double a, double b) { return exp(-a) - exp(-b); }

double cdf_norm(double a, double b)
{
  return 0.5 * (erf(b / M_SQRT2) - erf(a / M_SQRT2));
}

uint64_t get_u64(const char *str)
{
  char *eptr;
  uintmax_t umax;
  errno = 0;
  umax = strtoumax(str, &eptr, 0);
  if (*eptr != '\0') {
    fprintf(stderr, "Incomplete number string: %s\n", str);
    exit(EXIT_FAILURE);
  }
  if (errno != 0 || (umax > UINT64_MAX && (errno = ERANGE) != 0)) {
    perror(str);
    exit(EXIT_FAILURE);
  }
  return umax;
}

double get_double(const char *str)
{
  double d;
  char *eptr;
  errno = 0;
  d = strtod(str, &eptr);
  if (*eptr != '\0') {
    fprintf(stderr, "Incomplete number string: %s\n", str);
    exit(EXIT_FAILURE);
  }
  if (errno != 0) {
    perror(str);
    exit(EXIT_FAILURE);
  }
  return d;
}

int do_jump(const char *str, jupiter_random_seed *seed)
{
  uint64_t n = get_u64(str);
  switch (n) {
#define DO_JUMP(n)                \
  case n:                         \
    jupiter_random_jump##n(seed); \
    break
    DO_JUMP(32);
    DO_JUMP(48);
    DO_JUMP(64);
    DO_JUMP(96);
    DO_JUMP(128);
    DO_JUMP(160);
    DO_JUMP(192);
    DO_JUMP(224);
  default:
    fprintf(stderr, "Invalid jump shift: %" PRIu64 "", n);
    exit(EXIT_FAILURE);
  }
  printf("** Jumping 2**%" PRIu64 " steps...\n", n);
  return 1;
}

int test_ival(const char *str, jupiter_random_seed *seed)
{
  uint64_t exp = get_u64(str);
  uint64_t got = jupiter_random_nexti(seed);
  int pass = (exp == got);
  printf("** Next %#.16" PRIx64 " == %#.16" PRIx64 ": %s\n", got, exp,
         pass ? "PASS" : "FAIL");
  return pass;
}

enum consts_f
{
  fmant = FLT_MANT_DIG,
  fprdig = (fmant - 1) / 4 + 1,
#ifdef FLT_DECIMAL_DIG
  fprdig10 = FLT_DECIMAL_DIG,
#else
  fprdig10 = DECIMAL_DIG,
#endif
};

enum consts_d
{
  dmant = DBL_MANT_DIG,
  dprdig = (dmant - 1) / 4 + 1,
#ifdef DBL_DECIMAL_DIG
  dprdig10 = DBL_DECIMAL_DIG,
#else
  dprdig10 = DECIMAL_DIG,
#endif
};

int test_fval(const char *str, jupiter_random_seed *seed)
{
  uint64_t exp = get_u64(str);
  float got = jupiter_random_nextf(seed);
  float fexp = ldexpf((float)exp, -fmant);
  int pass = (fexp == got);
  printf("** Next %.*af == %.*af: %s\n", fprdig, got, fprdig, fexp,
         pass ? "PASS" : "FAIL");
  printf("..     (%.*e == %.*e)\n", fprdig10, got, fprdig10, fexp);
  return pass;
}

int test_dval(const char *str, jupiter_random_seed *seed)
{
  uint64_t exp = get_u64(str);
  double got = jupiter_random_nextd(seed);
  double fexp = ldexp((double)exp, -dmant);
  int pass = (fexp == got);
  printf("** Next %.*a == %.*a: %s\n", dprdig, got, dprdig, fexp,
         pass ? "PASS" : "FAIL");
  printf("..     (%.*e == %.*e)\n", dprdig10, got, dprdig10, fexp);
  return pass;
}

int test_eoval(const char *str, jupiter_random_seed *seed,
               double (*func)(jupiter_random_seed *seed))
{
  char *p;
  int pass;
  double d;
  double got;
  double fexp = get_double(str);
  got = func(seed);
  d = reldiff(got, fexp);
  pass = d < 1.0e-11;
  printf("** Next reldiff(%.*a, %.*a) < 1.e-11: %s\n", dprdig, got, dprdig,
         fexp, pass ? "PASS" : "FAIL");
  printf("..  reldiff(%.*e, %.*e) => %.6e\n", dprdig10, got, dprdig10, fexp, d);
  return pass;
}

void test_count_func(char sym, double (**func)(jupiter_random_seed *s),
                     double (**cdff)(double a, double b), double *min,
                     double *max)
{
  switch (sym) {
  case 'd':
    *func = jupiter_random_nextd;
    *cdff = cdf_unit;
    *min = 0.0;
    *max = 1.0;
    break;
  case 'e':
    *func = jupiter_random_nextde;
    *cdff = cdf_expr;
    *min = 0.0;
    *max = 5.0;
    break;
  case 'o':
    *func = jupiter_random_nextdn;
    *cdff = cdf_norm;
    *min = -2.0;
    *max = 2.0;
    break;
  default:
    printf("!! Invalid count test function\n");
    exit(EXIT_FAILURE);
  }
}

uint64_t range_bsearch(double v, const double *X, uint64_t n)
{
  uint64_t i, j, k;

  if (!isfinite(v))
    return (uint64_t)-1;
  if (v < X[0])
    return 0;
  if (v > X[n - 1])
    return n;

  i = 0;
  j = n - 1;
  k = i;
  while (i != j) {
    k = (j - i) / 2 + i;
    if (v < X[k]) {
      if (v >= X[k - 1])
        return k;
      j = k;
    } else {
      if (v < X[k + 1])
        return k + 1;
      i = k;
    }
  }
  if (v >= X[k + 1] && v < X[k])
    return k;
  return (uint64_t)-1;
}

int test_count(const char *nbinstr, const char *ncountstr,
               const char *torstr, jupiter_random_seed *seed,
               double (*func)(jupiter_random_seed *s),
               double (*cdff)(double a, double b), double min, double max)
{
  uint64_t nbin, ncnt;
  double tor, tot;
  uint64_t *bin;
  double *X;
  int pass;
  nbin = get_u64(nbinstr);
  ncnt = get_u64(ncountstr);
  tor = get_double(torstr);
  if (nbin > UINT64_MAX - 2) {
    printf("!! Overflow (.C)\n");
    exit(EXIT_FAILURE);
  }
  if (nbin == 0) {
    printf("!! Number of bins must not be 0 (.C)\n");
    exit(EXIT_FAILURE);
  }

  nbin += 2;
  X = (double *)malloc(sizeof(double) * (nbin - 1));
  bin = (uint64_t *)malloc(sizeof(uint64_t) * nbin);
  if (!X || !bin) {
    if (X)
      free(X);
    if (bin)
      free(bin);
    printf("!! Failed to allocate bins (.C)\n");
    exit(EXIT_FAILURE);
  }
  for (uint64_t i = 0; i < nbin - 1; ++i)
    X[i] = min + (max - min) * i / (nbin - 2);
  for (uint64_t i = 0; i < nbin; ++i)
    bin[i] = 0;

  for (uint64_t i = 0; i < ncnt; ++i) {
    uint64_t j;
    double val = func(seed);
    /* j = range_bsearch(val, X, nbin - 1); */ // slow :(
    if (val < min) {
      j = 0;
    } else if (val >= max){
      j = nbin - 1;
    } else {
      j = (uint64_t)((val - min) / (max - min) * (nbin - 2)) + 1;
      assert(X[j - 1] <= val && val < X[j]);
    }
    bin[j] += 1;
  }

  pass = 1;
  tot = 0.0;
  printf("** Count test for %" PRId64 " numbers... (tol: %14.6e)\n", ncnt, tor);
  for (uint64_t i = 0; i < nbin; ++i) {
    double r = (double)bin[i] / ncnt;
    if (i > 0 && i < nbin - 1) {
      double e = cdff(X[i - 1], X[i]);
      double d = reldiff(e, r);
      printf("..  [%12.4e, %12.4e): %16" PRId64 "\n"
             "..  %*s(exp. %16" PRId64 ", rel. %.6e)\n",
             X[i - 1], X[i], bin[i], 24, "", (uint64_t)(e * ncnt), d);
      if (d > tor)
        pass = 0;
    } else if (i == 0) {
      printf("..  [%12s, %12.4e): %16" PRId64 " %12.4e\n", "-inf", X[i], bin[i],
             r);
    } else {
      printf("..  [%12.4e, %12s): %16" PRId64 " %12.4e\n", X[i - 1], "inf",
             bin[i], r);
    }
    tot += r;
  }
  printf("..  total = %.6e (exp. 1.0)\n", tot);
  if (reldiff(tot, 1.0) >= 1.0e-6)
    pass = 0;
  printf("..  => %s\n", pass ? "PASS" : "FAIL");
  free(X);
  free(bin);
  return pass;
}

void test_quality_func(char sym, double (**func)(jupiter_random_seed *s),
                       double (**expf)(uint64_t n))
{
  switch (sym) {
  case 'd':
    *func = jupiter_random_nextd;
    *expf = inverse1; /* https://math.stackexchange.com/a/1839470 */
    break;
  case 'e':
    *func = jupiter_random_nextde;
    *expf = factorial;
    break;
  case 'o':
    *func = jupiter_random_nextdn;
    *expf = normal_dist_moments;
    break;
  default:
    printf("!! Invalid quality test function\n");
    exit(EXIT_FAILURE);
  }
}

int test_quality(const char *numstr, const char *momstr, const char *torstr,
                 jupiter_random_seed *seed,
                 double (*func)(jupiter_random_seed *s),
                 double (*expf)(uint64_t n))
{
  uint64_t trials, moments;
  double tor;
  double *X;
  int pass;
  trials = get_u64(numstr);
  moments = get_u64(momstr);
  tor = get_double(torstr);

  X = (double *)malloc(sizeof(double) * moments);
  if (!X) {
    printf("!! Failed to allocate moments vector (.Q)\n");
    exit(EXIT_FAILURE);
  }
  for (uint64_t i = 0; i < moments; ++i)
    X[i] = 0.0;
  for (uint64_t i = 0; i < trials; ++i) {
    double val, x_j;
    val = func(seed);
    x_j = val;
    for (uint64_t j = 0; j < moments; ++j, x_j *= val)
      X[j] += x_j;
  }
  pass = 1;
  printf("** Quality test for %" PRId64 " numbers...\n", trials);
  for (uint64_t i = 0; i < moments; ++i) {
    double x_j = X[i] / trials;
    double e = expf(i + 1);
    double rd = reldiff(x_j, e);
    double d = fabs(x_j - e);
    printf("..  X[%3" PRIu64
           "] = %14.6e (exp. %10.3e, rel. %10.3e, abs. %10.3e)\n",
           i + 1, x_j, e, rd, d);
    if (rd > tor && d > tor)
      pass = 0;
  }
  printf("..  => %s\n", pass ? "PASS" : "FAIL");
  free(X);
  return pass;
}

int test_nvals(const char *maxstr, const char *cntstr,
               jupiter_random_seed *seed)
{
  uint64_t n = get_u64(maxstr);
  uint64_t c = get_u64(cntstr);
  int pass = 1;
  if (n > UINT32_MAX) {
    printf("!! n = %" PRIu64 " is larger than UINT32_MAX (%" PRIu32 ")\n", n,
           UINT32_MAX);
    exit(EXIT_FAILURE);
  }
  for (uint64_t i = 0; i < c; ++i) {
    uint32_t u = jupiter_random_nextn(seed, n);
    int ipass = (u < n);
    pass = pass && ipass;
    printf("** Next %" PRIu32 " < %" PRIu64 ": %s\n", u, n,
           ipass ? "PASS" : "FAIL");
  }
  return pass;
}

int main(int argc, char **argv)
{
  jupiter_random_seed seed;

  if (argc < JUPITER_RANDOM_SEED_SIZE + 2) {
    printf("Usage: %s seed0..seed%d [\n"
           "        J[32|48|64|96|128|160|192|224]| # JUMP 2**N\n"
           "        [integer]|        # expected next 64bit\n"
           "        .f[integer]|      # float value test\n"
           "        .d[integer]|      # double value test\n"
           "        .e[double]|       # exponentially distributed value test\n"
           "        .o[double]|       # normally distributed value test\n"
           "        .Ce[bin] [count] [err]| # exponentially distribution test\n"
           "        .Co[bin] [count] [err]| # normally distribution test\n"
           "        .Cd[bin] [count] [err]| # uniform distribution test\n"
           "        .Qe[trials] [mom] [err]| # exp distribtion quality test\n"
           "        .Qo[trials] [mom] [err]| # norm distribtion quality test\n"
           "        .Qd[trials] [mom] [err]| # unif distribtion quality test\n"
           "        .n[max] [count]   # limited integral number test\n"
           "      ]...\n",
           argv[0], JUPITER_RANDOM_SEED_SIZE - 1);
    return EXIT_FAILURE;
  }

  for (int i = 0; i < JUPITER_RANDOM_SEED_SIZE; ++i) {
    seed.seed[i] = get_u64(argv[i + 1]);
    printf("** Set seed %d: %#.16" PRIx64 "\n", i, seed.seed[i]);
  }

  int ret = 0;
  for (int j = JUPITER_RANDOM_SEED_SIZE + 1; j < argc; ++j) {
    const char *str = argv[j];
    if (str[0] == 'J') {
      if (!do_jump(&str[1], &seed))
        ret = 1;
    } else if (str[0] == '.' && str[1] == 'f') {
      if (!test_fval(&str[2], &seed))
        ret = 1;
    } else if (str[0] == '.' && str[1] == 'd') {
      if (!test_dval(&str[2], &seed))
        ret = 1;
    } else if (str[0] == '.' && (str[1] == 'e' || str[1] == 'o')) {
      double (*func)(jupiter_random_seed *seed) =
        (*str == 'e') ? jupiter_random_nextde : jupiter_random_nextdn;
      if (!test_eoval(&str[2], &seed, func))
        ret = 1;
    } else if (str[0] == '.' && str[1] == 'C') {
      double (*func)(jupiter_random_seed *s);
      double (*cdff)(double a, double);
      double min, max;
      const char *t, *m, *e;
      if (j + 2 >= argc) {
        printf("!! Missing number of tests or error tolerance\n");
        exit(EXIT_FAILURE);
      }

      test_count_func(str[2], &func, &cdff, &min, &max);
      t = &str[3];
      m = argv[++j];
      e = argv[++j];
      if (!test_count(t, m, e, &seed, func, cdff, min, max))
        ret = 1;
    } else if (str[0] == '.' && str[1] == 'Q') {
      double (*func)(jupiter_random_seed *s);
      double (*expf)(uint64_t n);
      const char *t, *m, *e;
      if (j + 2 >= argc) {
        printf("!! Missing moment order or error tolerance\n");
        exit(EXIT_FAILURE);
      }

      test_quality_func(str[2], &func, &expf);
      t = &str[3];
      m = argv[++j];
      e = argv[++j];
      if (!test_quality(t, m, e, &seed, func, expf))
        ret = 1;
    } else if (str[0] == '.' && str[1] == 'n') {
      if (j + 1 >= argc) {
        printf("!! Count is missing for .n: .n[max-value] [count]\n");
        exit(EXIT_FAILURE);
      }
      if (!test_nvals(&str[2], argv[++j], &seed))
        ret = 1;
    } else {
      if (!test_ival(str, &seed))
        ret = 1;
    }
  }
  if (ret)
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
