#include "component_data.h"
#include "component_data_defs.h"
#include "component_info.h"
#include "component_info_defs.h"
#include "field_control.h"
#include "geometry/list.h"
#include "geometry/surface_shape.h"
#include "geometry/vector.h"
#include "init_component.h"
#include "random/random.h"
#include "serializer/defs.h"
#include "update_level_set_flags.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
/* YSE: Add string.h and stddef.h */
#include <string.h>
#include <stddef.h>
#include <limits.h>

#include <inttypes.h>
#include <time.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#ifdef GPU
#  include <cuda.h>
#  include <cutil.h>
#endif

/* YSE: Add System library for POSIX */
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
/* _POSIX_MAPPED_FILES is defined in unistd.h. */
#if defined(_POSIX_MAPPED_FILES) && _POSIX_MAPPED_FILES > 0
#include <sys/mman.h>
#include <fcntl.h>
#endif
#endif

/* YSE: Use CSV utility functions */
#include <errno.h>
#include "csv.h"
#include "csvutil.h"
#include "csvtmpl_format.h"
/* YSE: end */

#ifdef LPT
#include "lpt/LPTbnd.h"
#include "lpt/LPTdefs.h"
#endif

#ifdef LPTX
#include "lptx/defs.h"
#include "lptx/param.h"
#include "lptx/particle.h"
#include "lptx/ptflags.h"
#include "lptx/util.h"
#endif

/* YSE: Geometry functions */
#include "geometry/defs.h"
#include "geometry/data.h"
#include "geometry/init.h"
#include "geometry/file.h"
#include "geometry/shape.h"
#include "geometry/udata.h"
#include "geometry/svector.h"

#include "common_util.h"
#include "struct.h"
#include "func.h"

#include "os/os.h"

#include "serializer/msgpackx.h"
#include "boundary_util.h"

#include "if_binary.h"

#include "lpt.h"


/* YSE: New commonized IO functions */
int make_file_name(char **output,
                   const char *directory, const char *file_name_template,
                   const char *component_name,
                   int component_id, int iout, int rank,
                   binary_output_mode output_mode)
{
  int r;
  char *buf;
  r = 0;
  buf = NULL;

#ifndef JUPITER_MPI
  output_mode = BINARY_OUTPUT_BYPROCESS;
#endif
  if (output_mode != BINARY_OUTPUT_BYPROCESS) {
    rank = 0;
  }

  r = format_integers(&buf, file_name_template, "rin[s]c",
                      rank, component_id, iout, component_name);
  if (r < 0) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    return 1;
  }
  if (!directory) {
    directory = ".";
  }
  r = join_filenames(output, directory, buf);
  free(buf);
  if (r < 0) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    return 1;
  }
  return 0;
}

/**
 * @brief Split address offset into each axis index value
 *
 * @param addr Address offset
 * @param mx   X array size
 * @param my   Y array size
 * @param mz   Z array size
 *
 * @return Vector of each axis index.
 */
static inline geom_svec3
address_to_axis(ptrdiff_t addr, int mx, int my, int mz)
{
  geom_size_type ix, iy, iz;
  ptrdiff_t mxy;

  mxy  = mx;
  mxy *= my;

  iz = addr / mxy;
  ix = addr % mxy;
  iy = ix / mx;
  ix = ix % mx;
  return geom_svec3_c(ix, iy, iz);
}

/**
 * @brief Create chronogical information data (aka. time.dat) filename
 * @param output Output location
 * @param directory directory to be output
 * @param spec Deta specification of time info data.
 * @param iout Output number, -1 for restart data.
 * @return 0 if success, otherwise failed
 *
 * `make_file_name` for details.
 */
int make_time_file_name(char **output, const char *directory,
                        const struct data_spec *spec, int iout)
{
  return make_file_name(output, directory, spec->filename_template, spec->name,
                        0, iout, 0, BINARY_OUTPUT_UNIFY_MPI);
}

/**
 * @brief Create globally shared information data filename
 * @param output Output location
 * @param directory directory to be output
 * @param spec Data specification of data
 * @param iout Outupt number, -1 for restart data
 * @return 0 if success, otherwise failed
 *
 * `make_file_name` for details.
 */
int make_shared_file_name(char **output, const char *directory,
                          const struct data_spec *spec, int iout)
{
  /*
   * The function content is same as make_time_file_name(). Since they
   * are not hardcoded to "time" rather than `spec->name` for component
   * name.
   */
  return make_file_name(output, directory, spec->filename_template, spec->name,
                        0, iout, 0, BINARY_OUTPUT_UNIFY_MPI);
}

/**
 * @brief Calculate dimesion data from stencil and size
 * @param mpi MPI parallelization info
 * @param stmx Stencil size of X- (aka. West boundary).
 * @param stmy Stencil size of Y- (aka. South boundary).
 * @param stmz Stencil size of Z- (aka. Bottom boundary).
 * @param stpx Stencil size of X+ (aka. East boundary).
 * @param stpy Stencil size of Y+ (aka. North boundary).
 * @param stpz Stencil size of Z+ (aka. Top boundary).
 * @param mx X size including Stencil
 * @param my Y size including Stencil
 * @param mz Z size including Stencil
 * @param unit_size Number of element per cell (1 for scalar, 3 for vector)
 * @param local_dim (out) Local array dimension size (i.e. (unit_size, mx, my, mz))
 * @param local_subs (out) Local array subsize
 * @param local_start (out) Local array start point (i.e. (0, stmx, stmy, stmz))
 * @param global_dim (out) Global array dimension size (sum of local_subs)
 * @param global_subs (out) Global array subsize (for this rank)
 * @param global_start (out) Global array start point (for this rank)
 * @param global_sizes (out) Global array sizes (for each rank)
 * @param global_starts (out) Global array start points (for each rank)
 * @param nranks (out) Size of global_sizes and global_starts.
 * @return 0 if success, otherwise failed.
 *
 * If you don't need Global information, give NULL for global_dim.
 *
 * If you don't need size or start information of all ranks, give NULL
 * for global_sizes.
 *
 * This function allocates required memory for global_sizes and global_starts,
 * and set it to global_sizes.
 *
 * Use free(global_sizes) to deallocate allocated memory. This also
 * deallocates pointer of global_starts.
 *
 * @note This function is MPI-Collective call to get global information.
 * @note This function won't fail if global information is not required.
 * @note Result order of local_dim etc. is (cell-vector, X, Y, Z),
 *       for using with MPI_ORDER_FORTRAN. (aka. column-major)
 *
 * @note unit_size must be equal on all process ranks.
 */
int calculate_dim(mpi_param *mpi,
                  int stmx, int stmy, int stmz,
                  int stpx, int stpy, int stpz,
                  int mx, int my, int mz, int unit_size,
                  int local_dim[4], int local_subs[4], int local_start[4],
                  int global_dim[4], int global_subs[4], int global_start[4],
                  int **global_sizes, int **global_starts, int *nranks)
{
#ifdef JUPITER_MPI
  int *gszs;
  int *gstt;
  int r;
  int nrk;
  int jx;
  int jy;
  int jz;
  ptrdiff_t jj;
  ptrdiff_t jt;
#endif

  local_dim[0] = unit_size;
  local_dim[1] = mx;
  local_dim[2] = my;
  local_dim[3] = mz;
  local_start[0] = 0;
  local_start[1] = stmx;
  local_start[2] = stmy;
  local_start[3] = stmz;
  local_subs[0] = unit_size;
  local_subs[1] = mx - stmx - stpx;
  local_subs[2] = my - stmy - stpy;
  local_subs[3] = mz - stmz - stpz;

  /* compute local only */
  if (!global_dim) return 0;

#ifdef JUPITER_MPI
  MPI_Comm_size(mpi->CommJUPITER, &nrk);
  if (nrk != mpi->npe) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, __func__,
               "Constraint error: communicator modified after initialization?");
    return 1;
  }

  gszs = (int *)calloc(sizeof(int), nrk * 8);
  if (!gszs) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, __func__,
              CSV_ERR_NOMEM, 0, 0, NULL);
    return 1;
  }
  gstt = gszs + nrk * 4;
#endif

  global_subs[0] = unit_size;
  global_subs[1] = local_subs[1];
  global_subs[2] = local_subs[2];
  global_subs[3] = local_subs[3];
  global_dim[0] = unit_size;
  global_start[0] = 0;
#ifdef JUPITER_MPI
  MPI_Allgather(global_subs, 4, MPI_INT, gszs, 4, MPI_INT, mpi->CommJUPITER);
  global_dim[1] = 0;
  global_dim[2] = 0;
  global_dim[3] = 0;

  r = 0;
  for (jx = 0; jx < mpi->npe_x; ++jx) {
    jj = calc_address(jx, 0, 0, mpi->npe_x, mpi->npe_y, mpi->npe_z) * 4;
    if (jx == mpi->rank_x) {
      global_start[1] = global_dim[1];
    }
    for (jz = 0; jz < mpi->npe_z; ++jz) {
      for (jy = 0; jy < mpi->npe_y; ++jy) {
        jt = calc_address(jx, jy, jz, mpi->npe_x, mpi->npe_y, mpi->npe_z) * 4;
        gstt[jt + 1] = global_dim[1];
        if (gszs[jj + 1] != gszs[jt + 1]) {
          r = 1; /* Not constrained */
        }
      }
    }
    global_dim[1] += gszs[jj + 1];
  }
  for (jy = 0; jy < mpi->npe_y; ++jy) {
    jj = calc_address(0, jy, 0, mpi->npe_x, mpi->npe_y, mpi->npe_z) * 4;
    if (jy == mpi->rank_y) {
      global_start[2] = global_dim[2];
    }
    for (jz = 0; jz < mpi->npe_z; ++jz) {
      for (jx = 0; jx < mpi->npe_x; ++jx) {
        jt = calc_address(jx, jy, jz, mpi->npe_x, mpi->npe_y, mpi->npe_z) * 4;
        gstt[jt + 2] = global_dim[2];
        if (gszs[jj + 2] != gszs[jt + 2]) {
          r = 1; /* Not constrained */
        }
      }
    }
    global_dim[2] += gszs[jj + 2];
  }
  for (jz = 0; jz < mpi->npe_z; ++jz) {
    jj = calc_address(0, 0, jz, mpi->npe_x, mpi->npe_y, mpi->npe_z) * 4;
    if (jz == mpi->rank_z) {
      global_start[3] = global_dim[3];
    }
    for (jy = 0; jy < mpi->npe_y; ++jy) {
      for (jx = 0; jx < mpi->npe_x; ++jx) {
        jt = calc_address(jx, jy, jz, mpi->npe_x, mpi->npe_y, mpi->npe_z) * 4;
        gstt[jt + 3] = global_dim[3];
        if (gszs[jj + 3] != gszs[jt + 3]) {
          r = 1;
        }
      }
    }
    global_dim[3] += gszs[jj + 3];
  }
  for (jz = 0; jz < mpi->npe_z; ++jz) {
    for (jy = 0; jy < mpi->npe_y; ++jy) {
      for (jx = 0; jx < mpi->npe_x; ++jx) {
        jj = calc_address(jx, jy, jz, mpi->npe_x, mpi->npe_y, mpi->npe_z) * 4;
        if (gszs[jj] != global_dim[0]) {
          r = 1;
        }
      }
    }
  }
  if (r) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG, __func__,
               "Constraint error occured");
  }

#else
  global_dim[1] = global_subs[1];
  global_dim[2] = global_subs[2];
  global_dim[3] = global_subs[3];
  global_start[1] = 0;
  global_start[2] = 0;
  global_start[3] = 0;
#endif

#ifdef JUPITER_MPI
  if (global_sizes) {
    *global_sizes = gszs;
    if (global_starts) {
      *global_starts = gstt;
    }
  } else {
    free(gszs);
  }
  if (nranks) {
    *nranks = nrk;
  }
#else
  if (global_sizes) {
    *global_sizes = NULL;
  }
  if (global_starts) {
    *global_starts = NULL;
  }
  if (nranks) {
    *nranks = 0;
  }
#endif
  return 0;
}

static inline int
has_stencil(int dim[4], int sub[4], int stt[4])
{
  int i;
  for (i = 0; i < 4; ++i) {
    if (stt[i] > 0) return 1;
    if (dim[i] > sub[i]) return 1;
  }
  return 0;
}

int output_binary_generic(mpi_param *mpi, void *data,
                          int stmx, int stmy, int stmz,
                          int stpx, int stpy, int stpz,
                          int mx, int my, int mz, int unit_size,
                          const char *path, binary_output_mode output_mode)
{
  FILE *fp;
  char *buf;
  int r;
  int jx;
  int jy;
  int jz;
  int ldim[4], gdim[4], lsub[4], gsub[4], lstt[4], gstt[4];
  size_t ocount, retcount;
  int nrank;
  int *gszs;
  int *gsts;
  int **gszsp;
  int **gstsp;
  char *dir;

  CSVASSERT(mpi);
  CSVASSERT(data);
  CSVASSERT(stmx >= 0);
  CSVASSERT(stpx >= 0);
  CSVASSERT(stmy >= 0);
  CSVASSERT(stpy >= 0);
  CSVASSERT(stmz >= 0);
  CSVASSERT(stpz >= 0);
  CSVASSERT(mx > 0);
  CSVASSERT(my > 0);
  CSVASSERT(mz > 0);
  CSVASSERT(unit_size > 0);

#ifndef JUPITER_MPI
  output_mode = BINARY_OUTPUT_BYPROCESS;
#endif

  buf  = NULL;
  gszs = NULL;
  gsts = NULL;

  if (output_mode == BINARY_OUTPUT_UNIFY_GATHER && mpi->rank == 0) {
    gszsp = &gszs;
    gstsp = &gsts;
  } else {
    gszsp = NULL;
    gstsp = NULL;
  }

  r = extract_dirname_allocate(&dir, path);
  if (r < 0) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return 1;
  }

  errno = 0;
  r = make_directory_recursive(dir);
  if (r) {
    csvperror(dir, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
    free(dir);
    return 1;
  }
  free(dir);
  dir = NULL;

  r = calculate_dim(mpi, stmx, stmy, stmz, stpx, stpy, stpz,
                    mx, my, mz, unit_size, ldim, lsub, lstt, gdim, gsub, gstt,
                    gszsp, gstsp, &nrank);
  if (r != 0) goto clean;

  /*
   * Rank 0 outputs global size if Gather by JUPITER.
   */
  ocount = (size_t)lsub[0] * lsub[1] * lsub[2] * lsub[3];
  if (mpi->rank == 0 && output_mode == BINARY_OUTPUT_UNIFY_GATHER) {
    ocount = (size_t)gdim[0] * gdim[1] * gdim[2] * gdim[3];
  }

  if (output_mode == BINARY_OUTPUT_UNIFY_MPI) {
#ifdef JUPITER_MPI
    MPI_File file;
    MPI_Comm comm;
    MPI_Datatype grid_type;
    MPI_Datatype view_type;
    MPI_Datatype element_type;
    long line;

    grid_type = MPI_DATATYPE_NULL;
    view_type = MPI_DATATYPE_NULL;
    file = MPI_FILE_NULL;
    comm = mpi->CommJUPITER;

    element_type = MPI_CHAR;
    r = MPI_Type_create_subarray(4, ldim, lsub, lstt,
                                 MPI_ORDER_FORTRAN, element_type, &grid_type);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    r = MPI_Type_commit(&grid_type);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    r = MPI_Type_create_subarray(4, gdim, gsub, gstt,
                                 MPI_ORDER_FORTRAN, element_type, &view_type);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    r = MPI_Type_commit(&view_type);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
               "Writing %s...", path);
    r = MPI_File_open(comm, path, MPI_MODE_CREATE | MPI_MODE_WRONLY,
                      MPI_INFO_NULL, &file);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    r = MPI_File_set_size(file, 0);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    r = MPI_File_set_view(file, 0, element_type, view_type,
                          "native", MPI_INFO_NULL);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    r = MPI_File_write_all(file, data, 1, grid_type, MPI_STATUS_IGNORE);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

  mpi_fail:
    if (grid_type != MPI_DATATYPE_NULL) {
      MPI_Type_free(&grid_type);
    }
    if (view_type != MPI_DATATYPE_NULL) {
      MPI_Type_free(&view_type);
    }
    if (file != MPI_FILE_NULL) {
      MPI_File_close(&file);
    }
    if (r != 0) {
      csvperror(__FILE__, line, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI,
                0, r, NULL);
      goto clean;
    }
#else
    CSVUNREACHABLE();
#endif
  } else {
    if (output_mode == BINARY_OUTPUT_UNIFY_GATHER) {
#ifdef JUPITER_MPI
      MPI_Comm comm;
      MPI_Datatype view_type;
      MPI_Datatype element_type;
      MPI_Datatype *grid_types;
      long line;

      comm = mpi->CommJUPITER;
      element_type = MPI_CHAR;
      view_type = MPI_DATATYPE_NULL;
      grid_types = NULL;

      r = 0;
      line = -1;
      if (mpi->rank == 0) {
        buf = (char *)malloc(sizeof(char) * ocount);
        grid_types = (MPI_Datatype *)malloc(sizeof(MPI_Datatype) * nrank);
        if (!buf || !grid_types) {
          free(buf);
          free(grid_types);
          csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                    CSV_ERR_NOMEM, 0, 0, NULL);
          r = MPI_ERR_NO_MEM;
        } else {
          for (jx = 0; jx < nrank; ++jx) {
            grid_types[jx] = MPI_DATATYPE_NULL;
          }
          for (jx = 0; jx < nrank; ++jx) {
            r = MPI_Type_create_subarray(4, gdim, &gszs[4 * jx], &gsts[4 * jx],
                                         MPI_ORDER_FORTRAN, element_type,
                                         &grid_types[jx]);
            if (r != 0) { line = __LINE__; break; }

            r = MPI_Type_commit(&grid_types[jx]);
            if (r != 0) { line = __LINE__; break; }
          }
        }
      }
      if (for_any_rank(mpi, r != 0)) {
        if (line < 0) r = 0; /* Errors already notified */
        goto mpi_gather_fail;
      }

      r = MPI_Type_create_subarray(4, ldim, lsub, lstt, MPI_ORDER_FORTRAN,
                                   element_type, &view_type);
      if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_gather_fail; }

      r = MPI_Type_commit(&view_type);
      if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_gather_fail; }

      /* Gather data to rank 0 */
      if (mpi->rank == 0) {
        r = 0;
        for (jx = 1; jx < mpi->npe; ++jx) {
          int s;
          s = MPI_Recv(buf, 1, grid_types[jx], jx, jx, comm,
                       MPI_STATUS_IGNORE);
          r = r || (s != 0);
        }
      } else {
        r = MPI_Send(data, 1, view_type, 0, mpi->rank, comm);
      }
      if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_gather_fail; }

      /* Copy rank 0 data to global region */
      if (mpi->rank == 0) {
#pragma omp parallel for collapse(2)
        for (jz = 0; jz < lsub[3]; ++jz) {
          for (jy = 0; jy < lsub[2]; ++jy) {
            ptrdiff_t jf, jt;
            jf = calc_address(lstt[1], jy + lstt[2], jz + lstt[3],
                              ldim[1], ldim[2], ldim[3]) * unit_size;
            jt = calc_address(gstt[1], jy + gstt[2], jz + gstt[3],
                              gdim[1], gdim[2], gdim[3]) * unit_size;
            memcpy(buf + jt, (char *)data + jf, (size_t)unit_size * lsub[1]);
          }
        }
      }

      /* Set writing information */
      if (mpi->rank == 0) {
        for (jx = 0; jx < 4; ++jx) {
          lstt[jx] = 0;
          ldim[jx] = gdim[jx];
          lsub[jx] = gdim[jx];
        }
        data = buf;

      } else {
        data = NULL;
        path = NULL;
      }

    mpi_gather_fail:
      if (view_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&view_type);
      }
      if (grid_types) {
        for (jx = 0; jx < nrank; ++jx) {
          if (grid_types[jx] != MPI_DATATYPE_NULL) {
            MPI_Type_free(&grid_types[jx]);
          }
        }
        free(grid_types);
      }
      if (r != 0) {
        csvperror(__FILE__, line, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI,
                  0, r, NULL);
        goto clean;
      }
#else
      CSVUNREACHABLE();
#endif
    }

    /* YSE: Write data without stripping stencils */
    if (path) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
                 "Writing %s...", path);
      errno = 0;
      fp = fopen(path, "wb");
      if (!fp) {
        if (errno == 0) {
          csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_FOPEN, 0, 0, NULL);
        } else {
          csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
        }
        r = 1;
        goto clean;
      }

      if (has_stencil(ldim, lsub, lstt)) {
        int yzn = lsub[2] * lsub[3];
        int yzi;
        ocount = (size_t)lsub[1];
        for (yzi = 0; yzi < yzn; ++yzi) {
          ptrdiff_t jj;
          jz = yzi / lsub[2];
          jy = yzi % lsub[2];
          jj = calc_address(lstt[1], jy + lstt[2], jz + lstt[3],
                            ldim[1], ldim[2], ldim[3]) * unit_size;
          retcount = fwrite((char *)data + jj, unit_size, ocount, fp);
          if (retcount != ocount) break;
        }
      } else {
        ocount = (size_t)lsub[1] * lsub[2] * lsub[3];
        retcount = fwrite(data, unit_size, ocount, fp);
      }
      fclose(fp);
      if (retcount != ocount) {
        csvperrorf(path, 0, 0, CSV_EL_FATAL, NULL, "Data could not be written");
        r = 1;
        goto clean;
      }
    }
  }

  /* complete */
  r = 0;

 clean:
  free(buf);
  free(gszs);

  /* Share the status */
#ifdef JUPITER_MPI
  if (r) r = 1;
  MPI_Allreduce(MPI_IN_PLACE, &r, 1, MPI_INT, MPI_MAX, mpi->CommJUPITER);
#endif
  return r;
}

int output_binary(mpi_param *mpi, type *val,
                  int stmx, int stmy, int stmz,
                  int stpx, int stpy, int stpz,
                  int mx, int my, int mz, int unit_size,
                  const char *path, binary_output_mode output_mode,
                  int use_float)
{
  int r;
#ifdef JUPITER_DOUBLE
  float *flt = NULL;
#endif
  void *data;
  int jx;
  int jy;
  int jz;
  int lstt[4], ldim[4], lsub[4];
  int usz;
  size_t ocount;

  CSVASSERT(mpi);
  CSVASSERT(val);
  CSVASSERT(stmx >= 0);
  CSVASSERT(stpx >= 0);
  CSVASSERT(stmy >= 0);
  CSVASSERT(stpy >= 0);
  CSVASSERT(stmz >= 0);
  CSVASSERT(stpz >= 0);
  CSVASSERT(mx > 0);
  CSVASSERT(my > 0);
  CSVASSERT(mz > 0);
  CSVASSERT(unit_size > 0);

  r = calculate_dim(mpi, stmx, stmy, stmz, stpx, stpy, stpz,
                    mx, my, mz, unit_size, ldim, lsub, lstt,
                    NULL, NULL, NULL, NULL, NULL, NULL);
  if (r != 0) goto clean;

  ocount = (size_t)lsub[0] * lsub[1] * lsub[2] * lsub[3];
  data = val;
  usz = sizeof(type) * unit_size;

#ifdef JUPITER_DOUBLE
  flt = NULL;

  if (use_float) {
    flt = (float *)malloc(sizeof(float) * ocount);
    if (!flt) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                CSV_ERR_NOMEM, 0, 0, NULL);
      goto clean;
    }
#pragma omp parallel for collapse(3)
    for (jz = 0; jz < lsub[3]; ++jz) {
      for (jy = 0; jy < lsub[2]; ++jy) {
        for (jx = 0; jx < lsub[1]; ++jx) {
          int jj;
          ptrdiff_t jf, jt, jjf, jjt;
          jf = calc_address(jx + lstt[1], jy + lstt[2], jz + lstt[3],
                            ldim[1], ldim[2], ldim[3]);
          jt = calc_address(jx, jy, jz, lsub[1], lsub[2], lsub[3]);
          for (jj = 0; jj < unit_size; ++jj) {
            jjf = jf * unit_size + jj;
            jjt = jt * unit_size + jj;
            flt[jjt] = val[jjf];
          }
        }
      }
    }

    /* stencil is stripped */
    stmx = 0;
    stmy = 0;
    stmz = 0;
    stpx = 0;
    stpy = 0;
    stpz = 0;
    mx = lsub[1];
    my = lsub[2];
    mz = lsub[3];
    data = flt;
    usz = sizeof(float) * unit_size;
  }
#else
  use_float = 0;
#endif

  r = output_binary_generic(mpi, data, stmx, stmy, stmz, stpx, stpy, stpz,
                            mx, my, mz, usz, path, output_mode);

 clean:
#ifdef JUPITER_DOUBLE
  free(flt);
#endif

  return r;
}

int input_geometry_binary_unified(mpi_param *mpi, type *val,
                                  int stmx, int stmy, int stmz,
                                  int stpx, int stpy, int stpz,
                                  int mx, int my, int mz, int unit_size,
                                  const char *path, int use_float,
                                  geom_svec3 origin, geom_svec3 size,
                                  geom_svec3 repeat, geom_svec3 offset)
{
  struct input_geometry_binary_control {
    FILE  *fp;
    int    fd;
    type  *buf;
    float *flt;
  } c;

  int r;
  size_t len;
  int gdim[4], gsub[4], gstt[4], ldim[4], lsub[4], lstt[4];
  int fdim[4];
  geom_range3 range_rank, range_val, overlap;
  geom_range3 fwindow, rep_range;
  geom_svec3 sgstt, sgsub, sgend, slstt, slsub, slend;
  geom_svec3 rep_it, winstt;
  geom_size_type is, iu, sz;
  int packed_float;

  c.fd = -1;
  c.flt = NULL;
  c.buf = NULL;
  c.fp = NULL;

  packed_float = 0;
#ifndef JUPITER_DOUBLE
  /* In single version, no coversion required */
  use_float = 0;
#endif

  if (use_float) {
    len = sizeof(float);
  } else {
    len = sizeof(type);
  }
  len *= geom_svec3_x(size);
  len *= geom_svec3_y(size);
  len *= geom_svec3_z(size);
  if (len == 0) {
    csvperrorf(path, 0, 0, CSV_EL_WARN, NULL, "Requested size is 0");
    return 0;
  }
  if (len < 0) {
    csvperrorf(path, 0, 0, CSV_EL_FATAL, NULL, "Requested size overflowed");
    return -1;
  }

  r = calculate_dim(mpi, stmx, stmy, stmz, stpx, stpy, stpz, mx, my, mz,
                    unit_size, ldim, lsub, lstt, gdim, gsub, gstt,
                    NULL, NULL, NULL);
  if (r) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI,
              0, r, NULL);
    goto clean;
  }

  r = 0;
  sz  = mx;
  sz *= my;
  sz *= mz;
  sz *= unit_size;

#pragma omp parallel for
  for (is = 0; is < sz; ++is) {
    val[is] = 0.0;
  }

  csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
             "Reading %s...", path);
#if 1 /* This #if indicates IO region to separate */
#if defined(_POSIX_MAPPED_FILES) && _POSIX_MAPPED_FILES > 0
  errno = 0;
  c.fd = open(path, O_RDONLY);
  if (c.fd < 0) {
    csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
    return errno;
  }
  errno = 0;
  c.buf = mmap(NULL, len, PROT_READ, MAP_PRIVATE, c.fd, 0);
  if (c.buf == MAP_FAILED) {
    csvperror(path, 0, 0, CSV_EL_WARN, NULL, CSV_ERR_SYS, errno, 0, NULL);

    c.buf = NULL;
    close(c.fd);
    c.fd = -1;
  } else {
    if (use_float) {
      c.flt = (float *)c.buf;
      c.buf = NULL;
    }
  }
#endif /* fallback to C version if mmap failed */
  if (c.fd < 0) {
    size_t len_read;
    errno = 0;
    c.fp = fopen(path, "r");
    if (!c.fp) {
      if (errno == 0) {
        csvperrorf(path, 0, 0, CSV_EL_FATAL, NULL, "Cannot open file");
      } else {
        csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
      }
      goto clean;
      return -1;
    }
    c.buf = malloc(len);
    if (!c.buf) {
      csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
      fclose(c.fp);
      c.fp = NULL;
      goto clean;
    }
    len_read = fread(c.buf, sizeof(char), len, c.fp);
    if (len_read != len) {
      /* other methods cannot check this, so only warns here. */
      csvperrorf(path, 0, 0, CSV_EL_WARN, NULL,
                 "Requested size is %d bytes but could read %d bytes",
                 (int)len, (int)len_read);
    }
    if (use_float) {
      c.flt = (float *)c.buf;
      c.buf = NULL;
    }
  }
