#ifdef __GNUC__
#undef __GNUC__
#endif
#ifdef _MSC_VER
#undef _MSC_VER
#endif

#include <jupiter/geometry/geom_static_assert.h>

GEOM_STATIC_ASSERT(__STDC_VERSION__ == 199901L, "must be in C99");
GEOM_STATIC_ASSERT(1, "ok");
GEOM_STATIC_ASSERT(sizeof("") == 1, "ok");
