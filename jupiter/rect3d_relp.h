#ifndef JUPITER_RECT3D_RELP_H
#define JUPITER_RECT3D_RELP_H

/**
 * @file Relative pointer for structured grid array, and utilties for
 *       interpolation
 */

#include "struct.h"
#include "common_util.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @p voffset can be negative. In this case, @p nbpoints will be the number of
 * elements **after** current position, and @p nbpoints will be the number of
 * elements **before** current position.
 *
 * * If @p voffset is negative (ex. -1):
 *
 *       [0,......,c-1,c,c+1,....,n-1]
 *        `-nfpoints-'   `-nbpoints-'
 *
 * * If @p voffset is positive (ex. +1):
 *
 *       [0,......,c-1,c,c+1,....,n-1]
 *        `-nbpoints-'   `-nfpoints-'
 */
struct rect3d_relpointer_index
{
  ptrdiff_t voffset; ///< Offset for values on forward iteration
  int nfpoints;      ///< Number of available points in forward iteration
  int nbpoints;      ///< Number of available points in backward iteration
};

static inline int
rect3d_relpointer_index_movable(const struct rect3d_relpointer_index *idx,
                                int amount)
{
  if (amount > 0 && idx->nfpoints < amount)
    return 0;
  if (amount < 0 && idx->nbpoints < -amount)
    return 0;
  return 1;
}

/**
 * @return 1 if iteration direction is reversed (iterate towards the absolute
 *         index 0), 0 otherwise.
 */
static inline int
rect3d_relpointer_index_is_reverse(const struct rect3d_relpointer_index *idx)
{
  return idx->voffset < 0;
}

static inline ptrdiff_t
rect3d_relpointer_index__offset(const struct rect3d_relpointer_index *idx,
                                int amount)
{
  return idx->voffset * amount;
}

/**
 * returns offset for actual pointer for relative move
 */
static inline ptrdiff_t
rect3d_relpointer_index_move(struct rect3d_relpointer_index *idx, int amount)
{
  if (!rect3d_relpointer_index_movable(idx, amount))
    return 0;

  idx->nfpoints -= amount;
  idx->nbpoints += amount;
  return rect3d_relpointer_index__offset(idx, amount);
}

/**
 * returns offset for actual pointer for relative move, without moving index
 */
static inline ptrdiff_t
rect3d_relpointer_index_offset(const struct rect3d_relpointer_index *idx,
                               int amount)
{
  if (!rect3d_relpointer_index_movable(idx, amount))
    return 0;

  return rect3d_relpointer_index__offset(idx, amount);
}

static inline int
rect3d_relpointer_index_absmovable(const struct rect3d_relpointer_index *idx,
                                   int amount)
{
  if (!rect3d_relpointer_index_is_reverse(idx))
    return rect3d_relpointer_index_movable(idx, amount);
  return rect3d_relpointer_index_movable(idx, -amount);
}

/**
 * returns offset for actual pointer for absolute move (ignores signess of
 * voffset value)
 */
static inline ptrdiff_t
rect3d_relpointer_index_absmove(struct rect3d_relpointer_index *idx, int amount)
{
  if (!rect3d_relpointer_index_is_reverse(idx))
    return rect3d_relpointer_index_move(idx, amount);
  return rect3d_relpointer_index_move(idx, -amount);
}

static inline ptrdiff_t
rect3d_relpointer_index_absoffset(const struct rect3d_relpointer_index *idx,
                                  int amount)
{
  if (!rect3d_relpointer_index_is_reverse(idx))
    return rect3d_relpointer_index_offset(idx, amount);
  return rect3d_relpointer_index_offset(idx, -amount);
}

static inline int
rect3d_relpointer_index_nfwdpts(const struct rect3d_relpointer_index *idx)
{
  return idx->nfpoints;
}

static inline int
rect3d_relpointer_index_nbkwdpts(const struct rect3d_relpointer_index *idx)
{
  return idx->nbpoints;
}

static inline int
rect3d_relpointer_index_npospts(const struct rect3d_relpointer_index *idx)
{
  if (!rect3d_relpointer_index_is_reverse(idx))
    return rect3d_relpointer_index_nfwdpts(idx);
  return rect3d_relpointer_index_nbkwdpts(idx);
}

static inline int
rect3d_relpointer_index_nnegpts(const struct rect3d_relpointer_index *idx)
{
  if (!rect3d_relpointer_index_is_reverse(idx))
    return rect3d_relpointer_index_nbkwdpts(idx);
  return rect3d_relpointer_index_nfwdpts(idx);
}

/**
 * Flip iteration direction of pointer
 */
static inline void
rect3d_relpointer_index_flip(struct rect3d_relpointer_index *idx)
{
  int i = idx->nfpoints;
  idx->nfpoints = idx->nbpoints;
  idx->nbpoints = i;
  idx->voffset = -idx->voffset;
}

/**
 * Returns pointer with flipped iteration direction
 */
static inline struct rect3d_relpointer_index
rect3d_relpointer_index_flipped(const struct rect3d_relpointer_index *idx)
{
  struct rect3d_relpointer_index n;
  n.nbpoints = idx->nfpoints;
  n.nfpoints = idx->nbpoints;
  n.voffset = -idx->voffset;
  return n;
}

/**
 * Set by actual parameter
 */
static inline struct rect3d_relpointer_index
rect3d_relpointer_index_r(ptrdiff_t voff, int nf, int nb)
{
  return (struct rect3d_relpointer_index){.voffset = voff,
                                          .nfpoints = nf,
                                          .nbpoints = nb};
}

/**
 * Set by number of points and current index of [0, n)
 *
 * @param voff Axis offset (must be positive)
 * @param joff Array index offset (positive for forward, negative for backward)
 * @param i index
 * @param is first index (inclusive)
 * @param ie last index (inclusive)
 * @param n number of points
 */
static inline struct rect3d_relpointer_index
rect3d_relpointer_index_c(ptrdiff_t voff, int joff, int i, int is, int ie)
{
  int na, nb;

  voff *= joff;

  if (joff < 0)
    joff = -joff;

  na = (ie - i) / joff;
  nb = (i - is) / joff;

  if (voff >= 0)
    return rect3d_relpointer_index_r(voff, na, nb);
  return rect3d_relpointer_index_r(voff, nb, na);
}

/* rect3d_relpointer_index_[XYZ] are arbitrary range of array */
static inline struct rect3d_relpointer_index
rect3d_relpointer_index_X(domain *cdo, int joff, int jx, int is, int ie)
{
  return rect3d_relpointer_index_c(1, joff, jx, is, ie);
}

static inline struct rect3d_relpointer_index
rect3d_relpointer_index_Y(domain *cdo, int joff, int jy, int is, int ie)
{
  return rect3d_relpointer_index_c(cdo->mx, joff, jy, is, ie);
}

static inline struct rect3d_relpointer_index
rect3d_relpointer_index_Z(domain *cdo, int joff, int jz, int is, int ie)
{
  return rect3d_relpointer_index_c(cdo->mxy, joff, jz, is, ie);
}

static inline struct rect3d_relpointer_index
rect3d_relpointer_index_C(domain *cdo, int joff, int icompo, int is, int ie)
{
  return rect3d_relpointer_index_c(cdo->m, joff, icompo, is, ie);
}

/* rect3d_relpointer_index_[XYZ]m are for full range of array */
static inline struct rect3d_relpointer_index
rect3d_relpointer_index_Xm(domain *cdo, int joff, int jx)
{
  return rect3d_relpointer_index_X(cdo, joff, jx, 0, cdo->mx - 1);
}

static inline struct rect3d_relpointer_index
rect3d_relpointer_index_Ym(domain *cdo, int joff, int jy)
{
  return rect3d_relpointer_index_Y(cdo, joff, jy, 0, cdo->my - 1);
}

static inline struct rect3d_relpointer_index
rect3d_relpointer_index_Zm(domain *cdo, int joff, int jz)
{
  return rect3d_relpointer_index_Z(cdo, joff, jz, 0, cdo->mz - 1);
}

/* rect3d_relpointer_index_[XYZ]c are stencil range of array on cell center */
static inline struct rect3d_relpointer_index
rect3d_relpointer_index_Xc(domain *cdo, int joff, int jx, int stm, int stp)
{
  return rect3d_relpointer_index_X(cdo, joff, jx, cdo->stm - stm,
                                   cdo->stm + cdo->nx + stp - 1);
}

static inline struct rect3d_relpointer_index
rect3d_relpointer_index_Yc(domain *cdo, int joff, int jy, int stm, int stp)
{
  return rect3d_relpointer_index_Y(cdo, joff, jy, cdo->stm - stm,
                                   cdo->stm + cdo->ny + stp - 1);
}

static inline struct rect3d_relpointer_index
rect3d_relpointer_index_Zc(domain *cdo, int joff, int jz, int stm, int stp)
{
  return rect3d_relpointer_index_Z(cdo, joff, jz, cdo->stm - stm,
                                   cdo->stm + cdo->nz + stp - 1);
}

/* rect3d_relpointer_index_[XYZ]v are stencil range of array on cell vertex */
static inline struct rect3d_relpointer_index
rect3d_relpointer_index_Xv(domain *cdo, int joff, int jx, int stm, int stp)
{
  return rect3d_relpointer_index_X(cdo, joff, jx, cdo->stm - stm,
                                   cdo->stm + cdo->nx + stp);
}

static inline struct rect3d_relpointer_index
rect3d_relpointer_index_Yv(domain *cdo, int joff, int jy, int stm, int stp)
{
  return rect3d_relpointer_index_Y(cdo, joff, jy, cdo->stm - stm,
                                   cdo->stm + cdo->ny + stp);
}

static inline struct rect3d_relpointer_index
rect3d_relpointer_index_Zv(domain *cdo, int joff, int jz, int stm, int stp)
{
  return rect3d_relpointer_index_Z(cdo, joff, jz, cdo->stm - stm,
                                   cdo->stm + cdo->nz + stp);
}

/**
 * Pointer index with axis coordinate
 */
struct rect3d_relpointer_axis
{
  type *coords;      ///< Current coordinate
  ptrdiff_t coffset; ///< Offset for coords on forward iteration
  struct rect3d_relpointer_index index; ///< Index value
};

static inline int
rect3d_relpointer_axis_is_reverse(const struct rect3d_relpointer_axis *axis)
{
  return rect3d_relpointer_index_is_reverse(&axis->index);
}

/**
 * Number of available points in forward direction
 */
static inline int
rect3d_relpointer_axis_nfwdpts(const struct rect3d_relpointer_axis *axis)
{
  return rect3d_relpointer_index_nfwdpts(&axis->index);
}

/**
 * Number of available points in backward direction
 */
static inline int
rect3d_relpointer_axis_nbkwdpts(const struct rect3d_relpointer_axis *axis)
{
  return rect3d_relpointer_index_nbkwdpts(&axis->index);
}

