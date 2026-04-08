#include "jupiter/lpt.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Extract number of particles from lpt_ctrl data file
 */
int main(int argc, char **argv)
{
  jupiter_lpt_ctrl_data *data;
  if (argc < 2) {
    fprintf(stderr, "Usage: %s input_file\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (read_lpt_ctrl_data(argv[1], &data))
    return EXIT_FAILURE;

  printf("%d\n", data->npt);
  delete_lpt_ctrl_data(data);
  return EXIT_SUCCESS;
}
