/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */

/* This file is for POSIX. */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "asprintf.h"
#include "os.h"
#include <jupiter/geometry/geom_assert.h>
#include <jupiter/geometry/list.h>

/*!re2c
  re2c:define:YYCTYPE = "unsigned char";
  re2c:define:YYMARKER = "m";
  re2c:define:YYCURSOR = "p";
  re2c:yyfill:enable = 0;
  re2c:indent:string = "  ";

  dir = [/];
  end = "\x00";
*/

int get_current_directory(char **output)
{
  size_t size;
  long path_max;
  char *buf;
  char *ptr;

  path_max = pathconf(".", _PC_PATH_MAX);
  if (path_max == -1) {
    size = 1024;
  } else if (path_max > 10240) {
    size = 10240;
  } else {
    size = path_max;
  }

  buf = NULL;
  ptr = NULL;
  for (; ptr == NULL; size *= 2) {
    buf = realloc(buf, size);
    if (!buf) {
      return -1;
    }

    errno = 0;
    ptr = getcwd(buf, size);
    if (ptr == NULL && errno != ERANGE) {
      return -2;
    }
  }
  *output = buf;
  return strlen(buf) + 1;
}

char *canonicalize_path(const char *input)
{
  const char *p;
  const char *m;
  const char *t;
  char *buf;
  char *op;

  /* output will not be longer than original. */
  buf = (char *)malloc(strlen(input) + 1);
  if (!buf) return NULL;
  op = buf;

  p = input;
  for (;;) {
    t = p;
    /*!re2c
      re2c:indent:top = 2;

      *    { p = t; goto other; }
      end  { *op = '\0'; break; }
      dir+ { goto dir; }
      "." dir+ { continue; }
      "." end  { if (t == input) *op++ = '.'; *op = '\0'; break; }
      ".."(dir|end) { goto parent; }
    */

    GEOM_UNREACHABLE();

  other:
    for (;;) {
      t = p;
      /*!re2c
        re2c:indent:top = 3;

        *   { *op++ = *t; continue; }
        end { p = t; break; }
        dir { p = t; break; }
      */
      GEOM_UNREACHABLE();
    }
    continue;

  dir:
    if (t == input) { /* root */
      while (t != p) {
        *op++ = *t;
        t++;
      }
    } else {
      *op++ = '/';
    }
    continue;

  parent:
    {
      int is_abs;
      char *root;

      if (op == buf) {
        *op++ = '.';
        *op++ = '.';
        *op++ = '/';
        continue;
      }

      is_abs = 0;
      {
        char *p;
        char *m;
        p = buf;
        while (p < op) {
          /*!re2c
            re2c:indent:top = 3;

            * { p = buf; break; }
            dir+ { is_abs = 1; break; }
            ".." dir+ { continue; }
          */
          GEOM_UNREACHABLE();
        }
        root = p;
      }
      op -= 2;
      while (op >= root && *op != '/') {
        op--;
      }
      if (op < root) {
        if (is_abs) {
          op = root;
        } else {
          op = root;
          if (op != buf) {
            *op++ = '.';
            *op++ = '.';
            *op++ = '/';
          }
        }
      } else {
        *op++ = '/';
      }
      continue;
    }
  }

  return buf;
}


int join_filenames(char **output, const char *dir, const char *file)
{
  int n;

  GEOM_ASSERT(output);
  GEOM_ASSERT(dir);
  GEOM_ASSERT(file);

  n = strlen(dir) - 1;
  if (n < 0) {
    *output = jupiter_strdup(file);
    if (*output) {
      return strlen(*output) + 1;
    }
    return -1;
  }
  if (dir[n] == '/') {
    return jupiter_asprintf(output, "%s%s", dir, file);
  } else {
    return jupiter_asprintf(output, "%s/%s", dir, file);
  }
}

int change_directory(const char *path)
{
  GEOM_ASSERT(path);

  errno = 0;
  return chdir(path);
}

int make_directory(const char *path)
{
  GEOM_ASSERT(path);

  errno = 0;
  return mkdir(path, 0777);
}

int make_directory_recursive(const char *path)
{
  int r;
  int owd;
  char *buf;
  const char *p;
  const char *t;
  const char *tt;
  int wd;
  size_t mxpart;
  int flg;
  int mrk;

  GEOM_ASSERT(path);

  flg  = O_RDONLY | O_NOCTTY | O_NONBLOCK;
#ifdef O_NOFOLLOW
  flg |= O_NOFOLLOW;
#endif
#ifdef O_DIRECTORY
  flg |= O_DIRECTORY;
#endif

  mxpart = strlen(path);
  if (mxpart == 0) { /* Empty path */
    return 0;
  }
  mxpart += 1;

  r = 0;
  errno = 0;

  /*
   * fchdir should be available on all BSD platforms.
   *
   * fchdir should be available on any platforms which respect SUSv2
   * and later (_XOPEN_SOURCE >= 500).
   *
   * fchdir should be available on any platforms which respect POSIX.1-2008
   * and later (_POSIX_C_SOURCE >= 200809L)
   */

  owd = open(".", flg);
  if (owd < 0) {
    return 1;
  }

  buf = (char *)malloc(sizeof(char) * mxpart);
  if (!buf) {
    errno = ENOMEM;
    goto error;
  }

  p = path;
  t = path;
  mrk = 0;
  for (;;) {
    tt = p;
    /*!re2c
      re2c:indent:top = 2;

      *    { mrk = 1; continue; }
      end  { if (!mrk) break; goto make_dir; }
      dir+ { if (!mrk) continue; goto make_dir; }
     */

    GEOM_UNREACHABLE();

  make_dir:
    strncpy(buf, t, tt - t);
    buf[tt - t] = '\0';

    errno = 0;
    r = mkdir(buf, 0777);
    if (r < 0 && errno != EEXIST) {
      goto error;
    }
    if (errno == EEXIST) {
      errno = 0;
      r = chdir(buf);
      if (r < 0) goto error;
    } else {
      wd = open(buf, flg);
      if (wd < 0) goto error;
      r = fchdir(wd);
      close(wd);
      if (r < 0) goto error;
    }

    t = p;
    mrk = 0;
    if (*tt == '\0') break; /* end */
    continue;
  }

clean:
  fchdir(owd);
  close(owd);
  free(buf);
  return r;

error:
  r = 1;
  goto clean;
}