#endif

  slstt = geom_svec3_c(lstt[1], lstt[2], lstt[3]);
  slsub = geom_svec3_c(lsub[1], lsub[2], lsub[3]);
  slend = geom_svec3_add(slstt, slsub);
  range_val = geom_range3_c_vec(slstt, slend, 0);

  sgstt = geom_svec3_c(gstt[1], gstt[2], gstt[3]);
  sgsub = geom_svec3_c(gsub[1], gsub[2], gsub[3]);
  sgend = geom_svec3_add(sgstt, sgsub);
  range_rank = geom_range3_c_vec(sgstt, sgend, 0);

  rep_range = geom_range3_c_vec(geom_svec3_c(0, 0, 0), repeat, 0);

  fdim[0] = unit_size;
  fdim[1] = (int)geom_svec3_x(size);
  fdim[2] = (int)geom_svec3_y(size);
  fdim[3] = (int)geom_svec3_z(size);

  geom_range3_foreach(rep_it, rep_range, geom_svec3_c(1, 1, 1)) {
    geom_svec3 ststt, stsub, sdstt;
    int tstt[4], tsub[4], dstt[4];

    winstt = geom_svec3_mul_each_element(rep_it, offset);
    winstt = geom_svec3_add(winstt, origin);
    fwindow = geom_range3_c_vec(winstt, geom_svec3_add(winstt, size), 0);
    overlap = geom_range3_overlap(fwindow, range_rank);
    if (geom_range3_size(overlap) <= 0) continue;

    ststt = geom_range3_start(overlap);
    ststt = geom_svec3_sub(ststt, winstt);
    stsub = geom_range3_size_vec(overlap);
    sz = geom_range3_size(overlap);

    tsub[0] = unit_size;
    tsub[1] = (int)geom_svec3_x(stsub);
    tsub[2] = (int)geom_svec3_y(stsub);
    tsub[3] = (int)geom_svec3_z(stsub);

    sdstt = geom_range3_start(overlap);
    sdstt = geom_svec3_sub(sdstt, sgstt);
    sdstt = geom_svec3_add(sdstt, slstt);

    tstt[0] = 0;
    tstt[1] = (int)geom_svec3_x(ststt);
    tstt[2] = (int)geom_svec3_y(ststt);
    tstt[3] = (int)geom_svec3_z(ststt);

    dstt[0] = 0;
    dstt[1] = (int)geom_svec3_x(sdstt);
    dstt[2] = (int)geom_svec3_y(sdstt);
    dstt[3] = (int)geom_svec3_z(sdstt);

#if 1 /* See comment above */
#pragma omp parallel for
    for (is = 0; is < sz; ++is) {
      geom_svec3 ptr, pfrom, pto;
      ptrdiff_t lfrom, lto;

      ptr = address_to_axis(is, tsub[1], tsub[2], tsub[3]);
      pfrom = geom_svec3_add(ptr, ststt);
      pto = geom_svec3_add(ptr, sdstt);

      lfrom = calc_address((int)geom_svec3_x(pfrom),
                           (int)geom_svec3_y(pfrom),
                           (int)geom_svec3_z(pfrom),
                           fdim[1], fdim[2], fdim[3]);
      lto   = calc_address((int)geom_svec3_x(pto),
                           (int)geom_svec3_y(pto),
                           (int)geom_svec3_z(pto),
                           ldim[1], ldim[2], ldim[3]);

      for (iu = 0; iu < unit_size; ++iu) {
        double t;
        if (use_float) {
          t = c.flt[lfrom * unit_size + iu];
        } else {
          t = c.buf[lfrom * unit_size + iu];
        }
        val[lto * unit_size + iu] = t;
      }
    }
#endif
  }

clean:
#if 1 /* See comment above */
#if defined(_POSIX_MAPPED_FILES) && _POSIX_MAPPED_FILES > 0
  if (c.fd >= 0) {
    if (use_float) {
      munmap(c.flt, len);
    } else {
      munmap(c.buf, len);
    }
    c.flt = NULL;
    c.buf = NULL;
    close(c.fd);
  }
#endif
  if (c.fp) {
    fclose(c.fp);
  }
#endif

  free(c.flt);
  free(c.buf);

  return r;
}

int input_binary_generic(mpi_param *mpi, void *ret,
                         int stmx, int stmy, int stmz,
                         int stpx, int stpy, int stpz,
                         int mx, int my, int mz, int unit_size,
                         const char *path, binary_output_mode output_mode)
{
  FILE *fp;
  int r;
  char *buf;
  int jx;
  int jy;
  int jz;
  int ldim[4], gdim[4], lsub[4], gsub[4], lstt[4], gstt[4];
  size_t ocount, retcount;
  int *gszs;
  int *gsts;
  int **gszsp;
  int **gstsp;
  int nrank;

  CSVASSERT(mpi);
  CSVASSERT(ret);
  CSVASSERT(stmx >= 0);
  CSVASSERT(stpx >= 0);
  CSVASSERT(stmy >= 0);
  CSVASSERT(stpy >= 0);
  CSVASSERT(stmz >= 0);
  CSVASSERT(stpz >= 0);
  CSVASSERT(mx > 0);
  CSVASSERT(my > 0);
  CSVASSERT(mz > 0);
  CSVASSERT(unit_size > 0);

#ifndef JUPITER_MPI
  output_mode = BINARY_OUTPUT_BYPROCESS;
#endif

  gszs = NULL;
  gsts = NULL;
  if (mpi->rank == 0 && output_mode == BINARY_OUTPUT_UNIFY_GATHER) {
    gszsp = &gszs;
    gstsp = &gsts;
  } else {
    gszsp = NULL;
    gstsp = NULL;
  }
  buf = NULL;

  r = calculate_dim(mpi, stmx, stmy, stmz, stpx, stpy, stpz,
                    mx, my, mz, unit_size,
                    ldim, lsub, lstt, gdim, gsub, gstt,
                    gszsp, gstsp, &nrank);
  if (r) goto clean;

  ocount = (size_t)lsub[0] * lsub[1] * lsub[2] * lsub[3];
  if (mpi->rank == 0 && output_mode == BINARY_OUTPUT_UNIFY_GATHER) {
    ocount = (size_t)gdim[0] * gdim[1] * gdim[2] * gdim[3];
  }

  r = 1;

  if (output_mode == BINARY_OUTPUT_UNIFY_MPI) {
#ifdef JUPITER_MPI
    MPI_File file;
    MPI_Comm comm;
    MPI_Datatype grid_type;
    MPI_Datatype view_type;
    MPI_Datatype element_type;
    long line;

    grid_type = MPI_DATATYPE_NULL;
    view_type = MPI_DATATYPE_NULL;
    element_type = MPI_CHAR;
    file = MPI_FILE_NULL;
    comm = mpi->CommJUPITER;

    r = MPI_Type_create_subarray(4, ldim, lsub, lstt,
                                 MPI_ORDER_FORTRAN, element_type, &grid_type);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    r = MPI_Type_commit(&grid_type);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    r = MPI_Type_create_subarray(4, gdim, gsub, gstt,
                                 MPI_ORDER_FORTRAN, element_type, &view_type);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    r = MPI_Type_commit(&view_type);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
               "Reading %s...", path);
    r = MPI_File_open(comm, path, MPI_MODE_RDONLY, MPI_INFO_NULL, &file);
    if (for_any_rank(mpi, r != 0)) {
      int nr;
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, path,
                 "Falling back to non-shared reading");

      /* Fallback to open file as non-shared */
      nr = MPI_File_open(MPI_COMM_SELF, path, MPI_MODE_RDONLY,
                         MPI_INFO_NULL, &file);
      if (nr) {
        if (!r) {
          r = nr;
        }
      } else {
        r = nr;
      }
    }
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_file_fail; }

    r = MPI_File_set_view(file, 0, element_type, view_type,
                          "native", MPI_INFO_NULL);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    r = MPI_File_read_all(file, ret, 1, grid_type, MPI_STATUS_IGNORE);
    if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_fail; }

    while (0) {
    mpi_fail:
      csvperror(__FILE__, line, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI,
                0, r, NULL);
      r = 2;
      break;

    mpi_file_fail:
      csvperror(__FILE__, line, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI,
                0, r, NULL);
      r = 1;
      break;
    }

    /* clean up MPI resources */
    if (grid_type != MPI_DATATYPE_NULL) {
      MPI_Type_free(&grid_type);
    }
    if (view_type != MPI_DATATYPE_NULL) {
      MPI_Type_free(&view_type);
    }
    if (file != MPI_FILE_NULL) {
      MPI_File_close(&file);
    }
    if (r != 0) {
      goto clean;
    }
#else
    CSVUNREACHABLE();
#endif
  } else {
    if (output_mode == BINARY_OUTPUT_UNIFY_GATHER) {
      if (mpi->rank == 0) {
        buf = (char *)malloc(sizeof(char) * ocount);
        if (!buf) {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                    CSV_ERR_NOMEM, 0, 0, NULL);
          goto clean;
        }
      } else {
        /* Skip reading in this rank. */
        path = NULL;
      }
    } else if (has_stencil(ldim, lsub, lstt)) {
      CSVASSERT(output_mode == BINARY_OUTPUT_BYPROCESS);

      buf = (char *)malloc(sizeof(char) * ocount);
      if (!buf) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                  CSV_ERR_NOMEM, 0, 0, NULL);
        goto clean;
      }

      for (jx = 0; jx < 4; ++jx) {
        ldim[jx] = lsub[jx];
        lstt[jx] = 0;
      }
    }

    r = 0;
    while (path) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
                 "Reading %s...", path);
      errno = 0;
      fp = fopen(path, "rb");
      if (!fp) {
        if (errno == 0) {
          csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_FOPEN, 0, 0, NULL);
        } else {
          csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
        }
        r = 1;
        break;
      }
      if (buf) {
        retcount = fread(buf, sizeof(char), ocount, fp);
      } else {
        retcount = fread(ret, sizeof(char), ocount, fp);
      }
      fclose(fp);
      if (retcount != ocount) {
        csvperrorf(path, 0, 0, CSV_EL_FATAL, NULL,
                   "Data size does not match (required: %" PRIuMAX " byte(s),"
                   " found: %" PRIuMAX " byte(s))",
                   (uintmax_t) ocount, (uintmax_t) retcount);
        r = 2;
        break;
      }
      break;
    }
#ifdef JUPITER_MPI
    MPI_Allreduce(MPI_IN_PLACE, &r, 1, MPI_INT, MPI_MAX, mpi->CommJUPITER);
#endif
    if (r) goto clean;

    if (output_mode == BINARY_OUTPUT_UNIFY_GATHER) {
#ifdef JUPITER_MPI
      MPI_Comm comm;
      MPI_Datatype *grid_types;
      MPI_Datatype  view_type;
      MPI_Datatype  element_type;
      long line;

      comm = mpi->CommJUPITER;
      element_type = MPI_CHAR;
      view_type  = MPI_DATATYPE_NULL;
      grid_types = NULL;

      r = 0;
      line = -1;
      if (mpi->rank == 0) {
        grid_types = (MPI_Datatype *)malloc(sizeof(MPI_Datatype) * nrank);
        if (!grid_types) {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                    CSV_ERR_NOMEM, 0, 0, NULL);
          r = 2;
        } else {
          for (jx = 0; jx < nrank; ++jx) {
            grid_types[jx] = MPI_DATATYPE_NULL;
          }
          for (jx = 0; jx < nrank; ++jx) {
            r = MPI_Type_create_subarray(4, gdim, &gszs[4 * jx], &gsts[4 * jx],
                                         MPI_ORDER_FORTRAN, element_type,
                                         &grid_types[jx]);
            if (r != 0) { line = __LINE__; break; }

            r = MPI_Type_commit(&grid_types[jx]);
            if (r != 0) { line = __LINE__; break; }
          }
        }
      }
      if (for_any_rank(mpi, r != 0)) {
        if (line < 0) r = 0;
        goto mpi_gather_fail;
      }

      r = MPI_Type_create_subarray(4, ldim, lsub, lstt, MPI_ORDER_FORTRAN,
                                   element_type, &view_type);
      if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_gather_fail; }

      r = MPI_Type_commit(&view_type);
      if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_gather_fail; }

      if (mpi->rank == 0) {
        r = 0;
        for (jx = 1; jx < nrank; ++jx) {
          int s;
          s = MPI_Send(buf, 1, grid_types[jx], jx, 0, comm);
          r = r || (s != 0);
        }
      } else {
        r = MPI_Recv(ret, 1, view_type, 0, 0, comm, MPI_STATUS_IGNORE);
      }
      if (for_any_rank(mpi, r != 0)) { line = __LINE__; goto mpi_gather_fail; }

      if (mpi->rank == 0) {
#pragma omp parallel for collapse(2)
        for (jz = 0; jz < lsub[3]; ++jz) {
          for (jy = 0; jy < lsub[2]; ++jy) {
            ptrdiff_t jf, jt;
            ptrdiff_t cps;
            jf = calc_address(gstt[1], jy + gstt[2], jz + gstt[3],
                              gdim[1], gdim[2], gdim[3]) * unit_size;
            jt = calc_address(lstt[1], jy + lstt[2], jz + lstt[3],
                              ldim[1], ldim[2], ldim[3]) * unit_size;
            cps = (ptrdiff_t)lsub[1] * unit_size;
            memcpy((char *)ret + jt, &buf[jf], cps);
          }
        }
      }

    mpi_gather_fail:
      if (view_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&view_type);
      }
      if (grid_types) {
        for (jx = 0; jx < nrank; ++jx) {
          if (grid_types[jx] != MPI_DATATYPE_NULL) {
            MPI_Type_free(&grid_types[jx]);
          }
        }
        free(grid_types);
      }
      if (r != 0) {
        csvperror(__FILE__, line, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI, 0,
                  r, NULL);
        r = 2;
      }
      if (for_any_rank(mpi, r != 0)) { goto clean; }

#else
      CSVUNREACHABLE();
#endif
    } else if (buf) {
      CSVASSERT(output_mode == BINARY_OUTPUT_BYPROCESS);

#pragma omp parallel for collapse(2)
      for (jz = 0; jz < lsub[3]; ++jz) {
        for (jy = 0; jy < lsub[2]; ++jy) {
          ptrdiff_t jf, jt;
          ptrdiff_t cps;
          jf = calc_address(lstt[1], jy + lstt[2], jz + lstt[3],
                            ldim[1], ldim[2], ldim[3]) * unit_size;
          jt = calc_address(stmx, jy + stmy, jz + stmz, mx, my, mz) * unit_size;
          cps = (ptrdiff_t)lsub[1] * unit_size;
          memcpy((char *)ret + jt, &buf[jf], cps);
        }
      }
    }
  }

  /* complete */
  r = 0;

 clean:
  free(buf);
  return r;
}

int input_binary(mpi_param *mpi, type *val, float *fval,
                 int stmx, int stmy, int stmz,
                 int stpx, int stpy, int stpz,
                 int mx, int my, int mz, int unit_size,
                 const char *path, binary_output_mode output_mode,
                 int use_float)
{
  int r;
  int rstmx, rstmy, rstmz;
  int rstpx, rstpy, rstpz;
  int rmx;
  int rmy;
  int rmz;
  ptrdiff_t sz;

  CSVASSERT(val || fval);
  CSVASSERT(stmx >= 0);
  CSVASSERT(stmy >= 0);
  CSVASSERT(stmz >= 0);
  CSVASSERT(stpx >= 0);
  CSVASSERT(stpy >= 0);
  CSVASSERT(stpz >= 0);
  CSVASSERT(stmx + stpx >= 0);
  CSVASSERT(stmy + stpy >= 0);
  CSVASSERT(stmz + stpz >= 0);
  CSVASSERT(mx >= stmx + stpx);
  CSVASSERT(my >= stmy + stpy);
  CSVASSERT(mz >= stmz + stpz);
  CSVASSERT(unit_size > 0);
  CSVASSERT(path);
  CSVASSERT(mpi);

#ifndef JUPITER_DOUBLE
  use_float = 1;

  if (!fval) {
    fval = val;
    val = NULL;
  }
#endif

  if (use_float) {
    float *afval;

    afval = NULL;

    if (!fval) {
      /* Stencil is not required */
      rmx = mx - stpx - stmx;
      rmy = my - stpy - stmy;
      rmz = mz - stpz - stmz;
      sz  = rmx;
      sz *= rmy;
      sz *= rmz;

      afval = (float *)malloc(sizeof(float) * sz);
      if (for_any_rank(mpi, !afval)) {
        if (!afval) {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                    CSV_ERR_NOMEM, 0, 0, NULL);
        }
        return 2;
      }

      fval = afval;
      rstmx = rstmy = rstmz = 0;
      rstpx = rstpy = rstpz = 0;

    } else {
      rstmx = stmx;
      rstmy = stmy;
      rstmz = stmz;
      rstpx = stpx;
      rstpy = stpy;
      rstpz = stpz;
      rmx = mx;
      rmy = my;
      rmz = mz;
    }

    r = input_binary_generic(mpi, fval,
                             rstmx, rstmy, rstmz, rstpx, rstpy, rstpz,
                             rmx, rmy, rmz, sizeof(float) * unit_size,
                             path, output_mode);
    if (r == 0 && val) {
      int jx, jy, jz;
#pragma omp parallel for collapse(3)
      for (jz = 0; jz < rmz; ++jz) {
        for (jy = 0; jy < rmy; ++jy) {
          for (jx = 0; jx < rmx; ++jx) {
            ptrdiff_t jf, jt;
            int i;

            jf = calc_address(jx + rstmx, jy + rstmy, jz + rstmz,
                              rmx, rmy, rmz) * unit_size;
            jt = calc_address(jx + stmx, jy + stmy, jz + stmz,
                              mx, my, mz) * unit_size;
            for (i = 0; i < unit_size; ++i) {
              val[jt + i] = fval[jf + i];
            }
          }
        }
      }
    }
    free(afval);

  } else {
    type *aval;

    aval = NULL;

    if (!val) {
      /* Stencil is not required */
      rmx = mx - stpx - stmx;
      rmy = my - stpy - stmy;
      rmz = mz - stpz - stmz;
      sz  = rmx;
      sz *= rmy;
      sz *= rmz;

      aval = (type *)malloc(sizeof(type) * sz);
      if (for_any_rank(mpi, !aval)) {
        if (!aval) {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                    CSV_ERR_NOMEM, 0, 0, NULL);
        }
        free(aval);
        return 2;
      }

      val = aval;
      rstmx = rstmy = rstmz = 0;
      rstpx = rstpy = rstpz = 0;

    } else {
      rstmx = stmx;
      rstmy = stmy;
      rstmz = stmz;
      rstpx = stpx;
      rstpy = stpy;
      rstpz = stpz;
      rmx = mx;
      rmy = my;
      rmz = mz;
    }

    r = input_binary_generic(mpi, val,
                             rstmx, rstmy, rstmz, rstpx, rstpy, rstpz,
                             rmx, rmy, rmz, sizeof(type) * unit_size,
                             path, output_mode);
    if (r == 0 && fval) {
      int jx, jy, jz;
#pragma omp parallel for collapse(3)
      for (jz = 0; jz < rmz; ++jz) {
        for (jy = 0; jy < rmy; ++jy) {
          for (jx = 0; jx < rmx; ++jx) {
            ptrdiff_t jf, jt;
            int i;

            jf = calc_address(jx + rstmx, jy + rstmy, jz + rstmz,
                              rmx, rmy, rmz);
            jt = calc_address(jx + stmx, jy + stmy, jz + stmz,
                              mx, my, mz);

            for (i = 0; i < unit_size; ++i) {
              fval[jt + i] = val[jf + i];
            }
          }
        }
      }
    }

    free(aval);
  }

  return r;
}

/**
 * @brief Output binary for cell centered scalar integer data.
 * @param prm Parameter data
 * @param directory output data directory
 * @param name Component name
 * @param component_id Component ID or -1 if not applicable
 * @param val Array to output
 * @param iout Output number or -1 for restart output
 * @param unified non-0 to unify all rank data info 1 file.
 * @return 0 if success, otherwise failed.
 */
static int
output_binary_int(parameter *prm, const char *directory,
                  const struct data_spec *spec, int component_id,
                  int *val, int iout, binary_output_mode unified)
{
  int r;
  domain *cdo;
  char *path;

  CSVASSERT(prm);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->mpi);
  CSVASSERT(spec);

  cdo = prm->cdo;
  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     component_id, iout, prm->mpi->rank, unified);
  if (r) {
    path = NULL;
  }
  if (for_any_rank(prm->mpi, r)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    free(path);
    return 1;
  }

  r = output_binary_generic(prm->mpi, val, cdo->stm, cdo->stm, cdo->stm,
                            cdo->stp, cdo->stp, cdo->stp,
                            cdo->mx, cdo->my, cdo->mz, sizeof(int), path,
                            unified);
  free(path);
  return r;
}

/**
 * @brief Output binary for cell centered scalar data.
 * @param prm Parameter data
 * @param directory output data directory
 * @param name Component name
 * @param component_id Component ID or -1 if not applicable
 * @param val Array to output
 * @param iout Output number or -1 for restart output
 * @param unified non-0 to unify all rank data info 1 file.
 * @param use_float Output with float data (no effect in SINGLE precision
 * JUPITER)
 * @param unit_size Number of elements
 * @param nb Number of boundary cells to be included.
 * @return 0 if success, otherwise failed.
 */
static int output_binary_nb(parameter *prm, const char *directory,
                            const struct data_spec *spec, int component_id,
                            type *val, int iout, binary_output_mode unified,
                            int use_float, int unit_size, int nb)
{
  int r;
  domain *cdo;
  char *path;

  CSVASSERT(prm);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->mpi);
  CSVASSERT(spec);
  CSVASSERT(nb >= 0);
  CSVASSERT(prm->cdo->stm >= nb);
  CSVASSERT(prm->cdo->stp >= nb);

  cdo = prm->cdo;
  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     component_id, iout, prm->mpi->rank, unified);
  if (r) {
    path = NULL;
  }
  if (for_any_rank(prm->mpi, r)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    free(path);
    return 1;
  }

  r = output_binary(prm->mpi, val,
                    cdo->stm - nb, cdo->stm - nb, cdo->stm - nb,
                    cdo->stp - nb, cdo->stp - nb, cdo->stp - nb,
                    cdo->mx, cdo->my, cdo->mz, unit_size, path, unified,
                    use_float);
  free(path);
  return r;
}

/**
 * @brief Output binary for cell centered scalar data.
 * @param prm Parameter data
 * @param directory output data directory
 * @param name Component name
 * @param component_id Component ID or -1 if not applicable
 * @param val Array to output
 * @param iout Output number or -1 for restart output
 * @param unified non-0 to unify all rank data info 1 file.
 * @param use_float Output with float data (no effect in SINGLE precision
 * JUPITER)
 * @param nb Number of boundary cells to be included.
 * @return 0 if success, otherwise failed.
 */
static int output_binary_scalar_nb(parameter *prm, const char *directory,
                                   const struct data_spec *spec,
                                   int component_id, type *val, int iout,
                                   binary_output_mode unified, int use_float,
                                   int nb)
{
  return output_binary_nb(prm, directory, spec, component_id, val, iout,
                          unified, use_float, 1, nb);
}

/**
 * @brief Output binary for cell centered scalar data.
 * @param prm Parameter data
 * @param directory output data directory
 * @param name Component name
 * @param component_id Component ID or -1 if not applicable
 * @param val Array to output
 * @param iout Output number or -1 for restart output
 * @param unified non-0 to unify all rank data info 1 file.
 * @param use_float Output with float data (no effect in SINGLE precision JUPITER)
 * @return 0 if success, otherwise failed.
 */
static int
output_binary_scalar(parameter *prm, const char *directory,
                     const struct data_spec *spec, int component_id,
                     type *val, int iout, binary_output_mode unified,
                     int use_float)
{
  return output_binary_nb(prm, directory, spec, component_id, val,
                          iout, unified, use_float, 1, 0);
}

/**
 * @brief Output binary for cell centered AOS vector data.
 * @param prm Parameter data
 * @param directory output data directory
 * @param name Component name
 * @param component_id Component ID or -1 if not applicable
 * @param val Array to output
 * @param iout Output number or -1 for restart output
 * @param unified non-0 to unify all rank data info 1 file.
 * @param use_float Output with float data (no effect in SINGLE precision
 * JUPITER)
 * @return 0 if success, otherwise failed.
 */
static int output_binary_aos_vector(parameter *prm, const char *directory,
                                    const struct data_spec *spec,
                                    int component_id, type *val, int iout,
                                    binary_output_mode unified, int use_float)
{
  return output_binary_nb(prm, directory, spec, component_id, val, iout,
                          unified, use_float, 3, 0);
}

/**
 * @brief Output binary for cell centered vector data.
 * @param prm Parameter data
 * @param directory output directory
 * @param spec Output data specifiction
 * @param component_id Component ID or -1 if not applicable
 * @param u Array for X-component of vector (note: face data)
 * @param v Array for Y-component of vector (note: face data)
 * @param w Array for Z-component of vector (note: face data)
 * @param iout Output number or -1 for restart output
 * @param unified non-0 to unify all rank data info 1 file.
 * @param use_float Output with float data (no effect in SINGLE precision JUPITER)
 * @return 0 if success, otherwise failed.
 */
static int
output_binary_vector(parameter *prm, const char *directory,
                     const struct data_spec *spec, int component_id,
                     type *u, type *v, type *w,
                     int iout, int unified, int use_float)
{
  type *buf;
  size_t ocount;
  char *path;
  domain *cdo;
  int r;
  int jx;
  int jy;
  int jz;
  int stm;

  CSVASSERT(prm);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->mpi);

  cdo = prm->cdo;
  stm = cdo->stm;
  ocount = cdo->nx;
  ocount = ocount * cdo->ny * cdo->nz;
  ocount *= 3;
  buf = (type *)malloc(sizeof(type) * ocount);
  if (for_any_rank(prm->mpi, !buf)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    free(buf);
    return 1;
  }


  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     component_id, iout, prm->mpi->rank, unified);
  if (r) {
    path = NULL;
  }
  if (for_any_rank(prm->mpi, r)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    free(path);
    free(buf);
    return 1;
  }

#pragma omp parallel for collapse(3)
  for (jz = 0; jz < cdo->nz; ++jz) {
    for (jy = 0; jy < cdo->ny; ++jy) {
      for (jx = 0; jx < cdo->nx; ++jx) {
        int jj, jt;
        type uu, vv, ww, un, vn, wn;
        jj = stm + jx + cdo->mx * (stm + jy + cdo->my * (stm + jz));
        jt = jx + cdo->nx * (jy + cdo->ny * jz);
        jt *= 3;
        uu = u[jj];
        vv = v[jj];
        ww = w[jj];
        un = u[jj + 1];
        vn = v[jj + cdo->mx];
        wn = w[jj + cdo->mxy];
        buf[jt    ] = 0.5 * (uu + un);
        buf[jt + 1] = 0.5 * (vv + vn);
        buf[jt + 2] = 0.5 * (ww + wn);
      }
    }
  }

  r = output_binary(prm->mpi, buf, 0, 0, 0, 0, 0, 0,
                    cdo->nx, cdo->ny, cdo->nz, 3,
                    path, unified, use_float);
  free(buf);
  free(path);
  return r;
}

void set_self_comm_mpi(mpi_param *mpi)
{
  CSVASSERT(mpi);
  memset(mpi, 0, sizeof(mpi_param));
  set_mpi_for_rank(mpi, 1, 1, 1, 0, 0, 0, NULL);
  mpi->radiation_running = OFF;
#ifdef JUPITER_MPI
  mpi->CommJUPITER = MPI_COMM_SELF;
  mpi->CommLx = MPI_COMM_NULL;
  mpi->CommLy = MPI_COMM_NULL;
  mpi->CommLz = MPI_COMM_NULL;
  mpi->CommSx = MPI_COMM_NULL;
  mpi->CommSy = MPI_COMM_NULL;
  mpi->CommSz = MPI_COMM_NULL;
  MPI_Comm_size(mpi->CommJUPITER, &mpi->npe);
  MPI_Comm_rank(mpi->CommJUPITER, &mpi->rank);
  CSVASSERT(mpi->npe == 1);
  CSVASSERT(mpi->rank == 0);
#endif
}

