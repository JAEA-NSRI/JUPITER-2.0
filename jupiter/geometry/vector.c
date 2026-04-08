
#include "common.h"
#include "vector.h"

int geom_vec2_to_str(char **dest, geom_vec2 v, const char *fmt,
                     int width, int precision)
{
  double d[2];

  d[0] = geom_vec2_x(v);
  d[1] = geom_vec2_y(v);
  return geom_matrix_to_str(dest, d, 2, 1, ", ", "(", ")", 0,
                            fmt, width, precision);
}

int geom_vec3_to_str(char **dest, geom_vec3 v, const char *fmt,
                     int width, int precision)
{
  double d[3];

  d[0] = geom_vec3_x(v);
  d[1] = geom_vec3_y(v);
  d[2] = geom_vec3_z(v);
  return geom_matrix_to_str(dest, d, 3, 1, ", ", "(", ")", 0,
                            fmt, width, precision);
}

int geom_vec4_to_str(char **dest, geom_vec4 v, const char *fmt,
                     int width, int precision)
{
  double d[4];

  d[0] = geom_vec4_x(v);
  d[1] = geom_vec4_y(v);
  d[2] = geom_vec4_z(v);
  d[3] = geom_vec4_w(v);
  return geom_matrix_to_str(dest, d, 4, 1, ", ", "(", ")", 0,
                            fmt, width, precision);
}
