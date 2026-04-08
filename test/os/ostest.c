#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#include <jupiter/csvutil.h>
#include <jupiter/os/os.h>

static const char *dirpats[] = {
  "", "/", "//", "/a", "//a", "/a/b/c", "a//b/c", "a/b//c",
  "a", "../a", "..",
#ifdef WIN32
  "\\", "\\\\", "\\a", "\\\\a", "\\a\\b\\c", "a\\\\b\\c", "a\\b\\\\c",
  "..\\a",
#endif
  NULL
};

static int test_canonical(const char *pat, const char *expect)
{
  char *rest;
  int r;

  rest = canonicalize_path(pat);
  if (rest) {
    r = strcmp(expect, rest);
    if (r != 0) {
      fprintf(stderr,
              "Canonicalized path of \"%s\":\n"
              "    expect: \"%s\"\n"
              "   but got: \"%s\"\n",
              pat, expect, rest);
    } else {
      fprintf(stderr,
              "Canonicalized path of \"%s\" -> \"%s\"\n",
              pat, rest);
    }
    free(rest);
    return r != 0;
  }
  csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM,
            0, 0, NULL);
  return 1;
}

static int test_glob(const char *pat)
{
  int r;
  struct glob_data *glob;
  const char *path;

  glob = glob_new(pat);
  if (!glob) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_NOMEM,
              0, 0, NULL);
    return 1;
  }

  r = glob_run(glob);
  if (r) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, "Glob failed", NULL);
  }

  r = 0;
  while ((path = glob_next(glob)) != NULL) {
    fprintf(stderr, "globbed path: \"%s\"\n", path);
    r++;
  }
  if (r == 0) {
    fprintf(stderr, "glob: no matches found\n");
  }

  glob_free(glob);
  return 0;
}

int main(int argc, char **argv)
{
  const char **p;
  int r = 0;

#ifdef JUPITER_MPI
  MPI_Init(&argc, &argv);
#endif

  test_glob("c*");
  test_glob("*/CMakeFiles");

  errno = 0;
  make_directory("sandbox");
  csvperror("sandbox", 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0, NULL);

  errno = 0;
  make_directory("sandbox/error/error");
  csvperror("sandbox/error/error", 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0, NULL);

  errno = 0;
  make_directory_recursive("sandbox/foo/bar/too");
  csvperror("sandbox/foo/bar/too", 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0, NULL);

  errno = 0;
  make_directory_recursive("sandbox///trailing/slash/");
  csvperror("sandbox///trailing/slash", 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0, NULL);

  errno = 0;
  make_directory_recursive("//./tmp/test/fullpath");
  csvperror("//./tmp/test/fullpath", 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0, NULL);

  errno = 0;
  make_directory_recursive("//");
  csvperror("//", 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0, NULL);

  errno = 0;
  r = make_directory_recursive("sandbox/test");
  csvperror("sandbox/test", 0, 0, CSV_EL_ERROR, "mkdir", CSV_ERR_SYS, errno, 0, NULL);
  if (r == 0) {
    errno = 0;
    r = change_directory("sandbox/test");
    csvperror("sandbox/test", 0, 0, CSV_EL_ERROR, "cd", CSV_ERR_SYS, errno, 0, NULL);
  }
  if (r == 0) {
    errno = 0;
    r = make_directory_recursive("../foo/baz");
    csvperror("../foo/baz", 0, 0, CSV_EL_ERROR, "mkdir", CSV_ERR_SYS, errno, 0, NULL);
  }

  for (p = dirpats; *p; ++p) {
    char *b;
    r = extract_basename_allocate(&b, *p);
    if (r >= 0) {
      fprintf(stderr, "basename of '%s' -> '%s'\n", *p, b);
      free(b);
    } else {
      fprintf(stderr, "basename of '%s' (failed)\n", *p);
    }

    r = extract_dirname_allocate(&b, *p);
    if (r >= 0) {
      fprintf(stderr, "dirname of '%s' -> '%s'\n", *p, b);
      free(b);
    } else {
      fprintf(stderr, "dirname of '%s' (failed)\n", *p);
    }
  }

  test_canonical("../../../", "../../../");
  test_canonical("/../../", "/");
  test_canonical("/abc/def/../ghi", "/abc/ghi");
  test_canonical("/abc/def/../../ghi", "/ghi");
  test_canonical("abc/def/././ghi", "abc/def/ghi");
  test_canonical("abc/../../ghi", "../ghi");
  test_canonical("/abc/../../ghi", "/ghi");
  test_canonical("../abc/../../ghi", "../../ghi");
  test_canonical("//abc/def", "//abc/def");
  test_canonical("//abc//def", "//abc/def");
  test_canonical("./abc//def", "abc/def");
  test_canonical("//../ghi", "//ghi");
  test_canonical(".", ".");
  test_canonical("/", "/");
  test_canonical("", "");

#ifdef WIN32
  test_canonical("c:\\", "c:/");
  test_canonical("c:\\../windows", "c:/windows");
  test_canonical("abc\\/def//\\\\ghi", "abc/def/ghi");
#endif

  {
    double t, u;
    t = cpu_time();
    r = jupiter_sleep(1000);
    u = cpu_time();
    if (r != 0)
      fprintf(stderr, "sleep interrupted\n");
    fprintf(stderr, "time difference: %.6f\n", u - t);

    t = cpu_time();
    r = jupiter_sleep(1150);
    u = cpu_time();
    if (r != 0)
      fprintf(stderr, "sleep interrupted\n");
    fprintf(stderr, "time difference: %.6f\n", u - t);

    t = cpu_time();
    r = jupiter_sleep(350);
    u = cpu_time();
    if (r != 0)
      fprintf(stderr, "sleep interrupted\n");
    fprintf(stderr, "time difference: %.6f\n", u - t);
  }

#ifdef JUPITER_MPI
  MPI_Finalize();
#endif

  return 0;
}
