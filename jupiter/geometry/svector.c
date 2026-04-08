
#include <stdint.h>
#include <inttypes.h>

#include "common.h"
#include "svector.h"

int geom_svec3_to_str(char **buf, geom_svec3 vec, int width, int precision)
{

  return geom_asprintf(buf,
                       "(%*.*" PRIdMAX ","
                       " %*.*" PRIdMAX ","
                       " %*.*" PRIdMAX ")",
                       width, precision, (intmax_t) geom_svec3_x(vec),
                       width, precision, (intmax_t) geom_svec3_y(vec),
                       width, precision, (intmax_t) geom_svec3_z(vec));
}