static void set_z_comm_mpi(mpi_param *mpi, mpi_param *base_mpi)
{
  CSVASSERT(mpi);
  CSVASSERT(base_mpi);
  memset(mpi, 0, sizeof(mpi_param));
  set_mpi_for_rank(mpi, 1, 1, base_mpi->npe_z, 0, 0, base_mpi->rank_z, NULL);
  mpi->radiation_running = OFF;
#ifdef JUPITER_MPI
  MPI_Comm_split(base_mpi->CommJUPITER, base_mpi->rank_xy, base_mpi->rank_z,
                 &mpi->CommJUPITER);
  mpi->CommLx = MPI_COMM_NULL;
  mpi->CommLy = MPI_COMM_NULL;
  mpi->CommLz = MPI_COMM_NULL;
  mpi->CommSx = MPI_COMM_NULL;
  mpi->CommSy = MPI_COMM_NULL;
  mpi->CommSz = MPI_COMM_NULL;
  MPI_Comm_size(mpi->CommJUPITER, &mpi->npe);
  MPI_Comm_rank(mpi->CommJUPITER, &mpi->rank);
  CSVASSERT(mpi->npe == mpi->npe_z);
  CSVASSERT(mpi->rank == base_mpi->rank_z);
#endif
}

static void free_z_comm_mpi(mpi_param *mpi)
{
#ifdef JUPITER_MPI
  MPI_Comm_free(&mpi->CommJUPITER);
#endif
}


/**
 * @brief Output binary for LPT control metadata.
 * @param prm Parameter data
 * @param directory output data directory
 * @param name Component name
 * @param component_id Component ID or -1 if not applicable
 * @param data Data to output
 * @param iout Output number or -1 for restart output
 * @param unified non-0 to unify all rank data info 1 file.
 * @return 0 if success, otherwise failed.
 *
 * If @p unified affects only to the output filename. This function
 * always write only by rank 0.
 *
 * This function is not MPI-collective at this version, but this
 * property may be lost in the future version.
 *
 * @note Because this function is not MPI-collective, the result is
 * *not* shared among MPI ranks.
 */
static int
output_binary_lpt_ctrl(parameter *prm, const char *directory,
                       const struct data_spec *spec, int component_id,
                       jupiter_lpt_ctrl_data *data, int iout,
                       binary_output_mode unified)
{
  int r;
  char *path;
  char *adir;

  CSVASSERT(prm);
  CSVASSERT(prm->mpi);
  CSVASSERT(spec);

  path = NULL;
  if (prm->mpi->rank == 0) {
    r = make_file_name(&path, directory, spec->filename_template, spec->name,
                       component_id, iout, prm->mpi->rank, unified);
    if (r) {
      path = NULL;
      return 1;
    }

    r = extract_dirname_allocate(&adir, path);
    if (r < 0) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                CSV_ERR_NOMEM, 0, 0, NULL);
      free(path);
      return 1;
    }

    errno = 0;
    r = make_directory_recursive(adir);
    if (r) {
      csvperror(adir, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
      free(adir);
      free(path);
      return 1;
    }
    free(adir);

    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL, "Writing %s...", path);
    r = write_lpt_ctrl_data(path, data);
    free(path);
  } else {
    r = 0;
  }
  return r;
}

/**
 * @brief Output binary for LPT scalar or vector data without particle
 * @param prm Parameter data
 * @param path path to file
 * @return 0 if success, otherwise error occured
 *
 * This function does nothing.
 */
static int
output_binary_lpt_0_particles(parameter *prm, const char *path)
{
  CSVASSERT(prm);
  CSVASSERT(path);

  /* NOP */
  csvperrorf(path, 0, 0, CSV_EL_INFO, NULL,
             "No particle; file is not written");
  return 0;
}

/**
 * @brief Output binary for LPT scalar data.
 * @param prm Parameter data
 * @param directory output data directory
 * @param name Component name
 * @param component_id Component ID or -1 if not applicable
 * @param npt Number of particles
 * @param val Array to output
 * @param iout Output number or -1 for restart output
 * @param unified non-0 to unify all rank data info 1 file.
 * @param use_float Output with float data (no effect in SINGLE precision JUPITER)
 * @return 0 if success, otherwise failed.
 *
 * If @p unified affects only to the output filename. This function
 * always write only by rank 0.
 *
 * This function is not MPI-collective at this version, but this
 * property may be lost in the future version.
 *
 * @note Because this function is not MPI-collective, the result is
 * *not* shared among MPI ranks.
 */
static int
output_binary_lpt_scalar(parameter *prm, const char *directory,
                         const struct data_spec *spec, int component_id,
                         int npt, type *val, int iout, binary_output_mode unified,
                         int use_float)
{
  int r;
  char *path;
  mpi_param mpi;

  CSVASSERT(prm);
  CSVASSERT(prm->mpi);
  CSVASSERT(spec);

  set_self_comm_mpi(&mpi);

  if (prm->mpi->rank == 0) {
    r = make_file_name(&path, directory, spec->filename_template, spec->name,
                       component_id, iout, prm->mpi->rank, unified);
    if (r) {
      return 1;
    }

    if (npt > 0) {
      r = output_binary(&mpi, val, 0, 0, 0, 0, 0, 0, npt, 1, 1, 1, path,
                        BINARY_OUTPUT_BYPROCESS, use_float);
    } else {
      r = output_binary_lpt_0_particles(prm, path);
    }
    free(path);
  } else {
    r = 0;
  }
  return r;
}

/**
 * @brief Output binary for LPT scalar data.
 * @param prm Parameter data
 * @param directory output data directory
 * @param name Component name
 * @param component_id Component ID or -1 if not applicable
 * @param npt Number of particles
 * @param val_x Array of X axis value to output
 * @param val_y Array of Y axis value to output
 * @param val_z Array of Z axis value to output
 * @param iout Output number or -1 for restart output
 * @param unified non-0 to unify all rank data info 1 file.
 * @param use_float Output with float data (no effect in SINGLE precision JUPITER)
 * @return 0 if success, otherwise failed.
 *
 * If @p unified affects only to the output filename. This function
 * always write only by rank 0.
 *
 * This function is not MPI-collective at this version, but this
 * property may be lost in the future version.
 *
 * @note Because this function is not MPI-collective, the result is
 * *not* shared among MPI ranks.
 */
static int
output_binary_lpt_vector(parameter *prm, const char *directory,
                         const struct data_spec *spec, int component_id,
                         int npt, type *val_x, type *val_y, type *val_z,
                         int iout, binary_output_mode unified,
                         int use_float)
{
  int r;
  char *path;
  mpi_param mpi;

  CSVASSERT(prm);
  CSVASSERT(prm->mpi);
  CSVASSERT(spec);
  CSVASSERT(npt >= 0);

  set_self_comm_mpi(&mpi);

  if (prm->mpi->rank == 0) {
    type *val;
    ptrdiff_t nptv;
    int i;

    nptv = 0;

    if (npt > 0) {
      CSVASSERT(val_x);
      CSVASSERT(val_y);
      CSVASSERT(val_z);

      nptv  = npt;
      nptv *= 3 * sizeof(type);
      if (nptv / 3 / sizeof(type) != npt) {
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                   "Overflow detected");
        return 1;
      }
    }

    r = make_file_name(&path, directory, spec->filename_template, spec->name,
                       component_id, iout, prm->mpi->rank, unified);
    if (r) {
      return 1;
    }

    if (npt > 0) {
      val = (type *)malloc(sizeof(type) * nptv);
      if (!val) {
        free(path);
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                  CSV_ERR_NOMEM, 0, 0, NULL);
        return 1;
      }

#pragma omp parallel for
      for (i = 0; i < npt; ++i) {
        ptrdiff_t jj;
        jj = i;
        jj *= 3;
        val[jj]     = val_x[i];
        val[jj + 1] = val_y[i];
        val[jj + 2] = val_z[i];
      }

      r = output_binary(&mpi, val, 0, 0, 0, 0, 0, 0, npt, 1, 1, 3, path,
                        BINARY_OUTPUT_BYPROCESS, use_float);
      free(val);
    } else {
      r = output_binary_lpt_0_particles(prm, path);
    }
    free(path);
  } else {
    r = 0;
  }
  return r;
}

static int output_binary_lpt_scalar_int(parameter *prm, const char *directory,
                                        const struct data_spec *spec,
                                        int component_id, int npt, int *val,
                                        int iout, binary_output_mode unified)
{
  int r;
  char *path;
  mpi_param mpi;

  CSVASSERT(prm);
  CSVASSERT(prm->mpi);
  CSVASSERT(spec);

  set_self_comm_mpi(&mpi);

  if (prm->mpi->rank == 0) {
    r = make_file_name(&path, directory, spec->filename_template, spec->name,
                       component_id, iout, prm->mpi->rank, unified);
    if (r) {
      return 1;
    }

    if (npt > 0) {
      r = output_binary_generic(&mpi, val, 0, 0, 0, 0, 0, 0, npt, 1, 1,
                                sizeof(int), path, BINARY_OUTPUT_BYPROCESS);
    } else {
      r = output_binary_lpt_0_particles(prm, path);
    }
    free(path);
  } else {
    r = 0;
  }
  return r;
}

#ifdef LPTX
static int output_binary_lpt_scalar_ids(parameter *prm, const char *directory,
                                        const struct data_spec *spec,
                                        int component_id, int npt,
                                        LPTX_idtype *val,
                                        int iout, binary_output_mode unified)
{
  int r;
  char *path;
  mpi_param mpi;

  CSVASSERT(prm);
  CSVASSERT(prm->mpi);
  CSVASSERT(spec);

  set_self_comm_mpi(&mpi);

  if (prm->mpi->rank == 0) {
    r = make_file_name(&path, directory, spec->filename_template, spec->name,
                       component_id, iout, prm->mpi->rank, unified);
    if (r) {
      return 1;
    }

    if (npt > 0) {
      r = output_binary_generic(&mpi, val, 0, 0, 0, 0, 0, 0, npt, 1, 1,
                                sizeof(LPTX_idtype), path,
                                BINARY_OUTPUT_BYPROCESS);
    } else {
      r = output_binary_lpt_0_particles(prm, path);
    }
    free(path);
  } else {
    r = 0;
  }
  return r;
}
#endif

static int output_binary_lpt_scalar_rnds(parameter *prm, const char *directory,
                                         const struct data_spec *spec,
                                         int component_id, int npt,
                                         uint64_t *val, int iout,
                                         binary_output_mode unified)
{
  int r;
  char *path;
  mpi_param mpi;

  CSVASSERT(prm);
  CSVASSERT(prm->mpi);
  CSVASSERT(spec);

  set_self_comm_mpi(&mpi);

  if (prm->mpi->rank == 0) {
    r = make_file_name(&path, directory, spec->filename_template, spec->name,
                       component_id, iout, prm->mpi->rank, unified);
    if (r) {
      return 1;
    }

    if (npt > 0) {
      r = output_binary_generic(&mpi, val, 0, 0, 0, 0, 0, 0, npt, 1, 1,
                                JUPITER_RANDOM_SEED_SIZE * sizeof(uint64_t),
                                path, BINARY_OUTPUT_BYPROCESS);
    } else {
      r = output_binary_lpt_0_particles(prm, path);
    }
    free(path);
  } else {
    r = 0;
  }
  return r;
}

#ifdef LPTX
static int output_binary_lpt_scalar_flgs(parameter *prm, const char *directory,
                                        const struct data_spec *spec,
                                        int component_id, int npt,
                                         char *val,
                                        int iout, binary_output_mode unified)
{
  int r;
  char *path;
  mpi_param mpi;

  CSVASSERT(prm);
  CSVASSERT(prm->mpi);
  CSVASSERT(spec);

  set_self_comm_mpi(&mpi);

  if (prm->mpi->rank == 0) {
    r = make_file_name(&path, directory, spec->filename_template, spec->name,
                       component_id, iout, prm->mpi->rank, unified);
    if (r) {
      return 1;
    }

    if (npt > 0) {
      r = output_binary_generic(&mpi, val, 0, 0, 0, 0, 0, 0, npt, 1, 1,
                                LPTX_PTFLAG_MAX * sizeof(char),
                                path, BINARY_OUTPUT_BYPROCESS);
    } else {
      r = output_binary_lpt_0_particles(prm, path);
    }
    free(path);
  } else {
    r = 0;
  }
  return r;
}
#endif

/**
 * @brief Output time data file.
 * @param cdo Domain data
 * @param path Path to the output file.
 * @retval 0 Success
 * @retval non-0 Falied
 *
 * This function does not make any considerations on the 'timing' in
 * parallel processing. Only 1 process (typically, MPI rank 0) should
 * call this function.
 */
static int
output_time_file(domain *cdo, const char *path)
{
  FILE *fp;
  long l;
  int stat;
  char *dir;

  CSVASSERT(cdo);
  CSVASSERT(path);

  stat = extract_dirname_allocate(&dir, path);
  if (stat < 0) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM,
              0, 0, NULL);
    return 1;
  }

  errno = 0;
  stat = make_directory_recursive(dir);
  if (stat) {
    csvperror(dir, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
    free(dir);
    return 1;
  }
  free(dir);

  l = 0;
  stat = 0;
  errno = 0;
  csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL, "Writing %s...", path);
  fp = fopen(path, "w");
  if (!fp) {
    if (errno == 0) {
      csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_FOPEN, 0, 0, NULL);
    } else {
      csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
    }
    return 1;
  }
  fprintf(fp,"%d\n", cdo->icnt);
  fprintf(fp,"%f\n", cdo->time);
  fprintf(fp,"%d\n", cdo->iout);
  fprintf(fp,"%f\n", cdo->dtout);
  fprintf(fp,"%f\n", cdo->tout);
  stat = ferror(fp);
  fclose(fp);

  return stat;
}

static int
output_uplsflg_file(flags *flg, const char *path)
{
  FILE *fp;
  int stat;
  char *dir;

  CSVASSERT(flg);
  CSVASSERT(path);

  stat = extract_dirname_allocate(&dir, path);
  if (stat < 0) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return 1;
  }

  errno = 0;
  stat = make_directory_recursive(dir);
  if (stat) {
    csvperror(dir, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
    free(dir);
    return 1;
  }
  free(dir);

  stat = update_level_set_flags_write(path, "ls", &flg->update_level_set_ls);
  if (stat)
    return 1;

  stat = update_level_set_flags_write(path, "lls", &flg->update_level_set_lls);
  if (stat)
    return 1;

  stat = update_level_set_flags_write(path, "ll", &flg->update_level_set_ll);
  if (stat)
    return 1;

  return 0;
}

static int
output_boundary(parameter *prm, const char *directory,
                const struct data_spec *spec, variable *val, int iout,
                binary_output_mode unified)
{
  int r;
  domain *cdo;
  char *path;
  char *dir;
  FILE *fp;
  msgpackx_data *d;

  if (unified != BINARY_OUTPUT_BYPROCESS) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "UNIFIED output mode is not suppored yet for boundary data. Use "
               "BYPROCESS output mode instead.");
    return 1;
  }

  CSVASSERT(prm);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->mpi);
  CSVASSERT(spec);

  cdo = prm->cdo;

  path = NULL;
  dir = NULL;
  d = NULL;

  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     -1, iout, prm->mpi->rank, unified);
  if (r) {
    path = NULL;
  }
  if (for_any_rank(prm->mpi, r)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    goto error;
  }

  d = boundary_data_array_dump(&cdo->fluid_boundary_head,
                               &cdo->thermal_boundary_head,
                               cdo->nbx, cdo->nby, cdo->nbz,
                               &val->bnd_W, &val->bnd_E, &val->bnd_S,
                               &val->bnd_N, &val->bnd_B, &val->bnd_T);
  if (!d) {
    goto error;
  }

  r = extract_dirname_allocate(&dir, path);
  if (r < 0) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    goto error;
  }

  errno = 0;
  r = make_directory_recursive(dir);
  if (r) {
    csvperror(dir, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
    goto error;
  }
  free(dir);
  dir = NULL;

  csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
             "Writing %s...", path);

  fp = fopen(path, "wb");
  if (!fp) {
    csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
    goto error;
  }
  free(path);
  path = NULL;

  if (msgpackx_data_write(d, fp) < 0) {
    csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
    fclose(fp);
    goto error;
  }

  fclose(fp);
  msgpackx_data_delete(d);
  return 0;

error:
  if (d) msgpackx_data_delete(d);
  if (path) free(path);
  if (dir)  free(dir);
  return 1;
}

static int output_uplsflg(parameter *prm, const char *directory,
                          const struct data_spec *spec, int iout)
{
  int r;
  char *path;
  char *dir;
  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(prm->mpi);
  CSVASSERT(spec);

  path = NULL;
  dir = NULL;
  r = 0;

  do {
    if (prm->mpi->rank != 0)
      break;

    r = make_shared_file_name(&path, directory, spec, iout);
    if (r) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      path = NULL;
      break;
    }

    r = extract_dirname_allocate(&dir, path);
    if (r < 0) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      dir = NULL;
      break;
    }

    free(dir);
    dir = NULL;

    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
               "Writing %s...", path);
    r = output_uplsflg_file(prm->flg, path);
  } while(0);

  if (path)
    free(path);
  if (dir)
    free(dir);
  if (for_any_rank(prm->mpi, r != 0))
    return 1;
  return 0;
}

static int output_comp_data(parameter *prm, variable *val,
                            const char *directory, const struct data_spec *spec,
                            int iout, binary_output_mode uni)
{
  int r;
  char *path;
  char *dir;
  struct component_data_metadata mdata;

  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(prm->mpi);
  CSVASSERT(spec);

  path = NULL;
  dir = NULL;
  r = 0;

  do {
    if (prm->mpi->rank != 0)
      break;

    component_data_metadata_get(&mdata, prm->cdo, prm->flg, val, uni);

    r = make_shared_file_name(&path, directory, spec, iout);
    if (r) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      path = NULL;
      break;
    }

    r = extract_dirname_allocate(&dir, path);
    if (r < 0) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      dir = NULL;
      break;
    }

    r = make_directory_recursive(dir);
    free(dir);
    dir = NULL;

    if (r)
      break;

    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL, "Writing %s...", path);
    r = component_data_write(path, &prm->comps_data_head, &mdata);
  } while (0);

  if (path)
    free(path);
  if (dir)
    free(dir);
  if (for_any_rank(prm->mpi, r != 0))
    return 1;
  return 0;
}

int input_binary_int(parameter *prm, const char *directory,
                     const struct data_spec *spec, int component_id,
                     int *val, int iout, binary_output_mode unified)
{
  int r;
  domain *cdo;
  char *path;

  CSVASSERT(prm);
  CSVASSERT(prm->cdo);

  cdo = prm->cdo;
  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     component_id, iout, prm->mpi->rank, unified);
  if (r) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    return 1;
  }

  r = input_binary_generic(prm->mpi, val, cdo->stm, cdo->stm, cdo->stm,
                           cdo->stp, cdo->stp, cdo->stp,
                           cdo->mx, cdo->my, cdo->mz, sizeof(int),
                           path, unified);
  free(path);
  return r;
}

int input_binary_nb(parameter *prm, const char *directory,
                    const struct data_spec *spec, int component_id, type *val,
                    int iout, binary_output_mode unified, int use_float,
                    int unit_size, int nb)
{
  int r;
  domain *cdo;
  char *path;

  CSVASSERT(prm);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->cdo->stm >= nb);
  CSVASSERT(prm->cdo->stp >= nb);
  CSVASSERT(nb >= 0);

  cdo = prm->cdo;
  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     component_id, iout, prm->mpi->rank, unified);
  if (r) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return 2;
  }

  r = input_binary(prm->mpi, val, NULL,
                   cdo->stm - nb, cdo->stm - nb, cdo->stm - nb,
                   cdo->stp - nb, cdo->stp - nb, cdo->stp - nb,
                   cdo->mx, cdo->my, cdo->mz, 1, path, unified, use_float);
  free(path);
  return r;
}

int input_binary_scalar_nb(parameter *prm, const char *directory,
                           const struct data_spec *spec, int component_id,
                           type *val, int iout, binary_output_mode unified,
                           int use_float, int nb)
{
  return input_binary_nb(prm, directory, spec, component_id, val, iout, unified,
                         use_float, 1, nb);
}

int input_binary_scalar(parameter *prm, const char *directory,
                        const struct data_spec *spec, int component_id,
                        type *val, int iout, binary_output_mode unified,
                        int use_float)
{
  return input_binary_nb(prm, directory, spec, component_id, val, iout, unified,
                         use_float, 1, 0);
}

int input_binary_aos_vector(parameter *prm, const char *directory,
                            const struct data_spec *spec, int component_id,
                            type *val, int iout, binary_output_mode unified,
                            int use_float)
{
  return input_binary_nb(prm, directory, spec, component_id, val, iout, unified,
                         use_float, 3, 0);
}

int input_binary_lpt_ctrl(parameter *prm, const char *directory,
                          const struct data_spec *spec, int component_id,
                          jupiter_lpt_ctrl_data **data, int iout,
                          binary_output_mode unified)
{
  int r;
  domain *cdo;
  char *path;

  CSVASSERT(prm);

  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     component_id, iout, 0, unified);
  if (r) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    return 2;
  }

  csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
             "Reading %s...", path);
  r = read_lpt_ctrl_data(path, data);
  free(path);
  return r;
}

static
int input_binary_lpt_0_particle(parameter *prm, const char *path)
{
  CSVASSERT(prm);
  CSVASSERT(path);

  /* NOP */
  return 0;
}

int input_binary_lpt_scalar(parameter *prm, const char *directory,
                            const struct data_spec *spec, int component_id,
                            int npt, type *val, int iout,
                            binary_output_mode unified, int use_float)
{
  int r;
  domain *cdo;
  char *path;
  mpi_param mpi;

  CSVASSERT(prm);

  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     component_id, iout, 0, unified);
  if (r) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    return 2;
  }

  if (npt > 0) {
    set_self_comm_mpi(&mpi);
    r = input_binary(&mpi, val, NULL,
                     0, 0, 0, 0, 0, 0, npt, 1, 1, 1, path,
                     BINARY_OUTPUT_BYPROCESS, use_float);
  } else {
    r = input_binary_lpt_0_particle(prm, path);
  }
  free(path);
  return r;
}

int input_binary_lpt_scalar_int(parameter *prm, const char *directory,
                                const struct data_spec *spec, int component_id,
                                int npt, int *val, int iout,
                                binary_output_mode unified)
{
  int r;
  domain *cdo;
  char *path;
  mpi_param mpi;

  CSVASSERT(prm);

  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     component_id, iout, 0, unified);
  if (r) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return 2;
  }

  if (npt > 0) {
    set_self_comm_mpi(&mpi);
    r = input_binary_generic(&mpi, val, 0, 0, 0, 0, 0, 0, npt, 1, 1,
                             sizeof(int), path, BINARY_OUTPUT_BYPROCESS);
  } else {
    r = input_binary_lpt_0_particle(prm, path);
  }
  free(path);
  return r;
}

#ifdef LPTX
int input_binary_lpt_scalar_ids(parameter *prm, const char *directory,
                                const struct data_spec *spec, int component_id,
                                int npt, LPTX_idtype *val, int iout,
                                binary_output_mode unified)
{
  int r;
  domain *cdo;
  char *path;
  mpi_param mpi;

  CSVASSERT(prm);

  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     component_id, iout, 0, unified);
  if (r) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return 2;
  }

  if (npt > 0) {
    set_self_comm_mpi(&mpi);
    r = input_binary_generic(&mpi, val, 0, 0, 0, 0, 0, 0, npt, 1, 1,
                             sizeof(LPTX_idtype), path,
                             BINARY_OUTPUT_BYPROCESS);
  } else {
    r = input_binary_lpt_0_particle(prm, path);
  }
  free(path);
  return r;
}
#endif

int input_binary_lpt_scalar_rnds(parameter *prm, const char *directory,
                                 const struct data_spec *spec, int component_id,
                                 int npt, uint64_t *val, int iout,
                                 binary_output_mode unified)
{
  int r;
  domain *cdo;
  char *path;
  mpi_param mpi;

  CSVASSERT(prm);

  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     component_id, iout, 0, unified);
  if (r) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return 2;
  }

  if (npt > 0) {
    set_self_comm_mpi(&mpi);
    r = input_binary_generic(&mpi, val, 0, 0, 0, 0, 0, 0, npt, 1, 1,
                             JUPITER_RANDOM_SEED_SIZE * sizeof(uint64_t), path,
                             BINARY_OUTPUT_BYPROCESS);
  } else {
    r = input_binary_lpt_0_particle(prm, path);
  }
  free(path);
  return r;
}

#ifdef LPTX
int input_binary_lpt_scalar_flgs(parameter *prm, const char *directory,
                                 const struct data_spec *spec, int component_id,
                                 int npt, char *val,
                                 int iout, binary_output_mode unified)
{
  int r;
  domain *cdo;
  char *path;
  mpi_param mpi;

  CSVASSERT(prm);

  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     component_id, iout, 0, unified);
  if (r) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return 2;
  }

  if (npt > 0) {
    set_self_comm_mpi(&mpi);
    r = input_binary_generic(&mpi, val, 0, 0, 0, 0, 0, 0, npt, 1, 1,
                             LPTX_PTFLAG_MAX * sizeof(char),
                             path, BINARY_OUTPUT_BYPROCESS);
  } else {
    r = input_binary_lpt_0_particle(prm, path);
  }
  free(path);
  return r;
}
#endif

/**
 * @brief Input binary for staggered scalar (aka. face) data.
 * @param prm Parameter data
 * @param name Component name
 * @param component_id Component ID or -1 if not applicable
 * @param val Array to output
 * @param iout Output number or -1 for restart output
 * @param unified non-0 to unify all rank data info 1 file.
 * @param use_float Output with float data (no effect in SINGLE precision JUPITER)
 * @return 0 if success, otherwise failed.
 */
static int
input_binary_staggered(parameter *prm, char *directory,
                       const struct data_spec *spec, int component_id,
                       type *val, int iout, binary_output_mode unified,
                       int use_float)
{
  int r;
  domain *cdo;
  char *path;
  int stm;
  int stpx, stpy, stpz;

  CSVASSERT(prm);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->mpi);

  cdo = prm->cdo;
  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     component_id, iout, prm->mpi->rank, unified);
  if (r) {
    path = NULL;
  }
  if (for_any_rank(prm->mpi, r)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    free(path);
    return 1;
  }
  CSVASSERT(cdo->stp > 0);
  stm = cdo->stm;
  stpx = cdo->stp;
  stpy = cdo->stp;
  stpz = cdo->stp;
  if (prm->mpi->rank_x == prm->mpi->npe_x - 1) {
    if (stpx > 0) stpx--;
  }
  if (prm->mpi->rank_y == prm->mpi->npe_y - 1) {
    if (stpy > 0) stpy--;
  }
  if (prm->mpi->rank_z == prm->mpi->npe_z - 1) {
    if (stpz > 0) stpz--;
  }
  r = input_binary(prm->mpi, val, NULL,
                   stm, stm, stm, stpx, stpy, stpz,
                   cdo->mx, cdo->my, cdo->mz, 1, path, unified, use_float);
  free(path);
  return r;
}

int input_time_file(mpi_param *mpi, domain *cdo, const char *path)
{
  FILE *fp;
  long l;
  int stat;
  int rsize;
  int icnt;
  double time;
  int iout;
  double dtout;
  double tout;

  CSVASSERT(cdo);
  CSVASSERT(path);

  l = 0;
  stat = 0;
  fp = NULL;

  if (mpi->rank == 0) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
               "Reading %s...", path);
    errno = 0;
    fp = fopen(path, "r");
    if (!fp) {
      if (errno == 0) {
        csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_FOPEN, 0, 0, NULL);
      } else {
        csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
      }
      stat = 1;
      goto clean;
    }

    rsize = fscanf(fp,"%d\n",&icnt);
    if (rsize == 0) {
      stat = 1;
      csvperrorf(path, l, 0, CSV_EL_FATAL, NULL,
                 "Could not parse step count");
      goto clean;
    }

    rsize = fscanf(fp,"%lf\n",&time);
    if (rsize == 0) {
      stat = 1;
      csvperrorf(path, l, 0, CSV_EL_FATAL, NULL,
                 "Could not parse physical time");
      goto clean;
    }

    rsize = fscanf(fp,"%d\n",&iout);
    if (rsize == 0) {
      stat = 1;
      csvperrorf(path, l, 0, CSV_EL_FATAL, NULL,
                 "Could not parse output count");
      goto clean;
    }

    rsize = fscanf(fp,"%lf\n",&dtout);
    if (rsize == 0) {
      stat = 1;
      csvperrorf(path, l, 0, CSV_EL_FATAL, NULL,
                 "Could not parse output interval");
      goto clean;
    }

    rsize = fscanf(fp,"%lf\n",&tout);
    if (rsize == 0) {
      stat = 1;
      csvperrorf(path, l, 0, CSV_EL_FATAL, NULL,
                 "Could not parse last output time");
      goto clean;
    }

    if (time < cdo->time) {
      stat = 1;
      csvperrorf(path, l, 0, CSV_EL_ERROR, NULL,
                 "Current time (%g s) is before the start time (%g s)",
                 time, cdo->time);
      goto clean;
    }
  }

