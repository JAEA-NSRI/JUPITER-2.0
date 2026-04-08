#include <jupiter/random/random.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(JUPITER_RANDOM_TEST_F90_EXPORT)
#define JUPITER_RANDOM_TEST_F90_DECL __declspec(dllexport)
#elif defined(JUPITER_RANDOM_TEST_F90_IMPORT)
#define JUPITER_RANDOM_TEST_F90_DECL __declspec(dllimport)
#else
#define JUPITER_RANDOM_TEST_F90_DECL
#endif
#else
#define JUPITER_RANDOM_TEST_F90_DECL
#endif

JUPITER_RANDOM_TEST_F90_DECL
int random_test_f90(void);

int main(int argc, char **argv) { return !!random_test_f90(); }
