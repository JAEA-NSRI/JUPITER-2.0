#ifndef JUPITER_NON_UNIFORM_GRID_H
#define JUPITER_NON_UNIFORM_GRID_H

#include "common.h"
#include "csv.h"
#include "struct.h"

JUPITER_DECL
void non_uniform_grid_input_data_init(struct non_uniform_grid_input_data *p);

JUPITER_DECL
struct non_uniform_grid_input_data *non_uniform_grid_input_data_new(void);

JUPITER_DECL
void non_uniform_grid_input_data_delete(struct non_uniform_grid_input_data *p);

JUPITER_DECL
void non_uniform_grid_input_data_delete_all(
  struct non_uniform_grid_input_data *head);

/**
 * @brief Read CSV for variable_data_input
 * @param out_head List head (output)
 * @param nreg Number of regions
 * @param csv CSV data set (in, technically unused)
 * @param fname Name of the input CSV file
 * @param row CSV row to be read (in/out, technically unused)
 * @param col CSV column to be read (in/out)
 * @param stat If non-null pointer given, sets ON if any error occurred
 * @retval 0 success
 * @retval 1 input error occured
 * @retval 2 fatal error occured
 *
 * Before calling this function, initialize `out_head->high` with
 * the lowest coordinate.
 *
 * If @p n is negative, it reads until end of the row.
 */
JUPITER_DECL
int non_uniform_grid_input_read_csv(
  struct non_uniform_grid_input_data *out_head, int nreg, csv_data *csv,
  const char *fname, csv_row **row, csv_column **col, int *stat);

JUPITER_DECL
type non_uniform_grid_total_length(struct non_uniform_grid_input_data *head);
JUPITER_DECL
int non_uniform_grid_total_ndivs(struct non_uniform_grid_input_data *head);

JUPITER_DECL
struct non_uniform_grid_input_data *
non_uniform_grid_input_data_next(struct non_uniform_grid_input_data *p);

JUPITER_DECL
struct non_uniform_grid_input_data *
non_uniform_grid_input_data_prev(struct non_uniform_grid_input_data *p);

/**
 * @brief Returns the starting coordinate of this entry
 */
JUPITER_DECL
type non_uniform_grid_input_data_start(struct non_uniform_grid_input_data *p);

/**
 * @brief Returns the ending coordinate of this entry
 */
JUPITER_DECL
type non_uniform_grid_input_data_end(struct non_uniform_grid_input_data *p);

/**
 * @brief Returns the width of this entry
 */
JUPITER_DECL
type non_uniform_grid_input_data_width(struct non_uniform_grid_input_data *p);

/**
 * @brief Get function of input data
 */
JUPITER_DECL
non_uniform_grid_function
non_uniform_grid_input_data_function(struct non_uniform_grid_input_data *p);

/**
 * @brief Compute relative coordinate
 * @param p Entry to compute coordinate
 * @param j Local index number to compute
 * @return coordinate value
 *
 * If @p j < 0 || @p j > non_uniform_grid_input_data_ndiv(p), returns
 * coordinate with 'extended' by first or last delta.
 */
JUPITER_DECL
type non_uniform_grid_input_data_calc_relcoord(
  struct non_uniform_grid_input_data *p, int j);

/**
 * @brief Compute global coordinate
 * @param p Entry to compute coordinate
 * @param j Local index number to compute
 * @return coordinate value
 *
 * If @p j < 0 || @p j > non_uniform_grid_input_data_ndiv(p), returns
 * coordinate with 'extended' by first or last delta.
 */
JUPITER_DECL
type non_uniform_grid_input_data_calc_abscoord(
  struct non_uniform_grid_input_data *p, int j);

/**
 * @brief Get the number of divisions in the entry
 */
JUPITER_DECL
int non_uniform_grid_input_data_ndiv(struct non_uniform_grid_input_data *p);

/**
 * @brief Set the number of divisions to the entry
 */
JUPITER_DECL
void non_uniform_grid_input_data_set_ndiv(struct non_uniform_grid_input_data *p,
                                          int ndiv);

/**
 * @brief build mesh by input data
 * @param head List input
 * @param js Start cell index (inclusive)
 * @param je Last cell index  (exclusive)
 * @param circular Build circular mesh
 * @param v  Cell vertex coordinate array [je - js + 1] (out)
 * @return 0 if success, non-0 if failed (input list is empty)
 *
 * @p js and @p je needs global index for MPI parallelization,
 * starting with 0 and first entry of @p head.
 *
 * You can set the range of negative index (by using delta of first
 * cell for non-circular mesh) and larger index (by using delta of
 * last cell for non-circular mesh).
 */
JUPITER_DECL
int non_uniform_grid_build_mesh(struct non_uniform_grid_input_data *head, //
                                int js, int je, int circular, type *v);

/**
 * @brief Set derived variables for non-uniform grid
 * @param n  Number of points
 * @param v  Cell vertex coordinate array [n + 1] (in)
 * @param c  Cell center coordinate array [n]     (out)
 * @param dv Cell vertex delta array      [n]     (out)
 * @param dc Cell center delta array      [n - 1] (out)
 * @paarm dcp Cell center to positive face delta array [n] (out)
 * @param dcn Cell center to negative face delta array [n] (out)
 * @param dvp Cell face to positive center delta array [n + 1] (out)
 * @param dvn Cell face to negative center delta array [n + 1] (out)
 *
 * @p dvp[n] and @p dvn[0] will always be set to HUGE_VAL and
 * -HUGE_VAL, respectively.
 */
JUPITER_DECL
void non_uniform_grid_set_derived_vars(int n, type *v, type *c, type *dv,
                                       type *dc, type *dcp, type *dcn,
                                       type *dvp, type *dvn);

/**
 * @brief Calculate cell volume at (jx, jy, jz)
 * @param cdo Pointer to JUPITER domain
 * @param jx I-index, including stencil
 * @param jy J-index, including stencil
 * @param jz K-index, including stencil
 * @return result
 */
static inline type calc_cell_volume(domain *cdo, int jx, int jy, int jz)
{
  return cdo->dxv[jx] * cdo->dyv[jy] * cdo->dzv[jz];
}

#endif