/**
 * Number of available points in absolute positive direction
 */
static inline int
rect3d_relpointer_axis_npospts(const struct rect3d_relpointer_axis *axis)
{
  return rect3d_relpointer_index_npospts(&axis->index);
}

/**
 * Number of available points in absolute negative direction
 */
static inline int
rect3d_relpointer_axis_nnegpts(const struct rect3d_relpointer_axis *axis)
{
  return rect3d_relpointer_index_nnegpts(&axis->index);
}

static inline ptrdiff_t
rect3d_relpointer_axis_coffset(const struct rect3d_relpointer_axis *axis)
{
  return axis->coffset;
}

static inline ptrdiff_t
rect3d_relpointer_axis_voffset(const struct rect3d_relpointer_axis *axis)
{
  return axis->index.voffset;
}

static inline ptrdiff_t
rect3d_relpointer_axis_abscoffset(const struct rect3d_relpointer_axis *axis)
{
  if (!rect3d_relpointer_axis_is_reverse(axis))
    return axis->coffset;
  return -axis->coffset;
}

static inline ptrdiff_t
rect3d_relpointer_axis_absvoffset(const struct rect3d_relpointer_axis *axis)
{
  if (!rect3d_relpointer_axis_is_reverse(axis))
    return axis->index.voffset;
  return -axis->index.voffset;
}

/**
 * @brief Tests whether given pointer is movable by @p amount
 * @param axis rectilinear 3d grid relative pointer to move
 * @param amount Amount to move (positive for forward, negative for backward)
 * @return 0 if out of bounds, 1 if movable
 */
static inline int
rect3d_relpointer_axis_movable(const struct rect3d_relpointer_axis *axis,
                               int amount)
{
  return rect3d_relpointer_index_movable(&axis->index, amount);
}

/**
 * @brief Tests whether given pointer is movable by @p amount on gloabl dir
 * @param axis rectilinear 3d grid relative pointer to move
 * @param amount Amount to move (positive for + dir, negative for - dir)
 * @return 0 if out of bounds, 1 if movable
 */
static inline int
rect3d_relpointer_axis_absmovable(const struct rect3d_relpointer_axis *axis,
                                  int amount)
{
  return rect3d_relpointer_index_absmovable(&axis->index, amount);
}

static inline void
rect3d_relpointer_axis__cmove(struct rect3d_relpointer_axis *axis, int amount)
{
  axis->coords += axis->coffset * amount;
}

static inline void
rect3d_relpointer_axis__abscmove(struct rect3d_relpointer_axis *axis,
                                 int amount)
{
  if (rect3d_relpointer_axis_is_reverse(axis))
    amount = -amount;
  rect3d_relpointer_axis__cmove(axis, amount);
}

/**
 * Move pointer in specified direction of @p axis by @p amount
 * Returns offset for the array, 0 if reaching to limit (for non-0 @p amount).
 */
static inline ptrdiff_t
rect3d_relpointer_axis_move(struct rect3d_relpointer_axis *axis, int amount)
{
  ptrdiff_t off;

  if (amount == 0)
    return 0;

  off = rect3d_relpointer_index_move(&axis->index, amount);
  if (off != 0)
    rect3d_relpointer_axis__cmove(axis, amount);

  return off;
}

/**
 * Move pointer in abosolute direction (X+, Y+ or Z+ for positive) by @p amount
 * Returns offset for the array, 0 if reaching to limit (for non-0 @p amount).
 */
static inline ptrdiff_t
rect3d_relpointer_axis_absmove(struct rect3d_relpointer_axis *axis, int amount)
{
  if (!rect3d_relpointer_axis_is_reverse(axis))
    return rect3d_relpointer_axis_move(axis, amount);
  return rect3d_relpointer_axis_move(axis, -amount);
}

static inline ptrdiff_t
rect3d_relpointer_axis_offset(const struct rect3d_relpointer_axis *axis,
                              int amount)
{
  return rect3d_relpointer_index_offset(&axis->index, amount);
}

static inline ptrdiff_t
rect3d_relpointer_axis_absoffset(const struct rect3d_relpointer_axis *axis,
                                 int amount)
{
  return rect3d_relpointer_index_absoffset(&axis->index, amount);
}

static inline int rect3d_relpointer_axis__ndgetnvf(
  ptrdiff_t *off, const struct rect3d_relpointer_axis *axis, int amount,
  ptrdiff_t (*offsetter)(const struct rect3d_relpointer_axis *axis, int amount))
{
  ptrdiff_t offt;

  if (amount == 0)
    return 1;

  offt = offsetter(axis, amount);
  if (offt == 0)
    return 0;

  *off += offt;
  return 1;
}

/**
 * Reverse iteration direction
 */
static inline void
rect3d_relpointer_axis_flip(struct rect3d_relpointer_axis *axis)
{
  rect3d_relpointer_index_flip(&axis->index);
  axis->coffset = -axis->coffset;
}

/**
 * Returns the coordinate of current position
 */
static inline type
rect3d_relpointer_axis_coords(const struct rect3d_relpointer_axis *axis)
{
  return *axis->coords;
}

/**
 * Returns the coordinate at offset index
 */
static inline type
rect3d_relpointer_axis_ncoords(const struct rect3d_relpointer_axis *axis,
                               int amount)
{
  if (!rect3d_relpointer_axis_movable(axis, amount))
    return HUGE_VAL;

  return *(axis->coords + axis->coffset * amount);
}

static inline type
rect3d_relpointer_axis_absncoords(const struct rect3d_relpointer_axis *axis,
                                  int amount)
{
  if (!rect3d_relpointer_axis_is_reverse(axis))
    return rect3d_relpointer_axis_ncoords(axis, amount);
  return rect3d_relpointer_axis_ncoords(axis, -amount);
}

/**
 * Move to point that nearest to given coordinate @p c
 * Returns offset for moving main pointer.
 *
 * This function assumes the coordinate array is monotonically increasing.
 */
static inline ptrdiff_t
rect3d_relpointer_axis_move_nearest(struct rect3d_relpointer_axis *ptr, type c)
{
  struct rect3d_relpointer_index idx, i2;
  type *cbase, *ct;
  type cc, cp;
  int nb, nm, nn, np, isfwd;

  if (!isfinite(c))
    return 0;

  cc = rect3d_relpointer_axis_coords(ptr);
  if (cc == c)
    return 0;

  idx = ptr->index;
  idx.voffset = rect3d_relpointer_axis_coffset(ptr);
  cbase = ptr->coords;

  /* Make idx.voffset positive */
  if (rect3d_relpointer_axis_is_reverse(ptr))
    rect3d_relpointer_index_flip(&idx);

  /* Search for negative dir */
  if (c < cc)
    rect3d_relpointer_index_flip(&idx);

  nb = 0;
  nn = 0;
  np = rect3d_relpointer_index_nfwdpts(&idx);
  isfwd = !rect3d_relpointer_index_is_reverse(&idx);

  cp = isfwd ? -HUGE_VAL : HUGE_VAL;

  while (nn != np) {
    nm = (nn + np) / 2;
    if (nb != nm) {
      cbase += rect3d_relpointer_index_move(&idx, nm - nb);
      nb = nm;
    }
    cc = *cbase;
    if (isfwd ? c < cc : c > cc) {
      i2 = idx;
      cp = cc;
      cc = *(cbase + rect3d_relpointer_index_move(&i2, -1));
      if (isfwd ? c < cc : c > cc) {
        np = nm;
      } else {
        cbase += rect3d_relpointer_index_move(&idx, -1);
        break;
      }
    } else {
      i2 = idx;
      cp = *(cbase + rect3d_relpointer_index_move(&i2, 1));
      if (isfwd ? c >= cp : c <= cp) {
        if (nn == nm)
          break;
        nn = nm;
      } else {
        break;
      }
    }
  }

  if (isfwd) {
    if (cc >= cp)
      return PTRDIFF_MIN;

    if (c > cp || c - cc > cp - c)
      cbase += rect3d_relpointer_index_move(&idx, 1);
  } else {
    if (cp >= cc)
      return PTRDIFF_MAX;

    if (c < cp || c - cp <= cc - c)
      cbase += rect3d_relpointer_index_move(&idx, 1);
  }

  return rect3d_relpointer_axis_move(ptr, (cbase - ptr->coords) / ptr->coffset);
}

/**
 * return true if points same location.
 *
 * Use rect3d_relpointer_axis_in_same_axis() function to check for same axis
 */
static inline int
rect3d_relpointer_axis_eql(struct rect3d_relpointer_axis *pntr1,
                           struct rect3d_relpointer_axis *pntr2)
{
  if (pntr1 == pntr2)
    return 1;

  return pntr1->coords == pntr2->coords;
}

/**
 * @param pntr1 Axis data for vptr1
 * @param pntr2 Axis data for vptr2
 * @param vpntr1 Pointer1 to variable
 * @param vpntr2 Pointer2 to variable
 * @param unit_size Element size of @p vpntr1 and @p vpntr2.
 * @return returns true if pointers points in same axis
 *
 * @retval 0  Two pointers are not in same axis (or points different variable)
 * @retval 1  **Zero** or more forward move operations on @p pntr1 can reach @p
 *            pntr2.
 * @retval -1 One or more backward move operations on @p pntr1 can reach @p
 *            pntr2.
 *
 * If least one of @p vpntr1 or @p vpntr2 are NULL, alignment check will not
 * be performed.
 *
 * @note Its iteration direction is tested by the offset value.
 */
static inline int rect3d_relpointer_axis_in_same_axis(
  struct rect3d_relpointer_axis *pntr1, struct rect3d_relpointer_axis *pntr2,
  const void *vptr1, const void *vptr2, ptrdiff_t unit_size)
{
  ptrdiff_t ac1, ac2, avo1, avo2;
  int np1, np2;
  const char *cptr1 = vptr1;
  const char *cptr2 = vptr2;

  if (pntr1->coords > pntr2->coords)
    return -rect3d_relpointer_axis_in_same_axis(pntr2, pntr1, vptr2, vptr1,
                                                unit_size);

  /*
   * Assume pntr1->coords <= pntr2->coords && cptr1 <= cptr2.
   *
   * Otherwise, we assume these pointers point different array
   */
  if (cptr1 && cptr2 && cptr1 > cptr2)
    return 0;

  ac1 = rect3d_relpointer_axis_abscoffset(pntr1);
  np1 = rect3d_relpointer_axis_npospts(pntr1);
  if (pntr1->coords + ac1 * np1 < pntr2->coords)
    return 0; /* pntr2 exists further than boundary of pntr1 */

  ac2 = rect3d_relpointer_axis_abscoffset(pntr2);
  np2 = rect3d_relpointer_axis_nnegpts(pntr2);
  if (pntr2->coords - ac2 * np2 > pntr1->coords)
    return 0; /* pntr1 exists further than boundary of pntr2 */

  if (ac1 > 1 && (pntr2->coords - pntr1->coords) % ac1 != 0)
    return 0; /* pntr1 + ac1 * n never catch pntr2. */

  avo1 = rect3d_relpointer_axis_absvoffset(pntr1);
  avo2 = rect3d_relpointer_axis_absvoffset(pntr2);
  if (avo1 != avo2)
    return 0; /* different axis? */

  if (!cptr1 || !cptr2 || cptr1 == cptr2)
    return 1;

  if (cptr1 + avo1 * unit_size * np1 < cptr2)
    return 0; /* cptr2 exists further than iteration limit of cptr1 */

  if (cptr2 - avo2 * unit_size * np2 > cptr1)
    return 0; /* cptr1 exists further than iteration limit of cptr2 */

  if ((cptr2 - cptr1) % (avo1 * unit_size) != 0)
    return 0; /* cptr1 + avo1 * n never catch cptr2 */

  return 1;
}