clean:
  if (fp) {
    fclose(fp);
  }

#ifdef JUPITER_MPI
  MPI_Bcast(&stat, 1, MPI_INT, 0, mpi->CommJUPITER);
#endif
  if (stat == 0) {
    if (mpi->rank == 0) {
      cdo->icnt = icnt;
      cdo->iout = iout;

      if (fabs(cdo->dtout - dtout) >= 5.0e-7) {
        cdo->tout = (floor((time - cdo->time) / cdo->dtout) + 1.0) * cdo->dtout;
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
                   "At %g s: Output interval is changed."
                   " Next output will be %g s.",
                   time, cdo->tout);
      } else {
        cdo->tout = tout;
      }
      cdo->time = time;
    }

#ifdef JUPITER_MPI
    MPI_Bcast(&cdo->icnt, 1, MPI_INT, 0, mpi->CommJUPITER);
    MPI_Bcast(&cdo->time, 1, MPI_DOUBLE, 0, mpi->CommJUPITER);
    MPI_Bcast(&cdo->iout, 1, MPI_INT, 0, mpi->CommJUPITER);
    MPI_Bcast(&cdo->dtout, 1, MPI_DOUBLE, 0, mpi->CommJUPITER);
    MPI_Bcast(&cdo->tout, 1, MPI_DOUBLE, 0, mpi->CommJUPITER);
#endif
  }
  return stat;
}

int input_uplsflg_file(mpi_param *mpi, flags *flg, const char *path)
{
  int stat;
  msgpackx_error err = MSGPACKX_SUCCESS;
  ptrdiff_t eloc;

  CSVASSERT(flg);
  CSVASSERT(path);

  stat = 0;

  if (!mpi || mpi->rank == 0) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL, "Reading %s...", path);
    errno = 0;

    /* If first read fails, skips remainders since they definitly fail. */
    if (update_level_set_flags_read(path, "ls", &flg->update_level_set_ls, &err,
                                    &eloc)) {
      stat = 1;
    }
    if (!stat &&
        update_level_set_flags_read(path, "lls", &flg->update_level_set_lls,
                                    &err, &eloc)) {
      stat = 1;
    }
    if (!stat &&
        update_level_set_flags_read(path, "ll", &flg->update_level_set_ll, &err,
                                    &eloc)) {
      stat = 1;
    }
    if (stat) {
      if (err == MSGPACKX_ERR_SYS) {
        csvperror(path, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0, NULL);
      } else {
        csvperror(path, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0, err,
                  NULL);
      }
    }
    if (mpi) {
#ifdef JUPITER_MPI
      MPI_Bcast(&stat, 1, MPI_INT, 0, mpi->CommJUPITER);
#endif
      update_level_set_flags_share_flag(&flg->update_level_set_ls, mpi);
      update_level_set_flags_share_flag(&flg->update_level_set_lls, mpi);
      update_level_set_flags_share_flag(&flg->update_level_set_ll, mpi);
    }
  } else {
    CSVASSERT(mpi);
#ifdef JUPITER_MPI
    MPI_Bcast(&stat, 1, MPI_INT, 0, mpi->CommJUPITER);
#endif
    update_level_set_flags_share_flag(&flg->update_level_set_ls, mpi);
    update_level_set_flags_share_flag(&flg->update_level_set_lls, mpi);
    update_level_set_flags_share_flag(&flg->update_level_set_ll, mpi);
  }

  return stat;
}

/**
 * @brief Read boundary binary data
 * @param prm Parameter data
 * @param directory Directory of the data file.
 * @param spec Output data_spec for boundary data.
 * @param val Variable data to store the result.
 * @param iout Index number ot read in (-1 for restart data)
 * @param unified Read unified data or byprocess output data
 * @retval 0 Success.
 * @retval 1 Preconditional error or open error (can be reinitialize boundary)
 * @retval 2 Read error or parse error (should be abort)
 *
 * Currently, input of unified boundary data does not supported.
 */
static int
input_boundary(parameter *prm, const char *directory,
               const struct data_spec *spec, variable *val, int iout,
               binary_output_mode unified)
{
  int r;
  domain *cdo;
  char *path;
  FILE *fp;
  csv_error cserr;
  msgpackx_error merr;

  if (unified != BINARY_OUTPUT_BYPROCESS) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
               "UNIFIED input is not suppored yet for boundary data");
    return 1;
  }

  CSVASSERT(prm);
  CSVASSERT(prm->cdo);

  cdo = prm->cdo;
  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     -1, iout, prm->mpi->rank, unified);
  if (r) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    return 1;
  }

  csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
             "Reading %s...", path);
  fp = fopen(path, "rb");
  if (!fp) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
              CSV_ERR_FOPEN, errno, 0, NULL);
    free(path);
    return 1;
  }

  cserr = CSV_ERR_SUCC;
  merr = MSGPACKX_SUCCESS;
  r = boundary_data_array_read_from_file(fp, path, &cdo->fluid_boundary_head,
                                         &cdo->thermal_boundary_head,
                                         &prm->comps_data_head,
                                         prm->controls, &prm->control_head,
                                         cdo->nbx, cdo->nby, cdo->nbz,
                                         &val->bnd_W, &val->bnd_E,
                                         &val->bnd_S, &val->bnd_N,
                                         &val->bnd_B, &val->bnd_T,
                                         &cserr, &merr);
  if (cserr != CSV_ERR_SUCC) {
    csvperror(path, 0, 0, CSV_EL_ERROR, NULL, cserr, errno, merr, NULL);
    r = 2;
  }

  free(path);
  fclose(fp);

  if (r != 0) {
    return 2;
  }
  return 0;
}

int start_output_sync(mpi_param *mpi, const char *time_file)
{
  int r;

  CSVASSERT(mpi);
  CSVASSERT(time_file);

  r = 0;
  if (mpi->rank == 0) {
    /* test time_file and remove */
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
    /*
     * POSIX standard version
     *
     * This is safer than the C standard version.
     *
     * But cannot run on non-POSIX-compliant OSes (ex. Windows)
     */
    {
      struct stat st;
      errno = 0;
      r = lstat(time_file, &st);
      if (r == 0) { /* file exists */
        switch(st.st_mode & S_IFMT) {
        case S_IFLNK: /* Symbolic link */
          csvperrorf(time_file, 0, 0, CSV_EL_WARN, NULL,
                     "Removing a symbolic link (not the destination)");
          CSV_DOFALLTHRU(); // fall through
        case S_IFREG: /* Regular file */
          r = unlink(time_file);
          if (r != 0) {
            csvperror(time_file, 0, 0, CSV_EL_FATAL, "unlink",
                      CSV_ERR_SYS, errno, 0, NULL);
          }
          break;
        default: /* Directory, Device, Pipe, ... */
          csvperrorf(time_file, 0, 0, CSV_EL_FATAL, NULL,
                     "Not a regular file. Abort.");
          r = 1;
        }
      } else if (errno != ENOENT) {
        /* Some problem occured other than just not found. */
        csvperror(time_file, 0, 0, CSV_EL_FATAL, "lstat",
                  CSV_ERR_SYS, errno, 0, NULL);
        r = 1;
      } else {
        r = 0;
      }
      errno = 0;
    }
#else
    /*
     * C standard version
     *
     * If time_file is a symbolic link, this routine creates a file at
     * destination name of the link and (usually) removes the link
     * itself, not the destination file. We cannot fix this manner.
     * (POSIX version removes the link with warning)
     *
     * If time_file is a directory, 'r+' should fail.
     *
     * If time_file is a symbolic link, and...
     *   - If target is a regular file, 'r+' should succeed.
     *   - If target is a directory, 'r+' should fail.
     *   - If target does not exist, 'r+' should fail.
     *
     *    note: 'remove(time_file)` may remove the symblic link itself.
     *
     * If time_file does not exist, 'r+' should fail.
     *
     * If time_file is a special device, this routine may remove it.
     * (POSIX version makes an error)
     *
     * (Windows) I think Windows refuses fopen for a directory, but I
     *   could not find any document about that. By investigation, you
     *   will get an error "permission denied".
     */
    {
      FILE *fp;
      errno = 0;
      fp = fopen(time_file, "r+");
      if (fp) {
        fclose(fp);

        errno = 0;
        r = remove(time_file);
        if (r != 0) {
          if (errno != 0) {
            csvperror(time_file, 0, 0, CSV_EL_FATAL, NULL,
                      CSV_ERR_SYS, errno, 0, NULL);
          } else {
            csvperror(time_file, 0, 0, CSV_EL_FATAL, NULL,
                      CSV_ERR_FOPEN, errno, 0, NULL);
          }
        }
      }
    }
#endif
  }
#ifdef JUPITER_MPI
  MPI_Bcast(&r, 1, MPI_INT, 0, mpi->CommJUPITER);
#endif
  return r;
}

int finish_output_sync(mpi_param *mpi, const char *time_file)
{
  CSVASSERT(mpi);
  CSVASSERT(time_file);

#ifdef JUPITER_MPI
  MPI_Barrier(mpi->CommJUPITER);
#endif
  return 0;
}

#ifdef LPTX
static LPTX_bool lptx_particle_filter_inject(const LPTX_particle_data *p,
                                             void *arg)
{
  if (p->base.origin_id == JUPITER_LPTX_ORIGIN_INJECT)
    return LPTX_true;
  return LPTX_false;
}

struct lptx_particle_set_merge_data
{
  domain *cdo;
};

static void lptx_particle_set_merge(LPTX_particle_data *outp,
                                    const LPTX_particle_data *pa,
                                    const LPTX_particle_data *pb, void *arg)
{
  LPTX_type tap, tbp, tf;
  struct lptx_particle_set_merge_data *p;
  p = (struct lptx_particle_set_merge_data *)arg;

  tap = pa->base.start_time;
  tbp = pb->base.start_time;
  tf = p->cdo->time;
  if (tap > tf && tbp > tf) {
    if (pa != outp)
      LPTX_particle_copy(outp, pa);
    /* Use the value from input data if not started */
  } else {
    if (pb != outp)
      LPTX_particle_copy(outp, pb);
  }
}
#endif

static
int binary_in_lpt(int iout, parameter *prm, variable *val,
                  const char *dir, const struct data_output_spec *spec,
                  int flt, binary_output_mode uni)
{
  struct jupiter_lpt_ctrl_data *lpt_data;
  int r;
  int inp_npt;
  int data_npt;
  int maxcomp = 1;
  type *v;
  type *vx, *vy, *vz;
  const type **vv;
  int *iv;
  uint64_t *u64v;
  ptrdiff_t asz;
  ptrdiff_t usz;
  ptrdiff_t isz;
#ifdef LPT
  size_t ptidx[2];
#endif
#ifdef LPTX
  LPTX_idtype *idv;
  LPTX_idtype rn;
  LPTX_particle_set *set;
#endif

#ifdef LPTX
  /**
   * @todo Read particles only in rank 0 for LPTX. Currently, we read particles
   *       in all processes, but discard particles which has been read by ranks
   *       other than the rank 0.
   */
#endif

  //--- input lpt_ctrl
  r = input_binary_lpt_ctrl(prm, dir, &spec->lpt_ctrl, 0, &lpt_data, iout, uni);
  if (r) return r;

  set_lpt_ctrl_data(lpt_data);
  data_npt = lpt_data->npt;
  delete_lpt_ctrl_data(lpt_data);

#ifdef LPT
  inp_npt = cLPTgetnpt();
#else
  inp_npt = data_npt;
#endif

  if (data_npt < 0) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Number of particles in restart data is negative");
    return 1;
  }
  if (data_npt == 0) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
               "No particles stored in data");
    return 0;
  }

#ifdef LPTX
  if (maxcomp < 3)
    maxcomp = 3;
  {
    int ncompo;
    ncompo = component_info_ncompo(&prm->cdo->lpt_mass_fractions);
    if (ncompo > maxcomp)
      maxcomp = ncompo;
  }
#endif

  asz = sizeof(type);
  asz *= maxcomp;
  asz *= data_npt;

  if (data_npt > inp_npt) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
               "Input file specifies only LPT %d particle(s), "
               "but restart data has %d particle(s). "
               "Last %d particle(s) will be discarded.",
               inp_npt, data_npt, data_npt - inp_npt);
  }

#ifdef LPT
  ptidx[0] = 1;
  if (inp_npt < 0) {
    ptidx[1] = 0;
  } else {
    ptidx[1] = data_npt;
  }
#endif

  isz = 0;
  usz = 0;
#ifdef LPTX
  isz = LPTX_PTFLAG_MAX * sizeof(char);
  if (isz < 1 * sizeof(int))
    isz = 1 * sizeof(int);
  isz *= data_npt;

  usz = sizeof(uint64_t);
  usz *= JUPITER_RANDOM_SEED_SIZE;
  usz *= data_npt;
#endif

  v = NULL;
  iv = NULL;
  u64v = NULL;
  vv = NULL;
#ifdef LPTX
  idv = NULL;
  set = NULL;
#endif
  r = 0;
  do {
    v = (type *)malloc(asz);
    if (!v) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                CSV_ERR_NOMEM, 0, 0, NULL);
      r = 1;
      break;
    }

    if (isz > 0) {
      iv = (int *)malloc(isz);
      if (!iv) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                  CSV_ERR_NOMEM, 0, 0, NULL);
        r = 1;
        break;
      }
    }

    if (usz > 0) {
      u64v = (uint64_t *)malloc(usz);
      if (!u64v) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                  CSV_ERR_NOMEM, 0, 0, NULL);
        r = 1;
        break;
      }
    }

#ifdef LPTX
    if (data_npt > 0) {
      idv = (LPTX_idtype *)malloc(sizeof(LPTX_idtype) * data_npt);
      if (!idv) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                  CSV_ERR_NOMEM, 0, 0, NULL);
        r = 1;
        break;
      }
    }

    set = LPTX_param_new_particle_set(val->lpt_param, data_npt);
    if (!set) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      r = 1;
      break;
    }

    vx = v;
    vy = vx + data_npt;
    vz = vy + data_npt;
#else
    vx = v;
    vy = v;
    vz = v;
#endif

    if (maxcomp > 0) {
      vv = (const type **)malloc(sizeof(type *) * maxcomp);
      if (!vv) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        r = 1;
        break;
      }
    }
  } while (0);
  if (for_any_rank(prm->mpi, r)) {
    if (v)
      free(v);
    if (iv)
      free(iv);
    if (u64v)
      free(u64v);
    if (vv)
      free(vv);
    return 1;
  }

  //--- input lpt_ptid
#ifdef LPTX
  r = input_binary_lpt_scalar_ids(prm, dir, &spec->lpt_ptid, 0, data_npt, idv,
                                  iout, uni);
  if (r) goto cleanup;

#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  rn = LPTX_particle_set_set_particle_id(set, 0, idv, data_npt);
  if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_oid
#ifdef LPTX
  r = input_binary_lpt_scalar_ids(prm, dir, &spec->lpt_oid, 0, data_npt, idv,
                                  iout, uni);
  if (r) goto cleanup;

#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  rn = LPTX_particle_set_set_origin_id(set, 0, idv, data_npt);
  if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_xpt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_xpt, 0, data_npt, vx, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_XPT, 2, ptidx, vx);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_ypt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_ypt, 0, data_npt, vy, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_YPT, 2, ptidx, vy);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_zpt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_zpt, 0, data_npt, vz, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_ZPT, 2, ptidx, vz);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

#ifdef LPTX
#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    rn = LPTX_particle_set_set_position_d_soa(set, 0, vx, vy, vz, data_npt);
#else
    rn = LPTX_particle_set_set_position_f_soa(set, 0, vx, vy, vz, data_npt);
#endif
  }
  if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_uxpt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_uxpt, 0, data_npt, vx, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_UXPT, 2, ptidx, vx);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_uypt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_uypt, 0, data_npt, vy, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_UYPT, 2, ptidx, vy);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_uzpt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_uzpt, 0, data_npt, vz, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_UZPT, 2, ptidx, vz);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

#ifdef LPTX
#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    rn = LPTX_particle_set_set_velocity_d_soa(set, 0, vx, vy, vz, data_npt);
#else
    rn = LPTX_particle_set_set_velocity_f_soa(set, 0, vx, vy, vz, data_npt);
#endif
  }
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_fuxpt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_fuxpt, 0, data_npt, vx, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_FUXPT, 2, ptidx, vx);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_fuypt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_fuypt, 0, data_npt, vy, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_FUYPT, 2, ptidx, vy);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_fuzpt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_fuzpt, 0, data_npt, vz, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_FUZPT, 2, ptidx, vz);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

#ifdef LPTX
#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    rn = LPTX_particle_set_set_fuptd_soa(set, 0, vx, vy, vz, data_npt);
#else
    rn = LPTX_particle_set_set_fuptf_soa(set, 0, vx, vy, vz, data_npt);
#endif
  }
  if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_fduxt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_fduxt, 0, data_npt, vx, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_FDUXT, 2, ptidx, vx);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_fduyt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_fduyt, 0, data_npt, vy, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_FDUYT, 2, ptidx, vy);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_fduzt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_fduzt, 0, data_npt, vz, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_FDUZT, 2, ptidx, vz);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

#ifdef LPTX
#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    rn = LPTX_particle_set_set_fdutd_soa(set, 0, vx, vy, vz, data_npt);
#else
    rn = LPTX_particle_set_set_fdutf_soa(set, 0, vx, vy, vz, data_npt);
#endif
  }
  if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
#endif

#ifdef LPTX
  //--- input lpt_dTdt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_dTdt, 0, data_npt, v, iout, uni, flt);
  if (r) goto cleanup;

#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    rn = LPTX_particle_set_set_dTdtd(set, 0, v, data_npt);
#else
    rn = LPTX_paritcle_set_set_dTdtf(set, 0, v, data_npt);
#endif
  }
  if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_diapt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_diapt, 0, data_npt, v, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_DIAPT, 2, ptidx, v);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

#ifdef LPTX
#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    rn = LPTX_particle_set_set_diameter_d(set, 0, v, data_npt);
#else
    rn = LPTX_particle_set_set_diameter_f(set, 0, v, data_npt);
#endif
  }
  if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_timpt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_timpt, 0, data_npt, v, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_TIMPT, 2, ptidx, v);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

#ifdef LPTX
#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    rn = LPTX_particle_set_set_start_time_d(set, 0, v, data_npt);
#else
    rn = LPTX_particle_set_set_start_time_f(set, 0, v, data_npt);
#endif
  }
  if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
#endif

  //--- input lpt_denspt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_denspt, 0, data_npt, v, iout, uni, flt);
  if (r) goto cleanup;

#ifdef LPT
  r = cLPTsetpts_1f(LPT_FP_RHOPT, 2, ptidx, v);
  if (for_any_rank(prm->mpi, r != 0)) { r = 1; goto cleanup; }
#endif

#ifdef LPTX
#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    rn = LPTX_particle_set_set_density_d(set, 0, v, data_npt);
#else
    rn = LPTX_particle_set_set_density_f(set, 0, v, data_npt);
#endif
  }
  if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
#endif

#ifdef LPTX
  //--- input lpt_cppt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_cppt, 0, data_npt, v, iout, uni, flt);
  if (r) goto cleanup;

#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    rn = LPTX_particle_set_set_specific_heat_d(set, 0, v, data_npt);
#else
    rn = LPTX_particle_set_set_specific_heat_f(set, 0, v, data_npt);
#endif
  }
  if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
#endif

#ifdef LPTX
  //--- input lpt_thcpt
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_thcpt, 0, data_npt, v, iout, uni, flt);
  if (r) goto cleanup;

#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    rn = LPTX_particle_set_set_thermal_conductivity_d(set, 0, v, data_npt);
#else
    rn = LPTX_particle_set_set_thermal_conductivity_f(set, 0, v, data_npt);
#endif
  }
  if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
#endif

#ifdef LPTX
  //--- input lpt_seed
  r = input_binary_lpt_scalar_rnds(prm, dir, &spec->lpt_seed, 0, data_npt,
                                   u64v, iout, uni);
  if (r) goto cleanup;

#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  rn = LPTX_particle_set_set_ptseed_i(set, 0, u64v,
                                      data_npt * JUPITER_RANDOM_SEED_SIZE);
  if (for_any_rank(prm->mpi, rn != data_npt * JUPITER_RANDOM_SEED_SIZE)) {
    r = 1;
    goto cleanup;
  }
#endif

#ifdef LPTX
  //--- input lpt_flags
  r = input_binary_lpt_scalar_flgs(prm, dir, &spec->lpt_flags, 0, data_npt,
                                   (char *)iv, iout, uni);
  if (for_any_rank(prm->mpi, r && prm->mpi->rank == 0)) {
    //--- fallback to input lpt_exit
    r = input_binary_lpt_scalar_int(prm, dir, &spec->lpt_exit, 0, data_npt, iv, iout, uni);
    if (r) goto cleanup;

#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
    rn = LPTX_particle_set_set_exited_i(set, 0, iv, data_npt);
    if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
  } else {
#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
    rn = LPTX_particle_set_set_flagsall_c(set, 0, (char *)iv,
                                          data_npt * LPTX_PTFLAG_MAX);
    if (for_any_rank(prm->mpi, rn != data_npt * LPTX_PTFLAG_MAX)) { r = 1; goto cleanup; }
  }
#endif

#ifdef LPTX
  //--- input lpt_parceln
  r = input_binary_lpt_scalar(prm, dir, &spec->lpt_parceln, 0, data_npt, v, iout, uni, flt);
  if (r) goto cleanup;

#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    rn = LPTX_particle_set_set_parceln_d(set, 0, v, data_npt);
#else
    rn = LPTX_particle_set_set_parceln_f(set, 0, v, data_npt);
#endif
  }
  if (for_any_rank(prm->mpi, rn != data_npt)) { r = 1; goto cleanup; }
#endif

#ifdef LPTX
  //--- input lpt_Y
  {
    int ncompo = component_info_ncompo(&prm->cdo->lpt_mass_fractions);
    if (ncompo > 0) {
      for (int i = 0; i < ncompo; ++i) {
        component_data *d;
        type *vp;
        d = component_info_getc(&prm->cdo->lpt_mass_fractions, i);
        vp = &v[i * data_npt];
        r = input_binary_lpt_scalar(prm, dir, &spec->lpt_Y, d->comp_index,
                                    data_npt, vp, iout, uni, flt);
        if (r)
          goto cleanup;
        vv[i] = vp;
      }

#pragma omp parallel if (data_npt > LPTX_omp_small_threshold)
      {
#ifdef JUPITER_DOUBLE
        rn = LPTX_particle_set_set_vector_d_soa(set, 0, LPTX_VI_MASS_FRACTION,
                                                vv, ncompo, data_npt);
#else
        rn = LPTX_particle_set_set_vector_f_soa(set, 0, LPTX_VI_MASS_FRACTION,
                                                vv, ncompo, data_npt);
#endif
      }
      if (for_any_rank(prm->mpi, rn != data_npt)) {
        r = 1;
        goto cleanup;
      }
    }
  }
#endif

#ifdef LPTX
  if (prm->mpi->rank == 0) {
    /*
     * Merge particles for particles by manual injection (LPT_particle_set
     * input)
     */
    LPTX_particle_set *init = NULL;
    LPTX_particle_set *init_inject = NULL;
    LPTX_particle_set *init_others = NULL;
    LPTX_particle_set *data_inject = NULL;
    LPTX_particle_set *data_others = NULL;
    LPTX_particle_set *data_merged = NULL;
    struct lptx_particle_set_merge_data d = {.cdo = prm->cdo};
    int nip = 0;

    do {
      init = LPTX_param_join_particle_sets(val->lpt_param);
      if (!init) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        r = 1;
        break;
      }

      if (!LPTX_particle_set_filter(&init_inject, &init_others, init,
                                    lptx_particle_filter_inject, NULL)) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        r = 1;
        break;
      }

      if (!LPTX_particle_set_filter(&data_inject, &data_others, set,
                                    lptx_particle_filter_inject, NULL)) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        r = 1;
        break;
      }

      if (!LPTX_particle_set_merge(&data_merged, init_inject, data_inject,
                                   lptx_particle_set_merge, &d)) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        r = 1;
        break;
      }

      if (LPTX_particle_set_param(init))
        init = NULL;

      LPTX_param_delete_all_particles(val->lpt_param);
      if (data_merged) {
        if (LPTX_particle_set_number_of_particles(data_merged) > 0) {
          LPTX_particle_set_append(val->lpt_param, data_merged);
          data_merged = NULL;
        }
      }
      if (init_others) {
        if (LPTX_particle_set_number_of_particles(init_others) > 0) {
          LPTX_particle_set_append(val->lpt_param, init_others);
          init_others = NULL;
        }
      }
      if (data_others) {
        if (LPTX_particle_set_number_of_particles(data_others) > 0) {
          LPTX_particle_set_append(val->lpt_param, data_others);
          data_others = NULL;
        }
      }
    } while (0);
    if (init && !LPTX_particle_set_param(init))
      LPTX_particle_set_delete(init);
    if (init_inject)
      LPTX_particle_set_delete(init_inject);
    if (data_inject)
      LPTX_particle_set_delete(data_inject);
    if (init_others)
      LPTX_particle_set_delete(init_others);
    if (data_others)
      LPTX_particle_set_delete(data_others);
    if (data_merged)
      LPTX_particle_set_delete(data_merged);
  } else {
    LPTX_particle_set_delete(set);
    set = NULL;
  }
  if (for_any_rank(prm->mpi, r))
    r = 1;
#endif

cleanup:
#ifdef LPTX
  if (set)
    LPTX_particle_set_delete(set);
  free(idv);
#endif
  free(v);
  free(iv);
  free(u64v);
  free(vv);
  return r;
}

int binary_in(int iout, variable *val, parameter *prm,
              const char *dir, const struct data_output_spec *spec,
              int flt, binary_output_mode uni)
{
  int ic;
  mpi_param *mpi;
  domain *cdo;
  int m;
  int r;
  int bnd_reset;
  init_component not_in_restart;

  CSVASSERT(val);
  CSVASSERT(prm);
  CSVASSERT(prm->mpi);
  CSVASSERT(prm->cdo);

  mpi = prm->mpi;
  cdo = prm->cdo;
  m = cdo->m;

#ifdef JUPITER_DOUBLE
  if (for_any_rank(mpi, flt)) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
               "Cannot restart from single binary");
    return 1;
  }
#endif

#ifdef JUPITER_MPI
  r = init_component_mpi_bcast(&prm->flg->rebuild_components, 0,
                               prm->mpi->CommJUPITER);
  if (r != MPI_SUCCESS) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI, 0, r,
              NULL);
    return 1;
  }
