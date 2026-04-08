/**
 * @file common_util.h
 * @brief Common inlinable basic utilities
 */

#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include <stddef.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate address offset at (i, j, k)
 * @param i X-index
 * @param j Y-index
 * @param k Z-index
 * @param mx X-array size
 * @param my Y-array size
 * @param mz Z-array size
 * @return calculated address offset
 */
static inline ptrdiff_t
calc_address(int i, int j, int k, int mx, int my, int mz)
{
  ptrdiff_t ret;
  ret  = k;
  ret *= my;
  ret += j;
  ret *= mx;
  ret += i;
  return ret;
}

/**
 * @brief Calculate structural index from address
 * @param addr Address offset
 * @param mx X-array size
 * @param my Y-array size
 * @param mz Z-array size
 * @param i X-index (out)
 * @param j Y-index (out)
 * @param k Z-index (out)
 * @retval 0 success
 * @retval 1 out of bounds
 *
 * @note This function is theoretically completely inlinable, because
 *       all control flow paths are determinable and
 *       non-iterative. But, this function is a recursive function in
 *       this context, so the compiler may give up inlining this
 *       function if it does not do a data-flow analysis or such.
 */
static inline int
calc_struct_index(ptrdiff_t addr, int mx, int my, int mz,
                  int *i, int *j, int *k)
{
  ptrdiff_t mxy;

  if (addr < 0) {
    *i = -1;
    *j = -1;
    *k = -1;
    return 1;
  }

  mxy  = mx;
  mxy *= my;

  *k   = addr / mxy;
  addr = addr % mxy;
  *j   = addr / mx;
  *i   = addr % mx;

  if (*i < 0 || *i >= mx) return calc_struct_index(-1, 0, 0, 0, i, j, k);
  if (*j < 0 || *j >= my) return calc_struct_index(-1, 0, 0, 0, i, j, k);
  if (*k < 0 || *k >= mz) return calc_struct_index(-1, 0, 0, 0, i, j, k);
  return 0;
}

/**
 * @brief converts Gas Material ID to index of dc_calc_param
 * @param id ID number to be computed
 * @param NBaseCompo Number of Base (Solute/Liquid phase) materials
 * @param NGasCompo Number of Gas-only materials
 * @return index number, or -1 if out-of-range.
 */
static inline int
convert_dc_calc_gas_id_to_index(int id, int NBaseCompo, int NGasCompo)
{
  if (id == -1) {
    return 0;
  }
  if (id < NBaseCompo) {
    return -1;
  }
  id -= NBaseCompo;
  if (id < NGasCompo) {
    return id + 1;
  }
  return -1;
}

/**
 * @brief converts index of dc_calc_param to Gas Material ID
 * @param index index number to be computed
 * @param NBaseCompo Number of Base (Solute/Liquid phase) materials
 * @param NGasCompo Number of Gas-only materials
 * @return Material ID number, -2 if out-of-range.
 */
static inline int
convert_dc_calc_gas_index_to_id(int index, int NBaseCompo, int NGasCompo)
{
  if (index < 0) {
    return -2;
  }
  if (index == 0) {
    return -1;
  }
  if (index <= NGasCompo) {
    return NBaseCompo + index - 1;
  }
  return -2;
}

/**
 * @brief (manual) 1D thread distribution for OpenMP
 * @param is start index to distribute (inclusive) [in]
 * @param ie ending index to distribute (exclusive) [in]
 * @param nth number of threads in team [in]
 * @param ith thread number [in]
 * @param local_is local start index (inclusive) [out, optional]
 * @param local_ie local ending index (exclusive) [out, optional]
 *
 * This function is same as distribute_thread_1d() but allows to compute range
 * of indices for specific thread number.
 *
 * If @p nb and @p nbs is given,
 *
 * @p nth and @p ith is treated as given value even if OpenMP is not
 * enabled.
 */
static inline void distribute_thread_1di(ptrdiff_t is, ptrdiff_t ie, int nth,
                                         int ith, ptrdiff_t *local_is,
                                         ptrdiff_t *local_ie)
{
  ptrdiff_t n;
  n = ie - is;

  if (n > 0 && ith >= 0 && ith < nth) {
    ptrdiff_t npt;
    int rem, ovr;

    npt = n / nth;
    rem = n % nth;
    ovr = nth - rem;
    is += ith * npt;
    ie = is + npt;
    if (ith >= ovr) {
      int off = ith - ovr;
      is += off;
      ie += off + 1;
    }
  } else {
    ie = is;
  }

  if (local_is)
    *local_is = is;
  if (local_ie)
    *local_ie = ie;
}

