#ifndef JUPITER_RECT3D_BOUNDARY_H
#define JUPITER_RECT3D_BOUNDARY_H

#include "struct.h"
#include "rect3d_relp.h"

#ifdef __cplusplus
extern "C" {
#endif

struct rect3d1p_boundary
{
  struct rect3d1p boundary;
  struct rect3d1p interior;
  type vcoord; ///< Coordinate of boundary
};

static inline int rect3d1p_boundary_init_r(struct rect3d1p_boundary *ret,
                                           struct rect3d1p boundary_p, type vc)
{
  type c1;
  ret->vcoord = vc;
  ret->boundary = boundary_p;
  ret->interior = ret->boundary;
  rect3d1p_flip(&ret->interior);

  c1 = rect3d1p_getc(&ret->boundary);
  c1 = vc - (c1 - vc);
  return rect3d1p_move_nearest(&ret->interior, c1);
}

/* calculates coordinate at boundary */
static inline type rect3d1p_boundary__Xv(domain *cdo, int jb, int face)
{
  return cdo->xv[jb + face];
}

static inline type rect3d1p_boundary__Yv(domain *cdo, int jb, int face)
{
  return cdo->yv[jb + face];
}

static inline type rect3d1p_boundary__Zv(domain *cdo, int jb, int face)
{
  return cdo->zv[jb + face];
}

/* Initializer for boundary */
static inline int rect3d1p_boundary_mcj(int stm, int n) { return stm - 1; }

static inline int rect3d1p_boundary_mvj(int stm, int n) { return stm; }

static inline int rect3d1p_boundary_pcj(int stm, int n) { return stm + n; }

static inline int rect3d1p_boundary_pvj(int stm, int n) { return stm + n; }

static inline int rect3d1p_boundary__bndX(domain *cdo, int (*f)(int stm, int n))
{
  return f(cdo->stm, cdo->nx);
}

static inline int rect3d1p_boundary__bndY(domain *cdo, int (*f)(int stm, int n))
{
  return f(cdo->stm, cdo->ny);
}

static inline int rect3d1p_boundary__bndZ(domain *cdo, int (*f)(int stm, int n))
{
  return f(cdo->stm, cdo->nz);
}

/* jx <= jb, jy <= j1, jz <= j2 */
static inline void rect3d1p_boundary__shuffX(int *jbx, int *j1y, int *j2z)
{
  /* nop */
}

/* jx <= j1, jy <= jb, jz <= j2 */
static inline void rect3d1p_boundary__shuffY(int *jbx, int *j1y, int *j2z)
{
  int i = *j1y;
  *j1y = *jbx;
  *jbx = i;
}

/* jx <= j1, jy <= j2, jz <= jb */
static inline void rect3d1p_boundary__shuffZ(int *jbx, int *j1y, int *j2z)
{
  int i = *jbx;
  *jbx = *j1y;
  *j1y = *j2z;
  *j2z = i;
}

static inline int rect3d1p_boundary__mjoff(void) { return -1; }
static inline int rect3d1p_boundary__pjoff(void) { return 1; }

static inline int rect3d1p_boundary__cmface(void) { return 1; }
static inline int rect3d1p_boundary__cpface(void) { return 0; }
static inline int rect3d1p_boundary__vmface(void) { return 0; }
static inline int rect3d1p_boundary__vpface(void) { return 0; }

static inline type
rect3d1p_boundary__vc(domain *cdo, type (*vcf)(domain *cdo, int jb, int face),
                      int (*jbf)(domain *cdo), int (*face_f)(void))
{
  return vcf(cdo, jbf(cdo), face_f());
}

#define DEFINE_RECT3D1_BOUNDRAY_RELP_J(bnd, axis, cv, mp)                      \
  static inline void rect3d1p_boundary__##bnd##cv##_shuff(int *jbx, int *j1y,  \
                                                          int *j2z)            \
  {                                                                            \
    rect3d1p_boundary__shuff##axis(jbx, j1y, j2z);                             \
  }                                                                            \
                                                                               \
  static inline int rect3d1p_boundary__##bnd##cv##j(domain *cdo)               \
  {                                                                            \
    return rect3d1p_boundary__bnd##axis(cdo, rect3d1p_boundary_##mp##cv##j);   \
  }                                                                            \
                                                                               \
  static inline int rect3d1p_boundary__##bnd##cv##_joff(void)                  \
  {                                                                            \
    return rect3d1p_boundary__##mp##joff();                                    \
  }                                                                            \
                                                                               \
  static inline int rect3d1p_boundary__##bnd##cv##_face(void)                  \
  {                                                                            \
    return rect3d1p_boundary__##cv##mp##face();                                \
  }                                                                            \
                                                                               \
  static inline type rect3d1p_boundary__##bnd##cv##_vc(domain *cdo)            \
  {                                                                            \
    return rect3d1p_boundary__vc(cdo, rect3d1p_boundary__##axis##v,            \
                                 rect3d1p_boundary__##bnd##cv##j,              \
                                 rect3d1p_boundary__##bnd##cv##_face);         \
  }                                                                            \
                                                                               \
  static inline struct rect3d1p (*rect3d1p_boundary__##bnd##cv##_rect3d1_func( \
    void))(type * base, int jx, int jy, int jz, domain *cdo, int joff)         \
  {                                                                            \
    return rect3d1p_##axis##cv##m;                                             \
  }                                                                            \
                                                                               \
  static inline struct rect3d1cp (                                             \
    *rect3d1p_boundary__##bnd##cv##_rect3d1c_func(                             \
      void))(type * base, int jx, int jy, int jz, int icompo, domain *cdo,     \
             int joff, int is, int ie, int coff)                               \
  {                                                                            \
    return rect3d1cp_##axis##cv##m;                                            \
  }

