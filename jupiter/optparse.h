#ifndef JUPITER_OPTPARSE_H
#define JUPITER_OPTPARSE_H

#include "csv.h"
#include "geometry/bitarray.h"
#include "csvutil.h"
#include "jupiter/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct jupiter_options
{
  const char *flags_file;
  const char *param_file;
  const char *geom_file;
  const char *plist_file;
  const char *control_file;
  int restart;
  int restart_job;
  int post_s;
  int post_e;
  int print_my_rank;
  const char *print_rank;
  jupiter_print_levels print_levels;
};
typedef struct jupiter_options jupiter_options;

/**
 * @brief Option parser for JUPITER
 * @param opts Pointer to store parse result
 * @param argc Number of arguments
 * @param argv list of arguments to parse
 * @return CSV_ERR_SUCC if success, otherwise failed.
 *         CSV_ERR_NOMEM means successfully parsed, but argv is not
 *         changed.
 *
 * argc and argv will be modified and collects not parsed arguments.
 * For example, if you run JUPITER as following,
 * ```
 *    $ ./run -input param.txt foo.dat -- -flags flags.txt
 * ```
 * new argc will be 4 and new argv contains
 * ```
 *   argv[0] = "./run";     // Command name.
 *   argv[1] = "foo.dat";   // This parameter not flagged.
 *   argv[2] = "-flags";    // After `--` flag, parameters will not be parsed
 *   argv[3] = "flags.txt"; // and remained here.
 * ```
 * The content of argv is not changed on parse error.
 *
 * @note MPI standard allows to edit argv in `MPI_init`, and `argv`
 *       may differ to the one for running as serial process, before
 *       `MPI_Init` is called.  This means you must call `MPI_Init`
 *       before you call this function to remove those extranous
 *       parameters.
 */
JUPITER_DECL
csv_error jupiter_optparse(jupiter_options *opts, int *argc, char ***argv);

enum jupiter_file_inputs
{
  JUPITER_INPUT_FILE_FLAGS = 0,
  JUPITER_INPUT_FILE_PARAMS,
  JUPITER_INPUT_FILE_PLIST,
  JUPITER_INPUT_FILE_GEOM,
  JUPITER_INPUT_FILE_CONTROL,

  JUPITER_INPUT_FILE_LAST
};

struct jupiter_file_inputs_flags
{
  geom_bitarray_n(f, JUPITER_INPUT_FILE_LAST);
};
typedef struct jupiter_file_inputs_flags jupiter_file_inputs_flags;

static inline jupiter_file_inputs_flags jupiter_file_inputs_flags_all(void)
{
  jupiter_file_inputs_flags f;
  geom_bitarray_element_setall(f.f, JUPITER_INPUT_FILE_LAST, 1);
  return f;
}

static inline jupiter_file_inputs_flags jupiter_file_inputs_flags_zero(void)
{
  jupiter_file_inputs_flags f;
  geom_bitarray_element_setall(f.f, JUPITER_INPUT_FILE_LAST, 0);
  return f;
}

/**
 * Same as jupiter_optparse except for accepting specified file arguemnts only
 */
JUPITER_DECL
csv_error jupiter_optparse_files(jupiter_options *opts, int *argc, char ***argv,
                                 jupiter_file_inputs_flags *accepts);

static inline jupiter_print_levels jupiter_print_levels_zero(void)
{
  jupiter_print_levels lvls;
  geom_bitarray_element_setall(lvls.levels, CSV_EL_MAX, 0);
  return lvls;
}

static inline jupiter_print_levels jupiter_print_levels_all(void)
{
  jupiter_print_levels lvls;
  geom_bitarray_element_setall(lvls.levels, CSV_EL_MAX, 1);
  return lvls;
}

static inline jupiter_print_levels jupiter_print_levels_non_debug(void)
{
  jupiter_print_levels lvls = jupiter_print_levels_all();
  geom_bitarray_element_set(lvls.levels, CSV_EL_DEBUG, 0);
  return lvls;
}

static inline void
jupiter_options_init(jupiter_options *opts, const char *default_print_rank,
                     jupiter_print_levels default_print_levels)
{
  opts->flags_file = NULL;
  opts->param_file= NULL;
  opts->plist_file = NULL;
  opts->geom_file = NULL;
  opts->control_file = NULL;
  opts->restart = 0;
  opts->restart_job = 0;
  opts->post_s = -1;
  opts->post_e = -1;
  opts->print_my_rank = 1;
  opts->print_rank = default_print_rank;
  opts->print_levels = default_print_levels;
}

#ifdef __cplusplus
}
#endif

#endif