/**
 * @brief 1D block-wise thread distribution
 * @param n Thread-local size to split into blocks [in]
 * @param nbsest Requested block size [in]
 * @param nb Number of blocks [out]
 * @param nbs Block size [out]
 *
 * Computes enough number of block to be run by given @p nbsest-sized block. If
 * 0 or negative value passed for @p nbsest, @p nbs is set to 128.
 *
 * @note @p n needs to be thread-local size, not global.
 */
static inline void distribute_thread_1db(ptrdiff_t n, ptrdiff_t nbsest,
                                         int *nb, ptrdiff_t *nbs)
{
  if (nbsest <= 0)
    nbsest = 128;

  if (nb)
    *nb = (n > 0) ? ((n - 1) / nbsest + 1) : 0;
  if (nbs)
    *nbs = nbsest;
}

/**
 * @brief Computes maximum of @p nb among OpenMP threads.
 *
 * This function is intended for internal use by distribute_thread_*().
 *
 * This function returns @p nb if OpenMP is not enabled or temporary shared
 * storage @p shared_nb is not provided.
 */
static inline int distribute_thread_max_nb(int nb, int *shared_nb)
{
#ifdef _OPENMP
  int lnb;

  if (!shared_nb)
    return nb;

#pragma omp single
  *shared_nb = nb;
#pragma omp atomic read
  lnb = *shared_nb;
  if (lnb < nb) {
#pragma omp critical
    {
      if (*shared_nb < nb)
        *shared_nb = nb;
    }
  }
#pragma omp barrier
#pragma omp atomic read
  nb = *shared_nb;
#endif
  return nb;
}

/**
 * @brief (manual) 1D thread distribution for OpenMP, optionally block-wise
 * @param is start index (inclusive) [in]
 * @param ie ending index (exclusive) [in]
 * @param nbsest Requesting block size [in]
 * @param nth number of threads in team [out, optional]
 * @param ith thread number [out, optional]
 * @param local_is Local start index [out, optional]
 * @param local_ie Local ending index [out, optional]
 * @param nb Number of blocks to be run [out, optional]
 * @param nbs Block size [out, optional]
 * @param shared_nb thread-shared storage for computing @p nbs [optional]
 *
 * This function is only available in OpenMP thread-parallelized context.
 *
 * If OpenMP is not enabled (at compilation of main source), this function
 * treats number of threads in team to be 1.
 *
 * The behavior of this function is undefined if OpenMP parallelization is
 * nested or is not called in thread-parallelized context.
 *
 * @p nth and @p ith is always obtained from OpenMP for computation, but will
 * be not returned if corresponding parameter is NULL.
 *
 * If @p nb and @p nbs is given, computes enough number of block to be run by
 * given @p nbsest-sized block. If 0 or negative value passed for @p nbsest, @p
 * nbs is set to 128. And if number of threads in team is 1, @p nbs will always
 * be set to number of elements in loop to make the number of blocks to 1.
 *
 * If @p nb or @p nbs is NULL, @p nbsest is not used.
 *
 * In addition to @p nb and @p nbs, if @p shared_nb is provided, @p nb is set
 * to the maximum of @p nb in team and you can synchronize threads for each
 * blocks.
 */
static inline void distribute_thread_1d(ptrdiff_t is, ptrdiff_t ie,
                                        int *nth, int *ith, ptrdiff_t *local_is,
                                        ptrdiff_t *local_ie, int *nb,
                                        ptrdiff_t *nbs, int *shared_nb)
{
  int lnth;
  int lith;
  ptrdiff_t isl, iel;

#ifdef _OPENMP
  lnth = omp_get_num_threads();
  lith = omp_get_thread_num();
#else
  lnth = 1;
  lith = 0;
#endif

  distribute_thread_1di(is, ie, lnth, lith, &isl, &iel);
  if (nb && nbs) {
    ptrdiff_t n = iel - isl;

    if (lnth <= 1)
      *nbs = n;

    distribute_thread_1db(n, *nbs, nb, nbs);
    *nb = distribute_thread_max_nb(*nb, shared_nb);
  }

  if (local_is)
    *local_is = isl;
  if (local_ie)
    *local_ie = iel;
  if (nth)
    *nth = lnth;
  if (ith)
    *ith = lith;
}

