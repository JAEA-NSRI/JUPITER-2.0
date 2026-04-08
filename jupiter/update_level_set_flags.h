#ifndef UPDATE_LEVEL_SET_FLAGS_H
#define UPDATE_LEVEL_SET_FLAGS_H

#include "common.h"
#include "struct.h"
#include "func.h"
#include "serializer/defs.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize update level set flag
 * @param flags flags data to initialize
 * @param describe_reason Whether describe reason when marked
 *
 * Technically, it's not guaranteed not to unset update flag anywhere
 * else (because the struct content is public). In the function API,
 * only this function can unset the flag.
 */
static inline void
update_level_set_flags_init(update_level_set_flags *flags, int describe_reason)
{
  flags->force_update = OFF;
  flags->do_update = OFF;
  flags->describe_reason = describe_reason;
  flags->reason = UPDATE_LEVEL_SET_BY_NONE;
}

/**
 * @brief Returns true if specified flag wants to update the level set function
 * @param flags flags data to check
 * @return true if require update
 */
static inline int
update_level_set_flags_wants_update(const update_level_set_flags *flags)
{
  return flags->do_update == ON;
}

typedef void update_level_set_flags_describer_func_type(
  void *args, const update_level_set_flags *flags,
  update_level_set_reason reason);

/**
 * @brief Set function printer
 * @param func Function to print message
 * @param args Extra argument to pass args
 *
 * The function has global scope (but actually be static, i.e., not
 * accessible directly). It's ok to pass global or static data of @p
 * args.
 *
 * If given @p func is null, nothing will be printed.
 */
JUPITER_DECL
void update_level_set_flags_set_describe_reason_func(
  update_level_set_flags_describer_func_type *const func, void *args);

/**
 * @brief Get reason message for given reason of update
 * @param reason Reason to obtain
 * @return message
 *
 * The message can be concatenated by ".... is (will be) updated because ".
 */
JUPITER_DECL
const char *
update_level_set_flags_get_reason_str(update_level_set_reason reason);

/**
 * @brief Display the reason of update
 * @param flags flags to print
 *
 * A function need to be set before call this function to actually be printed
 * out (default is do nothing).
 */
JUPITER_DECL
void update_level_set_flags_describe_reason(
  const update_level_set_flags *flags);

/**
 * @brief Mark the flag to update the level set function
 * @param flags flags data to mark
 * @param reason Reason of updating level set
 */
static inline void
update_level_set_flags_mark_update(update_level_set_flags *flags,
                                   update_level_set_reason reason)
{
  flags->do_update = ON;
  flags->reason = reason;
  if (flags->describe_reason == ON)
    update_level_set_flags_describe_reason(flags);
}

/**
 * @brief Share status across MPI parallel ranks to ensure consistency.
 * @param flags Flags data to share.
 * @param mpi MPI communication information
 *
 * If the reasons are inconsistent, the reason will be set to
 * `UPDATE_LEVEL_SET_BY_NONE` for flags that is not marked.
 */
JUPITER_DECL
void update_level_set_flags_share_flag(update_level_set_flags *flags,
                                       mpi_param *mpi);

/**
 * @brief Check whether fl != 0 in a cell for marking level set update
 * @param fl fl array
 * @param cdo Domain information of fl
 * @param mpi MPI parallel information
 * @retval 1 if should be marked
 * @retval 0 if otherwise
 */
JUPITER_DECL
int update_level_set_flags_if_fl_exists(type *fl, domain *cdo, mpi_param *mpi);

/**
 * @brief Mark flags if fl != 0 in a cell
 * @param fl fl array
 * @param cdo Domain information of fl
 * @param mpi MPI parallel information
 * @param nflags Number of flags to mark
 * @param flags array of pointers to flags to mark
 * @param reason mark reason
 * @retval 1 if marked
 * @retval 0 if otherwise
 *
 * This function does not search in @p fl if all of @p flags has
 * already been marked by any reason. If @p fl has been searched, all
 * @p flags will be marked by given @p reason.
 */
static inline int update_level_set_flags_mark_if_fl_exists(
  type *fl, domain *cdo, mpi_param *mpi, int nflags,
  update_level_set_flags *flags[], update_level_set_reason reason)
{
  int need_search = 0;
  int i;

  for (i = 0; i < nflags; ++i) {
    if (!update_level_set_flags_wants_update(flags[i])) {
      need_search = 1;
      break;
    }
  }
  if (!for_any_rank(mpi, need_search))
    return 0;

  if (!update_level_set_flags_if_fl_exists(fl, cdo, mpi))
    return 0;

  for (i = 0; i < nflags; ++i)
    update_level_set_flags_mark_update(flags[i], reason);
  return 1;
}

/**
 * @brief Returns true if liquid inlet condition exists
 * @param boundary_list_head The head item of list
 * @retval 1 if exists
 * @retval 0 if otherwise
 *
 * @warning The given head item will not be counted in.
 *
 * This function is **not** MPI collective.
 */
JUPITER_DECL
int update_level_set_flags_if_liquid_inlet_exists(
  fluid_boundary_data *boundary_list_head);

/**
 * @brief Mark flags if liquid inlet condition exists
 * @param boundary_list_head The head item of list
 * @param nbcompo Number of 'base' components
 * @param nflags Number of flags to mark
 * @param flags array of pointers to flags to mark
 * @param reason mark reason
 * @retval 1 if marked
 * @retval 0 if otherwise
 *
 * This function does not search in @p boundary_list_head if all of @p
 * flags has already been marked by any reason. If @p
 * boundary_list_head has been searched, all @p flags will be marked
 * by given @p reason.
 *
 * This function is **not** MPI collective, because boundary list is
 * assumed to be common across MPI processes. If you suspect, you can
 * make it sure by update_level_set_flags_share_flag().
 */
static inline int update_level_set_flags_mark_if_liquid_inlet_exists(
  fluid_boundary_data *boundary_list_head, int nflags,
  update_level_set_flags *flags[], update_level_set_reason reason)
{
  int need_search = 0;
  int i;

  for (i = 0; i < nflags; ++i) {
    if (!update_level_set_flags_wants_update(flags[i])) {
      need_search = 1;
      break;
    }
  }
  if (!need_search)
    return 0;

  if (!update_level_set_flags_if_liquid_inlet_exists(boundary_list_head))
    return 0;

  for (i = 0; i < nflags; ++i)
    update_level_set_flags_mark_update(flags[i], reason);
  return 1;
}

/**
 * @brief Writes flags data into given file name
 * @param file_name File name to write
 * @param keyname to use
 * @param flags flags data
 * @return 0 if success, non-0 if error.
 *
 * If @p file_name does not exist, creates new one.
 *
 * This function does not do sync.
 */
JUPITER_DECL
int update_level_set_flags_write(const char *file_name, const char *keyname,
                                 update_level_set_flags *flags);

/**
 * @brief Read flags data into given file name
 * @param file_name File name to write
 * @param keyname to use
 * @param flags flags data
 * @param err Extra error information if given
 * @param eloc Sets error location (byte offset) if given
 * @return 0 if success, non-0 if error.
 */
JUPITER_DECL
int update_level_set_flags_read(const char *file_name, const char *keyname,
                                update_level_set_flags *flags,
                                msgpackx_error *err, ptrdiff_t *eloc);

#ifdef __cplusplus
}
#endif

#endif