DEFINE_RECT3D1_BOUNDRAY_RELP_J(W, X, c, m)
DEFINE_RECT3D1_BOUNDRAY_RELP_J(E, X, c, p)
DEFINE_RECT3D1_BOUNDRAY_RELP_J(S, Y, c, m)
DEFINE_RECT3D1_BOUNDRAY_RELP_J(N, Y, c, p)
DEFINE_RECT3D1_BOUNDRAY_RELP_J(B, Z, c, m)
DEFINE_RECT3D1_BOUNDRAY_RELP_J(T, Z, c, p)
DEFINE_RECT3D1_BOUNDRAY_RELP_J(W, X, v, m)
DEFINE_RECT3D1_BOUNDRAY_RELP_J(E, X, v, p)
DEFINE_RECT3D1_BOUNDRAY_RELP_J(S, Y, v, m)
DEFINE_RECT3D1_BOUNDRAY_RELP_J(N, Y, v, p)
DEFINE_RECT3D1_BOUNDRAY_RELP_J(B, Z, v, m)
DEFINE_RECT3D1_BOUNDRAY_RELP_J(T, Z, v, p)

static inline rect3d1p
rect3d1p__bnd(type *base, int j1, int j2, domain *cdo, int (*jbf)(domain *cdo),
              void (*jshuff)(int *jbx, int *j1y, int *j2z), int (*joff)(void),
              struct rect3d1p (*f(void))(type *base, int jx, int jy, int jz,
                                         domain *cdo, int joff))
{
  int jb = jbf(cdo);
  jshuff(&jb, &j1, &j2);
  return f()(base, jb, j1, j2, cdo, joff());
}

static inline rect3d1cp rect3d1cp__bnd(
  type *base, int j1, int j2, int icompo, domain *cdo, int is, int ie, int coff,
  int (*jbf)(domain *cdo), void (*jshuff)(int *jbx, int *j1y, int *j2z),
  int (*joff)(void),
  struct rect3d1cp (*f(void))(type *base, int jx, int jy, int jz, int icompo,
                              domain *cdo, int joff, int is, int ie, int coff))
{
  int jb = jbf(cdo);
  jshuff(&jb, &j1, &j2);
  return f()(base, jb, j1, j2, icompo, cdo, joff(), is, ie, coff);
}

static inline int rect3d1p_boundary__bnd(
  struct rect3d1p_boundary *ret, type *base, int j1, int j2, domain *cdo,
  int (*jbf)(domain *cdo), type (*vcf)(domain *cdo),
  void (*jshuff)(int *jbx, int *j1y, int *j2z), int (*joff)(void),
  struct rect3d1p (*f(void))(type *base, int jx, int jy, int jz, domain *cdo,
                             int joff))
{
  int jb;
  type vc;
  struct rect3d1p rp;

  jb = jbf(cdo);
  vc = vcf(cdo);
  jshuff(&jb, &j1, &j2);

  rp = f()(base, jb, j1, j2, cdo, joff());
  return rect3d1p_boundary_init_r(ret, rp, vc);
}