/**
 * @brief Test whether there is an element that matches given
 * conditional in 3D rectilinear domain
 *
 * @param mx X-axis size
 * @param my Y-axis size
 * @param mz Z-axis size
 * @param stmx X- stencil size
 * @param stmy Y- stencil size
 * @param stmz Z- stencil size
 * @param stpx X+ stencil size
 * @param stpy Y+ stencil size
 * @param stpz Z+ stencil size
 * @param func tester function
 * @param arg Extra argument for func
 * @param shared_nb thread-shared nb
 * @param shared_found thread-sherad found
 *
 * @p shared_nb is for internal use, and must be initialized with 0.
 *
 * @p sherad_found is for returning the result. It must be initialized
 * with -1. Result values are:
 *
 * * -1 not found (all of func(...) returns 0)
 * * -2 error occured
 * * One of locations which func(...) returns non-0.
 *
 * @note If it matches to more than 1 cell, this function returns one
 * of them, but there are no agreed consistency among multiple calls.
 *
 * @note Although arg is not const, but you should not modify the
 * content. It may be thread parallelized.
 *
 * @note func() will only be called for some part of given p array,
 * not all.
 *
 * If OpenMP is enabled, this function will not be fully inlined.
 *
 * This is same as struct_domain_find_if() but without `#pragma omp
 * parallel`. See struct_domain_find_if() for more info.
 */
static inline void
struct_domain_find_p_if(const int mx, const int my, const int mz,       //
                        const int stmx, const int stmy, const int stmz, //
                        const int stpx, const int stpy, const int stpz, //
                        int (*const func)(ptrdiff_t jj, void *arg), void *arg,
                        int *shared_nb, ptrdiff_t *shared_found)
{
  int jx, jy, jz;
  ptrdiff_t jj;
  ptrdiff_t j;
  int ib;
  ptrdiff_t is, ie, iis, iie, nbs;
  int nb;
  int onb;
  ptrdiff_t lfound = -1;

  const int nx = mx - stmx - stpx;
  const int ny = my - stmy - stpy;
  const int nz = mz - stmz - stpz;
  const ptrdiff_t n = (ptrdiff_t)nx * ny * nz;

  distribute_thread_1d(0, n, NULL, NULL, &is, &ie, &nb, &nbs, shared_nb);

  for (ib = 0; ib < nb; ++ib) {
    iis = is + nbs * ib;
    iie = iis + nbs;
    if (iie > ie)
      iie = ie;

    for (j = iis; j < iie; ++j) {
      if (calc_struct_index(j, nx, ny, nz, &jx, &jy, &jz)) {
        lfound = -2;
        break;
      }

      jj = calc_address(jx + stmx, jy + stmy, jz + stmz, mx, my, mz);

      if (func(jj, arg)) {
        lfound = jj;
        break;
      }
    }

    if (lfound != -1) {
#ifdef _OPENMP
#pragma omp atomic write
#endif
      shared_found[0] = lfound;
    }

    /* No barrier needed for the last block */
    if (ib < nb - 1) {
#ifdef _OPENMP
#pragma omp barrier
#pragma omp atomic read
#endif
      lfound = shared_found[0];
      if (lfound != -1)
        break;
    }
  }
}

/**
 * @brief Test whether there is an element that matches given
 * conditional in 3D rectilinear domain
 *
 * @param mx X-axis size
 * @param my Y-axis size
 * @param mz Z-axis size
 * @param stmx X- stencil size
 * @param stmy Y- stencil size
 * @param stmz Z- stencil size
 * @param stpx X+ stencil size
 * @param stpy Y+ stencil size
 * @param stpz Z+ stencil size
 * @param func tester function
 * @param arg Extra argument for func
 * @retval -1 not found (all of func(...) returns 0)
 * @retval -2 error occured
 * @return One of locations which func(...) returns non-0.
 *
 * @note If it matches to more than 1 cell, this function returns one
 * of them, but there are no agreed consistency among multiple calls.
 *
 * @note Although arg is not const, but you should not modify the content. If
 * you want to modify, ensure to use lock for OpenMP enabled context.
 *
 * @note func() will only be called for some part of given region,
 * not all.
 *
 * If OpenMP is enabled, this function will not be fully inlined.
 */
static inline ptrdiff_t
struct_domain_find_if(const int mx, const int my, const int mz,       //
                      const int stmx, const int stmy, const int stmz, //
                      const int stpx, const int stpy, const int stpz, //
                      int (*const func)(ptrdiff_t jj, void *arg), void *arg)
{
  int nbg = 0;
  ptrdiff_t found = -1;

#ifdef _OPENMP
#pragma omp parallel
#endif
  {
    struct_domain_find_p_if(mx, my, mz, stmx, stmy, stmz, stpx, stpy, stpz,
                            func, arg, &nbg, &found);
  }

  return found;
}

