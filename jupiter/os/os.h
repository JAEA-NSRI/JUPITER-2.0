
#ifndef JUPITER_OS_H
#define JUPITER_OS_H

#include "defs.h"

JUPITER_OS_DECL_START

/**
 * @addtogroup OS_specific
 * @brief OS dependent features
 */

/**
 * @ingroup OS_specific
 * @brief Get Current Directory
 * @param output Location to set result.
 * @retval -1 Allocation failed
 * @return Number of bytes written
 *
 * Get current directory full path.
 */
JUPITER_OS_DECL
int get_current_directory(char **output);

/**
 * @ingroup OS_specific
 * @brief Canonicalize Path
 * @param input NUL-terminated Input path name
 * @return canonicalized path, allocation failed if NULL.
 *
 * Makes an canonicalized paths.
 *
 * Relative paths are kept in relative. If you need absolute paths,
 * get current directory with get_current_directory(), join names with
 * join_filenames() and then use canonicalize_path() to resolve
 * relative elements.
 *
 * Returned paths are allocated by malloc().
 *
 * In POSIX environment, these paths will be cleaned:
 *  - Two or more continuous `/`s (except for beginning).
 *  - Handle directory names `.` and `..`.
 *
 * Difference to POSIX realpath() function is:
 *  - Does not access any actual disk including, i.e:
 *    - Does not follow symbolic links
 *    - Does not test read permission for each directories.
 *
 * In Win32 environment, these paths will be cleaned:
 *  - Two or more continuous `/` or `\`s (except for beginning).
 *  - Handle directory names `.` and `..`.
 *  - Convert `\` to `/`. (i.e., creates MinGW name)
 *
 * Difference to Win32 API PathCanonicalize(),
 * PathAllocCanonicalize(), PathCchCanonicalize() functions is:
 *  - Generates `/` separated path (see above)
 *    - These path names are accepted by Win32API, Standard C library,
 *      explorer (on Windows 10, at least), cmd (on Windows 10, at
 *      least), and powershell (on Windows 10, at least).
 *  - May generarate incorrect result for non-ASCII paths
 *  - `:`, `;`, `(`, `)`, `*`, `?` and any other invalid charactors
 *    except for `/` and `\` are regular part of filename.
 *  - `\?` and `\?\UNC` prefixes are not supported.
 *
 * Implementation note: This function must be done without accessing
 * actual disk (@p input may be a filename pattern, or does not exist).
 */
JUPITER_OS_DECL
char *canonicalize_path(const char *input);

/**
 * @ingroup OS_specific
 * @brief Join directory and file.
 * @param output Location to set result.
 * @param dir Directory
 * @param file Filename
 * @retval -1 Allocation failed
 * @retval -2 Other system error (refer errno)
 * @return Number of bytes written
 *
 * Joins directory and file names togather.
 *
 * In Windows, use separator '\'. In POSIX, use separitor '/'.
 */
JUPITER_OS_DECL
int join_filenames(char **output, const char *dir, const char *file);

/**
 * @ingroup OS_specific
 * @brief Change diretory
 *
 * @param path Path to change
 *
 * @retval 0 Success
 * @retval (others) Failed
 *
 * errno may be set to indicate the error.
 */
JUPITER_OS_DECL
int change_directory(const char *path);

/**
 * @ingroup OS_specific
 * @brief Create directory
 * @param path Path to make a directory.
 *
 * @retval 0 Success
 * @retval (others) Failed
 *
 * errno may be set to indicate the error.
 *
 * Whole directory part of given path must be exist. If the given path
 * is "a/b/c" and "a/b" does not exist, this function will fail.
 *
 * If path is already existed as a directory, this function does
 * nothing and returns 0.
 */
JUPITER_OS_DECL
int make_directory(const char *path);

/**
 * @ingroup OS_specific
 * @brief Create directory resursively
 *
 * @param path Path to make a directory.
 *
 * @retval 0 Success
 * @retval (others) Failed
 *
 * errno may be set to indicate the error.
 */
JUPITER_OS_DECL
int make_directory_recursive(const char *path);

/**
 * @ingroup OS_specific
 * @brief Extract directory part of path.
 *
 * @param buf Location to store the result
 * @param path Path to extract.
 *
 * @return Number of bytes written. Negative value when failed.
 *
 * Pointer to buf must be freed by free().
 */
JUPITER_OS_DECL
int extract_dirname_allocate(char **buf, const char *path);

/**
 * @ingroup OS_specific
 * @brief Extract filename part of path.
 *
 * @param buf Location to store the result
 * @param path Path to extract
 *
 * @return Number of bytes written.
 *
 * Pointer to buf must be freed by free().
 */
JUPITER_OS_DECL
int extract_basename_allocate(char **buf, const char *path);

/* Content of glob_data is OS_dependent. */
struct glob_data;

/**
 * @ingroup OS_specific
 *
 * Glob error handler
 *
 * @param path Pathname that the error produced, if applicable (NULL if not)
 * @param eerrno (translated) errno value
 * @param arg Extra argument that passed for glob_error_set().
 */
typedef void glob_error_func(struct glob_data *p, const char *path, int eerrno,
                             void *arg);

/**
 * @ingroup OS_specific
 * @brief Create a new glob instance.
 * @param pattern Glob pattern.
 * @return Created instance, NULL if failed.
 *
 * Format of pattern is not currently unspecified.
 *
 * '*' (matches any number of any charactors) must be supported.
 */
JUPITER_OS_DECL
struct glob_data *glob_new(const char *pattern);

/**
 * @ingroup OS_specific
 * @brief Set error handler while running glob
 * @param p Glob instance to set
 * @param func Function to set
 * @param arg Extra argument passed to @p func.
 */
JUPITER_OS_DECL
void glob_error_set(struct glob_data *p, glob_error_func *func, void *arg);

/**
 * @ingroup OS_specific
 * @brief Run glob
 * @param p Glob instance to run
 * @return 0 if success, otherwise failed.
 *
 * Note: The order of the result is implementation defined.
 *
 * @warning This function is **not** thread safe.
 */
JUPITER_OS_DECL
int glob_run(struct glob_data *p);

/**
 * @ingroup OS_specific
 * @brief Get next globbed entry
 * @param p Glob instance
 *
 * @return Path to text or NULL if getting next of the last path.
 */
JUPITER_OS_DECL
const char *glob_next(struct glob_data *p);

/**
 * @ingroup OS_specific
 * @brief Get previous globbed entry
 * @param p Glob instance
 *
 * @return Path to text or NULL if getting prev of the first path.
 */
JUPITER_OS_DECL
const char *glob_prev(struct glob_data *p);

/**
 * @ingroup OS_specific
 * @brief Goto first entry
 * @param p Glob instance
 */
JUPITER_OS_DECL
void glob_rewind(struct glob_data *p);

/**
 * @ingroup OS_specific
 * @brief Free allocated glob data.
 * @param p Glob instance
 */
JUPITER_OS_DECL
void glob_free(struct glob_data *p);

/**
 * @ingroup OS_specific
 * @brief Sleep given amount of time in milliseconds
 * @return 0 successfully sleeping for requested duration, -1 for interrupted.
 *
 * This function is used for testing purpose.
 */
JUPITER_OS_DECL
int jupiter_sleep(unsigned int milliseconds);

/**
 * Get CPU time in seconds
 */
JUPITER_OS_DECL
double cpu_time(void);

/**
 * @ingroup OS_specific
 * @brief Get terminal width
 * @return terminal width, or 0 if the output is not a terminal.
 */
JUPITER_OS_DECL
int jupiter_get_terminal_width(void);

JUPITER_OS_DECL_END

#endif