#define DEFINE_RECT3D1P_BND(bnd, cv, j1var, j2var)                             \
  static inline struct rect3d1p rect3d1p_##bnd##cv##m(type *base, int j1var,   \
                                                      int j2var, domain *cdo)  \
  {                                                                            \
    return rect3d1p__bnd(base, j1var, j2var, cdo,                              \
                         rect3d1p_boundary__##bnd##cv##j,                      \
                         rect3d1p_boundary__##bnd##cv##_shuff,                 \
                         rect3d1p_boundary__##bnd##cv##_joff,                  \
                         rect3d1p_boundary__##bnd##cv##_rect3d1_func);         \
  }                                                                            \
                                                                               \
  static inline struct rect3d1cp rect3d1cp_##bnd##cv##m(type *base, int j1var, \
                                                        int j2var, int icompo, \
                                                        domain *cdo, int is,   \
                                                        int ie, int coff)      \
  {                                                                            \
    return rect3d1cp__bnd(base, j1var, j2var, icompo, cdo, is, ie, coff,       \
                          rect3d1p_boundary__##bnd##cv##j,                     \
                          rect3d1p_boundary__##bnd##cv##_shuff,                \
                          rect3d1p_boundary__##bnd##cv##_joff,                 \
                          rect3d1p_boundary__##bnd##cv##_rect3d1c_func);       \
  }                                                                            \
                                                                               \
  static inline int rect3d1p_boundary_##bnd##cv(struct rect3d1p_boundary *ret, \
                                                type *base, domain *cdo,       \
                                                int j1var, int j2var)          \
  {                                                                            \
    return rect3d1p_boundary__bnd(                                             \
      ret, base, j1var, j2var, cdo, rect3d1p_boundary__##bnd##cv##j,           \
      rect3d1p_boundary__##bnd##cv##_vc, rect3d1p_boundary__##bnd##cv##_shuff, \
      rect3d1p_boundary__##bnd##cv##_joff,                                     \
      rect3d1p_boundary__##bnd##cv##_rect3d1_func);                            \
  }

DEFINE_RECT3D1P_BND(W, c, jy, jz)
DEFINE_RECT3D1P_BND(W, v, jy, jz)
DEFINE_RECT3D1P_BND(E, c, jy, jz)
DEFINE_RECT3D1P_BND(E, v, jy, jz)
DEFINE_RECT3D1P_BND(S, c, jx, jz)
DEFINE_RECT3D1P_BND(S, v, jx, jz)
DEFINE_RECT3D1P_BND(N, c, jx, jz)
DEFINE_RECT3D1P_BND(N, v, jx, jz)
DEFINE_RECT3D1P_BND(B, c, jx, jy)
DEFINE_RECT3D1P_BND(B, v, jx, jy)
DEFINE_RECT3D1P_BND(T, c, jx, jy)
DEFINE_RECT3D1P_BND(T, v, jx, jy)

/**
 * @memberof rect3d1p
 * @param base Base address of a variable
 * @param jx X axis index (for TBNS boundary)
 * @param jy Y axis index (for TBEW boundary)
 * @param jz Z axis index (for NSEW boundary)
 * @param cdo JUPITER domain data base Base address of a variable
 * @return constructed pointer.
 *
 * Get pointer for iterating inside boundary
 */
static inline struct rect3d1p rect3d1p_Bcm(type *base, int jx, int jy,
                                           domain *cdo);
static inline struct rect3d1p rect3d1p_Tcm(type *base, int jx, int jy,
                                           domain *cdo);
static inline struct rect3d1p rect3d1p_Scm(type *base, int jx, int jz,
                                           domain *cdo);
static inline struct rect3d1p rect3d1p_Ncm(type *base, int jx, int jz,
                                           domain *cdo);
static inline struct rect3d1p rect3d1p_Wcm(type *base, int jy, int jz,
                                           domain *cdo);
static inline struct rect3d1p rect3d1p_Ecm(type *base, int jy, int jz,
                                           domain *cdo);
static inline struct rect3d1p rect3d1p_Bvm(type *base, int jx, int jy,
                                           domain *cdo);
static inline struct rect3d1p rect3d1p_Tvm(type *base, int jx, int jy,
                                           domain *cdo);
static inline struct rect3d1p rect3d1p_Svm(type *base, int jx, int jz,
                                           domain *cdo);
static inline struct rect3d1p rect3d1p_Nvm(type *base, int jx, int jz,
                                           domain *cdo);
static inline struct rect3d1p rect3d1p_Wvm(type *base, int jy, int jz,
                                           domain *cdo);
static inline struct rect3d1p rect3d1p_Evm(type *base, int jy, int jz,
                                           domain *cdo);

/**
 * @brief Get pointer for iterating bottom boundary of cell center variables.
 * @memberof rect3d1cp
 * @param base Base address of a variable
 * @param jx X axis index
 * @param jy Y axis index
 * @param icompo Initial component index (@p base is start of component 0)
 * @param cdo JUPITER domain data base Base address of a variable
 * @param is Lower array boundary on component for iteration
 * @param ie Upper array boundary on component for iteration
 * @param coff Iteration offset for the component dimension
 *
 * @return constructed pointer.
 *
 * Get pointer for iterating inside boundary
 */
static inline struct rect3d1cp rect3d1cp_Bcm(type *base, int jx, int jy,
                                             int icompo, domain *cdo, int is,
                                             int ie, int coff);
static inline struct rect3d1cp rect3d1cp_Tcm(type *base, int jx, int jy,
                                             int icompo, domain *cdo, int is,
                                             int ie, int coff);
static inline struct rect3d1cp rect3d1cp_Scm(type *base, int jx, int jz,
                                             int icompo, domain *cdo, int is,
                                             int ie, int coff);
static inline struct rect3d1cp rect3d1cp_Ncm(type *base, int jx, int jz,
                                             int icompo, domain *cdo, int is,
                                             int ie, int coff);
static inline struct rect3d1cp rect3d1cp_Wcm(type *base, int jy, int jz,
                                             int icompo, domain *cdo, int is,
                                             int ie, int coff);
static inline struct rect3d1cp rect3d1cp_Ecm(type *base, int jy, int jz,
                                             int icompo, domain *cdo, int is,
                                             int ie, int coff);
static inline struct rect3d1cp rect3d1cp_Bvm(type *base, int jx, int jy,
                                             int icompo, domain *cdo, int is,
                                             int ie, int coff);
static inline struct rect3d1cp rect3d1cp_Tvm(type *base, int jx, int jy,
                                             int icompo, domain *cdo, int is,
                                             int ie, int coff);
static inline struct rect3d1cp rect3d1cp_Svm(type *base, int jx, int jz,
                                             int icompo, domain *cdo, int is,
                                             int ie, int coff);
static inline struct rect3d1cp rect3d1cp_Nvm(type *base, int jx, int jz,
                                             int icompo, domain *cdo, int is,
                                             int ie, int coff);
static inline struct rect3d1cp rect3d1cp_Wvm(type *base, int jy, int jz,
                                             int icompo, domain *cdo, int is,
                                             int ie, int coff);
static inline struct rect3d1cp rect3d1cp_Evm(type *base, int jy, int jz,
                                             int icompo, domain *cdo, int is,
                                             int ie, int coff);

static inline int rect3d1p_boundary_Wc(struct rect3d1p_boundary *ret,
                                       type *base, domain *cdo, int jy, int jz);
static inline int rect3d1p_boundary_Wv(struct rect3d1p_boundary *ret,
                                       type *base, domain *cdo, int jy, int jz);
static inline int rect3d1p_boundary_Ec(struct rect3d1p_boundary *ret,
                                       type *base, domain *cdo, int jy, int jz);
static inline int rect3d1p_boundary_Ev(struct rect3d1p_boundary *ret,
                                       type *base, domain *cdo, int jy, int jz);
static inline int rect3d1p_boundary_Sc(struct rect3d1p_boundary *ret,
                                       type *base, domain *cdo, int jx, int jz);
static inline int rect3d1p_boundary_Sv(struct rect3d1p_boundary *ret,
                                       type *base, domain *cdo, int jx, int jz);
static inline int rect3d1p_boundary_Nc(struct rect3d1p_boundary *ret,
                                       type *base, domain *cdo, int jx, int jz);
static inline int rect3d1p_boundary_Nv(struct rect3d1p_boundary *ret,
                                       type *base, domain *cdo, int jx, int jz);
static inline int rect3d1p_boundary_Bc(struct rect3d1p_boundary *ret,
                                       type *base, domain *cdo, int jx, int jy);
static inline int rect3d1p_boundary_Bv(struct rect3d1p_boundary *ret,
                                       type *base, domain *cdo, int jx, int jy);
static inline int rect3d1p_boundary_Tc(struct rect3d1p_boundary *ret,
                                       type *base, domain *cdo, int jx, int jy);
static inline int rect3d1p_boundary_Tv(struct rect3d1p_boundary *ret,
                                       type *base, domain *cdo, int jx, int jy);

static inline int rect3d1p_boundary_for_init(struct rect3d1p_boundary *p,
                                             int move_interior, type *ret_ci)
{
  if (move_interior) {
    type cb = rect3d1p_getc(&p->boundary);
    type vc = p->vcoord;
    type ci = 2.0 * vc - cb;

    rect3d1p_move_nearest(&p->interior, ci);
    if (ret_ci)
      *ret_ci = ci;
  }
  return 1;
}

static inline int rect3d1p_boundary_next(struct rect3d1p_boundary *p,
                                         int amount, int move_interior,
                                         type *ret_ci)
{
  if (!rect3d1p_move(&p->boundary, amount))
    return 0;

  if (move_interior) {
    type cb = rect3d1p_getc(&p->boundary);
    type vc = p->vcoord;
    type ci = 2.0 * vc - cb; /* need to check for precision: vc - (cb - vc) */

    rect3d1p_move_nearest(&p->interior, ci);
    if (ret_ci)
      *ret_ci = ci;
  }
  return 1;
}

/**
 * @brief Loop for rect3d1p_boundary with interior
 * @param p Pointer to iterate
 * @param i Counter variable
 * @param count Number of interations in this loop (actually upper limit of @i)
 * @param amount Moving amount per step
 * @param ci Pointer to store the interior coordinate (type *)
 *
 * @warning This macro never initialize @p i.
 *
 * If @p i < @p count after the loop, it means that the pointer reaches to the
 * limit of array.
 *
 * @p ci is allow to be NULL, if it is not needed.
 */
#define rect3d1p_boundary_for_with_int(p, i, count, amount, ci)                \
  for (int _f = rect3d1p_boundary_for_init((p), 1, (ci)); (i) < (count) && _f; \
       ++(i), _f = rect3d1p_boundary_next((p), (amount), 1, (ci)))

/**
 * @brief Loop for rect3d1p_boundary with interior
 * @param p Pointer to iterate
 * @param i Counter variable
 * @param count Number of interations in this loop (actually upper limit of @i)
 * @param amount Moving amount per step
 * @param pointer Pointer to store the interior coordinate (type *)
 *
 * @warning This macro never initialize @p i.
 *
 * If @p i < @p count after the loop, it means that the pointer reaches to the
 * limit of array.
 *
 * Interior pointer will not be affected.
 */
#define rect3d1p_boundary_for(p, i, count, amount)                             \
  for (int _f = rect3d1p_boundary_for_init((p), 0, NULL); (i) < (count) && _f; \
       ++(i), _f = rect3d1p_boundary_next((p), (amount), 0, NULL))

/* == boundary utility == */
/**
 * @brief Point symmetric boundary for scalar (cell center) variables
 * @param p Iterative pointer of desired direction
 * @param stencil_size Number of stencil cells
 * @param symmetry_val Value at boundary
 * @param interpolator Interpolator function to use
 * @param arg Extra argument for @p interpolator
 *
 * Initialize @p p with `rect3d_boundary_relp_[TBNSEW]c()`. @p p will be
 * modified.
 */
static void rect3d1p_boundary_point_symmetry_scalar(
  struct rect3d1p_boundary *p, int stencil_size, type symmetry_val,
  rect3d1_interpolator *interpolator, void *arg)
{
  int j;
  type ci;
  symmetry_val *= 2.0;

  j = 0;
  rect3d1p_boundary_for_with_int (p, j, stencil_size, 1, &ci) {
    type ti;
    ti = interpolator(&p->interior, ci, arg);
    rect3d1p_setv(&p->boundary, symmetry_val - ti);
  }
}

/**
 * @brief Point symmetric boundary for the parallel-to-boundary component of the
 *        staggerd vector (cell center) variable
 * @param p Iterative pointer of desired direction
 * @param stencil_size Number of stencil cells
 * @param symmetry_val Value at boundary
 * @param interpolator Interpolator function to use
 * @param arg Extra argument for @p interpolator
 *
 * Initialize @p p with `rect3d_boundary_relp_[TBNSEW]c()`. @p p will be
 * modified.
 */
static inline void rect3d1p_boundary_point_symmetry_stgpara(
  struct rect3d1p_boundary *p, int stencil_size, type symmetry_val,
  rect3d1_interpolator *interpolator, void *arg)
{
  rect3d1p_boundary_point_symmetry_scalar(p, stencil_size, symmetry_val,
                                          interpolator, arg);
}

/**
 * @brief Point symmetric boundary for the perpendicular-to-boundary component
 *        of the staggerd vector (cell face) variable
 * @param p Iterative pointer of desired direction
 * @param stencil_size Number of stencil cells
 * @param symmetry_val Value at boundary
 * @param interpolator Interpolator function to use
 * @param arg Extra argument for @p interpolator
 *
 * Initialize @p p with `rect3d_boundary_relp_[TBNSEW]v()`. @p p will be
 * modified.
 */
static inline void rect3d1p_boundary_point_symmetry_stgperp(
  struct rect3d1p_boundary *p, int stencil_size, type symmetry_val,
  rect3d1_interpolator *interpolator, void *arg)
{
  int j;
  type ci;

  rect3d1p_setv(&p->boundary, symmetry_val);
  if (!rect3d1p_move(&p->boundary, 1))
    return;

  symmetry_val *= 2.0;
  j = 0;
  rect3d1p_boundary_for_with_int (p, j, stencil_size, 1, &ci) {
    type ti;
    ti = interpolator(&p->interior, ci, arg);
    rect3d1p_setv(&p->boundary, symmetry_val - ti);
  }
}

/**
 * @brief Line symmetric boundary for scalar (cell center) variables
 * @param p Iterative pointer of desired direction
 * @param stencil_size Number of stencil cells
 * @param interpolator Interpolator function to use
 * @param arg Extra argument for @p interpolator
 *
 * Initialize @p p with `rect3d_boundary_relp_[TBNSEW]c()`. @p p will be
 * modified.
 */
static inline void rect3d1p_boundary_line_symmetry_scalar(
  struct rect3d1p_boundary *p, int stencil_size,
  rect3d1_interpolator *interpolator, void *arg)
{
  int j;
  type ci;

  j = 0;
  rect3d1p_boundary_for_with_int (p, j, stencil_size, 1, &ci) {
    type ti;
    ti = interpolator(&p->interior, ci, arg);
    rect3d1p_setv(&p->boundary, ti);
  }
}

/**
 * @brief Line symmetric boundary for parallel-to-boundary component of the
 *        staggared vector (cell center) variables
 * @param p Iterative pointer of desired direction
 * @param stencil_size Number of stencil cells
 * @param interpolator Interpolator function to use
 * @param arg Extra argument for @p interpolator
 *
 * Initialize @p p with `rect3d_boundary_relp_[TBNSEW]c()`. @p p will be
 * modified.
 */
static inline void rect3d1p_boundary_line_symmetry_stgpara(
  struct rect3d1p_boundary *p, int stencil_size,
  rect3d1_interpolator *interpolator, void *arg)
{
  rect3d1p_boundary_line_symmetry_scalar(p, stencil_size, interpolator, arg);
}

/**
 * @brief Line symmetric boundary for perpendicular-to-boundary component of the
 *        staggared vector (cell face) variables
 * @param p Iterative pointer of desired direction
 * @param stencil_size Number of stencil cells
 * @param interpolator Interpolator function to use
 * @param arg Extra argument for @p interpolator
 *
 * Initialize @p p with `rect3d_boundary_relp_[TBNSEW]v()`. @p p will be
 * modified.
 */
static inline void rect3d1p_boundary_line_symmetry_stgperp(
  struct rect3d1p_boundary *p, int stencil_size,
  rect3d1_interpolator *interpolator, void *arg)
{
  int j;
  type ci;

  if (!rect3d1p_move(&p->boundary, 1))
    return;

  j = 0;
  rect3d1p_boundary_for_with_int (p, j, stencil_size, 1, &ci) {
    type ti;
    ti = interpolator(&p->interior, ci, arg);
    rect3d1p_setv(&p->boundary, ti);
  }
}

/**
 * @brief Fill boundary values by given one
 * @param p Iterative pointer of desired direction
 * @param stencil_size Number of stencil cells
 * @param value Value to fill
 *
 * Initialize @p p with `rect3d_boundary_relp_[TBNSEW][cv]()`. @p p will be
 * modified.
 */
static inline void rect3d1p_boundary_fill(struct rect3d1p_boundary *p,
                                          int stencil_size, type value)
{
  int j;

  j = 0;
  rect3d1p_boundary_for (p, j, stencil_size, 1) {
    rect3d1p_setv(&p->boundary, value);
  }
}

#ifdef __cplusplus
}
#endif

#endif
