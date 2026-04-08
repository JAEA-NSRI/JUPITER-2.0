#include "test-util.h"
#include "serializer_test.h"

int main(int argc, char **argv)
{
  return run_test_main(argc, argv,
                       test_entry(buffer),
                       test_entry(byteswap),
                       test_entry(array),
                       test_entry(map),
                       test_entry(build),
                       test_entry(parse));
}
