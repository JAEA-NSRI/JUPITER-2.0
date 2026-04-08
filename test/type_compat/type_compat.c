#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#include "type_compat.h"

int c_test(void)
{
  int irank;
  int isint, isdbl;
  int idcnt, iicnt;
  int *ibuf;
  double *dbuf;
  double d;
  int i;

  MPI_Initialized(&irank);
  if (!irank) return 1;

  MPI_Comm_rank(MPI_COMM_WORLD, &irank);
  if (irank != 1) return 0;

  MPI_Recv(&isint, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(&isdbl, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  if (isint != sizeof(int)) {
    fprintf(stderr, "// sizeof(int): c: %zu; f: %d;\n",
            sizeof(int), isint);
  }
  if (isdbl != sizeof(double)) {
    fprintf(stderr, "// sizeof(double): c: %zu; f: %d;\n",
            sizeof(double), isdbl);
  }
  MPI_Recv(&iicnt, 1, MPI_INT, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(&idcnt, 1, MPI_INT, 0, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  if (isint > 0 && iicnt > 0) {
    ibuf = (int *)calloc(isint, iicnt);
  } else {
    ibuf = NULL;
  }
  if (isdbl > 0 && idcnt > 0) {
    dbuf = (double *)calloc(isdbl, idcnt);
  } else {
    dbuf = NULL;
  }
  i = 0;
  if (!ibuf || !dbuf) {
    i = 1;
  }
  MPI_Send(&i, 1, MPI_INT, 0, 99, MPI_COMM_WORLD);
  if (i) {
    return -1;
  }

  MPI_Recv(ibuf, iicnt, MPI_INTEGER, 0, 20, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(dbuf, idcnt, MPI_DOUBLE_PRECISION, 0, 21, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  for (i = 0; i < iicnt; ++i) {
    fprintf(stderr, "ibuf[%d] = %5d\n", i, ibuf[i]);
  }

  for (i = 0; i < idcnt; ++i) {
    fprintf(stderr, "dbuf[%d] = %e (%a)\n", i, dbuf[i], dbuf[i]);
  }

  i = 0;
  if (ibuf[0] !=  0) i = 1;
  if (ibuf[1] !=  1) i = 2;
  if (ibuf[2] != -2) i = 3;

  if (dbuf[0] != 1.0) i = 101;
  if (dbuf[1] != 0.5) i = 102;
  if (dbuf[2] != 2.0) i = 103;

  d = fabs(dbuf[3] - 0.1);
  fprintf(stderr, "// fabs(fort(0.1) - c(0.1)) = %e (%a)\n", d, d);
  if (d != 0.0) i = 104;

  return i;
}