static inline type rect3d_relpointer_axis__distance(
  const struct rect3d_relpointer_axis *axis, int off,
  ptrdiff_t (*mover)(struct rect3d_relpointer_axis *ax, int amount))
{
  type a, b;
  struct rect3d_relpointer_axis ax = *axis;
  if (mover(&ax, off) == 0) {
    if (off == 0)
      return 0.0;
    return HUGE_VAL;
  }

  a = rect3d_relpointer_axis_coords(axis);
  b = rect3d_relpointer_axis_coords(&ax);
  return b - a;
}

/**
 * Signed distance. If the offset point locates at negative direction, returns
 * negative value.
 */
static inline type
rect3d_relpointer_axis_sdistance(const struct rect3d_relpointer_axis *axis,
                                 int off)
{
  return rect3d_relpointer_axis__distance(axis, off,
                                          rect3d_relpointer_axis_move);
}

static inline type
rect3d_relpointer_axis_distance(const struct rect3d_relpointer_axis *axis,
                                int off)
{
  return fabs(rect3d_relpointer_axis_sdistance(axis, off));
}

static inline type
rect3d_relpointer_axis_abssdistance(const struct rect3d_relpointer_axis *axis,
                                    int off)
{
  return rect3d_relpointer_axis__distance(axis, off,
                                          rect3d_relpointer_axis_absmove);
}

static inline type
rect3d_relpointer_axis_absdistance(const struct rect3d_relpointer_axis *axis,
                                   int off)
{
  return fabs(rect3d_relpointer_axis_abssdistance(axis, off));
}

static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_r(type *coords, ptrdiff_t coffset,
                         struct rect3d_relpointer_index index)
{
  return (struct rect3d_relpointer_axis){.coords = coords,
                                         .coffset = coffset,
                                         .index = index};
}

/**
 * Convention of `rect3d_relpointer_axis_123(...)`
 *
 * 1. Axis to set (selects array of coordinate): X, Y or Z
 * 2. Topology to set (also selects array of coordinate)
 *    - c: cell center
 *    - v: cell vertex
 * 3. Parameter type
 *    - i: Externally provided index (cdo, joff, jx, idx)
 *    - m: Setup index for array limit specified by cdo (cdo, joff, jx)
 *    - (none): Setup index based on spencil size (cdo, joff, jx, stm, stp)
 *
 * @param cdo JUPITER Domain info
 * @param joff Iteration offset
 * @param jx X axis index (0 for lowest limit of array)
 * @param jy Y axis index
 * @param jz Z axis index
 * @param idx Index data to set
 * @param stm Additional stencil size for - side
 * @param stp Additional stencil size for + side
 * @param idx Array index info to set
 *
 * @note The consistency between @p idx and other parameters will not be
 *       checked.
 *
 * @note The validity of index and size parameters will not be checked.
 */

static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Xci(domain *cdo, int joff, int jx,
                           struct rect3d_relpointer_index idx)
{
  return rect3d_relpointer_axis_r(&cdo->xc[jx], joff, idx);
}

static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Xvi(domain *cdo, int joff, int jx,
                           struct rect3d_relpointer_index idx)
{
  return rect3d_relpointer_axis_r(&cdo->xv[jx], joff, idx);
}

static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Yci(domain *cdo, int joff, int jy,
                           struct rect3d_relpointer_index idx)
{
  return rect3d_relpointer_axis_r(&cdo->yc[jy], joff, idx);
}

static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Yvi(domain *cdo, int joff, int jy,
                           struct rect3d_relpointer_index idx)
{
  return rect3d_relpointer_axis_r(&cdo->yv[jy], joff, idx);
}

static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Zci(domain *cdo, int joff, int jz,
                           struct rect3d_relpointer_index idx)
{
  return rect3d_relpointer_axis_r(&cdo->zc[jz], joff, idx);
}

static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Zvi(domain *cdo, int joff, int jz,
                           struct rect3d_relpointer_index idx)
{
  return rect3d_relpointer_axis_r(&cdo->zv[jz], joff, idx);
}

static inline struct rect3d_relpointer_axis rect3d_relpointer_axis__m(
  domain *cdo, int joff, int j,
  struct rect3d_relpointer_axis (*fi)(domain *cdo, int joff, int j,
                                      struct rect3d_relpointer_index idx),
  struct rect3d_relpointer_index (*i)(domain *cdo, int joff, int j))
{
  return fi(cdo, joff, j, i(cdo, joff, j));
}

static inline struct rect3d_relpointer_axis rect3d_relpointer_axis__cv(
  domain *cdo, int joff, int j, int stm, int stp,
  struct rect3d_relpointer_axis (*fi)(domain *cdo, int joff, int j,
                                      struct rect3d_relpointer_index idx),
  struct rect3d_relpointer_index (*i)(domain *cdo, int joff, int j, int stm,
                                      int stp))
{
  return fi(cdo, joff, j, i(cdo, joff, j, stm, stp));
}

