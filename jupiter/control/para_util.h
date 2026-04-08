#ifndef JCNTRL_CONTROL_PARA_UTIL_H
#define JCNTRL_CONTROL_PARA_UTIL_H

#include "defs.h"

JUPITER_CONTROL_DECL_BEGIN

#ifdef _OPENMP
static inline int jcntrl_omp_get_error(int *shared_ret, int *local_ret)
{
  int ie;
  if (!*local_ret)
    return *local_ret;

#pragma omp atomic read
  ie = *shared_ret;
  if (!ie)
    *local_ret = ie;
  return ie;
}

static inline int jcntrl_omp_set_error(int *shared_ret, int *local_ret)
{
  *local_ret = 0;
#pragma omp atomic write
  *shared_ret = 0;
  return 0;
}
#endif

/**
 * @retval 0 Abort loop (as soon as possible)
 * @retval 1 Continue loop
 */
typedef int jcntrl_para_loop_func(jcntrl_size_type jj, void *arg);

/**
 * @brief Execute function @p func in thread parallel (if OpenMP is enabled)
 *        with reporting error
 * @param is Start index
 * @param ie End index (exclusive)
 * @param func function to execute
 * @param arg Argument passed for @p func
 * @param shared_ret Pointer to store shared error
 * @return the value of @p shared_ret (with atomically read)
 *
 * @note this function does not include `#pragma omp parallel` directive, where
 * @p arg should be thread-local data (recommended, but not required):
 *
 * ```
 *   int r = 1;
 *
 * #ifdef _OPENMP
 * #pragma omp parallel [if(...)]
 * #endif
 *   {
 *     struct your_data arg = { ... };
 *     jcntrl_para_loop(is, ie, func, &arg, &r);
 *   }
 *   if (!r)
 *     return 0;
 * ```
 *
 * To reduction:
 * ```
 * struct your_data
 * {
 *   your_type local_reduction;
 *   ...
 * };
 *
 * static int func(jcntrl_size_type jj, void *arg)
 * {
 *   struct your_data *p = arg;
 *   p->local_reduction [op]= ...;
 *   return 1;
 * }
 *
 * //...
 *
 *   int r = 1;
 *   your_type global_reduction = [initial value];
 *
 * #ifdef _OPENMP
 * #pragma omp parallel [if(...)]
 * #endif
 *   {
 *     struct your_data arg = { .local_reduction = 0, ... };
 *     if (jcntrl_para_loop(is, ie, func, &arg, &r)) {
 * #ifdef _OPENMP
 * #pragma omp critical // or atomic update if applicable
 *       {
 *         global_reduction = global_reduction [op] arg.local_reduction;
 *       }
 * #else
 *       global_reduction = arg.local_reduction;
 * #endif
 *     }
 *   }
 * ```
 */
static inline int jcntrl_para_loop(jcntrl_size_type is, jcntrl_size_type ie,
                                   jcntrl_para_loop_func *func, void *arg,
                                   int *shared_ret)
{
#ifdef _OPENMP
  int lret = 1;
#endif

#ifdef _OPENMP
#pragma omp for
#endif
  for (jcntrl_size_type jj = is; jj < ie; ++jj) {
#ifdef _OPENMP
    if (!jcntrl_omp_get_error(shared_ret, &lret))
      continue;
#endif

    if (!func(jj, arg)) {
#ifdef _OPENMP
      jcntrl_omp_set_error(shared_ret, &lret);
      continue;
#else
      *shared_ret = 0;
      return 0;
#endif
    }
  }

#ifdef _OPENMP
#pragma omp atomic read
  lret = *shared_ret;
  return lret;
#else
  return 1;
#endif
}

JUPITER_CONTROL_DECL_END

#endif