/**
 * @brief Find given number of elements at maximum that matches given
 * conditional in 3D rectilinear domain
 *
 * @param mx X-axis size
 * @param my Y-axis size
 * @param mz Z-axis size
 * @param stmx X- stencil size
 * @param stmy Y- stencil size
 * @param stmz Z- stencil size
 * @param stpx X+ stencil size
 * @param stpy Y+ stencil size
 * @param stpz Z+ stencil size
 * @param maxfind Number of elements to find
 * @param func tester function
 * @param arg Extra argument for func
 * @param shared_nb thread-shared nb
 * @param shared_found thread-sherad found
 *
 * @note Although arg is not const, but you should not modify the content. If
 * you want to modify, ensure to use lock for OpenMP enabled context.
 *
 * @note func() will only be called for some part of given region,
 * not all.
 *
 * @p shared_nb is for internal use, and must be initialized with 0.
 *
 * @p shared_found is for returning the result.
 *
 * This function does not create array. If needed, create it by your own and
 * pass through @p arg.

 */
static inline void
struct_domain_find_n_p_if(const int mx, const int my, const int mz,       //
                          const int stmx, const int stmy, const int stmz, //
                          const int stpx, const int stpy, const int stpz, //
                          ptrdiff_t maxfind,
                          int (*const func)(ptrdiff_t jj, void *arg), void *arg,
                          int *shared_nb, ptrdiff_t *shared_found)
{
  int jx, jy, jz;
  ptrdiff_t jj;
  ptrdiff_t j;
  int ib;
  ptrdiff_t is, ie, iis, iie, nbs;
  int nb;
  int onb;
  int lerr;
  ptrdiff_t lfound = 0;

  const int nx = mx - stmx - stpx;
  const int ny = my - stmy - stpy;
  const int nz = mz - stmz - stpz;
  const ptrdiff_t n = (ptrdiff_t)nx * ny * nz;

  lerr = 0;
  distribute_thread_1d(0, n, NULL, NULL, &is, &ie, &nb, &nbs, shared_nb);

  for (ib = 0; ib < nb; ++ib) {
    iis = is + nbs * ib;
    iie = iis + nbs;
    if (iie > ie)
      iie = ie;

    for (j = iis; j < iie; ++j) {
      if (calc_struct_index(j, nx, ny, nz, &jx, &jy, &jz)) {
        lerr = 1;
        break;
      }

      jj = calc_address(jx + stmx, jy + stmy, jz + stmz, mx, my, mz);

      if (func(jj, arg)) {
#ifdef _OPENMP
#pragma omp atomic update
#endif
        ++*shared_found;
      }

#ifdef _OPENMP
#pragma omp atomic read
#endif
      lfound = *shared_found;
      if (lfound >= maxfind)
        break;
    }

    if (lerr) {
#ifdef _OPENMP
#pragma omp atomic write
#endif
      shared_found[0] = -1;
    }

    /* No barrier needed for the last block */
    if (ib < nb - 1) {
#ifdef _OPENMP
#pragma omp barrier
#pragma omp atomic read
#endif
      lfound = shared_found[0];
      if (lfound < 0)
        break;
    }
  }
}

/**
 * @brief Find given number of elements at maximum that matches given
 * conditional in 3D rectilinear domain
 *
 * @param mx X-axis size
 * @param my Y-axis size
 * @param mz Z-axis size
 * @param stmx X- stencil size
 * @param stmy Y- stencil size
 * @param stmz Z- stencil size
 * @param stpx X+ stencil size
 * @param stpy Y+ stencil size
 * @param stpz Z+ stencil size
 * @param maxfind Number of elements to find
 * @param func tester function
 * @param arg Extra argument for func
 * @retval -1 error occured
 * @return Number of found elements
 *
 * @note Although arg is not const, but you should not modify the content. If
 * you want to modify, ensure to use lock for OpenMP enabled context.
 *
 * @note func() will only be called for some part of given region,
 * not all.
 *
 * If OpenMP is enabled, this function will not be fully inlined.
 *
 * This function does not create array. If needed, create it by your own and
 * pass through @p arg.
 */
static inline ptrdiff_t
struct_domain_find_n_if(const int mx, const int my, const int mz,       //
                        const int stmx, const int stmy, const int stmz, //
                        const int stpx, const int stpy, const int stpz, //
                        ptrdiff_t maxfind,
                        int (*const func)(ptrdiff_t jj, void *arg), void *arg)
{
  int nbg = 0;
  ptrdiff_t found = 0;

#ifdef _OPENMP
#pragma omp parallel
#endif
  {
    struct_domain_find_n_p_if(mx, my, mz, stmx, stmy, stmz, stpx, stpy, stpz,
                              maxfind, func, arg, &nbg, &found);
  }

  return found;
}

#ifdef __cplusplus
}
#endif

#endif
