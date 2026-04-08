#include <jupiter/geometry/geom_static_assert.h>

GEOM_STATIC_ASSERT(__STDC_VERSION__ == 199901L, "must be in C99");
GEOM_STATIC_ASSERT(1, "ok");
GEOM_STATIC_ASSERT(sizeof("") == 1, "ok");