#define DEFINE_RECT3D_RELPOINTER_AXIS_M(axis, cv, jvar)                    \
  static inline struct rect3d_relpointer_axis                              \
    rect3d_relpointer_axis_##axis##cv##m(domain *cdo, int joff, int jvar)  \
  {                                                                        \
    return rect3d_relpointer_axis__m(cdo, joff, jvar,                      \
                                     rect3d_relpointer_axis_##axis##cv##i, \
                                     rect3d_relpointer_index_##axis##m);   \
  }

#define DEFINE_RECT3D_RELPOINTER_AXIS_CV(axis, cv, jvar)                    \
  static inline struct rect3d_relpointer_axis                               \
    rect3d_relpointer_axis_##axis##cv(domain *cdo, int joff, int jvar,      \
                                      int stm, int stp)                     \
  {                                                                         \
    return rect3d_relpointer_axis__cv(cdo, joff, jvar, stm, stp,            \
                                      rect3d_relpointer_axis_##axis##cv##i, \
                                      rect3d_relpointer_index_##axis##cv);  \
  }

DEFINE_RECT3D_RELPOINTER_AXIS_M(X, c, jx)
DEFINE_RECT3D_RELPOINTER_AXIS_M(X, v, jx)
DEFINE_RECT3D_RELPOINTER_AXIS_M(Y, c, jy)
DEFINE_RECT3D_RELPOINTER_AXIS_M(Y, v, jy)
DEFINE_RECT3D_RELPOINTER_AXIS_M(Z, c, jz)
DEFINE_RECT3D_RELPOINTER_AXIS_M(Z, v, jz)

DEFINE_RECT3D_RELPOINTER_AXIS_CV(X, c, jx)
DEFINE_RECT3D_RELPOINTER_AXIS_CV(X, v, jx)
DEFINE_RECT3D_RELPOINTER_AXIS_CV(Y, c, jy)
DEFINE_RECT3D_RELPOINTER_AXIS_CV(Y, v, jy)
DEFINE_RECT3D_RELPOINTER_AXIS_CV(Z, c, jz)
DEFINE_RECT3D_RELPOINTER_AXIS_CV(Z, v, jz)

static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Xcm(domain *cdo, int joff, int jx);
static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Ycm(domain *cdo, int joff, int jy);
static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Zcm(domain *cdo, int joff, int jz);
static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Xvm(domain *cdo, int joff, int jx);
static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Yvm(domain *cdo, int joff, int jy);
static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Zvm(domain *cdo, int joff, int jz);

static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Xc(domain *cdo, int joff, int jx, int stm, int stp);
static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Yc(domain *cdo, int joff, int jy, int stm, int stp);
static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Zc(domain *cdo, int joff, int jz, int stm, int stp);
static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Xv(domain *cdo, int joff, int jx, int stm, int stp);
static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Yv(domain *cdo, int joff, int jy, int stm, int stp);
static inline struct rect3d_relpointer_axis
rect3d_relpointer_axis_Zv(domain *cdo, int joff, int jz, int stm, int stp);

/**
 * Pointer to be movable in one direction
 */
struct rect3d1p
{
  type *values;                       ///< current point value
  struct rect3d_relpointer_axis axis; ///< pointer of movable direction
};

static inline int rect3d1p_is_reverse(const struct rect3d1p *pntr)
{
  return rect3d_relpointer_axis_is_reverse(&pntr->axis);
}

static inline int rect3d1p_move_base(
  struct rect3d1p *pntr, int amount,
  ptrdiff_t (*mover)(struct rect3d_relpointer_axis *axis, int amount))
{
  ptrdiff_t off;

  off = mover(&pntr->axis, amount);
  if (amount != 0 && off == 0)
    return 0;

  pntr->values += off;
  return 1;
}

static inline int rect3d1p_move(struct rect3d1p *pntr, int amount)
{
  return rect3d1p_move_base(pntr, amount, rect3d_relpointer_axis_move);
}

static inline int rect3d1p_absmove(struct rect3d1p *pntr, int amount)
{
  return rect3d1p_move_base(pntr, amount, rect3d_relpointer_axis_absmove);
}

/**
 * returns true (1) if pointers points same location.
 *
 * Iteration direction is not considered.
 */
static inline int rect3d1p_eql(struct rect3d1p *pntr1, struct rect3d1p *pntr2)
{
  if (pntr1 == pntr2)
    return 1;
  return pntr1->values == pntr2->values;
}

/**
 * returns true if pointers points in same axis
 *
 * @retval 0  Two pointers are not in same axis (or points different variable)
 * @retval 1  **Zero** or more number of repeat call of
 *            rect3d1p_move(pntr1, 1) can reach @p pntr2
 * @retval -1 One or more number of repeat call of
 *            rect3d1p_move(pntr1, -1) can reach @p pntr2.
 *
 * @note This function return false (0) their iteration direction is different,
 *       even if they point same location (rect3d1p_eql() will true).
 *
 * @note Its iteration direction is tested by the offset value.
 */
static inline int rect3d1p_in_same_axis(struct rect3d1p *pntr1,
                                        struct rect3d1p *pntr2)
{
  return rect3d_relpointer_axis_in_same_axis(&pntr1->axis, &pntr2->axis,
                                             pntr1->values, pntr2->values,
                                             sizeof(*pntr1->values));
}

/**
 * Move pointer to the nearest point to @p c
 */
static inline int rect3d1p_move_nearest(struct rect3d1p *ret, type c)
{
  ret->values += rect3d_relpointer_axis_move_nearest(&ret->axis, c);
  return 1;
}

static inline int rect3d1p_movable(const struct rect3d1p *pntr, int amount)
{
  return rect3d_relpointer_axis_movable(&pntr->axis, amount);
}

static inline int rect3d1p_absmovable(const struct rect3d1p *pntr, int amount)
{
  return rect3d_relpointer_axis_absmovable(&pntr->axis, amount);
}

static inline void rect3d1p_flip(struct rect3d1p *pntr)
{
  rect3d_relpointer_axis_flip(&pntr->axis);
}

/**
 * Get pointing value
 */
static inline type rect3d1p_getv(const struct rect3d1p *pntr)
{
  return *pntr->values;
}

/**
 * Get pointing coordinate
 */
static inline type rect3d1p_getc(const struct rect3d1p *pntr)
{
  return rect3d_relpointer_axis_coords(&pntr->axis);
}

static inline type rect3d1p__getnv(
  const struct rect3d1p *pntr, int amount,
  ptrdiff_t (*offsetter)(const struct rect3d_relpointer_axis *ax, int amount))
{
  ptrdiff_t off;

  off = 0;
  if (!rect3d_relpointer_axis__ndgetnvf(&off, &pntr->axis, amount, offsetter))
    return HUGE_VAL;

  return *(pntr->values + off);
}

/**
 * Get neighbor pointing value
 */
static inline type rect3d1p_getnv(const struct rect3d1p *pntr, int off)
{
  return rect3d1p__getnv(pntr, off, rect3d_relpointer_axis_offset);
}

/**
 * Get neighbor pointing value with offset in absolute direction
 */
static inline type rect3d1p_getabsnv(const struct rect3d1p *pntr, int off)
{
  return rect3d1p__getnv(pntr, off, rect3d_relpointer_axis_absoffset);
}

/**
 * Get neighbor pointing coordinate
 */
static inline type rect3d1p_getnc(const struct rect3d1p *pntr, int off)
{
  return rect3d_relpointer_axis_ncoords(&pntr->axis, off);
}

/**
 * Get neighbor pointing coordinate with offset in absolute direction
 */
static inline type rect3d1p_getabsnc(const struct rect3d1p *pntr, int off)
{
  return rect3d_relpointer_axis_absncoords(&pntr->axis, off);
}

/**
 * Get distance to offset point
 */
static inline type rect3d1p_getnd(const struct rect3d1p *pntr, int off)
{
  return rect3d_relpointer_axis_distance(&pntr->axis, off);
}

static inline type rect3d1p_getabsnd(const struct rect3d1p *pntr, int off)
{
  return rect3d_relpointer_axis_absdistance(&pntr->axis, off);
}

/**
 * Get signed distance to offset point
 */
static inline type rect3d1p_getnsd(const struct rect3d1p *pntr, int off)
{
  return rect3d_relpointer_axis_sdistance(&pntr->axis, off);
}

static inline type rect3d1p_getabsnsd(const struct rect3d1p *pntr, int off)
{
  return rect3d_relpointer_axis_abssdistance(&pntr->axis, off);
}

/**
 * Set pointing value
 */
static inline void rect3d1p_setv(const struct rect3d1p *pntr, type v)
{
  *pntr->values = v;
}

static inline struct rect3d1p rect3d1p_r(type *values,
                                         struct rect3d_relpointer_axis axis)
{
  return (struct rect3d1p){.values = values, .axis = axis};
}

static inline struct rect3d1p rect3d1p_b(type *base, int jx, int jy, int jz, //
                                         int mx, int my, int mz,
                                         struct rect3d_relpointer_axis axis)
{
  return rect3d1p_r(&base[calc_address(jx, jy, jz, mx, my, mz)], axis);
}

static inline struct rect3d1p
rect3d1p__m(type *base, int jx, int jy, int jz, domain *cdo, int joff, int j,
            struct rect3d_relpointer_axis (*f)(domain *cdo, int joff, int j))
{
  return rect3d1p_b(base, jx, jy, jz, cdo->mx, cdo->my, cdo->mz,
                    f(cdo, joff, j));
}

static inline struct rect3d1p
rect3d1p__cv(type *base, int jx, int jy, int jz, domain *cdo, int joff, int j,
             int stm, int stp,
             struct rect3d_relpointer_axis (*fi)(domain *cdo, int joff, int j,
                                                 int stm, int stp))
{
  return rect3d1p_b(base, jx, jy, jz, cdo->mx, cdo->my, cdo->mz,
                    fi(cdo, joff, j, stm, stp));
}

#define DEFINE_RECT3D1P_M(axis, cv, jvar)                                     \
  static inline struct rect3d1p rect3d1p_##axis##cv##m(type *base, int jx,    \
                                                       int jy, int jz,        \
                                                       domain *cdo, int joff) \
  {                                                                           \
    return rect3d1p__m(base, jx, jy, jz, cdo, joff, jvar,                     \
                       rect3d_relpointer_axis_##axis##cv##m);                 \
  }

#define DEFINE_RECT3D1P_CV(axis, cv, jvar)                                 \
  static inline struct rect3d1p rect3d1p_##axis##cv(type *base, int jx,    \
                                                    int jy, int jz,        \
                                                    domain *cdo, int joff, \
                                                    int stm, int stp)      \
  {                                                                        \
    return rect3d1p__cv(base, jx, jy, jz, cdo, joff, jvar, stm, stp,       \
                        rect3d_relpointer_axis_##axis##cv);                \
  }

DEFINE_RECT3D1P_M(X, c, jx)
DEFINE_RECT3D1P_M(X, v, jx)
DEFINE_RECT3D1P_M(Y, c, jy)
DEFINE_RECT3D1P_M(Y, v, jy)
DEFINE_RECT3D1P_M(Z, c, jz)
DEFINE_RECT3D1P_M(Z, v, jz)

DEFINE_RECT3D1P_CV(X, c, jx)
DEFINE_RECT3D1P_CV(X, v, jx)
DEFINE_RECT3D1P_CV(Y, c, jy)
DEFINE_RECT3D1P_CV(Y, v, jy)
DEFINE_RECT3D1P_CV(Z, c, jz)
DEFINE_RECT3D1P_CV(Z, v, jz)

static inline struct rect3d1p rect3d1p_Xcm(type *base, int jx, int jy, int jz,
                                           domain *cdo, int joff);

static inline struct rect3d1p rect3d1p_Ycm(type *base, int jx, int jy, int jz,
                                           domain *cdo, int joff);

static inline struct rect3d1p rect3d1p_Zcm(type *base, int jx, int jy, int jz,
                                           domain *cdo, int joff);

static inline struct rect3d1p rect3d1p_Xvm(type *base, int jx, int jy, int jz,
                                           domain *cdo, int joff);

static inline struct rect3d1p rect3d1p_Yvm(type *base, int jx, int jy, int jz,
                                           domain *cdo, int joff);

static inline struct rect3d1p rect3d1p_Zvm(type *base, int jx, int jy, int jz,
                                           domain *cdo, int joff);

static inline struct rect3d1p rect3d1p_Xc(type *base, int jx, int jy, int jz,
                                          domain *cdo, int joff, int stm,
                                          int stp);

static inline struct rect3d1p rect3d1p_Yc(type *base, int jx, int jy, int jz,
                                          domain *cdo, int joff, int stm,
                                          int stp);

static inline struct rect3d1p rect3d1p_Zc(type *base, int jx, int jy, int jz,
                                          domain *cdo, int joff, int stm,
                                          int stp);

static inline struct rect3d1p rect3d1p_Xv(type *base, int jx, int jy, int jz,
                                          domain *cdo, int joff, int stm,
                                          int stp);

static inline struct rect3d1p rect3d1p_Yv(type *base, int jx, int jy, int jz,
                                          domain *cdo, int joff, int stm,
                                          int stp);

static inline struct rect3d1p rect3d1p_Zv(type *base, int jx, int jy, int jz,
                                          domain *cdo, int joff, int stm,
                                          int stp);

/**
 * Movable in one direction of space and components
 */
struct rect3d1cp
{
  struct rect3d1p relp;
  struct rect3d_relpointer_index component;
};

static inline int rect3d1cp_is_reverse(const struct rect3d1cp *pntr)
{
  return rect3d1p_is_reverse(&pntr->relp);
}

static inline int rect3d1cp_is_reverseC(const struct rect3d1cp *pntr)
{
  return rect3d_relpointer_index_is_reverse(&pntr->component);
}

/**
 * Move in axis
 */
static inline int rect3d1cp_move(struct rect3d1cp *pntr, int amount)
{
  return rect3d1p_move(&pntr->relp, amount);
}

static inline int rect3d1cp_absmove(struct rect3d1cp *pntr, int amount)
{
  return rect3d1p_absmove(&pntr->relp, amount);
}

static inline int rect3d1cp_move_nearest(struct rect3d1cp *pntr, type c)
{
  return rect3d1p_move_nearest(&pntr->relp, c);
}

/**
 * Move in component
 */
static inline int rect3d1cp_moveC_base(
  struct rect3d1cp *pntr, int amount,
  ptrdiff_t (*mover)(struct rect3d_relpointer_index *index, int amount))
{
  ptrdiff_t off;

  off = mover(&pntr->component, amount);
  if (amount != 0 && off == 0)
    return 1;

  pntr->relp.values += off;
  return 0;
}

static inline int rect3d1cp_moveC(struct rect3d1cp *pntr, int amount)
{
  return rect3d1cp_moveC_base(pntr, amount, rect3d_relpointer_index_move);
}

static inline int rect3d1cp_absmoveC(struct rect3d1cp *pntr, int amount)
{
  return rect3d1cp_moveC_base(pntr, amount, rect3d_relpointer_index_absmove);
}

static inline type rect3d1cp_getv(struct rect3d1cp *pntr)
{
  return rect3d1p_getv(&pntr->relp);
}

static inline type rect3d1cp_getc(struct rect3d1cp *pntr)
{
  return rect3d1p_getc(&pntr->relp);
}

static inline type rect3d1cp_getnv(struct rect3d1cp *pntr, int off)
{
  return rect3d1p_getnv(&pntr->relp, off);
}

static inline type rect3d1cp_getnc(struct rect3d1cp *pntr, int off)
{
  return rect3d1p_getnc(&pntr->relp, off);
}

static inline type rect3d1cp_getnd(struct rect3d1cp *pntr, int off)
{
  return rect3d1p_getnd(&pntr->relp, off);
}

static inline type rect3d1cp_getnsd(struct rect3d1cp *pntr, int off)
{
  return rect3d1p_getnsd(&pntr->relp, off);
}

static inline type rect3d1cp_getabsnv(struct rect3d1cp *pntr, int off)
{
  return rect3d1p_getabsnv(&pntr->relp, off);
}

static inline type rect3d1cp_getabsnc(struct rect3d1cp *pntr, int off)
{
  return rect3d1p_getabsnc(&pntr->relp, off);
}

static inline type rect3d1cp_getabsnd(struct rect3d1cp *pntr, int off)
{
  return rect3d1p_getabsnd(&pntr->relp, off);
}

static inline type rect3d1cp_getabsnsd(struct rect3d1cp *pntr, int off)
{
  return rect3d1p_getabsnsd(&pntr->relp, off);
}

static inline void rect3d1cp_setv(struct rect3d1cp *pntr, type v)
{
  rect3d1p_setv(&pntr->relp, v);
}

/**
 * Move pointer to the component of the lowest index (higest if reversed)
 */
static inline int rect3d1cp_rewindC(struct rect3d1cp *pntr)
{
  int amount;

  amount = rect3d_relpointer_index_nbkwdpts(&pntr->component);
  return rect3d1cp_moveC(pntr, -amount);
}

/**
 * Extract values on all components (limited by @p ncompo)
 *
 * Returns the number of components that has been extracted
 */
static inline int rect3d1cp_extractC(struct rect3d1cp *pntr, type *c,
                                     int ncompo)
{
  int i;

  c[0] = rect3d1cp_getv(pntr);
  for (i = 1; i < ncompo; ++i) {
    if (!rect3d1cp_moveC(pntr, 1))
      break;

    c[i] = rect3d1cp_getv(pntr);
  }
  return i;
}

/**
 * Return number of components (size of iteration range)
 */
static inline int rect3d1cp_ncompo(struct rect3d1cp *pntr)
{
  return rect3d_relpointer_index_nfwdpts(&pntr->component) +
         rect3d_relpointer_index_nbkwdpts(&pntr->component) + 1;
}

/**
 * Return current index of component (0-based) on given iteration direction
 */
static inline int rect3d1cp_icompo(struct rect3d1cp *pntr)
{
  return rect3d_relpointer_index_nbkwdpts(&pntr->component);
}

/**
 * Return current index of component (0-based) on absolute iteration direction
 */
static inline int rect3d1cp_absicompo(struct rect3d1cp *pntr)
{
  return rect3d_relpointer_index_nnegpts(&pntr->component);
}

static inline struct rect3d1cp
rect3d1cp_r(struct rect3d1p relp, struct rect3d_relpointer_index component)
{
  return (struct rect3d1cp){.relp = relp, .component = component};
}

static inline struct rect3d1cp
rect3d1cp_c(type *base, int jx, int jy, int jz, int icompo, int mx, int my,
            int mz, int m, struct rect3d_relpointer_axis axis,
            struct rect3d_relpointer_index component)
{
  type *ptr;
  struct rect3d1p relp;

  ptr = &base[calc_address(jx, jy, jz, mx, my, mz) + icompo * m];
  relp = rect3d1p_r(ptr, axis);
  return rect3d1cp_r(relp, component);
}

static inline struct rect3d1cp
rect3d1cp__m(type *base, int jx, int jy, int jz, int icompo, domain *cdo,
             int joff, int j, int coff, int is, int ie,
             struct rect3d_relpointer_axis (*fa)(domain *cdo, int joff, int j),
             struct rect3d_relpointer_index (*fc)(domain *cdo, int joff, int j,
                                                  int is, int ie))
{
  return rect3d1cp_c(base, jx, jy, jz, icompo, cdo->mx, cdo->my, cdo->mz,
                     cdo->m, fa(cdo, joff, j), fc(cdo, coff, icompo, is, ie));
}

static inline struct rect3d1cp
rect3d1cp__cv(type *base, int jx, int jy, int jz, int icompo, domain *cdo,
              int joff, int j, int stm, int stp, int coff, int is, int ie,
              struct rect3d_relpointer_axis (*fa)(domain *cdo, int joff, int j,
                                                  int stm, int stp),
              struct rect3d_relpointer_index (*fc)(domain *cdo, int joff, int j,
                                                   int is, int ie))
{
  return rect3d1cp_c(base, jx, jy, jz, icompo, cdo->mx, cdo->my, cdo->mz,
                     cdo->m, fa(cdo, joff, j, stm, stp),
                     fc(cdo, coff, icompo, is, ie));
}

#define DEFINE_RECT3D1CP_M(axis, cv, jvar)                                   \
  static inline struct rect3d1cp rect3d1cp_##axis##cv##m(type *base, int jx, \
                                                         int jy, int jz,     \
                                                         int icompo,         \
                                                         domain *cdo,        \
                                                         int joff, int coff, \
                                                         int is, int ie)     \
  {                                                                          \
    return rect3d1cp__m(base, jx, jy, jz, icompo, cdo, joff, jvar, coff, is, \
                        ie, rect3d_relpointer_axis_##axis##cv##m,            \
                        rect3d_relpointer_index_C);                          \
  }

