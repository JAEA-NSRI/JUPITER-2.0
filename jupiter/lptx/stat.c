#include "stat.h"
#include "defs.h"
#include "priv_util.h"
#include "struct_defs.h"
#include "util.h"

#include <stdlib.h>

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

#ifdef JUPITER_LPTX_MPI
int LPTX_particle_stat_gather(LPTX_particle_stat **outp,
                              const LPTX_particle_stat *stat, MPI_Comm comm)
{
  LPTX_particle_stat *p;
  int r;
  int nproc;
  MPI_Datatype stype, atype;
  MPI_Datatype types[] = LPTX_particle_stat_MPI_data(types);
  MPI_Aint displs[] = LPTX_particle_stat_MPI_data(displs);
  int block_lengths[] = LPTX_particle_stat_MPI_data(len);
  int count = sizeof(types) / sizeof(*types);

  stype = MPI_DATATYPE_NULL;
  atype = MPI_DATATYPE_NULL;

  do {
    r = MPI_Comm_size(comm, &nproc);
    if (r != MPI_SUCCESS)
      break;

    if (nproc <= 0) {
      r = MPI_ERR_COMM;
      break;
    }

    r = MPI_Type_create_struct(count, block_lengths, displs, types, &stype);
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Type_commit(&stype);
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Type_create_resized(stype, 0, sizeof(LPTX_particle_stat), &atype);
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Type_commit(&atype);
    if (r != MPI_SUCCESS)
      break;

    p = (LPTX_particle_stat *)calloc(nproc, sizeof(LPTX_particle_stat));
    if (!p) {
      r = MPI_ERR_NO_MEM;
      break;
    }
  } while (0);
  if (!LPTX_MPI_forall(r == MPI_SUCCESS, comm, &r)) {
    if (stype != MPI_DATATYPE_NULL)
      MPI_Type_free(&stype);
    if (atype != MPI_DATATYPE_NULL)
      MPI_Type_free(&atype);
    if (p)
      free(p);
    return r;
  }

  r = MPI_Allgather(stat, 1, atype, p, 1, atype, comm);
  MPI_Type_free(&stype);
  MPI_Type_free(&atype);

  *outp = p;
  return r;
}
#endif
