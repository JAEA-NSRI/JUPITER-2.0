#include <jupiter/geometry/geom_static_assert.h>

GEOM_STATIC_ASSERT(sizeof(char) != 1, "Error");

int main(void) { return 0; }