#endif

  /* YSE: Flags for resetting boundary, heat_source etc from geometry_in(). */
  init_component_copy(&not_in_restart, &prm->flg->rebuild_components);

  /* YSE: input binary formed boundary data. */
  if (!(init_component_is_set(&not_in_restart, INIT_COMPONENT_BOUNDARY) &&
        init_component_is_set(&not_in_restart,
                              INIT_COMPONENT_THERMAL_BOUNDARY))) {
    r = input_boundary(prm, dir, &spec->bnd, val, iout, uni);
#ifdef JUPITER_MPI
    MPI_Allreduce(MPI_IN_PLACE, &r, 1, MPI_INT, MPI_MAX, mpi->CommJUPITER);
#endif
    bnd_reset = 0;
    if (r == 2) return r;
    if (r == 1) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
                 "Resetting boundaries with given data by input files");
      init_component_set(&not_in_restart, INIT_COMPONENT_BOUNDARY);
      init_component_set(&not_in_restart, INIT_COMPONENT_THERMAL_BOUNDARY);
    } else {
      if (prm->flg->debug == ON) {
        dumb_visualize_boundary(prm, cdo, &val->bnd_W, cdo->nby, cdo->nbz,
                                "y", "z", "West boundary");
        dumb_visualize_boundary(prm, cdo, &val->bnd_E, cdo->nby, cdo->nbz,
                                "y", "z", "East boundary");
        dumb_visualize_boundary(prm, cdo, &val->bnd_S, cdo->nbx, cdo->nbz,
                                "x", "z", "South boundary");
        dumb_visualize_boundary(prm, cdo, &val->bnd_N, cdo->nbx, cdo->nbz,
                                "x", "z", "North boundary");
        dumb_visualize_boundary(prm, cdo, &val->bnd_B, cdo->nbx, cdo->nby,
                                "x", "y", "Bottom boundary");
        dumb_visualize_boundary(prm, cdo, &val->bnd_T, cdo->nbx, cdo->nby,
                                "x", "y", "Top boundary");
      }
    }
  }

  //--- input qgeom
  if (!init_component_is_set(&not_in_restart, INIT_COMPONENT_FIXED_HSOURCE)) {
    r = input_binary_scalar(prm, dir, &spec->qgeom, 0, val->qgeom, iout, uni, flt);
    if (r == 1) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_INFO, NULL,
                 "Resetting input heat source given by geometry input file");
      init_component_set(&not_in_restart,
                         INIT_COMPONENT_FIXED_HSOURCE);
    } else if (r) {
      return r;
    }
  }

  if (prm->flg->solute_diff != ON) {
    for (ic = 0; ic < cdo->NBaseComponent; ++ic) {
      //--- input fs
      r = input_binary_scalar(prm, dir, &spec->fs, ic, &val->fs[ic*m], iout, uni, flt);
      if (r) return r;
      //--- input fl
      r = input_binary_scalar(prm, dir, &spec->fl, ic, &val->fl[ic*m], iout, uni, flt);
      if (r) return r;

      if (prm->flg->phase_change == ON) {
        if (val->df && prm->flg->melting == ON) {
          //--- input df
          r = input_binary_scalar(prm, dir, &spec->df, ic, &val->df[ic*m], iout, uni, flt);
          if (r) return r;
        }
        if (val->dfs && prm->flg->solidification == ON) {
          //--- input dfs
          r = input_binary_scalar(prm, dir, &spec->dfs, ic, &val->dfs[ic*m], iout, uni, flt);
          if (r) return r;
        }
      }
    }

    if(prm->flg->multi_layer == ON){

      for (int ilayer = 0; ilayer < cdo->NumberOfLayer; ilayer++) {
        //--- input fl_layer
        r = input_binary_scalar(prm, dir, &spec->fl_layer, ilayer, &val->fl_layer[ilayer*m], iout, uni, flt);
        if (r) return r;
        //--- input label_layer
        r = input_binary_int(prm, dir, &spec->label_layer, ilayer, &val->label_layer[ilayer*m], iout, uni);
        if (r) return r;
      }

    }

  } else {
    for (ic = 0; ic < cdo->NumberOfComponent; ++ic) {
      //--- input Y
      r = input_binary_scalar(prm, dir, &spec->Y, ic, &val->Y[ic*m], iout, uni, flt);
      if (r) return r;
    }
    for (ic = 0; ic < cdo->NBaseComponent; ++ic) {
      //--- input Vf
      r = input_binary_scalar(prm, dir, &spec->Vf, ic, &val->Vf[ic*m], iout, uni, flt);
      if (r) return r;
    }
    //--- input fs
    r = input_binary_scalar(prm, dir, &spec->fs, 0, val->fs, iout, uni, flt);
    if (r) return r;
    //--- input fl
    r = input_binary_scalar(prm, dir, &spec->fl, 0, val->fl, iout, uni, flt);
    if (r) return r;

    if (prm->flg->phase_change == ON) {
      if (val->df && prm->flg->melting == ON) {
        //--- input df
        r = input_binary_scalar(prm, dir, &spec->df, 0, val->df, iout, uni, flt);
        if (r) return r;
      }
      if (val->dfs && prm->flg->solidification == ON) {
        //--- input dfs
        r = input_binary_scalar(prm, dir, &spec->dfs, 0, val->dfs, iout, uni, flt);
        if (r) return r;
      }
    }
  }

  //--- input ll
  r = input_binary_scalar(prm, dir, &spec->ll, 0, val->ll, iout, uni, flt);
  if (r) return r;
  //--- input ls
  r = input_binary_scalar(prm, dir, &spec->ls, 0, val->ls, iout, uni, flt);
  if (r) return r;
  //--- input lls
  r = input_binary_scalar(prm, dir, &spec->lls, 0, val->lls, iout, uni, flt);
  if (r) return r;

  //--- input t
  r = input_binary_scalar(prm, dir, &spec->t, 0, val->t, iout, uni, flt);
  if (r) return r;

  //--- input u
  r = input_binary_scalar(prm, dir, &spec->u, 0, val->u, iout, uni, flt);
  if (r) return r;
  //--- input v
  r = input_binary_scalar(prm, dir, &spec->v, 0, val->v, iout, uni, flt);
  if (r) return r;
  //--- input w
  r = input_binary_scalar(prm, dir, &spec->w, 0, val->w, iout, uni, flt);
  if (r) return r;
  //--- input p
  r = input_binary_scalar(prm, dir, &spec->p, 0, val->p, iout, uni, flt);
  if (r) return r;

  if (prm->flg->phase_change == ON) {
    if (val->entha) {
      //--- input entha
      r = input_binary_scalar(prm, dir, &spec->entha, 0, val->entha, iout, uni, flt);
      if (r) return r;
    }

    if (val->mushy) {
      //--- input mushy
      r = input_binary_int(prm, dir, &spec->mushy, 0, val->mushy, iout, uni);
      if (r) return r;
    }
  }

  // 2020/06/22
  if (prm->flg->porous == ON) {
    //--- input eps
    r = input_binary_scalar(prm, dir, &spec->eps, 0, val->eps, iout, uni, flt);
    if (r) return r;
    //--- input perm
    r = input_binary_scalar(prm, dir, &spec->perm, 0, val->perm, iout, uni, flt);
    if (r) return r;
    if (prm->flg->two_energy == ON) {
      //--- input epss
      r = input_binary_scalar(prm, dir, &spec->epss, 0, val->epss, iout, uni, flt);
      if (r) return r;
      //--- input tf
      r = input_binary_scalar(prm, dir, &spec->tf, 0, val->tf, iout, uni, flt);
      if (r) return r;
      //--- input ts
      r = input_binary_scalar(prm, dir, &spec->ts, 0, val->ts, iout, uni, flt);
      if (r) return r;
    }
  }

  if (prm->flg->oxidation == ON) {
    //--- input ox_dt
    r = input_binary_scalar(prm, dir, &spec->ox_dt, 0, val->ox_dt, iout, uni, flt);
    if (r) return r;
    //--- input ox_dt_local
    r = input_binary_scalar(prm, dir, &spec->ox_dt_local, 0, val->ox_dt_local, iout, uni, flt);
    if (r) return r;
    //--- input ox_flag
    r = input_binary_int(prm, dir, &spec->ox_flag, 0, val->ox_flag, iout, uni);
    if (r) return r;
    //--- input ox_lset
    r = input_binary_scalar(prm, dir, &spec->ox_lset, 0, val->ox_lset, iout, uni, flt);
    if (r) return r;
    //--- input ox_vof
    r = input_binary_scalar(prm, dir, &spec->ox_vof, 0, val->ox_vof, iout, uni, flt);
    if (r) return r;
    //--- input ox_h2
    if (prm->flg->solute_diff == ON && spec->ox_h2.outf == ON) {
      r = input_binary_scalar(prm, dir, &spec->ox_h2, 0, val->ox_h2, iout, uni, flt);
      if (r) return r;
    }
    //--- input ox_lset_h2o
    r = input_binary_scalar(prm, dir, &spec->ox_lset_h2o, 0, val->ox_lset_h2o, iout, uni, flt);
    if (r) return r;
    //--- input ox_lset_h2o_s
    r = input_binary_scalar(prm, dir, &spec->ox_lset_h2o_s, 0, val->ox_lset_h2o_s, iout, uni, flt);
    if (r) return r;
  }

  if (prm->flg->lpt_calc == ON) {
    r = binary_in_lpt(iout, prm, val, dir, spec, flt, uni);
    if (r) return r;

    if (!init_component_is_set(&not_in_restart, INIT_COMPONENT_LPT_PEWALL_N)) {
      r = input_binary_scalar_nb(prm, dir, &spec->lpt_ewall, 0, val->lpt_pewall,
                                 iout, uni, flt, 1);
      if (r == 1) {
        init_component_set(&not_in_restart, INIT_COMPONENT_LPT_PEWALL_N);
      } else if (r) {
        return r;
      }
    }
  }

  if (init_component_any(&not_in_restart)) {
    if (prm->flg->geom_in == ON) {
      r = geometry_in_with(val, prm, not_in_restart,
                           &prm->flg->rebuild_components, prm->geom_sets);
      if (r) return r;
    }
    if (init_component_is_set(&not_in_restart, INIT_COMPONENT_BOUNDARY) ||
        init_component_is_set(&not_in_restart,
                              INIT_COMPONENT_THERMAL_BOUNDARY) ||
        init_component_is_set(&prm->flg->rebuild_components,
                              INIT_COMPONENT_BOUNDARY) ||
        init_component_is_set(&prm->flg->rebuild_components,
                              INIT_COMPONENT_THERMAL_BOUNDARY)) {
      init_boundary(prm->mpi, prm->cdo, prm->flg, val);
    }
  }

  return 0;
}

static int
output_binary_scalar_h2_absorp(mpi_param *mpi, parameter *prm,
                               const char *directory,
                               const struct data_spec *spec, type *val,
                               int iout, binary_output_mode unified,
                               int use_float)
{
  int r;
  domain *cdo;
  char *path;

  CSVASSERT(mpi);
  CSVASSERT(prm);
  CSVASSERT(prm->mpi);
  CSVASSERT(prm->cdo);
  CSVASSERT(spec);

  cdo = prm->cdo;
  r = make_file_name(&path, directory, spec->filename_template, spec->name,
                     -1, iout, prm->mpi->rank, unified);
  if (r) {
    path = NULL;
  }
  if (for_any_rank(mpi, r)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    free(path);
    return 1;
  }

  r = output_binary(mpi, val, 0, 0, 0, 0, 0, 0, 1, 1, cdo->nz, 1,
                    path, unified, use_float);
  free(path);
  return r;
}

static int binary_out_base_h2_absorp(int iout, parameter *prm, const char *dir,
                                     binary_output_mode uni, int flt,
                                     variable *val,
                                     const struct data_output_spec *spec)
{
  int r;
  int inited;
  mpi_param z_mpi;
  inited = 0;
  r = 0;

  if (spec->h2_absorp_eval.outf == ON) {
    if (!inited) {
      set_z_comm_mpi(&z_mpi, prm->mpi);
      inited = 1;
    }
    if (prm->mpi->rank_xy == 0) {
      r = output_binary_scalar_h2_absorp(&z_mpi, prm, dir, &spec->h2_absorp_eval, val->h2_absorp_eval, iout, uni, flt);
    }
    if (for_any_rank(prm->mpi, r)) {
      r = 1;
      goto clean;
    }
  }

  if (spec->h2_absorp_Ks.outf == ON) {
    if (!inited) {
      set_z_comm_mpi(&z_mpi, prm->mpi);
      inited = 1;
    }
    if (prm->mpi->rank_xy == 0) {
      r = output_binary_scalar_h2_absorp(&z_mpi, prm, dir, &spec->h2_absorp_Ks,
                                         val->h2_absorp_Ks, iout, uni, flt);
    }
    if (for_any_rank(prm->mpi, r)) {
      r = 1;
      goto clean;
    }
  }

  if (spec->h2_absorp_P.outf == ON) {
    if (!inited) {
      set_z_comm_mpi(&z_mpi, prm->mpi);
      inited = 1;
    }
    if (prm->mpi->rank_xy == 0) {
      r = output_binary_scalar_h2_absorp(&z_mpi, prm, dir, &spec->h2_absorp_P,
                                         val->h2_absorp_P, iout, uni, flt);
    }
    if (for_any_rank(prm->mpi, r)) {
      r = 1;
      goto clean;
    }
  }

  if (spec->h2_absorp_T.outf == ON) {
    if (!inited) {
      set_z_comm_mpi(&z_mpi, prm->mpi);
      inited = 1;
    }
    if (prm->mpi->rank_xy == 0) {
      r = output_binary_scalar_h2_absorp(&z_mpi, prm, dir, &spec->h2_absorp_T,
                                         val->h2_absorp_T, iout, uni, flt);
    }
    if (for_any_rank(prm->mpi, r)) {
      r = 1;
      goto clean;
    }
  }

  if (spec->h2_absorp_Zr.outf == ON) {
    if (!inited) {
      set_z_comm_mpi(&z_mpi, prm->mpi);
      inited = 1;
    }
    if (prm->mpi->rank_xy == 0) {
      r = output_binary_scalar_h2_absorp(&z_mpi, prm, dir, &spec->h2_absorp_Zr,
                                         val->h2_absorp_Zr, iout, uni, flt);
    }
    if (for_any_rank(prm->mpi, r)) {
      r = 1;
      goto clean;
    }
  }

clean:
  if (inited) {
    free_z_comm_mpi(&z_mpi);
  }
  return r;
}

#ifdef LPTX
static int lptx_vecset(type *base, type **var_x, type **var_y, type **var_z,
                       int npt)
{
  if (npt <= 0)
    return 0;
  if (*var_x)
    return 0;

  *var_x = base;
  *var_y = base + npt;
  *var_z = base + 2 * npt;
  return 1;
}

static int lptx_get_pospt(type *base, type **var_x, type **var_y, type **var_z,
                          int npt, LPTX_param *param)
{
  if (!lptx_vecset(base, var_x, var_y, var_z, npt))
    return 0;

#pragma omp parallel if (npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    LPTX_get_particle_position_d_soa(param, 0, *var_x, *var_y, *var_z, npt);
#else
    LPTX_get_particle_position_f_soa(param, 0, *var_x, *var_y, *var_z, npt);
#endif
  }
  return 0;
}

static int lptx_get_upt(type *base, type **var_x, type **var_y, type **var_z,
                        int npt, LPTX_param *param)
{
  if (!lptx_vecset(base, var_x, var_y, var_z, npt))
    return 0;

#pragma omp parallel if (npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    LPTX_get_particle_velocity_d_soa(param, 0, *var_x, *var_y, *var_z, npt);
#else
    LPTX_get_particle_velocity_f_soa(param, 0, *var_x, *var_y, *var_z, npt);
#endif
  }
  return 0;
}

static int lptx_get_fupt(type *base, type **var_x, type **var_y, type **var_z,
                        int npt, LPTX_param *param)
{
  if (!lptx_vecset(base, var_x, var_y, var_z, npt))
    return 0;

#pragma omp parallel if (npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    LPTX_get_particle_fuptd_soa(param, 0, *var_x, *var_y, *var_z, npt);
#else
    LPTX_get_particle_fuptf_soa(param, 0, *var_x, *var_y, *var_z, npt);
#endif
  }
  return 0;
}

static int lptx_get_fdut(type *base, type **var_x, type **var_y, type **var_z,
                         int npt, LPTX_param *param)
{
  if (!lptx_vecset(base, var_x, var_y, var_z, npt))
    return 0;

#pragma omp parallel if (npt > LPTX_omp_small_threshold)
  {
#ifdef JUPITER_DOUBLE
    LPTX_get_particle_fdutd_soa(param, 0, *var_x, *var_y, *var_z, npt);
#else
    LPTX_get_particle_fdutf_soa(param, 0, *var_x, *var_y, *var_z, npt);
#endif
  }
  return 0;
}
#endif

static int binary_out_base_lpt(int iout, parameter *prm, variable *val,
                               const char *dir, binary_output_mode uni, int flt,
                               const struct data_output_spec *spec)
{
  int r;
  ptrdiff_t maxcomp; /* Maximum number of components including vector */
  ptrdiff_t vecncomp; /* Maximum number of flexible componets */
  jupiter_lpt_ctrl_data *lpt_ctrl_data;
  type *lpt_val, *lpt_val_x, *lpt_val_y, *lpt_val_z;
  type **lpt_valv;
  int *lpt_ival;
  uint64_t *lpt_u64val;
  ptrdiff_t npt;
  ptrdiff_t asz;
  ptrdiff_t isz;
  ptrdiff_t usz;
#ifdef LPT
  size_t lpt_all[2];
#endif
#ifdef LPTX
  LPTX_idtype *lpt_idval;
  LPTX_idtype number_of_vectors;
  LPTX_idtype *number_of_components; /* Number of components for each vectors */
#endif
  const char *variable_failed_to_obtain = NULL;

  vecncomp = 0;
  maxcomp = 3;
  r = 0;
  lpt_val = NULL;
  lpt_ival = NULL;
  lpt_u64val = NULL;
  lpt_valv = NULL;
#ifdef LPTX
  lpt_idval = NULL;
  number_of_components = NULL;
#endif

#ifdef LPT
  lpt_ctrl_data = get_lpt_ctrl_data();
#else
  lpt_ctrl_data = new_lpt_ctrl_data();
#endif
  if (for_any_rank(prm->mpi, !lpt_ctrl_data)) {
    if (!lpt_ctrl_data) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM,
                0, 0, NULL);
    }
    r = 1;
    goto fin;
  }

#ifdef LPTX
  if (val->lpt_param) {
    LPTX_particle_set *setp;
    LPTX_idtype nnpt;

#ifdef JUPITER_MPI
    r = LPTX_param_gather_particles(val->lpt_param, 0);
    if (for_any_rank(prm->mpi, r != MPI_SUCCESS)) {
      if (r != MPI_SUCCESS)
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI, 0, r,
                  NULL);
      r = 1;
      goto fin;
    }
#endif

    if (prm->mpi->rank == 0) {
      nnpt = LPTX_param_get_total_particles(val->lpt_param);
    } else {
      nnpt = 0;
    }

    lpt_ctrl_data->npt = nnpt;
    if (for_any_rank(prm->mpi, lpt_ctrl_data->npt != nnpt)) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Too many (overflowing) number of particles will be written");
      r = 1;
      goto fin;
    }

    number_of_vectors =
      LPTX_param_get_allocated_particle_vectors(val->lpt_param);
    if (number_of_vectors > 0) {
      number_of_components =
        (LPTX_idtype *)malloc(sizeof(LPTX_idtype) * number_of_vectors);
      if (!number_of_components) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        r = 1;
        goto fin;
      }

      LPTX_param_get_allocated_particle_vector_sizes(number_of_components,
                                                     number_of_vectors,
                                                     val->lpt_param);
      for (LPTX_idtype iv = 0; iv < number_of_vectors; ++iv) {
        if (number_of_components[iv] > vecncomp)
          vecncomp = number_of_components[iv];
      }
    }
    if (vecncomp > maxcomp)
      maxcomp = vecncomp;

    if (vecncomp > 0) {
      lpt_valv = (type **)malloc(sizeof(type *) * vecncomp);
      if (!lpt_valv) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        r = 1;
        goto fin;
      }
    }
  }
#endif

  npt = lpt_ctrl_data->npt;

#ifdef LPT
  /* cLPTgetpts function takes indices in Fortran-manner (i.e., 1-origin) */
  lpt_all[0] = 1;
  lpt_all[1] = npt;
#endif
  asz = maxcomp * npt * sizeof(type);
  isz = 0;
  usz = 0;
#ifdef LPTX
  if (sizeof(int) * 1 < sizeof(char) * LPTX_PTFLAG_MAX){
    isz = sizeof(char) * LPTX_PTFLAG_MAX;
  } else {
    isz = sizeof(int) * 1;
  }
  isz *= npt;
  usz = sizeof(uint64_t) * JUPITER_RANDOM_SEED_SIZE * npt;
#endif

  {
    int iovf;
    iovf = (asz < 0 || (asz / maxcomp / sizeof(type) != npt));
    if (for_any_rank(prm->mpi, iovf)) {
      if (iovf) {
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                   "Overflow detected");
      }
      r = 1;
      goto fin;
    }
  }
  if (asz == 0) {
    asz = ((maxcomp > 0) ? maxcomp : 3) * sizeof(type);
  }
  lpt_val = (type *)malloc(asz);
  if (for_any_rank(prm->mpi, !lpt_val)) {
    if (!lpt_val) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM,
                0, 0, NULL);
    }
    r = 1;
    goto fin;
  }

  lpt_ival = NULL;
  if (isz > 0)
    lpt_ival = (int *)malloc(isz);
  if (for_any_rank(prm->mpi, isz > 0 && !lpt_ival)) {
    if (!lpt_ival) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
    }
    r = 1;
    goto fin;
  }

  lpt_u64val = NULL;
  if (usz > 0)
    lpt_u64val = (uint64_t *)malloc(usz);
  if (for_any_rank(prm->mpi, usz > 0 && !lpt_u64val)) {
    if (!lpt_u64val)
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM,
                0, 0, NULL);
    r = 1;
    goto fin;
  }

#ifdef LPTX
  lpt_idval = NULL;
  if (npt > 0)
    lpt_idval = (LPTX_idtype *)malloc(sizeof(LPTX_idtype) * npt);
  if (for_any_rank(prm->mpi, npt > 0 && !lpt_idval)) {
    if (!lpt_idval)
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM,
                0, 0, NULL);
    r = 1;
    goto fin;
  }
#endif

  //--- output lpt_ctrl
  if (spec->lpt_ctrl.outf == ON) {
    r = output_binary_lpt_ctrl(prm, dir, &spec->lpt_ctrl, 0, lpt_ctrl_data, iout, uni);
    if (for_any_rank(prm->mpi, r)) goto fin;
  }

  //--- output lpt_ptid
#ifdef LPTX
  if (spec->lpt_ptid.outf == ON) {
    LPTX_get_particle_particle_id(val->lpt_param, 0, lpt_idval, npt);

    r = output_binary_lpt_scalar_ids(prm, dir, &spec->lpt_ptid, 0, npt,
                                     lpt_idval, iout, uni);
    if (r) goto fin;
  }
#endif

  //--- output lpt_oid
#ifdef LPTX
  if (spec->lpt_oid.outf == ON) {
    LPTX_get_particle_origin_id(val->lpt_param, 0, lpt_idval, npt);

    r = output_binary_lpt_scalar_ids(prm, dir, &spec->lpt_oid, 0, npt,
                                     lpt_idval, iout, uni);
    if (r) goto fin;
  }