int extract_dirname_allocate(char **buf, const char *path)
{
  const char *lsl;
  const char *lslp;
  const char *p;
  const char *t;
  const char *tt;

  lsl = NULL;

  p = path;
  t = path;
  for (;;) {
    tt = p;
    /*!re2c
      re2c:indent:top = 2;

      *        { continue; }
      end      { break; }
      dir+/end { if (!lsl) { lsl = tt; lslp = p; } break; }
      dir+     { lsl = tt; lslp = p; continue; }
    */

    GEOM_UNREACHABLE();
  }
  if (lsl) {
    if (lsl == path) lsl = lslp;
    return jupiter_asprintf(buf, "%.*s", lsl - path, path);
  }
  return jupiter_asprintf(buf, "%s", ".");
}

int extract_basename_allocate(char **buf, const char *path)
{
  const char *root;
  const char *lsl;
  const char *lslp;
  const char *p;
  const char *t;
  const char *tt;

  lsl = NULL;
  lslp = NULL;
  root = NULL;

  p = path;
  t = path;
  for (;;) {
    tt = p;
    /*!re2c
      re2c:indent:top = 2;

      *        { continue; }
      end      { break; }
      dir+/end { lslp = tt; break; }
      dir+     { lsl = p; continue; }
    */

    GEOM_UNREACHABLE();
  }
  if (lsl) {
    if (lslp) {
      return jupiter_asprintf(buf, "%.*s", lslp - lsl, lsl);
    } else {
      return jupiter_asprintf(buf, "%s", lsl);
    }
  }
  if (lslp) {
    if (lslp != path) {
      return jupiter_asprintf(buf, "%.*s", lslp - path, path);
    } else {
      return jupiter_asprintf(buf, "%s", path);
    }
  }
  return jupiter_asprintf(buf, "%s", path);
}

/* Content of glob_data is OS-dependent. */
struct glob_data
{
  const char *pat;
  ptrdiff_t curs;
  glob_t glob;
  glob_error_func *func;
  void *earg;
};

struct glob_data *glob_new(const char *pattern)
{
  struct glob_data *p;
  p = (struct glob_data *)malloc(sizeof(struct glob_data));
  if (!p) return NULL;
  p->pat = pattern;
  p->curs = -1;
  p->glob.gl_offs = 0;
  p->glob.gl_pathc = 0;
  return p;
}

void glob_error_set(struct glob_data *p, glob_error_func *func, void *arg)
{
  GEOM_ASSERT(p);
  p->func = func;
  p->earg = arg;
}

static struct glob_data *glob_run_p = NULL;

static int glob_err(const char *epath, int eerrno)
{
  if (glob_run_p->func)
    glob_run_p->func(glob_run_p, epath, eerrno, glob_run_p->earg);
  return 0;
}

int glob_run(struct glob_data *p)
{
  int r;
  glob_run_p = p;
  r = glob(p->pat, GLOB_MARK, glob_err, &p->glob);
  glob_run_p = NULL;
  if (r == GLOB_NOMATCH) {
    p->glob.gl_pathc = 0;
    return 0;
  }
  if (r == 0) {
    return 0;
  }
  return 1;
}

const char *glob_next(struct glob_data *p)
{
  p->curs++;
  if (p->curs < 0 || (size_t)p->curs >= p->glob.gl_pathc) {
    p->curs = p->glob.gl_pathc;
    return NULL;
  }
  return p->glob.gl_pathv[p->curs];
}

const char *glob_prev(struct glob_data *p)
{
  p->curs--;
  if (p->curs < 0) {
    p->curs = -1;
    return NULL;
  }
  return p->glob.gl_pathv[p->curs];
}

void glob_rewind(struct glob_data *p)
{
  p->curs = -1;
}

void glob_free(struct glob_data *p)
{
  globfree(&p->glob);
  free(p);
}

double cpu_time(void)
{
  struct timeval tm;
  double t;
  static int base_sec = 0,  base_usec = 0;

  /*
   * YSE: Function gettimeofday is obsolescent.
   *      See gettimeofday(2) or gettimeofday(3p) for replacement.
   */
  gettimeofday(&tm, NULL);
  if(base_sec == 0 && base_usec == 0) {
    base_sec = tm.tv_sec;   base_usec = tm.tv_usec;
    t = 0.0;
  } else {
    t = (double) (tm.tv_sec-base_sec) +
      ((double) (tm.tv_usec-base_usec))/1.0e6;
  }
  return t;
}

int jupiter_sleep(unsigned int milliseconds)
{
  struct timespec tm;
  tm.tv_sec = milliseconds / 1000;
  tm.tv_nsec = (milliseconds % 1000) * 1000000;

  return nanosleep(&tm, NULL);
}

int jupiter_get_terminal_width(void)
{
  struct winsize wsz;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsz) == -1)
    return 0;

  return wsz.ws_col;
}