#define DEFINE_RECT3D1CP_CV(axis, cv, jvar)                                    \
  static inline struct rect3d1cp rect3d1cp_##axis##cv(type *base, int jx,      \
                                                      int jy, int jz,          \
                                                      int icompo, domain *cdo, \
                                                      int joff, int stm,       \
                                                      int stp, int coff,       \
                                                      int is, int ie)          \
  {                                                                            \
    return rect3d1cp__cv(base, jx, jy, jz, icompo, cdo, joff, jvar, stm, stp,  \
                         coff, is, ie, rect3d_relpointer_axis_##axis##cv,      \
                         rect3d_relpointer_index_C);                           \
  }

DEFINE_RECT3D1CP_M(X, c, jx)
DEFINE_RECT3D1CP_M(X, v, jx)
DEFINE_RECT3D1CP_M(Y, c, jy)
DEFINE_RECT3D1CP_M(Y, v, jy)
DEFINE_RECT3D1CP_M(Z, c, jz)
DEFINE_RECT3D1CP_M(Z, v, jz)

DEFINE_RECT3D1CP_CV(X, c, jx)
DEFINE_RECT3D1CP_CV(X, v, jx)
DEFINE_RECT3D1CP_CV(Y, c, jy)
DEFINE_RECT3D1CP_CV(Y, v, jy)
DEFINE_RECT3D1CP_CV(Z, c, jz)
DEFINE_RECT3D1CP_CV(Z, v, jz)

/**
 * @brief 2D-world relative pointer.
 *
 * You can assign any axes of X, Y, or Z to each 'relative' axes. These
 * 'relative' axes are just for the convention, no much meanings. How axes
 * should be assigned are defined by the function uses the pointer.
 */
struct rect3d2p
{
  type *values;
  struct rect3d_relpointer_axis right; ///< Right axis
  struct rect3d_relpointer_axis up;    ///< Up axis
};

static inline int rect3d2p_move_base(
  struct rect3d2p *p, int to_r, int to_u,
  ptrdiff_t (*mover)(struct rect3d_relpointer_axis *axes, int amount))
{
  ptrdiff_t off_r, off_u;

  off_r = mover(&p->right, to_r);
  off_u = mover(&p->up, to_u);

  p->values += off_r + off_u;

  if (to_r != 0 && off_r == 0)
    return 0;
  if (to_u != 0 && off_u == 0)
    return 0;
  return 1;
}

static inline int rect3d2p_move(struct rect3d2p *p, int to_r, int to_u)
{
  return rect3d2p_move_base(p, to_r, to_u, rect3d_relpointer_axis_move);
}

static inline int rect3d2p_moveR(struct rect3d2p *p, int to_r)
{
  return rect3d2p_move(p, to_r, 0);
}

static inline int rect3d2p_moveU(struct rect3d2p *p, int to_u)
{
  return rect3d2p_move(p, 0, to_u);
}

static inline int rect3d2p_absmove(struct rect3d2p *p, int to_r, int to_u)
{
  return rect3d2p_move_base(p, to_r, to_u, rect3d_relpointer_axis_absmove);
}

static inline int rect3d2p_absmoveR(struct rect3d2p *p, int to_r)
{
  return rect3d2p_absmove(p, to_r, 0);
}

static inline int rect3d2p_absmoveU(struct rect3d2p *p, int to_u)
{
  return rect3d2p_absmove(p, 0, to_u);
}

static inline int rect3d2p_move_nearest(struct rect3d2p *p, type r, type u)
{
  ptrdiff_t off;
  off = rect3d_relpointer_axis_move_nearest(&p->right, r);
  off += rect3d_relpointer_axis_move_nearest(&p->up, u);
  p->values += off;
  return 1;
}

static inline int rect3d2p_moveR_nearest(struct rect3d2p *p, type r)
{
  p->values += rect3d_relpointer_axis_move_nearest(&p->right, r);
  return 1;
}

static inline int rect3d2p_moveU_nearest(struct rect3d2p *p, type u)
{
  p->values += rect3d_relpointer_axis_move_nearest(&p->up, u);
  return 1;
}

static inline int rect3d2p_movable_base(
  const struct rect3d2p *p, int to_r, int to_u,
  int (*checker)(const struct rect3d_relpointer_axis *ax, int amount))
{
  return checker(&p->right, to_r) && checker(&p->up, to_u);
}

static inline int rect3d2p_movable(const struct rect3d2p *p, int to_r, int to_u)
{
  return rect3d2p_movable_base(p, to_r, to_u, rect3d_relpointer_axis_movable);
}

static inline int rect3d2p_movableR(const struct rect3d2p *p, int to_r)
{
  return rect3d2p_movable(p, to_r, 0);
}

static inline int rect3d2p_movableU(const struct rect3d2p *p, int to_u)
{
  return rect3d2p_movable(p, 0, to_u);
}

static inline int rect3d2p_absmovable(const struct rect3d2p *p, int to_r,
                                      int to_u)
{
  return rect3d2p_movable_base(p, to_r, to_u,
                               rect3d_relpointer_axis_absmovable);
}

static inline int rect3d2p_absmovableR(const struct rect3d2p *p, int to_r)
{
  return rect3d2p_absmovable(p, to_r, 0);
}

static inline int rect3d2p_absmovableU(const struct rect3d2p *p, int to_u)
{
  return rect3d2p_absmovable(p, 0, to_u);
}

static inline type rect3d2p_getcR(struct rect3d2p *p)
{
  return rect3d_relpointer_axis_coords(&p->right);
}

static inline type rect3d2p_getcU(struct rect3d2p *p)
{
  return rect3d_relpointer_axis_coords(&p->up);
}

static inline type rect3d2p_getv(struct rect3d2p *p) { return *p->values; }

