#include "jupiter/random/random.h"

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#ifdef JUPITER_RANDOM_MPI
#include <mpi.h>
#endif

#ifdef JUPITER_RANDOM_MPI
static int mpimain(void)
{
  MPI_Datatype type;
  MPI_Aint lb, ub;
  int r = 0;
  jupiter_random_mpi_seed_type(&type);
  MPI_Type_get_extent(type, &lb, &ub);
  MPI_Type_free(&type);

  fprintf(stderr, "type: jupiter_random_seed\n");
  fprintf(stderr, "- lb = %" PRIdMAX " (expect 0)\n", (intmax_t)lb);
  fprintf(stderr, "- ub = %" PRIdMAX " (expect %" PRIuMAX ")\n\n",
          (intmax_t)ub, (uintmax_t)sizeof(jupiter_random_seed));
  if (lb != 0 || ub != sizeof(jupiter_random_seed))
    r = 1;

  jupiter_random_mpi_seed_counter_type(&type);
  MPI_Type_get_extent(type, &lb, &ub);
  MPI_Type_free(&type);

  fprintf(stderr, "type: jupiter_random_seed_counter\n");
  fprintf(stderr, "- lb = %" PRIdMAX " (expect 0)\n", lb);
  fprintf(stderr, "- ub = %" PRIdMAX " (expect %" PRIuMAX ")\n\n", (intmax_t)ub,
          (uintmax_t)sizeof(jupiter_random_seed_counter));
  if (lb != 0 || ub != sizeof(jupiter_random_seed_counter))
    r = 1;

  return r;
}
#endif

int main(int argc, char **argv)
{
  int r = 0;

#ifdef JUPITER_RANDOM_MPI
  MPI_Init(&argc, &argv);
  r = mpimain();
  MPI_Finalize();
#endif
  return r;
}
