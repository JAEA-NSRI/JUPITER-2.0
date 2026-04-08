#include "random.h"

#include <stddef.h>
#include <stdint.h>

static uint64_t x = UINT64_C(0x38c8cc49677ee74a);

static uint64_t next(void)
{
  /* ref: https://prng.di.unimi.it/splitmix64.c */
  uint64_t z = (x += UINT64_C(0x9e3779b97f4a7c15));
  z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
  return z ^ (z >> 31);
}

void jupiter_random_seed_fill_random(jupiter_random_seed *seed)
{
  for (int i = 0; i < JUPITER_RANDOM_SEED_SIZE; ++i)
    seed->seed[i] = next();
}