static inline type rect3d2p__getnv(
  const struct rect3d2p *pntr, int to_r, int to_u,
  ptrdiff_t (*offsetter)(const struct rect3d_relpointer_axis *axis, int amount))
{
  ptrdiff_t off, offt;

  off = 0;
  if (!rect3d_relpointer_axis__ndgetnvf(&off, &pntr->right, to_r, offsetter))
    return HUGE_VAL;

  if (!rect3d_relpointer_axis__ndgetnvf(&off, &pntr->up, to_u, offsetter))
    return HUGE_VAL;

  return *(pntr->values + off);
}

static inline type rect3d2p_getnv(const struct rect3d2p *p, int to_r, int to_u)
{
  return rect3d2p__getnv(p, to_r, to_u, rect3d_relpointer_axis_offset);
}

static inline type rect3d2p_getnvR(const struct rect3d2p *p, int to_r)
{
  return rect3d2p_getnv(p, to_r, 0);
}

static inline type rect3d2p_getnvU(const struct rect3d2p *p, int to_u)
{
  return rect3d2p_getnv(p, 0, to_u);
}

static inline type rect3d2p_getncR(const struct rect3d2p *p, int to_r)
{
  return rect3d_relpointer_axis_ncoords(&p->right, to_r);
}

static inline type rect3d2p_getncU(const struct rect3d2p *p, int to_u)
{
  return rect3d_relpointer_axis_ncoords(&p->up, to_u);
}

static inline type rect3d2p_getabsnv(const struct rect3d2p *p, int to_r,
                                     int to_u)
{
  return rect3d2p__getnv(p, to_r, to_u, rect3d_relpointer_axis_absoffset);
}

static inline type rect3d2p_getabsnvR(const struct rect3d2p *p, int to_r)
{
  return rect3d2p_getabsnv(p, to_r, 0);
}

static inline type rect3d2p_getabsnvU(const struct rect3d2p *p, int to_u)
{
  return rect3d2p_getabsnv(p, 0, to_u);
}

static inline type rect3d2p_getabsncR(const struct rect3d2p *p, int to_r)
{
  return rect3d_relpointer_axis_absncoords(&p->right, to_r);
}

static inline type rect3d2p_getabsncU(const struct rect3d2p *p, int to_u)
{
  return rect3d_relpointer_axis_absncoords(&p->up, to_u);
}

static inline type
rect3d2p_getnd_base(const struct rect3d2p *p, int to_r, int to_u,
                    type (*d)(const struct rect3d_relpointer_axis *ax, int off),
                    type (*distf)(type dr, type du))
{
  type dr, du;
  dr = du = 0.0;
  if (to_r != 0)
    dr = d(&p->right, to_r);
  if (to_u != 0)
    du = d(&p->up, to_u);
  return distf(dr, du);
}

static inline type rect3d2p_taxicab(type dr, type du) { return dr + du; }

static inline type rect3d2p_length(type dr, type du)
{
  return sqrt(dr * dr + du * du);
}

static inline type rect3d2p_getndt(const struct rect3d2p *p, int to_r, int to_u)
{
  return rect3d2p_getnd_base(p, to_r, to_u, rect3d_relpointer_axis_distance,
                             rect3d2p_taxicab);
}

static inline type rect3d2p_getnd(const struct rect3d2p *p, int to_r, int to_u)
{
  return rect3d2p_getnd_base(p, to_r, to_u, rect3d_relpointer_axis_distance,
                             rect3d2p_length);
}

static inline type rect3d2p_getndR(const struct rect3d2p *p, int to_r)
{
  return rect3d_relpointer_axis_distance(&p->right, to_r);
}

static inline type rect3d2p_getndU(const struct rect3d2p *p, int to_u)
{
  return rect3d_relpointer_axis_distance(&p->up, to_u);
}

static inline type rect3d2p_getabsndt(const struct rect3d2p *p, int to_r,
                                      int to_u)
{
  return rect3d2p_getnd_base(p, to_r, to_u, rect3d_relpointer_axis_absdistance,
                             rect3d2p_taxicab);
}

static inline type rect3d2p_getabsnd(const struct rect3d2p *p, int to_r,
                                     int to_u)
{
  return rect3d2p_getnd_base(p, to_r, to_u, rect3d_relpointer_axis_absdistance,
                             rect3d2p_length);
}

static inline type rect3d2p_getabsndR(const struct rect3d2p *p, int to_r)
{
  return rect3d_relpointer_axis_absdistance(&p->right, to_r);
}

static inline type rect3d2p_getabsndU(const struct rect3d2p *p, int to_u)
{
  return rect3d_relpointer_axis_absdistance(&p->up, to_u);
}

static inline struct rect3d1p rect3d2p_to1R(const struct rect3d2p *p)
{
  return rect3d1p_r(p->values, p->right);
}

static inline struct rect3d1p rect3d2p_to1U(const struct rect3d2p *p)
{
  return rect3d1p_r(p->values, p->up);
}

static inline struct rect3d2p rect3d2p_c(type *values,
                                         struct rect3d_relpointer_axis raxis,
                                         struct rect3d_relpointer_axis uaxis)
{
  return ((struct rect3d2p){.values = values, .right = raxis, .up = uaxis});
}

static inline struct rect3d2p rect3d2p_b(type *base, int jx, int jy, int jz,
                                         int mx, int my, int mz,
                                         struct rect3d_relpointer_axis raxis,
                                         struct rect3d_relpointer_axis uaxis)
{
  return rect3d2p_c(&base[calc_address(jx, jy, jz, mx, my, mz)], raxis, uaxis);
}

static inline struct rect3d2p
rect3d2p__m(type *base, int jx, int jy, int jz, domain *cdo, int joff_r, int jr,
            int joff_u, int ju,
            struct rect3d_relpointer_axis (*fr)(domain *cdo, int joff, int j),
            struct rect3d_relpointer_axis (*fu)(domain *cdo, int joff, int j))
{
  return rect3d2p_b(base, jx, jy, jz, cdo->mx, cdo->my, cdo->mz,
                    fr(cdo, joff_r, jr), fu(cdo, joff_u, ju));
}

