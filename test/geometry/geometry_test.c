#include "test-util.h"
#include "geometry_test.h"

#include <jupiter/geometry/global.h>

int main(int argc, char **argv)
{
  geom_initialize();
  return run_test_main(argc, argv,
                       test_entry(variant),
                       test_entry(rbtree),
                       test_entry(math),
                       test_entry(vector),
                       test_entry(abuilder),
                       test_entry(trip),
                       test_entry(polynomial),
                       test_entry(bitarray));
}
