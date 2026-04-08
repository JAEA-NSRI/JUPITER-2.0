
#include "test-util.h"

#include "geometry_test.h"

#include <stdio.h>
#include <float.h>
#include <jupiter/geometry/geom_math.h>

#ifdef DBL_DIG
enum { dig = DBL_DIG };
#else
enum { dig = 15 };
#endif

static int
sensitive_check_f(double (*f)(double), const char *fn, const char *comptxt,
                  double at, double toward, int comp)
{
  double t, fv, fb;
  int stat;

  t = nexttoward(at, toward);
  if (t == at) {
    test_compare_fail(1, __FILE__, __LINE__, comptxt,
                      "nextotward(%.*g, %.*g) == %.*g",
                      t, toward, at);
    return 0;
  }

  fb = f(at);
  fv = f(t);
  if (comp < 0) {
    stat = (fb > fv);
  } else if (comp > 0) {
    stat = (fb < fv);
  } else {
    stat = 1;
  }

  stat = test_compare_fail(stat, __FILE__, __LINE__, comptxt,
                           "%s(%.*g)=%.*g", fn, 22, t, 22, fv);
  return stat;
}

#define sensitive_check(f, v, t, c) \
  sensitive_check_f(geom_##f, #f, "geom_" #f "(" #v ") " #c " geom_" #f "(nexttoward(" #v ", " #t "))", v, t, (1 c 0) ? 1 : ((0 c 1) ? -1 : 0))

int math_test(void)
{
  int ecnt = 0;
  volatile double cos45, sin45;
  double t;

  fprintf(stderr, "digits: %d HUGE_VAL: %.*g\n", dig, dig, HUGE_VAL);

  fprintf(stderr, "\n---- basic calcs ----\n");
  if (test_compare(geom_degree_norm(   0.0),   0.0)) ecnt++;
  if (test_compare(geom_degree_norm( 180.0), 180.0)) ecnt++;
  if (test_compare(geom_degree_norm( 270.0), -90.0)) ecnt++;
  if (test_compare(geom_degree_norm(-270.0),  90.0)) ecnt++;
  if (test_compare(geom_degree_norm(1090.0),  10.0)) ecnt++;
  if (test_compare(geom_degree_norm(-730.0), -10.0)) ecnt++;
  if (test_compare(geom_degree_norm(1.1e+15),  0.0)) ecnt++;

  if (test_compare(geom_deg_to_rad(180.0), GEOM_M_PI)) ecnt++;
  if (test_compare(geom_rad_to_deg(GEOM_M_PI), 180.0)) ecnt++;
  if (test_compare(geom_deg_to_rad(90.0), GEOM_M_PI * 0.5)) ecnt++;

  if (test_compare(geom_sign( 100.0),  1.0)) ecnt++;
  if (test_compare(geom_sign(-100.0), -1.0)) ecnt++;
  if (test_compare(geom_sign(   0.0),  0.0)) ecnt++;
  test_compare_f(geom_sign(-0.0), 0.0,
                 "(ignored: %.*g)", dig, geom_sign(-0.0));

  if (test_compare(geom_sind(   0.0),  0.0)) ecnt++;
  if (test_compare(geom_sind(  30.0),  0.5)) ecnt++;
  if (test_compare(geom_sind(  90.0),  1.0)) ecnt++;
  if (test_compare(geom_sind( 150.0),  0.5)) ecnt++;
  if (test_compare(geom_sind( 180.0),  0.0)) ecnt++;
  if (test_compare(geom_sind( -30.0), -0.5)) ecnt++;
  if (test_compare(geom_sind( -90.0), -1.0)) ecnt++;
  if (test_compare(geom_sind(-150.0), -0.5)) ecnt++;
  if (test_compare(geom_sind(-180.0),  0.0)) ecnt++;

  if (test_compare(geom_cosd(  60.0),  0.5)) ecnt++;
  if (test_compare(geom_cosd( 120.0), -0.5)) ecnt++;
  if (test_compare(geom_cosd( 180.0), -1.0)) ecnt++;
  if (test_compare(geom_cosd(  90.0),  0.0)) ecnt++;
  if (test_compare(geom_cosd(   0.0),  1.0)) ecnt++;
  if (test_compare(geom_cosd( -60.0),  0.5)) ecnt++;
  if (test_compare(geom_cosd(-120.0), -0.5)) ecnt++;
  if (test_compare(geom_cosd(-180.0), -1.0)) ecnt++;
  if (test_compare(geom_cosd( -90.0),  0.0)) ecnt++;

  if (test_compare(geom_tand(  45.0),  1.0)) ecnt++;
  if (test_compare(geom_tand( 135.0), -1.0)) ecnt++;
  if (test_compare(geom_tand( -45.0), -1.0)) ecnt++;
  if (test_compare(geom_tand(-135.0),  1.0)) ecnt++;
  if (test_compare(geom_tand(   0.0),  0.0)) ecnt++;
  if (test_compare(geom_tand( 180.0),  0.0)) ecnt++;
  if (test_compare(geom_tand(-180.0),  0.0)) ecnt++;
  if (test_compare(geom_tand(  90.0),  HUGE_VAL)) ecnt++;
  if (test_compare(geom_tand( -90.0), -HUGE_VAL)) ecnt++;

  if (test_compare(geom_asind( 1.0),   90.0)) ecnt++;
  if (test_compare(geom_asind( 0.5),   30.0)) ecnt++;
  if (test_compare(geom_asind( 0.0),    0.0)) ecnt++;
  if (test_compare(geom_asind(-0.5),  -30.0)) ecnt++;
  if (test_compare(geom_asind(-1.0),  -90.0)) ecnt++;

  if (test_compare(geom_acosd( 1.0),    0.0)) ecnt++;
  if (test_compare(geom_acosd( 0.5),   60.0)) ecnt++;
  if (test_compare(geom_acosd( 0.0),   90.0)) ecnt++;
  if (test_compare(geom_acosd(-0.5),  120.0)) ecnt++;
  if (test_compare(geom_acosd(-1.0),  180.0)) ecnt++;

  if (test_compare(geom_atand( 1.0),   45.0)) ecnt++;
  if (test_compare(geom_atand(-1.0),  -45.0)) ecnt++;
  if (test_compare(geom_atand( 0.0),    0.0)) ecnt++;
  if (test_compare(geom_atand( HUGE_VAL),  90.0)) ecnt++;
  if (test_compare(geom_atand(-HUGE_VAL), -90.0)) ecnt++;

  fprintf(stderr, "\n.... Unnormalized angles may fail\n");
  test_compare_f(geom_sind(360.0), 0.0,
                 "(ignored: %.*g)", dig, geom_sind(360.0));
  test_compare_f(geom_sind(540.0), 0.0,
                 "(ignored: %.*g)", dig, geom_sind(540.0));

  test_compare_f(geom_tand(270.0), -HUGE_VAL,
                 "(ignored: tan(270)=%.*g)",
                 dig, geom_tand(270.0));

  fprintf(stderr, "\n--- both cos45 and sin45 are sqrt(2.0)/2.0?\n");
  cos45 = geom_cosd(45.0);
  sin45 = geom_sind(45.0);
  test_compare_f(cos45, sin45,
                 "(ignored: cos45=%.*g, sin45=%.*g)",
                 dig, cos45, dig, sin45);

  t = cos45 * cos45 + sin45 * sin45;
  test_compare_f(cos45 * cos45 + sin45 * sin45, 1.0,
                 "(ignored: %.*g)", dig, t);

  fprintf(stderr, "\n---- sensitiveness around predefined or not ----\n");
  if (sensitive_check(sind,   0.0,  HUGE_VAL, >=)) ecnt++;
  if (sensitive_check(sind,   0.0, -HUGE_VAL, <=)) ecnt++;
  if (sensitive_check(sind,  30.0,  HUGE_VAL, <=)) ecnt++;
  if (sensitive_check(sind,  30.0, -HUGE_VAL, >=)) ecnt++;
  if (sensitive_check(sind,  90.0,  HUGE_VAL, <=)) ecnt++;
  if (sensitive_check(sind,  90.0, -HUGE_VAL, <=)) ecnt++;
  if (sensitive_check(sind, 180.0,  HUGE_VAL, >=)) ecnt++;
  if (sensitive_check(sind, 180.0, -HUGE_VAL, <=)) ecnt++;

  if (sensitive_check(cosd,   0.0,  HUGE_VAL, <=)) ecnt++;
  if (sensitive_check(cosd,   0.0, -HUGE_VAL, <=)) ecnt++;
  if (sensitive_check(cosd,  60.0,  HUGE_VAL, >=)) ecnt++;
  if (sensitive_check(cosd,  60.0, -HUGE_VAL, <=)) ecnt++;
  if (sensitive_check(cosd,  90.0,  HUGE_VAL, >=)) ecnt++;
  if (sensitive_check(cosd,  90.0, -HUGE_VAL, <=)) ecnt++;
  if (sensitive_check(cosd, 180.0,  HUGE_VAL, >=)) ecnt++;
  if (sensitive_check(cosd, 180.0, -HUGE_VAL, >=)) ecnt++;

  fprintf(stderr, "\n---- atan2d spec check ----\n");
  test_compare_f(geom_atan2d(HUGE_VAL, HUGE_VAL), 45.0,
                 "ignored: atan2d(inf, inf)=%.*g",
                 dig, geom_atan2d(HUGE_VAL, HUGE_VAL));
  test_compare_f(geom_rad_to_deg(atan2(HUGE_VAL, HUGE_VAL)), 45.0,
                 "ignored: (sysinfo) atan2(inf, inf) == %.*g",
                 dig, atan2(HUGE_VAL, HUGE_VAL));

  test_compare_f(geom_atan2d(HUGE_VAL, -HUGE_VAL), 135.0,
                 "ignored: atan2d(inf, -inf)=%.*g", dig,
                 geom_atan2d(HUGE_VAL, -HUGE_VAL));
  test_compare_f(geom_rad_to_deg(atan2(HUGE_VAL, -HUGE_VAL)), 135.0,
                 "ignored: (sysinfo) atan2(inf, -inf) == %.*g [rad]", dig,
                 atan2(HUGE_VAL, -HUGE_VAL));

  test_compare_f(geom_atan2d(-HUGE_VAL, HUGE_VAL), -45.0,
                 "ignored: atan2d(inf, -inf)=%.*g",
                 dig, geom_atan2d(-HUGE_VAL, HUGE_VAL));
  test_compare_f(geom_rad_to_deg(atan2(-HUGE_VAL, HUGE_VAL)), -45.0,
                 "ignored: (sysinfo) atan2(inf, -inf) == %.*g [rad]", dig,
                 atan2(-HUGE_VAL, HUGE_VAL));

  test_compare_f(geom_atan2d(-HUGE_VAL, -HUGE_VAL), -135.0,
                 "ignored: atan2d(inf, -inf)=%.*g",
                 dig, geom_atan2d(-HUGE_VAL, -HUGE_VAL));
  test_compare_f(geom_rad_to_deg(atan2(-HUGE_VAL, -HUGE_VAL)), -135.0,
                 "ignored: (sysinfo) atan2(-inf, -inf) == %.*g [rad]", dig,
                 atan2(-HUGE_VAL, -HUGE_VAL));

  test_compare_f(geom_atan2d(-0.0, 0.0), -0.0,
                 "ignored: atan2d(-0.0, 0.0)=%.*g",
                 dig, geom_atan2d(-0.0, 0.0));
  test_compare_f(geom_rad_to_deg(atan2(-0.0, 0.0)), -0.0,
                 "ignored: (sysinfo) atan2(-0.0, 0.0) == %.*g [rad]", dig,
                 atan2(-0.0, 0.0));

  test_compare_f(geom_atan2d(HUGE_VAL, 0.0), 90.0,
                 "ignored: atan2d(HUGE_VAL, 0.0)=%.*g",
                 dig, geom_atan2d(HUGE_VAL, 0.0));
  test_compare_f(geom_rad_to_deg(atan2(HUGE_VAL, 0.0)), 90.0,
                 "ignored: (sysinfo) atan2(HUGE_VAL, 0.0) == %.*g [rad]", dig,
                 atan2(HUGE_VAL, 0.0));

  test_compare_f(geom_atan2d(-HUGE_VAL, 0.0), -90.0,
                 "ignored: atan2d(-HUGE_VAL, 0.0)=%.*g",
                 dig, geom_atan2d(-HUGE_VAL, 0.0));
  test_compare_f(geom_rad_to_deg(atan2(-HUGE_VAL, 0.0)), -90.0,
                 "ignored: (sysinfo) atan2(-HUGE_VAL, 0.0) == %.*g [rad]", dig,
                 atan2(-HUGE_VAL, 0.0));

  test_compare_f(geom_atan2d(0.0, HUGE_VAL), 0.0,
                 "ignored: atan2d(0.0, HUGE_VAL)=%.*g",
                 dig, geom_atan2d(0.0, HUGE_VAL));
  test_compare_f(geom_rad_to_deg(atan2(0.0, HUGE_VAL)), 0.0,
                 "ignored: (sysinfo) atan2(0.0, HUGE_VAL) == %.*g [rad]", dig,
                 atan2(0.0, HUGE_VAL));

  test_compare_f(geom_atan2d(0.0, -HUGE_VAL), 180.0,
                 "ignored: atan2d(0.0, -HUGE_VAL)=%.*g",
                 dig, geom_atan2d(0.0, -HUGE_VAL));
  test_compare_f(geom_rad_to_deg(atan2(0.0, -HUGE_VAL)), 180.0,
                 "ignored: (sysinfo) atan2(0.0, -HUGE_VAL) == %.*g [rad]", dig,
                 atan2(0.0, -HUGE_VAL));

  test_compare_f(geom_atan2d(-0.0, -HUGE_VAL), -180.0,
                 "ignored: atan2d(-0.0, -HUGE_VAL)=%.*g",
                 dig, geom_atan2d(-0.0, -HUGE_VAL));
  test_compare_f(geom_rad_to_deg(atan2(-0.0, -HUGE_VAL)), -180.0,
                 "ignored: (sysinfo) atan2(-0.0, -HUGE_VAL) == %.*g [rad]", dig,
                 atan2(-0.0, -HUGE_VAL));


  return ecnt;

}
