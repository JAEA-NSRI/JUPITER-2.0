#ifdef __GNUC__
#undef __GNUC__
#endif
#ifdef _MSC_VER
#undef _MSC_VER
#endif

#include <jupiter/geometry/geom_static_assert.h>

GEOM_STATIC_ASSERT(sizeof(char) != 1, "Error");

int main(void) { return 0; }
