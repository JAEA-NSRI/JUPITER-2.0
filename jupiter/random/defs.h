#ifndef JUPITER_RANDOM_DEFS_H
#define JUPITER_RANDOM_DEFS_H

#include "random_base.h"

#include <stdint.h>

JUPITER_RANDOM_DECL_START

enum jupiter_random_seed_consts
{
  JUPITER_RANDOM_SEED_SIZE = 4,
};

struct jupiter_random_seed
{
  uint64_t seed[JUPITER_RANDOM_SEED_SIZE];
};
typedef struct jupiter_random_seed jupiter_random_seed;

JUPITER_RANDOM_DECL_END

#endif