#endif

  lpt_val_x = NULL;
  lpt_val_y = NULL;
  lpt_val_z = NULL;

  //--- output lpt_xpt
  if (spec->lpt_xpt.outf == ON) {
#ifdef LPT
    lpt_val_x = lpt_val;
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_XPT, 2, lpt_all, lpt_val_x);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_xpt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_pospt(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                   val->lpt_param);
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_xpt, 0, npt,
                                 lpt_val_x, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_ypt
  if (spec->lpt_ypt.outf == ON) {
#ifdef LPT
    lpt_val_y = lpt_val + npt;
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_YPT, 2, lpt_all, lpt_val_y);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_ypt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_pospt(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                   val->lpt_param);
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_ypt, 0, npt,
                                 lpt_val_y, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_zpt
  if (spec->lpt_zpt.outf == ON) {
#ifdef LPT
    lpt_val_z = lpt_val + 2 * npt;
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_ZPT, 2, lpt_all, lpt_val_z);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_zpt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_pospt(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                   val->lpt_param);
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_zpt, 0, npt,
                                 lpt_val_z, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_pospt
  if (spec->lpt_pospt.outf == ON) {
#ifdef LPT
    if (npt > 0) {
      r = cLPTgetpts_3f(LPT_FP_XPT, LPT_FP_YPT, LPT_FP_ZPT, 2, lpt_all,
                        lpt_val_x ? NULL : (lpt_val_x = lpt_val),
                        lpt_val_y ? NULL : (lpt_val_y = lpt_val + npt),
                        lpt_val_z ? NULL : (lpt_val_z = lpt_val + 2 * npt));
    } else {
      lpt_val_x = lpt_val_y = lpt_val_z = lpt_val;
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_xpt, lpt_ypt or lpt_zpt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_pospt(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                   val->lpt_param);
#endif

    r = output_binary_lpt_vector(prm, dir, &spec->lpt_pospt, 0, npt,
                                 lpt_val_x, lpt_val_y, lpt_val_z,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  lpt_val_x = lpt_val_y = lpt_val_z = NULL;

  //--- output lpt_uxpt
  if (spec->lpt_uxpt.outf == ON) {
#ifdef LPT
    lpt_val_x = lpt_val;
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_UXPT, 2, lpt_all, lpt_val_x);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_uxpt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_upt(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                 val->lpt_param);
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_uxpt, 0, npt, lpt_val_x,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_uypt
  if (spec->lpt_uypt.outf == ON) {
#ifdef LPT
    lpt_val_y = lpt_val + npt;
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_UYPT, 2, lpt_all, lpt_val_y);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_uypt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_upt(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                 val->lpt_param);
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_uypt, 0, npt, lpt_val_y,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_uzpt
  if (spec->lpt_uzpt.outf == ON) {
#ifdef LPT
    lpt_val_z = lpt_val + 2 * npt;
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_UZPT, 2, lpt_all, lpt_val_z);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_uzpt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_upt(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                 val->lpt_param);
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_uzpt, 0, npt, lpt_val_z,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_upt
  if (spec->lpt_upt.outf == ON) {
#ifdef LPT
    if (npt > 0) {
      r = cLPTgetpts_3f(LPT_FP_UXPT, LPT_FP_UYPT, LPT_FP_UZPT, 2, lpt_all,
                        lpt_val_x ? NULL : (lpt_val_x = lpt_val),
                        lpt_val_y ? NULL : (lpt_val_y = lpt_val + npt),
                        lpt_val_z ? NULL : (lpt_val_z = lpt_val + 2 * npt));
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_uxpt, lpt_uypt, and/or lpt_uzpt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_upt(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                 val->lpt_param);
#endif

    r = output_binary_lpt_vector(prm, dir, &spec->lpt_upt, 0, npt,
                                 lpt_val_x, lpt_val_y, lpt_val_z, iout,
                                 uni, flt);
    if (r) goto fin;
  }

  lpt_val_x = lpt_val_y = lpt_val_z = NULL;

  //--- output lpt_uf
  if (spec->lpt_uf.outf == ON) {
    lpt_val_x = lpt_val;
    lpt_val_y = lpt_val_x + npt;
    lpt_val_z = lpt_val_y + npt;
#ifdef LPT
    if (npt > 0) {
      r = cLPTgetpts_3f(LPT_FP_UXF, LPT_FP_UYF, LPT_FP_UZF, 2, lpt_all,
                        lpt_val_x, lpt_val_y, lpt_val_z);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_uxf, lpt_uyf, and/or lpt_uzf";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_vecset(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt);
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_fluid_velocity_d_soa(val->lpt_param, 0, lpt_val_x,
                                             lpt_val_y, lpt_val_z, npt);
#else
      LPTX_get_particle_fluid_velocity_f_soa(val->lpt_param, 0, lpt_val_x,
                                             lpt_val_y, lpt_val_z, npt);
#endif
    }
#endif

    r = output_binary_lpt_vector(prm, dir, &spec->lpt_uf, 0, npt, lpt_val_x,
                                 lpt_val_y, lpt_val_z, iout, uni, flt);
    if (r) goto fin;
  }

  lpt_val_x = lpt_val_y = lpt_val_z = NULL;

  //--- output lpt_timpt
  if (spec->lpt_timpt.outf == ON) {
#ifdef LPT
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_TIMPT, 2, lpt_all, lpt_val);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_timpt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_start_time_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_start_time_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_timpt, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output_lpt_muf
  if (spec->lpt_muf.outf == ON) {
    /* LPT interface does not support getting fluid mu (yet) */
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_fluid_viscosity_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_fluid_viscosity_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_muf, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_densf
  if (spec->lpt_densf.outf == ON) {
    /* LPT interface does not support getting fluid density (yet) */
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_fluid_density_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_fluid_density_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_densf, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_cpf
  if (spec->lpt_cpf.outf == ON) {
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_fluid_specific_heat_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_fluid_specific_heat_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_cpf, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_thcf
  if (spec->lpt_thcf.outf == ON) {
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_fluid_thermal_conductivity_d(val->lpt_param, 0, lpt_val,
                                                     npt);
#else
      LPTX_get_particle_fluid_thermal_conductivity_f(val->lpt_param, 0, lpt_val,
                                                     npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_thcf, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_tempf
  if (spec->lpt_tempf.outf == ON) {
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_fluid_temperature_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_fluid_temperature_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_tempf, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  lpt_val_x = lpt_val_y = lpt_val_z = NULL;

  //--- output lpt_gradTf
  if (spec->lpt_gradTf.outf == ON) {
#ifdef LPTX
    lptx_vecset(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt);
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_fluid_temperature_grad_d_soa(val->lpt_param, 0,
                                                     lpt_val_x, lpt_val_y,
                                                     lpt_val_z, npt);
#else
      LPTX_get_particle_fluid_temperature_grad_d_soa(val->lpt_param, 0,
                                                     lpt_val_x, lpt_val_y,
                                                     lpt_val_z, npt);
#endif
    }
#endif

    r = output_binary_lpt_vector(prm, dir, &spec->lpt_gradTf, 0, npt, lpt_val_x,
                                 lpt_val_y, lpt_val_z, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_pathf
  if (spec->lpt_pathf.outf == ON) {
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_mean_free_path_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_mean_free_path_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_pathf, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_denspt
  if (spec->lpt_denspt.outf == ON) {
#ifdef LPT
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_RHOPT, 2, lpt_all, lpt_val);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_denspt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_density_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_density_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_denspt, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_cppt
  if (spec->lpt_cppt.outf == ON) {
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_specific_heat_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_specific_heat_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_cppt, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_thcpt
  if (spec->lpt_thcpt.outf == ON) {
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_thermal_conductivity_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_thermal_conductivity_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_thcpt, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_htrpt
  if (spec->lpt_htrpt.outf == ON) {
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_heat_transfer_rate_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_heat_transfer_rate_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_htrpt, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_tothtpt
  if (spec->lpt_tothtpt.outf == ON) {
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_total_heat_transfer_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_total_heat_transfer_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_tothtpt, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_diapt
  if (spec->lpt_diapt.outf == ON) {
#ifdef LPT
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_DIAPT, 2, lpt_all, lpt_val);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_diapt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_diameter_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_diameter_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_diapt, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_temppt
  if (spec->lpt_temppt.outf == ON) {
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_temperature_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_temperature_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_temppt, 0, npt, lpt_val,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  lpt_val_x = lpt_val_y = lpt_val_z = NULL;

  //--- output lpt_inipospt
  if (spec->lpt_inipospt.outf == ON) {
#ifdef LPTX
    lptx_vecset(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt);
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_init_position_d_soa(val->lpt_param, 0, lpt_val_x,
                                            lpt_val_y, lpt_val_z, npt);
#else
      LPTX_get_particle_init_position_f_soa(val->lpt_param, 0, lpt_val_x,
                                            lpt_val_y, lpt_val_z, npt);
#endif
    }
#endif

    r = output_binary_lpt_vector(prm, dir, &spec->lpt_inipospt, 0, npt,
                                 lpt_val_x, lpt_val_y, lpt_val_z, iout, uni,
                                 flt);
    if (r) goto fin;
  }

  lpt_val_x = lpt_val_y = lpt_val_z = NULL;

  //--- output lpt_iniupt
  if (spec->lpt_iniupt.outf == ON) {
#ifdef LPTX
    lptx_vecset(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt);
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_init_velocity_d_soa(val->lpt_param, 0, lpt_val_x,
                                            lpt_val_y, lpt_val_z, npt);
#else
      LPTX_get_particle_init_velocity_f_soa(val->lpt_param, 0, lpt_val_x,
                                            lpt_val_y, lpt_val_z, npt);
#endif
    }
#endif

    r = output_binary_lpt_vector(prm, dir, &spec->lpt_iniupt, 0, npt,
                                 lpt_val_x, lpt_val_y, lpt_val_z, iout, uni,
                                 flt);
    if (r) goto fin;
  }

  //--- output lpt_initemppt
  if (spec->lpt_initemppt.outf == ON) {
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_init_temperature_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_init_temperature_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_initemppt, 0, npt,
                                 lpt_val, iout, uni, flt);
    if (r) goto fin;
  }

#ifdef LPTX
  //--- output lpt_seed
  if (spec->lpt_seed.outf == ON) {
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    LPTX_get_particle_ptseed_i(val->lpt_param, 0, lpt_u64val,
                               npt * JUPITER_RANDOM_SEED_SIZE);

    r = output_binary_lpt_scalar_rnds(prm, dir, &spec->lpt_seed, 0, npt,
                                      lpt_u64val, iout, uni);
    if (r) goto fin;
  }
#else
  CSVASSERT(spec->lpt_seed.outf != ON);
#endif

#ifdef LPTX
  //--- output lpt_exit
  if (spec->lpt_exit.outf == ON) {
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    LPTX_get_particle_exited_i(val->lpt_param, 0, lpt_ival, npt);

    r = output_binary_lpt_scalar_int(prm, dir, &spec->lpt_exit, 0, npt,
                                     lpt_ival, iout, uni);
    if (r) goto fin;
  }
#else
  CSVASSERT(spec->lpt_seed.outf != ON);
#endif

#ifdef LPTX
  //--- output lpt_flags
  if (spec->lpt_flags.outf == ON) {
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    LPTX_get_particle_flagsall_c(val->lpt_param, 0, (char *)lpt_ival,
                                 npt * LPTX_PTFLAG_MAX);

    r = output_binary_lpt_scalar_flgs(prm, dir, &spec->lpt_flags, 0, npt,
                                      (char *)lpt_ival, iout, uni);
    if (r) goto fin;
  }
#else
  CSVASSERT(spec->lpt_flags.outf != ON);
#endif

  //--- output lpt_parceln
  if (spec->lpt_parceln.outf == ON) {
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_parceln_d(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_parceln_f(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_parceln, 0, npt,
                                 lpt_val, iout, uni, flt);
    if (r) goto fin;
  }

  lpt_val_x = lpt_val_y = lpt_val_z = NULL;

  //--- output lpt_fbpt
  if (spec->lpt_fbpt.outf == ON) {
#ifdef LPTX
    lptx_vecset(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt);
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_fbptd_soa(val->lpt_param, 0, lpt_val_x, lpt_val_y,
                                  lpt_val_z, npt);
#else
      LPTX_get_particle_fbptf_soa(val->lpt_param, 0, lpt_val_x, lpt_val_y,
                                  lpt_val_z, npt);
#endif
    }
#endif

    r = output_binary_lpt_vector(prm, dir, &spec->lpt_fbpt, 0, npt, lpt_val_x,
                                 lpt_val_y, lpt_val_z, iout, uni, flt);
    if (r)
      goto fin;
  }

  lpt_val_x = lpt_val_y = lpt_val_z = NULL;

  //--- output lpt_fTpt
  if (spec->lpt_fTpt.outf == ON) {
#ifdef LPTX
    lptx_vecset(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt);
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_fTptd_soa(val->lpt_param, 0, lpt_val_x, lpt_val_y,
                                  lpt_val_z, npt);
#else
      LPTX_get_particle_fTptf_soa(val->lpt_param, 0, lpt_val_x, lpt_val_y,
                                  lpt_val_z, npt);
#endif
    }
#endif

    r = output_binary_lpt_vector(prm, dir, &spec->lpt_fTpt, 0, npt, lpt_val_x,
                                 lpt_val_y, lpt_val_z, iout, uni, flt);
    if (r)
      goto fin;
  }

  lpt_val_x = lpt_val_y = lpt_val_z = NULL;

  //--- output lpt_fuxpt
  if (spec->lpt_fuxpt.outf == ON) {
#ifdef LPT
    lpt_val_x = lpt_val;
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_FUXPT, 2, lpt_all, lpt_val_x);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_fuxpt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_fupt(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                  val->lpt_param);
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_fuxpt, 0, npt, lpt_val_x,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_fuypt
  if (spec->lpt_fuypt.outf == ON) {
#ifdef LPT
    lpt_val_y = lpt_val + npt;
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_FUYPT, 2, lpt_all, lpt_val_y);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_fuypt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_fupt(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                  val->lpt_param);
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_fuypt, 0, npt, lpt_val_y,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_fuzpt
  if (spec->lpt_fuzpt.outf == ON) {
#ifdef LPT
    lpt_val_z = lpt_val + 2 * npt;
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_FUZPT, 2, lpt_all, lpt_val_z);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_fuzpt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_fupt(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                  val->lpt_param);
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_fuzpt, 0, npt, lpt_val_z,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_fupt
  if (spec->lpt_fupt.outf == ON) {
#ifdef LPT
    if (npt > 0) {
      r = cLPTgetpts_3f(LPT_FP_FUXPT, LPT_FP_FUYPT, LPT_FP_FUZPT, 2, lpt_all,
                        lpt_val_x ? NULL : (lpt_val_x = lpt_val),
                        lpt_val_y ? NULL : (lpt_val_y = lpt_val + npt),
                        lpt_val_z ? NULL : (lpt_val_z = lpt_val + 2 * npt));
    } else {
      lpt_val_x = lpt_val_y = lpt_val_z = lpt_val;
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_fuxpt, lpt_fuypt and/or lpt_fuzpt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_fupt(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                  val->lpt_param);
#endif

    r = output_binary_lpt_vector(prm, dir, &spec->lpt_fupt, 0, npt,
                                 lpt_val_x, lpt_val_y, lpt_val_z,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  lpt_val_x = lpt_val_y = lpt_val_z = NULL;

  //--- output lpt_fduxt
  if (spec->lpt_fduxt.outf == ON) {
#ifdef LPT
    lpt_val_x = lpt_val;
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_FDUXT, 2, lpt_all, lpt_val_x);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_fduxt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_fdut(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                  val->lpt_param);
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_fduxt, 0, npt, lpt_val_x,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_fduyt
  if (spec->lpt_fduyt.outf == ON) {
#ifdef LPT
    lpt_val_y = lpt_val + npt;
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_FDUYT, 2, lpt_all, lpt_val_y);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_fduyt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_fdut(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                  val->lpt_param);
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_fduyt, 0, npt, lpt_val_y,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_fduzt
  if (spec->lpt_fduzt.outf == ON) {
#ifdef LPT
    lpt_val_z = lpt_val + 2 * npt;
    if (npt > 0) {
      r = cLPTgetpts_1f(LPT_FP_FDUZT, 2, lpt_all, lpt_val_z);
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_fduzt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_fdut(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                  val->lpt_param);
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_fduzt, 0, npt, lpt_val_z,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_fdt
  if (spec->lpt_fdt.outf == ON) {
#ifdef LPT
    if (npt > 0) {
      r = cLPTgetpts_3f(LPT_FP_FDUXT, LPT_FP_FDUYT, LPT_FP_FDUZT, 2, lpt_all,
                        lpt_val_x ? NULL : (lpt_val_x = lpt_val),
                        lpt_val_y ? NULL : (lpt_val_y = lpt_val + npt),
                        lpt_val_z ? NULL : (lpt_val_z = lpt_val + 2 * npt));
    } else {
      lpt_val_x = lpt_val_y = lpt_val_z = lpt_val;
    }
    if (for_any_rank(prm->mpi, r)) {
      variable_failed_to_obtain = "lpt_fduxt, lpt_fduyt and/or lpt_fduzt";
      goto failed_to_obtain;
    }
#endif
#ifdef LPTX
    lptx_get_fdut(lpt_val, &lpt_val_x, &lpt_val_y, &lpt_val_z, npt,
                  val->lpt_param);
#endif

    r = output_binary_lpt_vector(prm, dir, &spec->lpt_fdt, 0, npt,
                                 lpt_val_x, lpt_val_y, lpt_val_z,
                                 iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_dTdt
  if (spec->lpt_dTdt.outf == ON) {
#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_dTdtd(val->lpt_param, 0, lpt_val, npt);
#else
      LPTX_get_particle_dTdtf(val->lpt_param, 0, lpt_val, npt);
#endif
    }
#endif

    r = output_binary_lpt_scalar(prm, dir, &spec->lpt_dTdt, 0, npt,
                                 lpt_val, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lpt_Y
#ifdef LPTX
#define has_Y (number_of_vectors > LPTX_VI_MASS_FRACTION)
#else
#define has_Y 0
#endif
  if (spec->lpt_Y.outf == ON && has_Y &&
      component_info_ncompo(&prm->cdo->lpt_mass_fractions) > 0) {
#undef has_Y
    ptrdiff_t nc = 0;
    type *t = lpt_val;

#ifdef LPTX
    nc = number_of_components[LPTX_VI_MASS_FRACTION];
    CSVASSERT(component_info_ncompo(&prm->cdo->lpt_mass_fractions) == nc);
#endif
    for (ptrdiff_t i = 0; i < nc; ++i) {
      lpt_valv[i] = t;
      t += npt;
    }

#ifdef LPTX
#pragma omp parallel if (npt > LPTX_omp_small_threshold)
    {
#ifdef JUPITER_DOUBLE
      LPTX_get_particle_vector_d_soa(val->lpt_param, 0, LPTX_VI_MASS_FRACTION,
                                     lpt_valv, nc, npt);
#else
      LPTX_get_particle_vector_f_soa(val->lpt_param, 0, LPTX_VI_MASS_FRACTION,
                                     lpt_valv, nc, npt);
#endif
    }
#endif

    for (ptrdiff_t ic = 0; ic < nc; ++ic) {
      component_data *d;
      d = component_info_getc(&prm->cdo->lpt_mass_fractions, ic);
      CSVASSERT(d);
      r = output_binary_lpt_scalar(prm, dir, &spec->lpt_Y, d->comp_index, npt,
                                   lpt_valv[ic], iout, uni, flt);
    }
  }

fin:
  delete_lpt_ctrl_data(lpt_ctrl_data);
  free(lpt_val);
  free(lpt_ival);
  free(lpt_u64val);
  free(lpt_valv);
#ifdef LPTX
  free(lpt_idval);
  free(number_of_components);
#endif
  return r;

#ifdef LPT
failed_to_obtain:
  CSVASSERT(variable_failed_to_obtain);
  if (r) {
    csvperrorf(__FILE__, 0, 0, CSV_EL_ERROR, NULL,
               "Failed to obtain the particle data of %s",
               variable_failed_to_obtain);
  }
  r = 1;
  goto fin;
#endif
}

int binary_out_base(int iout, variable *val, material *mtl, parameter *prm,
                    const char *dir, binary_output_mode uni, int flt,
                    const struct data_output_spec *spec)
{
  domain *cdo;
  int ic;
  int m;
  int r;
  char *time_file;
  mpi_param *mpi;

  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->mpi);
  CSVASSERT(val);
  CSVASSERT(mtl);

  if(prm->flg->binary != ON) return 0;

  cdo = prm->cdo;
  mpi = prm->mpi;
  m = cdo->m;

  r = make_time_file_name(&time_file, dir, &spec->time, iout);
  if (r) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, __func__,
              CSV_ERR_NOMEM, 0, 0, NULL);
    return 1;
  }

  r = start_output_sync(mpi, time_file);
  if (r) goto clean;

  if (spec->comp_data.outf == ON) {
    r = output_comp_data(prm, val, dir, &spec->comp_data, iout, uni);
    if (r)
      goto fin;
  }

  if (prm->flg->solute_diff != ON) {
    for (ic = 0; ic < cdo->NBaseComponent; ++ic) {
      //--- output fs
      if (spec->fs.outf == ON) {
        r = output_binary_scalar(prm, dir, &spec->fs, ic, &val->fs[ic*m], iout, uni, flt);
        if (r) goto fin;
      }

      //--- output fl
      if (spec->fl.outf == ON) {
        r = output_binary_scalar(prm, dir, &spec->fl, ic, &val->fl[ic*m], iout, uni, flt);
        if (r) goto fin;
      }

      //--- output df
      if (val->df && spec->df.outf == ON) {
        r = output_binary_scalar(prm, dir, &spec->df, ic, &val->df[ic*m], iout, uni, flt);
        if (r) goto fin;
      }

      //--- output dfs
      if (val->dfs && spec->dfs.outf == ON) {
        r = output_binary_scalar(prm, dir, &spec->dfs, ic, &val->dfs[ic*m], iout, uni, flt);
        if (r) goto fin;
      }
    }
  } else {
    for (ic = 0; ic < cdo->NumberOfComponent; ++ic) {
      //--- output Y
      if (spec->Y.outf == ON) {
        r = output_binary_scalar(prm, dir, &spec->Y, ic, &val->Y[ic*m], iout, uni, flt);
        if (r) goto fin;
      }
    }
    for (ic = 0; ic < cdo->NBaseComponent; ++ic) {
      //--- output Vf
      if (spec->Vf.outf == ON) {
        r = output_binary_scalar(prm, dir, &spec->Vf, ic, &val->Vf[ic*m], iout, uni, flt);
        if (r) goto fin;
      }
    }
    //--- output Yt
    if (spec->Yt.outf == ON) {
      r = output_binary_scalar(prm, dir, &spec->Yt, 0, val->Yt, iout, uni, flt);
      if (r) goto fin;
    }
    //--- output fs
    if (spec->fs.outf == ON) {
      r = output_binary_scalar(prm, dir, &spec->fs, 0, val->fs, iout, uni, flt);
      if (r) goto fin;
    }
    //--- output fl
    if (spec->fl.outf == ON) {
      output_binary_scalar(prm, dir, &spec->fl, 0, val->fl, iout, uni, flt);
      if (r) goto fin;
    }
    //--- output df
    if (val->df && spec->df.outf == ON) {
      output_binary_scalar(prm, dir, &spec->df, 0, val->df, iout, uni, flt);
      if (r) goto fin;
    }
    //--- output dfs
    if (val->dfs && spec->dfs.outf == ON) {
      output_binary_scalar(prm, dir, &spec->dfs, 0, val->dfs, iout, uni, flt);
      if (r) goto fin;
    }
  }

  if (spec->uvw.outf == ON) {
    //--- output vector
    r = output_binary_vector(prm, dir, &spec->uvw, 0, val->u, val->v, val->w,
                             iout, uni, flt);
    if (r) goto fin;
  }

  //--- output VOF flux
  if (spec->flux.outf == ON && val->vfx && val->vfy && val->vfz) {
    r = output_binary_vector(prm, dir, &spec->flux, 0,
                             val->vfx, val->vfy, val->vfz, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ll
  if (spec->ll.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ll, 0, val->ll, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output lls
  if (spec->lls.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->lls, 0, val->lls, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ls
  if (spec->ls.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ls, 0, val->ls, iout, uni, flt);
    if (r) goto fin;
  }

  if(prm->flg->multi_layer == ON){

    for (int ilayer = 0; ilayer < cdo->NumberOfLayer; ++ilayer) {
      //--- output fl_layer
      if (spec->fl_layer.outf == ON) {
        r = output_binary_scalar(prm, dir, &spec->fl_layer, ilayer, &val->fl_layer[ilayer*m], iout, uni, flt);
        if (r) goto fin;
      }

      //--- output fls_layer
      if (spec->fls_layer.outf == ON) {
        r = output_binary_scalar(prm, dir, &spec->fls_layer, ilayer, &val->fls_layer[ilayer*m], iout, uni, flt);
        if (r) goto fin;
      }

      //--- output ll_layer
      if (spec->ll_layer.outf == ON) {
        r = output_binary_scalar(prm, dir, &spec->ll_layer, ilayer, &val->ll_layer[ilayer*m], iout, uni, flt);
        if (r) goto fin;
      }

      //--- output fls_layer
      if (spec->lls_layer.outf == ON) {
        r = output_binary_scalar(prm, dir, &spec->lls_layer, ilayer, &val->lls_layer[ilayer*m], iout, uni, flt);
        if (r) goto fin;
      }

      //--- output curv_layer
      if (spec->curv_layer.outf == ON) {
        r = output_binary_scalar(prm, dir, &spec->curv_layer, ilayer, &val->curv_layer[ilayer*m], iout, uni, flt);
        if (r) goto fin;
      }

      //--- output label_layer
      if (spec->label_layer.outf == ON) {
        r = output_binary_int(prm, dir, &spec->label_layer, ilayer, &val->label_layer[ilayer*m], iout, uni);
        if (r) goto fin;
      }

    }

    //--- output label_layer
    if (spec->liquid_film.outf == ON) {
      r = output_binary_scalar(prm, dir, &spec->liquid_film, 0, val->liquid_film, iout, uni, flt);
      if (r) goto fin;
    }


  }



  //--- output t
  if (spec->t.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->t, 0, val->t, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output u
  if (spec->u.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->u, 0, val->u, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output v
  if (spec->v.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->v, 0, val->v, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output w
  if (spec->w.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->w, 0, val->w, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output p
  if (spec->p.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->p, 0, val->p, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output q
  if (spec->q.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->q, 0, mtl->q, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output qgeom
  if (spec->qgeom.outf == ON && val->qgeom) {
    r = output_binary_scalar(prm, dir, &spec->qgeom, 0, val->qgeom, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output qpt
  if (spec->qpt.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->qpt, 0, val->qpt, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output t_soli
  if (spec->t_soli.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->t_soli, 0, mtl->t_soli, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output t_liq
  if (spec->t_liq.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->t_liq, 0, mtl->t_liq, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output latent
  if (spec->latent.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->latent, 0, mtl->latent, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output diff_g
  if (spec->diff_g.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->diff_g, 0, mtl->Dcg, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output dens
  if (spec->dens.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->dens, 0, mtl->dens, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output denss
  if (mtl->denss && spec->denss.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->denss, 0, mtl->denss, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output densf
  if (mtl->dens_f && spec->densf.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->densf, 0, mtl->dens_f, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output specht
  if (spec->specht.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->specht, 0, mtl->specht, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output spechts
  if (spec->spechts.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->spechts, 0, mtl->specht, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output spechtf
  if (mtl->c_f && spec->spechtf.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->spechtf, 0, mtl->c_f, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output mu
  if (spec->mu.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->mu, 0, mtl->mu, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output thc
  if (spec->thc.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->thc, 0, mtl->thc, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output thcs
  if (mtl->thcs && spec->thcs.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->thcs, 0, mtl->thcs, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output thcf
  if (mtl->thcf && spec->thcf.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->thcf, 0, mtl->thcf, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output emi
  if (spec->emi.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->emi, 0, mtl->emi, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output entha
  if (val->entha && spec->entha.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->entha, 0, val->entha, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output mushy
  if (val->mushy && spec->mushy.outf == ON) {
    r = output_binary_int(prm, dir, &spec->mushy, 0, val->mushy, iout, uni);
    if (r) goto fin;
  }

  //--- output rad
  if (spec->rad.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->rad, 0, mtl->rad, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ox_dt
  if (spec->ox_dt.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ox_dt, 0, val->ox_dt, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ox_flag
  if (spec->ox_flag.outf == ON) {
    r = output_binary_int(prm, dir, &spec->ox_flag, 0, val->ox_flag, iout, uni);
    if (r) goto fin;
  }

  //--- output ox_dt_local
  if (spec->ox_dt_local.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ox_dt_local, 0, val->ox_dt_local, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ox_lset
  if (spec->ox_lset.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ox_lset, 0, val->ox_lset, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ox_vof
  if (spec->ox_vof.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ox_vof, 0, val->ox_vof, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ox_q
  if (spec->ox_q.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ox_q, 0, val->ox_q, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ox_h2
  if (spec->ox_h2.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ox_h2, 0, val->ox_h2, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ox_f_h2o
  if (spec->ox_f_h2o.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ox_f_h2o, 0, val->ox_f_h2o, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ox_lset_h2o
  if (spec->ox_lset_h2o.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ox_lset_h2o, 0, val->ox_lset_h2o, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ox_lset_h2o_s
  if (spec->ox_lset_h2o_s.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ox_lset_h2o_s, 0, val->ox_lset_h2o_s, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ox_kp
  if (spec->ox_kp.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ox_kp, 0, mtl->ox_kp, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ox_dens
  if (spec->ox_dens.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ox_dens, 0, mtl->ox_dens, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output ox_recess_rate
  if (spec->ox_recess_rate.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ox_recess_rate, 0, mtl->ox_recess_k, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output h2_absorption variables
  r = binary_out_base_h2_absorp(iout, prm, dir, uni, flt, val, spec);
  if (r) goto fin;

  //--- output lpt variables
  r = binary_out_base_lpt(iout, prm, val, dir, uni, flt, spec);
  if (r) goto fin;

  //--- output lpt_ewall
  if (spec->lpt_ewall.outf == ON) {
    r = output_binary_scalar_nb(prm, dir, &spec->lpt_ewall, 0, val->lpt_pewall, iout, uni, flt, 1);
    if (r) goto fin;
  }

  //--- output bnd
  if (spec->bnd.outf == ON) {
    r = output_boundary(prm, dir, &spec->bnd, val, iout, uni);
    if (r) goto fin;
  }
  //--- output bnd_norm_u
  if (val->bnd_norm_u && spec->bnd_norm_u.outf == ON) {
    r = output_binary_aos_vector(prm, dir, &spec->bnd_norm_u, 0, val->bnd_norm_u, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output bnd_norm_v
  if (val->bnd_norm_v && spec->bnd_norm_v.outf == ON) {
    r = output_binary_aos_vector(prm, dir, &spec->bnd_norm_v, 0, val->bnd_norm_v, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output bnd_norm_w
  if (val->bnd_norm_w && spec->bnd_norm_w.outf == ON) {
    r = output_binary_aos_vector(prm, dir, &spec->bnd_norm_w, 0, val->bnd_norm_w, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output mass_source_g
  if (val->mass_source_g && spec->mass_source_g.outf == ON) {
    int ncompo = component_info_ncompo(&cdo->mass_source_g_comps);
    for (ic = 0; ic < ncompo; ++ic) {
      struct component_data *d;
      int igcompo;
      d = component_info_getc(&cdo->mass_source_g_comps, ic);
      igcompo = d->comp_index;
      r = output_binary_scalar(prm, dir, &spec->mass_source_g, igcompo, &val->mass_source_g[ic * cdo->m], iout, uni, flt);
      if (r) goto fin;
    }
  }

  //--- output uplsflg
  if (spec->uplsflg.outf == ON) {
    r = output_uplsflg(prm, dir, &spec->uplsflg, iout);
    if (r) goto fin;
  }

  //--- output eps 
  if (spec->eps.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->eps, 0, val->eps, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output epss
  if (spec->epss.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->epss, 0, val->epss, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output perm
  if (spec->perm.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->perm, 0, val->perm, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output tf
  if (spec->tf.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->tf, 0, val->tf, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output ts
  if (spec->ts.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->ts, 0, val->ts, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output sgm
  if (spec->sgm.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->sgm, 0, val->sgm, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output fs_ibm
  if (spec->fs_ibm.outf == ON) {
    type *fs;
    if (val->fs_ibm) {
      fs = val->fs_ibm;
    } else if (prm->flg->solute_diff == ON) {
      fs = val->fs;
    } else {
      fs = val->fs_sum;
    }
    r = output_binary_scalar(prm, dir, &spec->fs_ibm, 0, fs, iout, uni, flt);
    if (r) goto fin;
  }
  //--- output ls_ibm
  if (spec->ls_ibm.outf == ON) {
    type *ls;
    if (val->ls_ibm) {
      ls = val->ls_ibm;
    } else {
      ls = val->ls;
    }
    r = output_binary_scalar(prm, dir, &spec->ls_ibm, 0, ls, iout, uni, flt);
    if (r) goto fin;
  }

  //--- output div_u
  if (spec->div_u.outf == ON) {
    r = output_binary_scalar(prm, dir, &spec->div_u, 0, mtl->div_u, iout, uni, flt);
    if (r) goto fin;
  }

 fin:
  if (r) r = 1;
#ifdef JUPITER_MPI
  MPI_Allreduce(MPI_IN_PLACE, &r, 1, MPI_INT, MPI_MAX, mpi->CommJUPITER);
#endif
  if (r) goto clean;

  if (mpi->rank == 0) {
    r = output_time_file(cdo, time_file);
  }
#ifdef JUPITER_MPI
  MPI_Bcast(&r, 1, MPI_INT, 0, mpi->CommJUPITER);
#endif
  if (r) goto clean;

  r = finish_output_sync(mpi, time_file);
  if (r) goto clean;

 clean:
  free(time_file);
  return r;
}

int binary_out(int iout, variable *val, material *mtl, parameter *prm)
{
  binary_output_mode uni;
  int flt;
  const char *dir;
  const struct data_output_spec *spec;

  CSVASSERT(prm);
  CSVASSERT(prm->flg);

  if (iout < 0) {
    uni = prm->flg->restart_output_mode;
    flt = 0;
    dir = prm->flg->restart_data.writedir;
    spec = &prm->flg->restart_data;
  } else {
    uni = prm->flg->output_mode;
    if (prm->flg->use_double_binary == ON) {
      flt = 0;
    } else {
      flt = 1;
    }
    dir = prm->flg->output_data.writedir;
    spec = &prm->flg->output_data;
  }

  return binary_out_base(iout, val, mtl, prm, dir, uni, flt, spec);
}

int binary_out_job(variable *val, material *mtl, parameter *prm)
{
  return binary_out(-1, val, mtl, prm);
}

static void extract_boundary_vof(domain *cdo, type *output[6], type *input)
{
  int stm_diff;

  CSVASSERT(cdo);
  CSVASSERT(output);
  CSVASSERT(input);
  CSVASSERT(cdo->stm >= cdo->stmb);

  stm_diff = cdo->stm - cdo->stmb;

#pragma omp parallel
  {
    int jx, jy, jz;
    int jfx, jfy, jfz;
    ptrdiff_t jf, jt;

    if (output[0]) { /* z- */
#pragma omp for collapse(2)
      for (jy = 0; jy < cdo->nby; ++jy) {
        for (jx = 0; jx < cdo->nbx; ++jx) {
          jfx = jx + stm_diff;
          jfy = jy + stm_diff;
          jfz = cdo->stm /* + 0 */;
          jf = calc_address(jfx, jfy, jfz, cdo->mx, cdo->my, cdo->mz);
          jt = calc_address(jx, jy, 0, cdo->nbx, cdo->nby, 1);
          output[0][jt] = input[jf];
        }
      }
    }
    if (output[1]) { /* z+ */
#pragma omp for collapse(2)
      for (jy = 0; jy < cdo->nby; ++jy) {
        for (jx = 0; jx < cdo->nbx; ++jx) {
          jfx = jx + stm_diff;
          jfy = jy + stm_diff;
          jfz = cdo->stm + cdo->nz - 1;
          jf = calc_address(jfx, jfy, jfz, cdo->mx, cdo->my, cdo->mz);
          jt = calc_address(jx, jy, 0, cdo->nbx, cdo->nby, 1);
          output[1][jt] = input[jf];
        }
      }
    }
    if (output[2]) { /* y- */
#pragma omp for collapse(2)
      for (jz = 0; jz < cdo->nbz; ++jz) {
        for (jx = 0; jx < cdo->nbx; ++jx) {
          jfx = jx + stm_diff;
          jfy = cdo->stm /* + 0 */;
          jfz = jz + stm_diff;
          jf = calc_address(jfx, jfy, jfz, cdo->mx, cdo->my, cdo->mz);
          jt = calc_address(jx, 0, jz, cdo->nbx, 1, cdo->nbz);
          output[2][jt] = input[jf];
        }
      }
    }
    if (output[3]) { /* y+ */
#pragma omp for collapse(2)
      for (jz = 0; jz < cdo->nbz; ++jz) {
        for (jx = 0; jx < cdo->nbx; ++jx) {
          jfx = jx + stm_diff;
          jfy = cdo->stm + cdo->ny - 1;
          jfz = jz + stm_diff;
          jf = calc_address(jfx, jfy, jfz, cdo->mx, cdo->my, cdo->mz);
          jt = calc_address(jx, 0, jz, cdo->nbx, 1, cdo->nbz);
          output[3][jt] = input[jf];
        }
      }
    }
    if (output[4]) { /* x- */
#pragma omp for collapse(2)
      for (jz = 0; jz < cdo->nbz; ++jz) {
        for (jy = 0; jy < cdo->nby; ++jy) {
          jfx = cdo->stm /* + 0 */;
          jfy = jy + stm_diff;
          jfz = jz + stm_diff;
          jf = calc_address(jfx, jfy, jfz, cdo->mx, cdo->my, cdo->mz);
          jt = calc_address(0, jy, jz, 1, cdo->nby, cdo->nbz);
          output[4][jt] = input[jf];
        }
      }
    }
    if (output[5]) { /* x+ */
#pragma omp for collapse(2)
      for (jz = 0; jz < cdo->nbz; ++jz) {
        for (jy = 0; jy < cdo->nby; ++jy) {
          jfx = cdo->stm + cdo->nx - 1;
          jfy = jy + stm_diff;
          jfz = jz + stm_diff;
          jf = calc_address(jfx, jfy, jfz, cdo->mx, cdo->my, cdo->mz);
          jt = calc_address(0, jy, jz, 1, cdo->nby, cdo->nbz);
          output[5][jt] = input[jf];
        }
      }
    }
  }
}

static surface_boundary_data **allocate_surface_bnd_array(int m)
{
  size_t sz;
  surface_boundary_data **p;

  sz = m;
  if (sz * 3 < sz)
    return NULL;

  sz *= 3;
  if (sz * sizeof(surface_boundary_data *) < sz)
    return NULL;

  p = (surface_boundary_data **)malloc(sz * sizeof(surface_boundary_data *));
  if (!p)
    return NULL;

#pragma omp parallel for
  for (size_t i = 0; i < sz; ++i) {
    p[i] = NULL;
  }
  return p;
}

static type *allocate_bnd_norm(int m)
{
  type *p;
  size_t sz, nelement;

  sz = m;
  if (sz == 0)
    return NULL;

  if (sz * 3 < sz)
    return NULL;
  sz *= 3;
  nelement = sz;

  if (sz * sizeof(type) < sz)
    return NULL;
  sz *= sizeof(type);

  p = (type *)malloc(sz);
  if (!p)
    return NULL;

#pragma omp parallel for
  for (size_t i = 0; i < nelement; ++i) {
    p[i] = 0.0;
  }
  return p;
}

static int geometry_allocate_surface_boundary_set(variable *val, int m)
{
  CSVASSERT(val);
  CSVASSERT(m > 0);

  if (!val->surface_bnd) {
    val->surface_bnd = allocate_surface_bnd_array(m);
    if (!val->surface_bnd) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      return 1;
    }
  }
  if (!val->bnd_norm_u) {
    val->bnd_norm_u = allocate_bnd_norm(m);
    if (!val->bnd_norm_u) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      return 1;
    }
  }
  if (!val->bnd_norm_v) {
    val->bnd_norm_v = allocate_bnd_norm(m);
    if (!val->bnd_norm_v) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      return 1;
    }
  }
  if (!val->bnd_norm_w) {
    val->bnd_norm_w = allocate_bnd_norm(m);
    if (!val->bnd_norm_w) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      return 1;
    }
  }

  return 0;
}

static void geometry_surface_boundary_set_n(int *n_out, int *stm_out,
                                            int *stp_out, int n, int stm,
                                            int stp, int mpi_neighbor_m,
                                            int mpi_neighbor_p)
{
  CSVASSERT(stm >= 1);
  CSVASSERT(stp >= 1);

  if (mpi_neighbor_m != -1) {
    n += stm;
    stm = 0;
  }
  if (mpi_neighbor_p != -1) {
    n += stp - 1;
    stp = 1;
  }
  *n_out = n;
  *stm_out = stm;
  *stp_out = stp;
}

static int geometry_set_surface_boundary(
  parameter *prm, const char *geom_name, geom_shape_data *shape_data,
  geom_surface_shape_data *surface_shape_data, geom_init_element *init_el,
  surface_boundary_data **surface_boundary_array,
  surface_boundary_data *surface_boundary_head, //
  type *vof, type *bnd_norm_u, type *bnd_norm_v, type *bnd_norm_w, //
  int nx, int ny, int nz, int stmx, int stmy, int stmz,
  int stpx, int stpy, int stpz, type *x, type *y, type *z)
{
  geom_data_element *data_el;
  surface_boundary_data *sbdata;
  const geom_user_defined_data *ud;
  struct surface_boundary_init_data *sdata;
  int mx, my, mz, nxp, nyp, nzp;
  ptrdiff_t n, m;
  type v_th;

  CSVASSERT(prm);
  CSVASSERT(init_el);
  CSVASSERT(surface_boundary_array);
  CSVASSERT(surface_boundary_head);
  CSVASSERT(vof);
  CSVASSERT(bnd_norm_u);
  CSVASSERT(bnd_norm_v);
  CSVASSERT(bnd_norm_w);
  CSVASSERT(nx > 0);
  CSVASSERT(ny > 0);
  CSVASSERT(nz > 0);
  CSVASSERT(stmx >= 0);
  CSVASSERT(stmy >= 0);
  CSVASSERT(stmz >= 0);
  CSVASSERT(stpx >= 1);
  CSVASSERT(stpy >= 1);
  CSVASSERT(stpz >= 1);

  /* If no shape data or surface shape data, just ignoring the init. */
  if (!shape_data)
    return 0;
  if (!surface_shape_data)
    return 0;

  data_el = geom_surface_shape_data_parent(surface_shape_data);

  ud = geom_init_element_get_comp_data(init_el);
  sdata = (struct surface_boundary_init_data *)geom_user_defined_data_get(ud);
  v_th = sdata->threshold;

  sbdata = sdata->data;
  sbdata->geom_name = geom_data_element_get_name(data_el);
  surface_boundary_data_move(sbdata, surface_boundary_head);

  nxp = nx + 1;
  nyp = ny + 1;
  nzp = nz + 1;
  n = nxp;
  n *= nyp;
  n *= nzp;
  mx = nx + stmx + stpx;
  my = ny + stmy + stpy;
  mz = nz + stmz + stpz;
  m = mx;
  m *= my;
  m *= mz;

#pragma omp parallel for schedule(dynamic)
  for (ptrdiff_t i = 0; i < 3 * n; ++i) {
    ptrdiff_t j, jm;
    geom_vec2 uv;
    geom_vec3 pnt;
    geom_vec3 spnt;
    geom_vec3 norm;
    geom_shape_element *e;
    int ix, iy, iz, jx, jy, jz, ixyz, iaxis;
    double xc, yc, zc, u, v;
    int calc;
    type *dest_array;

    calc_struct_index(i, 3, n, 1, &iaxis, &ixyz, &iz);
    calc_struct_index(ixyz, nxp, nyp, nzp, &ix, &iy, &iz);
    jx = ix + stmx;
    jy = iy + stmy;
    jz = iz + stmz;
    j = calc_address(jx, jy, jz, mx, my, mz);

    /* Ensure both jx and jx - 1 is inside array boundary */
    switch(iaxis) {
    case 0:
      calc = (jx > 0 && jx < mx && iy < ny && iz < nz);
      break;
    case 1:
      calc = (jy > 0 && jy < my && ix < nx && iz < nz);
      break;
    case 2:
      calc = (jz > 0 && jz < mz && ix < nx && iy < ny);
      break;
    default:
      calc = 0;
      break;
    }
    if (!calc)
      continue;

    switch(iaxis) {
    case 0:
      jm = calc_address(jx - 1, jy, jz, mx, my, mz);
      break;
    case 1:
      jm = calc_address(jx, jy - 1, jz, mx, my, mz);
      break;
    case 2:
      jm = calc_address(jx, jy, jz - 1, mx, my, mz);
      break;
    }
    if (v_th != 0.0) {
      calc = (vof[j] < v_th && vof[jm] >= v_th) ||
             (vof[j] >= v_th && vof[jm] < v_th);
    } else {
      calc = (vof[j] <= 0.0 && vof[jm] > 0.0) ||
             (vof[j] > 0.0 && vof[jm] <= 0.0);
    }
    if (!calc)
      continue;

    xc = 0.5 * (x[jx + 1] + x[jx]);
    yc = 0.5 * (y[jy + 1] + y[jy]);
    zc = 0.5 * (z[jz + 1] + z[jz]);

    switch(iaxis) {
    case 0:
      xc = x[jx];
      dest_array = bnd_norm_u;
      break;
    case 1:
      yc = y[jy];
      dest_array = bnd_norm_v;
      break;
    case 2:
      zc = z[jz];
      dest_array = bnd_norm_w;
      break;
    }
    pnt = geom_vec3_c(xc, yc, zc);
    e = NULL;
    geom_shape_data_nearest_surface(shape_data, pnt, -1, 0.0, &e, &uv, &spnt,
                                    &norm);
    if (e) {
      u = geom_vec2_x(uv);
      v = geom_vec2_y(uv);
      if (geom_surface_shape_data_inout_test_at(surface_shape_data, u, v)) {
        dest_array[3 * j + 0] = geom_vec3_x(norm);
        dest_array[3 * j + 1] = geom_vec3_y(norm);
        dest_array[3 * j + 2] = geom_vec3_z(norm);
        surface_boundary_array[3 * j + iaxis] = sbdata;
      }
    }
  }

  return 0;
}

static geom_vec3 defp_cell_center(parameter *prm, int mat_id, //
                                  int jx, int jy, int jz, void *defp_data)
{
  return geom_vec3_c(prm->cdo->xc[jx], prm->cdo->yc[jy], prm->cdo->zc[jz]);
}

static geom_vec3 defp_x_face(parameter *prm, int mat_id, //
                             int jx, int jy, int jz, void *defp_data)
{
  return geom_vec3_c(prm->cdo->xv[jx], prm->cdo->yc[jy], prm->cdo->zc[jz]);
}

static geom_vec3 defp_y_face(parameter *prm, int mat_id, //
                             int jx, int jy, int jz, void *defp_data)
{
  return geom_vec3_c(prm->cdo->xc[jx], prm->cdo->yv[jy], prm->cdo->zc[jz]);
}

static geom_vec3 defp_z_face(parameter *prm, int mat_id, //
                             int jx, int jy, int jz, void *defp_data)
{
  return geom_vec3_c(prm->cdo->xc[jx], prm->cdo->yc[jy], prm->cdo->zv[jz]);
}

static geom_vec3 defp_W_face(parameter *prm, int mat_id, //
                             int jx, int jy, int jz, void *defp_data)
{
  return geom_vec3_c(prm->cdo->xv[jx], prm->cdo->yc[jy], prm->cdo->zc[jz]);
}

static geom_vec3 defp_E_face(parameter *prm, int mat_id, //
                             int jx, int jy, int jz, void *defp_data)
{
  return geom_vec3_c(prm->cdo->xv[jx + 1], prm->cdo->yc[jy], prm->cdo->zc[jz]);
}

static geom_vec3 defp_S_face(parameter *prm, int mat_id, //
                             int jx, int jy, int jz, void *defp_data)
{
  return geom_vec3_c(prm->cdo->xc[jx], prm->cdo->yv[jy], prm->cdo->zc[jz]);
}

static geom_vec3 defp_N_face(parameter *prm, int mat_id, //
                             int jx, int jy, int jz, void *defp_data)
{
  return geom_vec3_c(prm->cdo->xc[jx], prm->cdo->yv[jy + 1], prm->cdo->zc[jz]);
}

static geom_vec3 defp_B_face(parameter *prm, int mat_id, //
                             int jx, int jy, int jz, void *defp_data)
{
  return geom_vec3_c(prm->cdo->xc[jx], prm->cdo->yc[jy], prm->cdo->zv[jz]);
}

static geom_vec3 defp_T_face(parameter *prm, int mat_id, //
                             int jx, int jy, int jz, void *defp_data)
{
  return geom_vec3_c(prm->cdo->xc[jx], prm->cdo->yc[jy], prm->cdo->zv[jz + 1]);
}

static int
geometry_set_by_init(parameter *prm, geom_init_element *init_el, type *target,
                     int mat_id, int nloop_mat,
                     geom_vec3 (*defp_f)(parameter *prm, int mat_id, //
                                         int jx, int jy, int jz, //
                                         void *defp_data),
                     void *defp_data, type *vfp,
                     int nx, int ny, int nz, int stmx, int stmy, int stmz,
                     int vstmx, int vstmy, int vstmz,
                     type (*modifier)(parameter *prm, void *data, type nv,
                                      int mat_id, int jx, int jy, int jz,
                                      geom_vec3 position),
                     void *modifier_data)
{
  domain *cdo;
  int r;
  int mat_s, mat_e;
  ptrdiff_t icnt;
  r = 0;

  CSVASSERT(prm);
  CSVASSERT(prm->cdo);
  CSVASSERT(defp_f);

  cdo = prm->cdo;

  if (mat_id >= 0) {
    CSVASSERT(nloop_mat == 1);
    mat_s = mat_id;
    mat_e = mat_id + 1;
  } else {
    CSVASSERT(nloop_mat >= 0);
    mat_s = 0;
    mat_e = nloop_mat;
  }

  icnt  = nx;
  icnt *= ny;
  icnt *= nz;

#pragma omp parallel
  {
    int imat;
    ptrdiff_t ii;
    struct jupiter_init_func_args if_arg;

    for (imat = mat_s; imat < mat_e; ++imat) {
#pragma omp for
      for (ii = 0; ii < icnt; ++ii) {
        int vjx, vjy, vjz;
        int jx, jy, jz;
        type x, y, z;
        type nv;
        geom_vec3 p, dp;
        ptrdiff_t loc, loc0, locv;

        calc_struct_index(ii, nx, ny, nz, &jx, &jy, &jz);
        vjx = jx + vstmx;
        vjy = jy + vstmy;
        vjz = jz + vstmz;
        locv = calc_address(vjx, vjy, vjz, cdo->mx, cdo->my, cdo->mz);
        if (locv < 0) {
 #pragma omp atomic write
          r = 1;
          continue;
        }

        jx += stmx;
        jy += stmy;
        jz += stmz;

        loc0 = calc_address(jx, jy, jz, cdo->mx, cdo->my, cdo->mz);
        if (loc0 < 0) {
#pragma omp atomic write
          r = 1;
          continue;
        }

        if_arg.cell = loc0;
        loc = loc0 + imat * cdo->m;

        p = defp_f(prm, mat_id, jx, jy, jz, defp_data);
        x = geom_vec3_x(p);
        y = geom_vec3_y(p);
        z = geom_vec3_z(p);
        nv = geom_init_element_calc_at(init_el, target[loc], vfp[locv],
                                       x, y, z, &if_arg, NULL);
        if (modifier) {
          nv = modifier(prm, modifier_data, nv, imat, jx, jy, jz, p);
        }
        target[loc] = nv;
      }
    }
  }
  return r;
}

struct vof_fg_modifier_data
{
  type *fs;
  type *fl;
  type *fg;
};

static type vof_fsfl_modifier(parameter *prm, void *data, type nv,
                              int mat_id, int jx, int jy, int jz,
                              geom_vec3 position)
{
  return clip(nv);
}

static type vof_fg_modifier(parameter *prm, void *data, type nv, int mat_id,
                            int jx, int jy, int jz, geom_vec3 position)
{
  struct vof_fg_modifier_data *fp;
  type *fs, *fl;
  domain *cdo;
  phase_value *phv;
  ptrdiff_t loc;
  type flsn;

  fp = (struct vof_fg_modifier_data *)data;
  cdo = prm->cdo;
  phv = prm->phv;
  loc = calc_address(jx, jy, jz, cdo->mx, cdo->my, cdo->mz);
  loc += mat_id * cdo->m;

  fs = fp->fs;
  fl = fp->fl;

  if (mat_id < cdo->NBaseComponent) {
    flsn = clip(1.0 - nv);
    if (phv->comps[mat_id].sform == SOLID_FORM_UNUSED) {
      fl[loc] = flsn;
    } else {
      type fls = clip(fs[loc] + fl[loc]);
      if (fls > 0.0) {
        fs[loc] = clip(fs[loc] / fls * flsn);
        fl[loc] = clip(fl[loc] / fls * flsn);
      } else {
        fs[loc] = 0.0;
        fl[loc] = 0.0;
      }
    }
  }
  return clip(nv);
}

static type *allocate_vof_temp(int ncompo, int m)
{
  int i, nn;
  size_t n;
  type *f;
  n  = m;
  n *= ncompo;

  f = (type *)malloc(sizeof(type) * n);
  if (!f) return NULL;

  nn = n;
  if (nn < 0 || nn != n) {
    free(f);
    return NULL;
  }

#pragma omp parallel for
  for (i = 0; i < n; ++i) {
    f[i] = 0.0;
  }
  return f;
}

static type *allocate_tmp_fs(domain *cdo)
{
  type *fs;
  fs = allocate_vof_temp(cdo->NBaseComponent, cdo->m);
  if (!fs) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }
  return fs;
}

static type *allocate_tmp_fl(domain *cdo)
{
  type *fl;
  fl = allocate_vof_temp(cdo->NBaseComponent, cdo->m);
  if (!fl) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }
  return fl;
}

static type *allocate_tmp_fg(domain *cdo)
{
  type *fg;
  fg = allocate_vof_temp(cdo->NumberOfComponent, cdo->m);
  if (!fg) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }
  return fg;
}

// Input Geometry data (with giving geom_data entry)
int  geometry_in_with(variable *val, parameter *prm, init_component tgt,
                      init_component *modified, geom_data *data)
{
  /* YSE: Removed unused variables. */
  domain *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  int j;
  /* YSE: Add variables for geomtry data */
  geom_data_element *data_el;
  type *fl, *fs, *fg;
  type *fla, *fsa;
  type *bnd_work_base;
  type *bnd_work[6];
  int retval;
  init_component modified_components;
  /* YSE: end */

  modified_components = init_component_zero();
  if (modified)
    init_component_clear(modified);

  if (!data) return 0;

  retval = 0;
  fla = NULL; /* Locally Allocated fl */
  fsa = NULL; /* Locally Allocated fs */
  fg = NULL;  /* Always allocates memory for fg */

  /* Reflection of boundary data (continium of 6 directions) */
  bnd_work_base = NULL;
  for (j = 0; j < 6; ++j) {
    bnd_work[j] = NULL;
  }

  if (prm->flg->solute_diff == ON) {
    fl = NULL; /* Allocate if neccesary. */
    fs = NULL;
  } else {
    fl = val->fl;
    fs = val->fs;
  }

  data_el = geom_data_get_element(data);
  for (; data_el; data_el = geom_data_element_next(data_el)) {
    geom_file_data *fdata;
    geom_init_data *idata;
    geom_init_element *init_el;
    geom_shape_data *shape;
    geom_surface_shape_data *surface_shape;
    const geom_user_defined_data *ud;
    jupiter_geom_ext_eldata *ext_eldata;
    int ret;
    int boundary_for_vof_converted;
    init_component init_components;

    idata = geom_data_element_get_init(data_el);

    /* Check what components should be initialized */
    init_components = init_component_zero();
    if (idata) {
      init_el = geom_init_data_get_element(idata);
      for (; init_el; init_el = geom_init_element_next(init_el)) {
        int comp_id = geom_init_element_get_comp_id(init_el);
        enum init_component_id id;
        CSVASSERT(comp_id >= 0 && comp_id < INIT_COMPONENT_MAX);
        id = (enum init_component_id)comp_id;
        if (init_component_is_set(&tgt, id))
          init_component_set(&init_components, id);
      }
    }

    ud = geom_data_element_get_extra_data(data_el);
    ext_eldata = geom_user_defined_data_get(ud);
    if (init_component_is_set(&tgt, INIT_COMPONENT_GEOM_DUMP)) {
      if (ext_eldata && ext_eldata->dump_file)
        init_component_set(&init_components, INIT_COMPONENT_GEOM_DUMP);
    }

    /* No components should be initialized from this data element */
    if (!init_component_any(&init_components)) {
      continue;
    }

    boundary_for_vof_converted = 0;
    fdata = geom_data_element_get_file(data_el);
    shape = geom_data_element_get_shape(data_el);
    surface_shape = geom_data_element_get_surface_shape(data_el);
    if (fdata) {
      binary_output_mode unif;
      const geom_user_defined_data *ud;
      const char *afname;
      geom_svec3 rep, offs, size, orig;
      jupiter_geom_ext_file_data *ext_fd;

      afname = geom_file_data_get_alt_file_path(fdata);
      ud = geom_file_data_get_extra_data(fdata);
      ext_fd = geom_user_defined_data_get(ud);
      CSVASSERT(ext_fd);
      CSVASSERT(afname);

      unif = ext_fd->read_mode;
      if (unif != BINARY_OUTPUT_BYPROCESS) {
        size = geom_file_data_get_size(fdata);
        offs = geom_file_data_get_offset(fdata);
        rep = geom_file_data_get_repeat(fdata);
        orig = geom_file_data_get_origin(fdata);
      } else {
        size = geom_svec3_c(cdo->gnx, cdo->gny, cdo->gnz);
        offs = geom_svec3_c(0, 0, 0);
        rep  = geom_svec3_c(1, 1, 1);
        orig = geom_svec3_c(0, 0, 0);
      }
      if (unif != BINARY_OUTPUT_BYPROCESS && !(
            geom_svec3_eql(size, geom_svec3_c(cdo->gnx, cdo->gny, cdo->gnz)) &&
            geom_svec3_eql(offs, geom_svec3_c(0, 0, 0)) &&
            geom_svec3_eql(rep,  geom_svec3_c(1, 1, 1)) &&
            geom_svec3_eql(orig, geom_svec3_c(0, 0, 0)))) {
        ret = input_geometry_binary_unified(mpi, val->work,
                                            cdo->stm, cdo->stm, cdo->stm,
                                            cdo->stp, cdo->stp, cdo->stp,
                                            cdo->mx, cdo->my, cdo->mz,
                                            1, afname, 1, orig,
                                            size, rep, offs);
      } else {
        ret = input_binary(mpi, val->work, NULL,
                           cdo->stm, cdo->stm, cdo->stm,
                           cdo->stp, cdo->stp, cdo->stp,
                           cdo->mx, cdo->my, cdo->mz, 1, afname, unif, 1);
      }
      if (ret != 0) {
        prm->status = ON;
        continue;
      }

    } else if (shape) {
      int nsub;
      int subl;
      geom_error gerr;

      nsub = geom_shape_data_get_nsub_cell(shape);
      subl = nsub * nsub * nsub;
      CSVASSERT(subl > 0);

      /* Printing warning is currently disabled. */
      gerr = geom_shape_data_update_all_transform(shape, NULL, NULL, NULL);
      if (gerr == GEOM_ERR_SHAPE_OP_SHOULD_SET) {
        gerr = GEOM_SUCCESS;
      }
      if (for_any_rank(mpi, gerr != GEOM_SUCCESS)) {
        if (gerr != GEOM_SUCCESS) {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                    CSV_ERR_GEOMETRY, 0, gerr, NULL);
        }
        prm->status = ON;
        continue;
      }

#pragma omp parallel for
      for (j = 0; j < cdo->m; ++j) {
        int jx, jy, jz;
        int js;
        int rest;
        geom_vec3 base, d, dh;

        calc_struct_index(j, cdo->mx, cdo->my, cdo->mz, &jx, &jy, &jz);
        base = geom_vec3_c(cdo->xv[jx], cdo->yv[jy], cdo->zv[jz]);
        d = geom_vec3_c(cdo->dxv[jx], cdo->dyv[jy], cdo->dzv[jz]);
        d = geom_vec3_factor(d, 1.0 / nsub);
        dh = geom_vec3_factor(d, 0.5);

        rest = 0;
        for (js = 0; js < subl; ++js) {
          int jsx, jsy, jsz;
          double x, y, z;
          geom_vec3 t;
          calc_struct_index(js, nsub, nsub, nsub, &jsx, &jsy, &jsz);
          t = geom_vec3_c(jsx, jsy, jsz);
          t = geom_vec3_mul_each_element(t, d);
          t = geom_vec3_add(t, dh);
          t = geom_vec3_add(base, t);
          x = geom_vec3_x(t);
          y = geom_vec3_y(t);
          z = geom_vec3_z(t);
          rest += geom_shape_data_inbody_test_at(shape, x, y, z);
        }
        val->work[j] = (double)rest / subl;
      }
    } else {
#pragma omp parallel for
      for (j = 0; j < cdo->m; ++j) {
        val->work[j] = 1.0;
      }
    }
    if (ext_eldata && ext_eldata->dump_file) {
      if (init_component_is_set(&init_components, INIT_COMPONENT_GEOM_DUMP)) {
        ret = output_binary(mpi, val->work, cdo->stm, cdo->stm, cdo->stm,
                            cdo->stp, cdo->stp, cdo->stp,
                            cdo->mx, cdo->my, cdo->mz, 1,
                            ext_eldata->dump_file, ext_eldata->dump_united, 1);
        if (ret) {
          prm->status = ON;
        }
      }
    }

    if (init_component_is_set(&init_components,
                              INIT_COMPONENT_SURFACE_BOUNDARY)) {
      if (geometry_allocate_surface_boundary_set(val, cdo->m)) {
        prm->status = ON;
        continue;
      }
    }

    if (!idata) continue;

    init_el = geom_init_data_get_element(idata);
    for (; init_el; init_el = geom_init_element_next(init_el)) {
      int comp_id;
      enum init_component_id cid;
      const geom_user_defined_data *ud;
      void *d;
      type *target;
      int mat_id;
      int nloop_mat;
      struct boundary_init_data *bnd_data;
      struct tboundary_init_data *tbnd_data;
      struct init_lpt_pewall_data *pewall_data;
      int ret;

      bnd_data = NULL;
      tbnd_data = NULL;

      comp_id = geom_init_element_get_comp_id(init_el);
      ud = geom_init_element_get_comp_data(init_el);
      d = geom_user_defined_data_get(ud);

      CSVASSERT(comp_id >= 0 && comp_id < INIT_COMPONENT_MAX);
      cid = (enum init_component_id)comp_id;
      if (!init_component_is_set(&init_components, cid)) {
        continue;
      }

      ret = jupiter_init_func_binary_read_data(prm->cdo, prm->mpi, init_el, 0);
      if (ret) {
        prm->status = ON;
        continue;
      }

      mat_id = 0;
      nloop_mat = 1;
      target = NULL;

#ifndef NDEBUG
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG, __func__,
                 "Initializing '%s' for '%s'",
                 geom_init_element_get_comp_name(init_el),
                 geom_data_element_get_name(data_el));
#endif
      init_component_set(&modified_components, cid);
      switch (cid) {
      case INIT_COMPONENT_VOF:
        {
          struct init_vof_data *vof_data;
          struct vof_fg_modifier_data vof_fg_mod_data;
          vof_data = d;
          CSVASSERT(vof_data);
          CSVASSERT(vof_data->comp.d);
          mat_id = vof_data->comp.d->jupiter_id;
          if (mat_id == -1) {
            if (vof_data->phase == GEOM_PHASE_GAS) {
              nloop_mat = cdo->NumberOfComponent;
            } else {
              nloop_mat = cdo->NBaseComponent;
            }
          } else {
            nloop_mat = 1;
            mat_id = vof_data->comp.d->comp_index;
          }
          switch(vof_data->phase) {
          case GEOM_PHASE_LIQUID:
            if (!fl) {
              fla = allocate_tmp_fl(cdo);
              if (!fla) {
                return -1;
              }
              fl = fla;
            }
            geometry_set_by_init(prm, init_el, fl, mat_id, nloop_mat,
                                 defp_cell_center, NULL, val->work,
                                 cdo->mx, cdo->my, cdo->mz, 0, 0, 0, 0, 0, 0,
                                 vof_fsfl_modifier, NULL);
            break;
          case GEOM_PHASE_SOLID:
            if (!fs) {
              fsa = allocate_tmp_fs(cdo);
              if (!fsa) {
                return -1;
              }
              fs = fsa;
            }
            geometry_set_by_init(prm, init_el, fs, mat_id, nloop_mat,
                                 defp_cell_center, NULL, val->work,
                                 cdo->mx, cdo->my, cdo->mz, 0, 0, 0, 0, 0, 0,
                                 vof_fsfl_modifier, NULL);
            break;
          case GEOM_PHASE_GAS:
            if (!fs) {
              fsa = allocate_tmp_fs(cdo);
              if (!fsa) {
                return -1;
              }
              fs = fsa;
            }
            if (!fl) {
              fla = allocate_tmp_fl(cdo);
              if (!fla) {
                return -1;
              }
              fl = fla;
            }
            if (!fg) {
              fg = allocate_tmp_fg(cdo);
              if (!fg) {
                return -1;
              }
            }
            if (mat_id < cdo->NBaseComponent) {
#pragma omp parallel
              {
                int imat;
                for (imat = 0; imat < nloop_mat; ++imat) {
#pragma omp for
                  for (j = 0; j < cdo->m; ++j) {
                    size_t loc;
                    if (mat_id < 0) {
                      loc = j + imat * cdo->m;
                    } else {
                      loc = j + mat_id * cdo->m;
                    }
                    if (imat < cdo->NBaseComponent) {
                      fg[loc] = clip(1.0 - (fl[loc] + fs[loc]));
                    }
                  }
                }
              }
            }
            vof_fg_mod_data.fg = fg;
            vof_fg_mod_data.fs = fs;
            vof_fg_mod_data.fl = fl;
            geometry_set_by_init(prm, init_el, fg, mat_id, nloop_mat,
                                 defp_cell_center, NULL, val->work,
                                 cdo->mx, cdo->my, cdo->mz, 0, 0, 0, 0, 0, 0,
                                 vof_fg_modifier, &vof_fg_mod_data);

            break;
          case GEOM_PHASE_INVALID:
          default:
            CSVUNREACHABLE();
            break;
          }
        }
        break;
      case INIT_COMPONENT_BOUNDARY:
        bnd_data = (struct boundary_init_data *)d;
        goto boundary_case;
      case INIT_COMPONENT_THERMAL_BOUNDARY:
        tbnd_data = (struct tboundary_init_data *)d;
        goto boundary_case;
      case INIT_COMPONENT_SURFACE_BOUNDARY:
      {
        const char *geom_name;
        int *nrk = prm->mpi->nrk;
        int r, nx, ny, nz, stmx, stmy, stmz, stpx, stpy, stpz;
        geometry_surface_boundary_set_n(&nz, &stmz, &stpz, cdo->nz, cdo->stm,
                                        cdo->stp, nrk[0], nrk[1]);
        geometry_surface_boundary_set_n(&ny, &stmy, &stpy, cdo->ny, cdo->stm,
                                        cdo->stp, nrk[2], nrk[3]);
        geometry_surface_boundary_set_n(&nx, &stmx, &stpx, cdo->nx, cdo->stm,
                                        cdo->stp, nrk[4], nrk[5]);
        CSVASSERT(nx + stmx + stpx == cdo->mx);
        CSVASSERT(ny + stmy + stpy == cdo->my);
        CSVASSERT(nz + stmz + stpz == cdo->mz);

        geom_name = geom_data_element_get_name(data_el);
        r = geometry_set_surface_boundary(prm, geom_name, shape, surface_shape,
                                          init_el, val->surface_bnd,
                                          &cdo->surface_boundary_head,
                                          val->work, val->bnd_norm_u,
                                          val->bnd_norm_v, val->bnd_norm_w, nx,
                                          ny, nz, stmx, stmy, stmz, stpx, stpy,
                                          stpz, cdo->x, cdo->y, cdo->z);
        if (r)
          prm->status = ON;
      }
        break;
      case INIT_COMPONENT_PRESSURE:
        geometry_set_by_init(prm, init_el, val->p, 0, 1, defp_cell_center, NULL,
                             val->work, cdo->mx, cdo->my, cdo->mz, 0, 0, 0,
                             0, 0, 0, NULL, NULL);
        break;
      case INIT_COMPONENT_TEMPERATURE:
        geometry_set_by_init(prm, init_el, val->t, 0, 1, defp_cell_center, NULL,
                             val->work, cdo->mx, cdo->my, cdo->mz, 0, 0, 0,
                             0, 0, 0, NULL, NULL);
        break;
      case INIT_COMPONENT_VELOCITY_U:
        geometry_set_by_init(prm, init_el, val->u, 0, 1, defp_x_face, NULL,
                             val->work, cdo->mx, cdo->my, cdo->mz, 0, 0, 0,
                             0, 0, 0, NULL, NULL);
        break;
      case INIT_COMPONENT_VELOCITY_V:
        geometry_set_by_init(prm, init_el, val->v, 0, 1, defp_y_face, NULL,
                             val->work, cdo->mx, cdo->my, cdo->mz, 0, 0, 0,
                             0, 0, 0, NULL, NULL);
        break;
      case INIT_COMPONENT_VELOCITY_W:
        geometry_set_by_init(prm, init_el, val->w, 0, 1, defp_z_face, NULL,
                             val->work, cdo->mx, cdo->my, cdo->mz, 0, 0, 0,
                             0, 0, 0, NULL, NULL);
        break;
      case INIT_COMPONENT_FIXED_HSOURCE:
        geometry_set_by_init(prm, init_el, val->qgeom, 0, 1, defp_cell_center,
                             NULL, val->work, cdo->mx, cdo->my, cdo->mz,
                             0, 0, 0, 0, 0, 0, NULL, NULL);
        break;
      case INIT_COMPONENT_LPT_PEWALL_N:
        CSVASSERT(val->lpt_pewall);
        pewall_data = (struct init_lpt_pewall_data *)d;
        if (pewall_data->is_boundary) {
          boundary_direction dir;
          dir = boundary_direction_normalize(pewall_data->dir);
          /*
           * Assign values on the first stencil cells outside of the calculation
           * domain from the fraction values `val->work` on the first cells of
           * the calculation domain and the coordinate of the surface of the
           * calculation domain.
           */
          if (dir & BOUNDARY_DIR_WEST) {
            geometry_set_by_init(prm, init_el, val->lpt_pewall, 0, 1,
                                 defp_E_face, NULL, val->work,
                                 1, cdo->ny, cdo->nz,
                                 cdo->stm - 1, cdo->stm, cdo->stm,
                                 cdo->stm, cdo->stm, cdo->stm, NULL, NULL);
          }
          if (dir & BOUNDARY_DIR_EAST) {
            geometry_set_by_init(prm, init_el, val->lpt_pewall, 0, 1,
                                 defp_W_face, NULL, val->work,
                                 1, cdo->ny, cdo->nz,
                                 cdo->stm + cdo->nx, cdo->stm, cdo->stm,
                                 cdo->stm + cdo->nx - 1, cdo->stm, cdo->stm,
                                 NULL, NULL);
          }
          if (dir & BOUNDARY_DIR_SOUTH) {
            geometry_set_by_init(prm, init_el, val->lpt_pewall, 0, 1,
                                 defp_N_face, NULL, val->work,
                                 cdo->nx, 1, cdo->nz,
                                 cdo->stm, cdo->stm - 1 , cdo->stm,
                                 cdo->stm, cdo->stm, cdo->stm, NULL, NULL);
          }
          if (dir & BOUNDARY_DIR_NORTH) {
            geometry_set_by_init(prm, init_el, val->lpt_pewall, 0, 1,
                                 defp_S_face, NULL, val->work,
                                 cdo->nx, 1, cdo->nz,
                                 cdo->stm, cdo->stm + cdo->ny, cdo->stm,
                                 cdo->stm, cdo->stm + cdo->ny - 1, cdo->stm,
                                 NULL, NULL);
          }
          if (dir & BOUNDARY_DIR_BOTTOM) {
            geometry_set_by_init(prm, init_el, val->lpt_pewall, 0, 1,
                                 defp_T_face, NULL, val->work,
                                 cdo->nx, cdo->ny, 1,
                                 cdo->stm, cdo->stm, cdo->stm - 1,
                                 cdo->stm, cdo->stm, cdo->stm, NULL, NULL);
          }
          if (dir & BOUNDARY_DIR_TOP) {
            geometry_set_by_init(prm, init_el, val->lpt_pewall, 0, 1,
                                 defp_B_face, NULL, val->work,
                                 cdo->nx, cdo->ny, 1,
                                 cdo->stm, cdo->stm, cdo->stm + cdo->nz,
                                 cdo->stm, cdo->stm, cdo->stm + cdo->nz - 1,
                                 NULL, NULL);
          }
          break;
        }
        geometry_set_by_init(prm, init_el, val->lpt_pewall, 0, 1,
                             defp_cell_center, NULL, val->work,
                             cdo->nx, cdo->ny, cdo->nz, cdo->stm,
                             cdo->stm, cdo->stm, cdo->stm, cdo->stm, cdo->stm,
                             NULL, NULL);
        break;
      default:
        CSVUNREACHABLE();
        break;
      }
      continue;

    boundary_case:
      {
        boundary_direction dir;
        fluid_boundary_data *fl_data;
        thermal_boundary_data *th_data;
        const boundary_direction dirs[] = {  /* See mpi->nrk */
          BOUNDARY_DIR_BOTTOM, BOUNDARY_DIR_TOP,   /* z */
          BOUNDARY_DIR_SOUTH,  BOUNDARY_DIR_NORTH, /* y */
          BOUNDARY_DIR_WEST,   BOUNDARY_DIR_EAST,  /* x */
        };

        dir = BOUNDARY_DIR_NONE;
        if (bnd_data) {
          dir = bnd_data->dir;
        }
        if (tbnd_data) {
          dir = tbnd_data->dir;
        }
        if (dir == BOUNDARY_DIR_NONE) {
#ifndef NDEBUG
          csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG, __func__,
                     "No boundary direction specified");
#endif
          continue;
        }
        dir = boundary_direction_normalize(dir);

        if (!bnd_work_base) {
          ptrdiff_t req;
          ptrdiff_t szs[6];

          req = 0;
          for (j = 0; j < 6; ++j) {
            ptrdiff_t sz;
            sz = 0;
            szs[j] = -1;
            if (mpi->nrk[j] == -1) {
              switch(j) {
              case 0: /* z */
              case 1:
                sz = cdo->nbx * cdo->nby;
                break;
              case 2: /* y */
              case 3:
                sz = cdo->nbx * cdo->nbz;
                break;
              case 4: /* x */
              case 5:
                sz = cdo->nby * cdo->nbz;
                break;
              default:
                CSVUNREACHABLE();
                break;
              }
              if (sz > 0) {
                req += sz;
                szs[j] = sz;
              }
            }
          }

          if (req > 0) {
            bnd_work_base = (type *) malloc(sizeof(type) * req);
            if (!bnd_work_base) {
              csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                        CSV_ERR_NOMEM, 0, 0, NULL);
              retval = 1;
              goto clean;
            }

            req = 0;
            for (j = 0; j < 6; ++j) {
              if (szs[j] > 0) {
                bnd_work[j] = bnd_work_base + req;
                req += szs[j];
              }
            }
          }
        }

        if (!boundary_for_vof_converted) {
          boundary_for_vof_converted = 1;
          extract_boundary_vof(cdo, bnd_work, val->work);
        }

        fl_data = NULL;
        th_data = NULL;

        /* unset bit for MPI-connected directions */
        for (j = 0; j < 6; ++j) {
          if (!bnd_work[j])
            dir = dir & ~dirs[j];
        }

        for (j = 0; j < 6; ++j) {
          if (dir & dirs[j]) {
            if (bnd_data) {
              fl_data = bnd_data->data;
              fluid_boundary_data_move(fl_data, &cdo->fluid_boundary_head);
            }
            if (tbnd_data) {
              th_data = tbnd_data->data;
              thermal_boundary_data_move(th_data, &cdo->thermal_boundary_head);
            }
            break;
          }
        }

        if (dir & BOUNDARY_DIR_BOTTOM) {
          if (fl_data) {
            fluid_boundary_set_by_vof(cdo->nbx, cdo->nby, 1, fl_data,
                                      bnd_data->threshold, val->bnd_B.fl,
                                      bnd_work[0]);
          }
          if (th_data) {
            thermal_boundary_set_by_vof(cdo->nbx, cdo->nby, 1, th_data,
                                        tbnd_data->threshold, val->bnd_B.th,
                                        bnd_work[0]);
          }
        }
        if (dir & BOUNDARY_DIR_TOP) {
          if (fl_data) {
            fluid_boundary_set_by_vof(cdo->nbx, cdo->nby, 1, fl_data,
                                      bnd_data->threshold, val->bnd_T.fl,
                                      bnd_work[1]);
          }
          if (th_data) {
            thermal_boundary_set_by_vof(cdo->nbx, cdo->nby, 1, th_data,
                                        tbnd_data->threshold, val->bnd_T.th,
                                        bnd_work[1]);
          }
        }
        if (dir & BOUNDARY_DIR_SOUTH) {
          if (fl_data) {
            fluid_boundary_set_by_vof(cdo->nbx, 1, cdo->nbz, fl_data,
                                      bnd_data->threshold, val->bnd_S.fl,
                                      bnd_work[2]);
          }
          if (th_data) {
            thermal_boundary_set_by_vof(cdo->nbx, 1, cdo->nbz, th_data,
                                        tbnd_data->threshold, val->bnd_S.th,
                                        bnd_work[2]);
          }
        }
        if (dir & BOUNDARY_DIR_NORTH) {
          if (fl_data) {
            fluid_boundary_set_by_vof(cdo->nbx, 1, cdo->nbz, fl_data,
                                      bnd_data->threshold, val->bnd_N.fl,
                                      bnd_work[3]);
          }
          if (th_data) {
            thermal_boundary_set_by_vof(cdo->nbx, 1, cdo->nbz, th_data,
                                        tbnd_data->threshold, val->bnd_N.th,
                                        bnd_work[3]);
          }
        }
        if (dir & BOUNDARY_DIR_WEST) {
          if (fl_data) {
            fluid_boundary_set_by_vof(1, cdo->nby, cdo->nbz, fl_data,
                                      bnd_data->threshold, val->bnd_W.fl,
                                      bnd_work[4]);
          }
          if (th_data) {
            thermal_boundary_set_by_vof(1, cdo->nby, cdo->nbz, th_data,
                                        tbnd_data->threshold, val->bnd_W.th,
                                        bnd_work[4]);
          }
        }
        if (dir & BOUNDARY_DIR_EAST) {
          if (fl_data) {
            fluid_boundary_set_by_vof(1, cdo->nby, cdo->nbz, fl_data,
                                      bnd_data->threshold, val->bnd_E.fl,
                                      bnd_work[5]);
          }
          if (th_data) {
            thermal_boundary_set_by_vof(1, cdo->nby, cdo->nbz, th_data,
                                        tbnd_data->threshold, val->bnd_E.th,
                                        bnd_work[5]);
          }
        }
      }
      continue;
    }
  }

  /* YSE: Reflect user-provided fs and fl data to Y */
  if (init_component_is_set(&modified_components, INIT_COMPONENT_VOF) &&
      prm->flg->solute_diff == ON) {
#pragma omp parallel for
    for (j = 0; j < cdo->m; ++j) {
      int icompo;
      size_t loc;
      type fssum;
      type flsum;
      type fsv;
      type flv;

      fssum = 0.0;
      flsum = 0.0;

      for (icompo = 0; icompo < cdo->NumberOfComponent; ++icompo) {
        loc = j + icompo * cdo->m;
        if (icompo < cdo->NBaseComponent) {
          fsv = fs ? fs[loc] : 0.0;
          flv = fl ? fl[loc] : 0.0;
          fssum += fsv;
          flsum += flv;
          val->Y[loc] = clip(fsv + flv);
        } else {
          if (fg) {
            val->Y[loc] = fg[loc];
          } else {
            val->Y[loc] = 0.0;
          }
        }
      }

      if (fssum > 0.0) fssum = 1.0;
      val->fs[j] = clip(fssum);
      val->fl[j] = clip(flsum);
    }
  }

clean:
  free(fla);
  free(fsa);
  free(fg);
  free(bnd_work_base);

  if (modified)
    init_component_copy(modified, &modified_components);

  return retval;
}

// Input Geometry data (VOF function)
int  geometry_in(int icnt, variable *val, parameter *prm, init_component tgt)
{
  int r;

  if(icnt != -1 || prm->flg->geom_in == OFF) return 0;

  return geometry_in_with(val, prm, tgt, NULL, prm->geom_sets);
}

int restart(int iout, variable *val, material *mtl, parameter *prm)
{
  /* YSE: Add mpi */
  domain  *cdo;
  mpi_param *mpi;
  char *time_file = NULL;
  char *uplsflg_file = NULL;
  int flt;
  binary_output_mode uni;
  const char *dir;
  const struct data_output_spec *spec;
  int r;
  int j;

  if(iout < 0) return 0;

  /* YSE: add Assertion */
  CSVASSERT(prm);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->mpi);
  CSVASSERT(prm->flg);

  cdo = prm->cdo;
  mpi = prm->mpi;

  /* Init LPT module */
  lpt_set_jupiter_error_function();
  if (prm->flg->lpt_calc == ON) {
    init_lpt(1, prm->flg, cdo, mpi, val, &r);
  }

  time_file = NULL;
  uplsflg_file = NULL;

  /*
   * YSE: From param.c, iout == 0 means restart from last job.
   * `make_(time_)file_name` uses iout == -1 for restart_job name.
   */
  if (iout == 0) iout = -1;

  if (iout < 0) {
    uni = prm->flg->restart_input_mode;
    flt = 0;
    dir = prm->flg->restart_data.readdir;
    spec = &prm->flg->restart_data;
  } else {
    uni = prm->flg->output_mode;
    if (prm->flg->use_double_binary == ON) {
      flt = 0;
    } else {
      flt = 1;
    }
    dir = prm->flg->output_data.readdir;
    spec = &prm->flg->output_data;
  }

  //-- input time
  r = make_time_file_name(&time_file, dir, &spec->time, iout);
  if (r != 0) {
    time_file = NULL;
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    goto clean;
  }

  r = input_time_file(mpi, cdo, time_file);
  if (r) goto clean;

  //-- input uplsflg
  r = make_shared_file_name(&uplsflg_file, dir, &spec->uplsflg, iout);
  if (r != 0) {
    uplsflg_file = NULL;
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    goto clean;
  }

  r = input_uplsflg_file(mpi, prm->flg, uplsflg_file);
  if (r) goto clean;

  //-- input variables(fs, fl, t, u, v)
  r = binary_in(iout, val, prm, dir, spec, flt, uni);
  if (r) goto clean;

  //-- apply boundary condition
  bcf_VOF(0,val->fs, val, prm);
  bcf_VOF(1,val->fl, val, prm);
  if(prm->flg->multi_layer){
    for(int ilayer=0; ilayer<cdo->NumberOfLayer; ilayer++){
      bcf(&val->fl_layer[ilayer*cdo->m], prm);
    }    
  }
  bct(val->t, val, prm);
  bcp(val->p, val, prm);

#pragma omp parallel
  {
    int m = prm->cdo->m;
    int ncompo = prm->cdo->NumberOfComponent;
    phase_value_component *comps = prm->phv->comps;

    if (prm->flg->solute_diff == ON) {
#pragma omp for
      for (j = 0; j < m; ++j) {
        int icompo;
        type valY;
        valY = 0.0;
        for (icompo = 0; icompo < ncompo; icompo++)
          valY += val->Y[j + icompo * m];
        val->Yt[j] = valY;
        val->fs_sum[j] = val->fs[j];
        val->fl_sum[j] = val->fl[j];
      }
    } else {
#pragma omp for
      for (j = 0; j < m; ++j) {
        int icompo;
        type vals, vall;
        vals = 0.0;
        vall = 0.0;
        for (icompo = 0; icompo < ncompo; icompo++) {
          vals += val->fs[j + icompo * m];
          vall += val->fl[j + icompo * m];
        }
        val->fs_sum[j] = vals;
        val->fl_sum[j] = vall;
      }

      if (val->fs_ibm) {
#pragma omp for
        for (j = 0; j < m; ++j) {
          type vals_ibm;
          int icompo;
          vals_ibm = 0.0;
          for (icompo = 0; icompo < ncompo; ++icompo) {
            if (comps[icompo].sform == SOLID_FORM_IBM) {
              vals_ibm += val->fs[j + icompo * m];
            }
          }
          val->fs_ibm[j] = vals_ibm;
        }
      }
    }
#pragma omp for
    for (j = 0; j < prm->cdo->m; ++j) {
      val->fls[j] = val->fs_sum[j] + val->fl_sum[j];
    }

    if(prm->flg->multi_layer==ON){
      for(int ilayer=0; ilayer<cdo->NumberOfLayer; ilayer++){
#pragma omp for
        for (j = 0; j < prm->cdo->m; ++j) {
          int jl = j + ilayer*cdo->m;
          val->fls_layer[jl] = val->fs_sum[j] + val->fl_layer[jl];
        }        
      }
    }

  }

  /*
   * YSE: To be consistent on the result between restarted or not, we
   *      are decided to update Vp every steps, and dump Vf and reload
   *      it.
   *
   * To compute boundary values for Vf, Vp is required.
   */
  init_partial_volume(val, mtl, prm);//added by Chai
  /* init_Vf(val, mtl, prm);//added by Chai */

  if (prm->flg->solute_diff == ON) {
    bcs(val->Y, val->Vf, val, prm, mtl);
  }

  set_diff_func(val, mtl, prm);//added by Chai
  for(int j = 0; j < prm->cdo->m; j++)
    val->t_pre[j] = val->t[j];//added by chai
  //-- calc physical properties
  materials(mtl, val, prm);
  //-- calc Level Set function

  if(prm->flg->multi_layer==OFF){
    // YSE: Level set function will read from restart data.
    //Level_Set(1, 200, val->ls, val->fs_sum, prm);
    //Level_Set(1, 200, val->ll, val->fl_sum, prm);
    //Level_Set(1, 200, val->lls, val->work, prm);
    bcf(val->ll, prm);
    bcf(val->ls, prm);
    bcf(val->lls, prm);

    // Rebuild normal vector
    normal_vector_cell(val->nvsx, val->nvsy, val->nvsz, NULL, val->ls, cdo);
    normal_vector_cell(val->nvlx, val->nvly, val->nvlz, val->curv, val->lls, cdo);
    // normal_vector_cell(val->nvlx, val->nvly, val->nvlz, val->curv, val->ll, cdo);

    if (val->ls_ibm && val->fs_ibm) {
      Level_Set(1, cdo->ls_iteration, val->ls_ibm, val->fs_ibm, prm);
      bcf(val->ls_ibm, prm);

      if (val->nvibmx && val->nvibmy && val->nvibmz) {
        normal_vector_cell(val->nvibmx, val->nvibmy, val->nvibmz, NULL, val->ls_ibm, cdo);
      }
    }
  }else{ /*--- multi_layer model ---*/

    // Level set function will be rebuilt with fl_layer and fls_layer, made by restart data.
    // same process is in  init_variables()
    // ls is NOT defined layer-wise
    Level_Set(1, cdo->ls_iteration, val->ls, val->fs_sum, prm);
    bcf(val->ls, prm);
    normal_vector_cell(val->nvsx, val->nvsy, val->nvsz, NULL, val->ls, cdo);

    // ll and lls are layer-wise
    for(int ilayer= 0; ilayer<cdo->NumberOfLayer; ilayer++){
      Level_Set(1, cdo->ls_iteration, &val->ll_layer[ilayer*cdo->m] , &val->fl_layer[ilayer*cdo->m] , prm);
      bcf(&val->ll_layer[ilayer*cdo->m], prm);

      Level_Set(1, cdo->ls_iteration, &val->lls_layer[ilayer*cdo->m], &val->fls_layer[ilayer*cdo->m], prm);
      bcf(&val->lls_layer[ilayer*cdo->m], prm);    
      normal_vector_cell(&val->nvlx_layer[ilayer*cdo->m], &val->nvly_layer[ilayer*cdo->m], &val->nvlz_layer[ilayer*cdo->m], &val->curv_layer[ilayer*cdo->m], &val->lls_layer[ilayer*cdo->m], cdo);
    }

    // ls_ibmもlayer毎に定義されておらず、通常の取り扱いと同じ。
    if (val->ls_ibm && val->fs_ibm) {
      Level_Set(1, cdo->ls_iteration, val->ls_ibm, val->fs_ibm, prm);
      bcf(val->ls_ibm, prm);

      if (val->nvibmx && val->nvibmy && val->nvibmz) {
        normal_vector_cell(val->nvibmx, val->nvibmy, val->nvibmz, NULL, val->ls_ibm, cdo);
      }
    }

  }

  //Y bcu(val->u, val->v, val->w, val, prm);
  /* YSE: Refill only outer-boundary (not IBM) */
  bcu_correct(val->u, val->v, val->w, val, mtl, prm);

  /* Send LPT constants to LPT module */
  if (prm->flg->lpt_calc == ON) {
    lpt_send_constant_field_vars(prm->cdo, val, &r);
  }

clean:
  if (time_file)
    free(time_file);
  if (uplsflg_file)
    free(uplsflg_file);
  if (for_any_rank(mpi, r)) {
    prm->status = ON;
    return 1;
  }

  return 0;
}

type output_data(int flag, variable *val, material *mtl, parameter *prm)
{
  flags  *flg = prm->flg;
  domain *cdo = prm->cdo;
  type   time0 = cpu_time();
  int r;

  r = 0;
  if(flag == 0 && cdo->time >= cdo->tout) {
    //binary_out(cdo->iout, val, mtl, prm);
    r = binary_out(cdo->iout, val, mtl, prm);
    analysis_mass_flowrate(val, mtl, prm);
    cdo->iout++;
    cdo->tout += cdo->dtout;
  }
  if(flag == 1){
    r = binary_out_job(val, mtl, prm);
  }
  if (r) {
    prm->status = ON;
  }

  return cpu_time() - time0;
}