#define DEFINE_RECT3D2P_M(rax, uax, rcv, ucv, rv, uv)                        \
  static inline struct rect3d2p rect3d2p_##rax##uax##rcv##ucv##m(            \
    type *base, int jx, int jy, int jz, domain *cdo, int joff_r, int joff_u) \
  {                                                                          \
    return rect3d2p__m(base, jx, jy, jz, cdo, joff_r, rv, joff_u, uv,        \
                       rect3d_relpointer_axis_##rax##rcv##m,                 \
                       rect3d_relpointer_axis_##uax##ucv##m);                \
  }

DEFINE_RECT3D2P_M(X, Y, c, c, jx, jy)
DEFINE_RECT3D2P_M(X, Y, c, v, jx, jy)
DEFINE_RECT3D2P_M(X, Y, v, c, jx, jy)
DEFINE_RECT3D2P_M(X, Y, v, v, jx, jy)
DEFINE_RECT3D2P_M(Y, X, c, c, jy, jx)
DEFINE_RECT3D2P_M(Y, X, c, v, jy, jx)
DEFINE_RECT3D2P_M(Y, X, v, c, jy, jx)
DEFINE_RECT3D2P_M(Y, X, v, v, jy, jx)

DEFINE_RECT3D2P_M(X, Z, c, c, jx, jz)
DEFINE_RECT3D2P_M(X, Z, c, v, jx, jz)
DEFINE_RECT3D2P_M(X, Z, v, c, jx, jz)
DEFINE_RECT3D2P_M(X, Z, v, v, jx, jz)
DEFINE_RECT3D2P_M(Z, X, c, c, jz, jx)
DEFINE_RECT3D2P_M(Z, X, c, v, jz, jx)
DEFINE_RECT3D2P_M(Z, X, v, c, jz, jx)
DEFINE_RECT3D2P_M(Z, X, v, v, jz, jx)

DEFINE_RECT3D2P_M(Y, Z, c, c, jy, jz)
DEFINE_RECT3D2P_M(Y, Z, c, v, jy, jz)
DEFINE_RECT3D2P_M(Y, Z, v, c, jy, jz)
DEFINE_RECT3D2P_M(Y, Z, v, v, jy, jz)
DEFINE_RECT3D2P_M(Z, Y, c, c, jz, jy)
DEFINE_RECT3D2P_M(Z, Y, c, v, jz, jy)
DEFINE_RECT3D2P_M(Z, Y, v, c, jz, jy)
DEFINE_RECT3D2P_M(Z, Y, v, v, jz, jy)

/**
 * @brief 3D-world relative pointer.
 *
 * You can assign any axes of X, Y, or Z to each 'relative' axes. These
 * 'relative' axes are just for the convention, no much meanings. How axes
 * should be assigned are defined by the function uses the pointer.
 */
struct rect3d3p
{
  type *values;
  struct rect3d_relpointer_axis right;   ///< Right axis
  struct rect3d_relpointer_axis up;      ///< Up axis
  struct rect3d_relpointer_axis forward; ///< Forward axis
};

static inline int rect3d3p_move_base(
  struct rect3d3p *p, int to_r, int to_u, int to_f,
  ptrdiff_t (*mover)(struct rect3d_relpointer_axis *axes, int amount))
{
  ptrdiff_t off_r, off_u, off_f;

  off_r = mover(&p->right, to_r);
  off_u = mover(&p->up, to_u);
  off_f = mover(&p->forward, to_f);

  p->values += off_r + off_u + off_f;

  if (to_r != 0 && off_r == 0)
    return 0;
  if (to_u != 0 && off_u == 0)
    return 0;
  if (to_f != 0 && off_f == 0)
    return 0;

  return 1;
}

static inline int rect3d3p_move(struct rect3d3p *p, int to_r, int to_u,
                                int to_f)
{
  return rect3d3p_move_base(p, to_r, to_u, to_f, rect3d_relpointer_axis_move);
}

static inline int rect3d3p_moveR(struct rect3d3p *p, int to_r)
{
  return rect3d3p_move(p, to_r, 0, 0);
}

static inline int rect3d3p_moveU(struct rect3d3p *p, int to_u)
{
  return rect3d3p_move(p, 0, to_u, 0);
}

static inline int rect3d3p_moveF(struct rect3d3p *p, int to_f)
{
  return rect3d3p_move(p, 0, 0, to_f);
}

static inline int rect3d3p_absmove(struct rect3d3p *p, int to_r, int to_u,
                                   int to_f)
{
  return rect3d3p_move_base(p, to_r, to_u, to_f,
                            rect3d_relpointer_axis_absmove);
}

static inline int rect3d3p_absmoveR(struct rect3d3p *p, int to_r)
{
  return rect3d3p_absmove(p, to_r, 0, 0);
}

static inline int rect3d3p_absmoveU(struct rect3d3p *p, int to_u)
{
  return rect3d3p_absmove(p, 0, to_u, 0);
}

static inline int rect3d3p_absmoveF(struct rect3d3p *p, int to_f)
{
  return rect3d3p_absmove(p, 0, 0, to_f);
}

static inline int rect3d3p_move_nearest(struct rect3d3p *p, type r, type u,
                                        type f)
{
  ptrdiff_t off;
  off = rect3d_relpointer_axis_move_nearest(&p->right, r);
  off += rect3d_relpointer_axis_move_nearest(&p->up, u);
  off += rect3d_relpointer_axis_move_nearest(&p->forward, f);
  p->values += off;
  return 1;
}

static inline int rect3d3p_moveR_nearest(struct rect3d3p *p, type r)
{
  p->values += rect3d_relpointer_axis_move_nearest(&p->right, r);
  return 1;
}

static inline int rect3d3p_moveU_nearest(struct rect3d3p *p, type u)
{
  p->values += rect3d_relpointer_axis_move_nearest(&p->up, u);
  return 1;
}

static inline int rect3d3p_moveF_nearest(struct rect3d3p *p, type f)
{
  p->values += rect3d_relpointer_axis_move_nearest(&p->forward, f);
  return 1;
}

static inline int rect3d3p_movable_base(
  const struct rect3d3p *p, int to_r, int to_u, int to_f,
  int (*checker)(const struct rect3d_relpointer_axis *ax, int amount))
{
  return checker(&p->right, to_r) && checker(&p->up, to_u) &&
         checker(&p->forward, to_f);
}

static inline int rect3d3p_movable(const struct rect3d3p *p, int to_r, int to_u,
                                   int to_f)
{
  return rect3d3p_movable_base(p, to_r, to_u, to_f,
                               rect3d_relpointer_axis_movable);
}

static inline int rect3d3p_movableR(const struct rect3d3p *p, int to_r)
{
  return rect3d3p_movable(p, to_r, 0, 0);
}

static inline int rect3d3p_movableU(const struct rect3d3p *p, int to_u)
{
  return rect3d3p_movable(p, 0, to_u, 0);
}

static inline int rect3d3p_movableF(const struct rect3d3p *p, int to_f)
{
  return rect3d3p_movable(p, 0, 0, to_f);
}

static inline int rect3d3p_absmovable(const struct rect3d3p *p, int to_r,
                                      int to_u, int to_f)
{
  return rect3d3p_movable_base(p, to_r, to_u, to_f,
                               rect3d_relpointer_axis_absmovable);
}

static inline int rect3d3p_absmovableR(const struct rect3d3p *p, int to_r)
{
  return rect3d3p_absmovable(p, to_r, 0, 0);
}

static inline int rect3d3p_absmovableU(const struct rect3d3p *p, int to_u)
{
  return rect3d3p_absmovable(p, 0, to_u, 0);
}

static inline int rect3d3p_absmovableF(const struct rect3d3p *p, int to_f)
{
  return rect3d3p_absmovable(p, 0, 0, to_f);
}

static inline type rect3d3p_getcR(struct rect3d3p *p)
{
  return rect3d_relpointer_axis_coords(&p->right);
}

static inline type rect3d3p_getcU(struct rect3d3p *p)
{
  return rect3d_relpointer_axis_coords(&p->up);
}

static inline type rect3d3p_getcF(const struct rect3d3p *p)
{
  return rect3d_relpointer_axis_coords(&p->forward);
}

static inline type rect3d3p_getv(const struct rect3d3p *p)
{
  return *p->values;
}

static inline type rect3d3p__getnv(
  const struct rect3d3p *pntr, int to_r, int to_u, int to_f,
  ptrdiff_t (*offsetter)(const struct rect3d_relpointer_axis *axis, int amount))
{
  ptrdiff_t off;
  off = 0;
  if (!rect3d_relpointer_axis__ndgetnvf(&off, &pntr->right, to_r, offsetter))
    return HUGE_VAL;
  if (!rect3d_relpointer_axis__ndgetnvf(&off, &pntr->up, to_u, offsetter))
    return HUGE_VAL;
  if (!rect3d_relpointer_axis__ndgetnvf(&off, &pntr->forward, to_f, offsetter))
    return HUGE_VAL;

  return *(pntr->values + off);
}

static inline type rect3d3p_getnv(const struct rect3d3p *p, int to_r, int to_u,
                                  int to_f)
{
  return rect3d3p__getnv(p, to_r, to_u, to_f, rect3d_relpointer_axis_offset);
}

static inline type rect3d3p_getnvR(const struct rect3d3p *p, int to_r)
{
  return rect3d3p_getnv(p, to_r, 0, 0);
}

static inline type rect3d3p_getnvU(const struct rect3d3p *p, int to_u)
{
  return rect3d3p_getnv(p, 0, to_u, 0);
}

static inline type rect3d3p_getnvF(const struct rect3d3p *p, int to_f)
{
  return rect3d3p_getnv(p, 0, 0, to_f);
}

static inline type rect3d3p_getncR(const struct rect3d3p *p, int to_r)
{
  return rect3d_relpointer_axis_ncoords(&p->right, to_r);
}

static inline type rect3d3p_getncU(const struct rect3d3p *p, int to_u)
{
  return rect3d_relpointer_axis_ncoords(&p->up, to_u);
}

static inline type rect3d3p_getncF(const struct rect3d3p *p, int to_f)
{
  return rect3d_relpointer_axis_ncoords(&p->forward, to_f);
}

static inline type rect3d3p_getabsnv(const struct rect3d3p *p, int to_r,
                                     int to_u, int to_f)
{
  return rect3d3p__getnv(p, to_r, to_u, to_f, rect3d_relpointer_axis_absoffset);
}

static inline type rect3d3p_getabsnvR(const struct rect3d3p *p, int to_r)
{
  return rect3d3p_getabsnv(p, to_r, 0, 0);
}

static inline type rect3d3p_getabsnvU(const struct rect3d3p *p, int to_u)
{
  return rect3d3p_getabsnv(p, 0, to_u, 0);
}

static inline type rect3d3p_getabsnvF(const struct rect3d3p *p, int to_f)
{
  return rect3d3p_getabsnv(p, 0, 0, to_f);
}

static inline type rect3d3p_getabsncR(const struct rect3d3p *p, int to_r)
{
  return rect3d_relpointer_axis_absncoords(&p->right, to_r);
}

static inline type rect3d3p_getabsncU(const struct rect3d3p *p, int to_u)
{
  return rect3d_relpointer_axis_absncoords(&p->up, to_u);
}

static inline type rect3d3p_getabsncF(const struct rect3d3p *p, int to_f)
{
  return rect3d_relpointer_axis_absncoords(&p->forward, to_f);
}

static inline type
rect3d3p_getnd_base(const struct rect3d3p *p, int to_r, int to_u, int to_f,
                    type (*d)(const struct rect3d_relpointer_axis *ax, int off),
                    type (*distf)(type dr, type du, type df))
{
  type dr, du, df;
  dr = du = df = 0.0;
  if (to_r != 0)
    dr = d(&p->right, to_r);
  if (to_u != 0)
    du = d(&p->up, to_u);
  if (to_f != 0)
    df = d(&p->forward, to_f);
  return distf(dr, du, df);
}

static inline type rect3d3p_taxicab(type dr, type du, type df)
{
  return dr + du + df;
}

static inline type rect3d3p_length(type dr, type du, type df)
{
  return sqrt(dr * dr + du * du + df * df);
}

static inline type rect3d3p_getndt(const struct rect3d3p *p, int to_r, int to_u,
                                   int to_f)
{
  return rect3d3p_getnd_base(p, to_r, to_u, to_f,
                             rect3d_relpointer_axis_distance, rect3d3p_taxicab);
}

static inline type rect3d3p_getnd(const struct rect3d3p *p, int to_r, int to_u,
                                  int to_f)
{
  return rect3d3p_getnd_base(p, to_r, to_u, to_f,
                             rect3d_relpointer_axis_distance, rect3d3p_length);
}

static inline type rect3d3p_getndR(const struct rect3d3p *p, int to_r)
{
  return rect3d_relpointer_axis_distance(&p->right, to_r);
}

static inline type rect3d3p_getndU(const struct rect3d3p *p, int to_u)
{
  return rect3d_relpointer_axis_distance(&p->up, to_u);
}

static inline type rect3d3p_getndF(const struct rect3d3p *p, int to_f)
{
  return rect3d_relpointer_axis_distance(&p->forward, to_f);
}

static inline void rect3d3p_setv(struct rect3d3p *p, type value)
{
  *p->values = value;
}

static inline struct rect3d1p rect3d3p_to1R(const struct rect3d3p *p)
{
  return rect3d1p_r(p->values, p->right);
}

static inline struct rect3d1p rect3d3p_to1U(const struct rect3d3p *p)
{
  return rect3d1p_r(p->values, p->up);
}

static inline struct rect3d1p rect3d3p_to1F(const struct rect3d3p *p)
{
  return rect3d1p_r(p->values, p->forward);
}

static inline struct rect3d2p rect3d3p_to2RU(const struct rect3d3p *p)
{
  return rect3d2p_c(p->values, p->right, p->up);
}

static inline struct rect3d2p rect3d3p_to2UR(const struct rect3d3p *p)
{
  return rect3d2p_c(p->values, p->up, p->right);
}

static inline struct rect3d2p rect3d3p_to2RF(const struct rect3d3p *p)
{
  return rect3d2p_c(p->values, p->right, p->forward);
}

static inline struct rect3d2p rect3d3p_to2FR(const struct rect3d3p *p)
{
  return rect3d2p_c(p->values, p->forward, p->right);
}

static inline struct rect3d2p rect3d3p_to2UF(const struct rect3d3p *p)
{
  return rect3d2p_c(p->values, p->up, p->forward);
}

static inline struct rect3d2p rect3d3p_to2FU(const struct rect3d3p *p)
{
  return rect3d2p_c(p->values, p->forward, p->up);
}

static inline struct rect3d3p rect3d3p_c(type *values,
                                         struct rect3d_relpointer_axis raxis,
                                         struct rect3d_relpointer_axis uaxis,
                                         struct rect3d_relpointer_axis faxis)
{
  return (struct rect3d3p){.values = values,
                           .right = raxis,
                           .up = uaxis,
                           .forward = faxis};
}

static inline struct rect3d3p rect3d3p_b(type *base, int jx, int jy, int jz,
                                         int mx, int my, int mz,
                                         struct rect3d_relpointer_axis raxis,
                                         struct rect3d_relpointer_axis uaxis,
                                         struct rect3d_relpointer_axis faxis)
{
  return rect3d3p_c(&base[calc_address(jx, jy, jz, mx, my, mz)], raxis, uaxis,
                    faxis);
}

static inline struct rect3d3p
rect3d3p__m(type *base, int jx, int jy, int jz, domain *cdo, int joff_r, int jr,
            int joff_u, int ju, int joff_f, int jf,
            struct rect3d_relpointer_axis (*fr)(domain *cdo, int joff, int j),
            struct rect3d_relpointer_axis (*fu)(domain *cdo, int joff, int j),
            struct rect3d_relpointer_axis (*ff)(domain *cdo, int joff, int j))
{
  return rect3d3p_b(base, jx, jy, jz, cdo->mx, cdo->my, cdo->mz,
                    fr(cdo, joff_r, jr), fu(cdo, joff_u, ju),
                    ff(cdo, joff_f, jf));
}

#define DEFINE_RECT3D3P_M(rax, uax, fax, rcv, ucv, fcv, rv, uv, fv)           \
  static inline struct rect3d3p rect3d3p_##rax##uax##fax##rcv##ucv##fcv##m(   \
    type *base, int jx, int jy, int jz, domain *cdo, int joff_r, int joff_u,  \
    int joff_f)                                                               \
  {                                                                           \
    return rect3d3p__m(base, jx, jy, jz, cdo, joff_r, rv, joff_u, uv, joff_f, \
                       fv, rect3d_relpointer_axis_##rax##rcv##m,              \
                       rect3d_relpointer_axis_##uax##ucv##m,                  \
                       rect3d_relpointer_axis_##fax##fcv##m);                 \
  }

DEFINE_RECT3D3P_M(X, Y, Z, c, c, c, jx, jy, jz)
DEFINE_RECT3D3P_M(X, Y, Z, v, c, c, jx, jy, jz)
DEFINE_RECT3D3P_M(X, Y, Z, c, v, c, jx, jy, jz)
DEFINE_RECT3D3P_M(X, Y, Z, c, c, v, jx, jy, jz)
DEFINE_RECT3D3P_M(X, Y, Z, v, v, c, jx, jy, jz)
DEFINE_RECT3D3P_M(X, Y, Z, c, v, v, jx, jy, jz)
DEFINE_RECT3D3P_M(X, Y, Z, v, c, v, jx, jy, jz)
DEFINE_RECT3D3P_M(X, Y, Z, v, v, v, jx, jy, jz)

DEFINE_RECT3D3P_M(X, Z, Y, c, c, c, jx, jz, jy)
DEFINE_RECT3D3P_M(X, Z, Y, v, c, c, jx, jz, jy)
DEFINE_RECT3D3P_M(X, Z, Y, c, v, c, jx, jz, jy)
DEFINE_RECT3D3P_M(X, Z, Y, c, c, v, jx, jz, jy)
DEFINE_RECT3D3P_M(X, Z, Y, v, v, c, jx, jz, jy)
DEFINE_RECT3D3P_M(X, Z, Y, c, v, v, jx, jz, jy)
DEFINE_RECT3D3P_M(X, Z, Y, v, c, v, jx, jz, jy)
DEFINE_RECT3D3P_M(X, Z, Y, v, v, v, jx, jz, jy)

DEFINE_RECT3D3P_M(Y, X, Z, c, c, c, jy, jx, jz)
DEFINE_RECT3D3P_M(Y, X, Z, v, c, c, jy, jx, jz)
DEFINE_RECT3D3P_M(Y, X, Z, c, v, c, jy, jx, jz)
DEFINE_RECT3D3P_M(Y, X, Z, c, c, v, jy, jx, jz)
DEFINE_RECT3D3P_M(Y, X, Z, v, v, c, jy, jx, jz)
DEFINE_RECT3D3P_M(Y, X, Z, c, v, v, jy, jx, jz)
DEFINE_RECT3D3P_M(Y, X, Z, v, c, v, jy, jx, jz)
DEFINE_RECT3D3P_M(Y, X, Z, v, v, v, jy, jx, jz)

DEFINE_RECT3D3P_M(Y, Z, X, c, c, c, jy, jz, jx)
DEFINE_RECT3D3P_M(Y, Z, X, v, c, c, jy, jz, jx)
DEFINE_RECT3D3P_M(Y, Z, X, c, v, c, jy, jz, jx)
DEFINE_RECT3D3P_M(Y, Z, X, c, c, v, jy, jz, jx)
DEFINE_RECT3D3P_M(Y, Z, X, v, v, c, jy, jz, jx)
DEFINE_RECT3D3P_M(Y, Z, X, c, v, v, jy, jz, jx)
DEFINE_RECT3D3P_M(Y, Z, X, v, c, v, jy, jz, jx)
DEFINE_RECT3D3P_M(Y, Z, X, v, v, v, jy, jz, jx)

DEFINE_RECT3D3P_M(Z, Y, X, c, c, c, jz, jy, jx)
DEFINE_RECT3D3P_M(Z, Y, X, v, c, c, jz, jy, jx)
DEFINE_RECT3D3P_M(Z, Y, X, c, v, c, jz, jy, jx)
DEFINE_RECT3D3P_M(Z, Y, X, c, c, v, jz, jy, jx)
DEFINE_RECT3D3P_M(Z, Y, X, v, v, c, jz, jy, jx)
DEFINE_RECT3D3P_M(Z, Y, X, c, v, v, jz, jy, jx)
DEFINE_RECT3D3P_M(Z, Y, X, v, c, v, jz, jy, jx)
DEFINE_RECT3D3P_M(Z, Y, X, v, v, v, jz, jy, jx)

DEFINE_RECT3D3P_M(Z, X, Y, c, c, c, jz, jx, jy)
DEFINE_RECT3D3P_M(Z, X, Y, v, c, c, jz, jx, jy)
DEFINE_RECT3D3P_M(Z, X, Y, c, v, c, jz, jx, jy)
DEFINE_RECT3D3P_M(Z, X, Y, c, c, v, jz, jx, jy)
DEFINE_RECT3D3P_M(Z, X, Y, v, v, c, jz, jx, jy)
DEFINE_RECT3D3P_M(Z, X, Y, c, v, v, jz, jx, jy)
DEFINE_RECT3D3P_M(Z, X, Y, v, c, v, jz, jx, jy)
DEFINE_RECT3D3P_M(Z, X, Y, v, v, v, jz, jx, jy)

struct rect3d3cp
{
  struct rect3d3p relp;
  struct rect3d_relpointer_index component;
};

/* TODO: Implenment functions */

typedef struct rect3d1p rect3d1p;
typedef struct rect3d1cp rect3d1cp;
typedef struct rect3d2p rect3d2p;
typedef struct rect3d3p rect3d3p;
typedef struct rect3d3cp rect3d3cp;

/* == Interpolators == */
/**
 * @brief Function to specify interpolation method
 * @param pntr Base point to interpolate
 * @param ret_coord Coordinate value which the caller wants
 * @param arg Arbitrary extra argument
 *
 * Calculate the interpolation of @p values at @p ret_coord and return it.
 *
 * It is called as @p *coords is the nearest to @p ret_coord. but may be @p
 * *coords < @p ret_coord and @p *coords > @p ret_coord. The function will
 * also be called even if @p *coords == @p ret_coord.
 *
 * Sum of @p nnpoints and @p nppoints may be less than the number of your
 * model requires, because of the array boundary limit.
 */
typedef type rect3d1_interpolator(const struct rect3d1p *pntr, type ret_coord,
                                  void *arg);

/**
 * Interpolation just returns the nearest value (aka. Voronoi interpolation).
 *
 * @p arg is not used in this interpolator
 */
static inline type rect3d1_interp_nearest(const struct rect3d1p *pntr,
                                          type ret_coord, void *arg)
{
  return rect3d1p_getv(pntr);
}

/**
 * Linear interpolation with nearest two points
 *
 * @p arg is not used in this interpolator
 */
static inline type rect3d1_interp_linear(const struct rect3d1p *pntr,
                                         type ret_coord, void *arg)
{
  struct rect3d1p pm, pp;
  type cm, cp, vm, vp, n, d;

  pm = *pntr;
  cm = rect3d1p_getc(&pm);
  if (cm > ret_coord && rect3d1p_absmovable(&pm, -1)) {
    rect3d1p_absmove(&pm, -1);
  }

  pp = pm;
  if (rect3d1p_absmovable(&pp, 1)) {
    rect3d1p_absmove(&pp, 1);
  } else {
    rect3d1p_absmove(&pm, -1);
  }

  cm = rect3d1p_getc(&pm);
  cp = rect3d1p_getc(&pp);
  vm = rect3d1p_getv(&pm);
  vp = rect3d1p_getv(&pp);

  cm = ret_coord - cm;
  cp = cp - ret_coord;

  n = vm * cp + vp * cm;
  d = cp + cm;
  if (d != 0.0)
    return n / d;
  return vm;
}

typedef type rect3d2_interpolator(const struct rect3d2p *pntr, type rcoord,
                                  type ucoord, void *arg);

static inline type rect3d2_interp_linear(const struct rect3d2p *pntr,
                                         type rcoord, type ucoord, void *arg)
{
  struct rect3d2p pm, pp;
  type drm, drp, dum, dup, d00, d01, d10, d11, v00, v01, v10, v11, n, d;

  pm = *pntr;

  drm = rect3d2p_getcR(&pm);
  if (drm > rcoord && rect3d2p_absmovableR(&pm, -1)) {
    rect3d2p_absmoveR(&pm, -1);
  }

  drm = rect3d2p_getcU(&pm);
  if (drm > ucoord && rect3d2p_absmovableU(&pm, -1)) {
    rect3d2p_absmoveU(&pm, -1);
  }

  pp = pm;
  if (rect3d2p_absmovable(&pp, 1, 1)) {
    rect3d2p_absmove(&pp, 1, 1);
  } else {
    if (rect3d2p_absmovableR(&pp, 1)) {
      rect3d2p_absmoveR(&pp, 1);
      rect3d2p_absmoveU(&pm, -1);
    } else if (rect3d2p_absmovableU(&pp, 1)) {
      rect3d2p_absmoveU(&pp, 1);
      rect3d2p_absmoveR(&pm, -1);
    } else {
      rect3d2p_absmove(&pm, -1, -1);
    }
  }

  drm = rect3d2p_getcR(&pm);
  drp = rect3d2p_getcR(&pp);
  dum = rect3d2p_getcU(&pm);
  dup = rect3d2p_getcU(&pp);

  drm = rcoord - drm;
  drp = drp - rcoord;
  dum = ucoord - dum;
  dup = dup - ucoord;

  v00 = rect3d2p_getv(&pm);
  v11 = rect3d2p_getv(&pp);

  rect3d2p_absmoveR(&pm, 1);
  rect3d2p_absmoveR(&pp, -1);

  v10 = rect3d2p_getv(&pm);
  v01 = rect3d2p_getv(&pp);

  d00 = drp * dup;
  d01 = drp * dum;
  d10 = drm * dup;
  d11 = drm * dum;

  n = v00 * d00 + v01 * d01 + v10 * d10 + v11 * d11;
  d = d00 + d01 + d10 + d11;
  if (d != 0.0)
    return n / d;
  return v00;
}

typedef type rect3d3_interpolator(const struct rect3d3p *pntr, type rcoord,
                                  type ucoord, type fcoord, void *arg);

#ifdef __cplusplus
}
#endif

#endif
