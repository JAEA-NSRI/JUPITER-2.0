/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */

/* This file is for win32. */

#include "os.h"
#include "asprintf.h"
#include <jupiter/geometry/list.h>
#include <jupiter/geometry/geom_assert.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <windows.h>

/*!re2c
  re2c:define:YYCTYPE = "unsigned char";
  re2c:define:YYMARKER = "m";
  re2c:define:YYCURSOR = "p";
  re2c:yyfill:enable = 0;
  re2c:indent:string = "  ";

  dir = ("\\"|"/");
  end = "\x00";
*/

/*
 * Obtained from: http://repo.or.cz/w/git/mingw.git/blame/15bf7ff5c74b0ef56b0fa4ab9f483399f9a20f35:/compat/mingw.c
 *
 * You must obey LGPL-2.1.
 */
static int err_win_to_posix(DWORD winerr)
{
  int error = ENOSYS;
  switch(winerr) {
  case ERROR_ACCESS_DENIED: error = EACCES; break;
  case ERROR_ACCOUNT_DISABLED: error = EACCES; break;
  case ERROR_ACCOUNT_RESTRICTION: error = EACCES; break;
  case ERROR_ALREADY_ASSIGNED: error = EBUSY; break;
  case ERROR_ALREADY_EXISTS: error = EEXIST; break;
  case ERROR_ARITHMETIC_OVERFLOW: error = ERANGE; break;
  case ERROR_BAD_COMMAND: error = EIO; break;
  case ERROR_BAD_DEVICE: error = ENODEV; break;
  case ERROR_BAD_DRIVER_LEVEL: error = ENXIO; break;
  case ERROR_BAD_EXE_FORMAT: error = ENOEXEC; break;
  case ERROR_BAD_FORMAT: error = ENOEXEC; break;
  case ERROR_BAD_LENGTH: error = EINVAL; break;
  case ERROR_BAD_PATHNAME: error = ENOENT; break;
  case ERROR_BAD_PIPE: error = EPIPE; break;
  case ERROR_BAD_UNIT: error = ENODEV; break;
  case ERROR_BAD_USERNAME: error = EINVAL; break;
  case ERROR_BROKEN_PIPE: error = EPIPE; break;
  case ERROR_BUFFER_OVERFLOW: error = ENAMETOOLONG; break;
  case ERROR_BUSY: error = EBUSY; break;
  case ERROR_BUSY_DRIVE: error = EBUSY; break;
  case ERROR_CALL_NOT_IMPLEMENTED: error = ENOSYS; break;
  case ERROR_CANNOT_MAKE: error = EACCES; break;
  case ERROR_CANTOPEN: error = EIO; break;
  case ERROR_CANTREAD: error = EIO; break;
  case ERROR_CANTWRITE: error = EIO; break;
  case ERROR_CRC: error = EIO; break;
  case ERROR_CURRENT_DIRECTORY: error = EACCES; break;
  case ERROR_DEVICE_IN_USE: error = EBUSY; break;
  case ERROR_DEV_NOT_EXIST: error = ENODEV; break;
  case ERROR_DIRECTORY: error = EINVAL; break;
  case ERROR_DIR_NOT_EMPTY: error = ENOTEMPTY; break;
  case ERROR_DISK_CHANGE: error = EIO; break;
  case ERROR_DISK_FULL: error = ENOSPC; break;
  case ERROR_DRIVE_LOCKED: error = EBUSY; break;
  case ERROR_ENVVAR_NOT_FOUND: error = EINVAL; break;
  case ERROR_EXE_MARKED_INVALID: error = ENOEXEC; break;
  case ERROR_FILENAME_EXCED_RANGE: error = ENAMETOOLONG; break;
  case ERROR_FILE_EXISTS: error = EEXIST; break;
  case ERROR_FILE_INVALID: error = ENODEV; break;
  case ERROR_FILE_NOT_FOUND: error = ENOENT; break;
  case ERROR_GEN_FAILURE: error = EIO; break;
  case ERROR_HANDLE_DISK_FULL: error = ENOSPC; break;
  case ERROR_INSUFFICIENT_BUFFER: error = ENOMEM; break;
  case ERROR_INVALID_ACCESS: error = EACCES; break;
  case ERROR_INVALID_ADDRESS: error = EFAULT; break;
  case ERROR_INVALID_BLOCK: error = EFAULT; break;
  case ERROR_INVALID_DATA: error = EINVAL; break;
  case ERROR_INVALID_DRIVE: error = ENODEV; break;
  case ERROR_INVALID_EXE_SIGNATURE: error = ENOEXEC; break;
  case ERROR_INVALID_FLAGS: error = EINVAL; break;
  case ERROR_INVALID_FUNCTION: error = ENOSYS; break;
  case ERROR_INVALID_HANDLE: error = EBADF; break;
  case ERROR_INVALID_LOGON_HOURS: error = EACCES; break;
  case ERROR_INVALID_NAME: error = EINVAL; break;
  case ERROR_INVALID_OWNER: error = EINVAL; break;
  case ERROR_INVALID_PARAMETER: error = EINVAL; break;
  case ERROR_INVALID_PASSWORD: error = EPERM; break;
  case ERROR_INVALID_PRIMARY_GROUP: error = EINVAL; break;
  case ERROR_INVALID_SIGNAL_NUMBER: error = EINVAL; break;
  case ERROR_INVALID_TARGET_HANDLE: error = EIO; break;
  case ERROR_INVALID_WORKSTATION: error = EACCES; break;
  case ERROR_IO_DEVICE: error = EIO; break;
  case ERROR_IO_INCOMPLETE: error = EINTR; break;
  case ERROR_LOCKED: error = EBUSY; break;
  case ERROR_LOCK_VIOLATION: error = EACCES; break;
  case ERROR_LOGON_FAILURE: error = EACCES; break;
  case ERROR_MAPPED_ALIGNMENT: error = EINVAL; break;
  case ERROR_META_EXPANSION_TOO_LONG: error = E2BIG; break;
  case ERROR_MORE_DATA: error = EPIPE; break;
  case ERROR_NEGATIVE_SEEK: error = ESPIPE; break;
  case ERROR_NOACCESS: error = EFAULT; break;
  case ERROR_NONE_MAPPED: error = EINVAL; break;
  case ERROR_NOT_ENOUGH_MEMORY: error = ENOMEM; break;
  case ERROR_NOT_READY: error = EAGAIN; break;
  case ERROR_NOT_SAME_DEVICE: error = EXDEV; break;
  case ERROR_NO_DATA: error = EPIPE; break;
  case ERROR_NO_MORE_SEARCH_HANDLES: error = EIO; break;
  case ERROR_NO_PROC_SLOTS: error = EAGAIN; break;
  case ERROR_NO_SUCH_PRIVILEGE: error = EACCES; break;
  case ERROR_OPEN_FAILED: error = EIO; break;
  case ERROR_OPEN_FILES: error = EBUSY; break;
  case ERROR_OPERATION_ABORTED: error = EINTR; break;
  case ERROR_OUTOFMEMORY: error = ENOMEM; break;
  case ERROR_PASSWORD_EXPIRED: error = EACCES; break;
  case ERROR_PATH_BUSY: error = EBUSY; break;
  case ERROR_PATH_NOT_FOUND: error = ENOENT; break;
  case ERROR_PIPE_BUSY: error = EBUSY; break;
  case ERROR_PIPE_CONNECTED: error = EPIPE; break;
  case ERROR_PIPE_LISTENING: error = EPIPE; break;
  case ERROR_PIPE_NOT_CONNECTED: error = EPIPE; break;
  case ERROR_PRIVILEGE_NOT_HELD: error = EACCES; break;
  case ERROR_READ_FAULT: error = EIO; break;
  case ERROR_SEEK: error = EIO; break;
  case ERROR_SEEK_ON_DEVICE: error = ESPIPE; break;
  case ERROR_SHARING_BUFFER_EXCEEDED: error = ENFILE; break;
  case ERROR_SHARING_VIOLATION: error = EACCES; break;
  case ERROR_STACK_OVERFLOW: error = ENOMEM; break;
  case ERROR_SWAPERROR: error = ENOENT; break;
  case ERROR_TOO_MANY_MODULES: error = EMFILE; break;
  case ERROR_TOO_MANY_OPEN_FILES: error = EMFILE; break;
  case ERROR_UNRECOGNIZED_MEDIA: error = ENXIO; break;
  case ERROR_UNRECOGNIZED_VOLUME: error = ENODEV; break;
  case ERROR_WAIT_NO_CHILDREN: error = ECHILD; break;
  case ERROR_WRITE_FAULT: error = EIO; break;
  case ERROR_WRITE_PROTECT: error = EROFS; break;
  }
  return error;
}

int get_current_directory(char **output)
{
  char *buf;
  DWORD size;

  size = MAX_PATH;

  size = GetCurrentDirectory(0, NULL);
  if (size == 0) {
    errno = err_win_to_posix(GetLastError());
    return -2;
  }

  buf = malloc(size);
  if (!buf) {
    return -1;
  }

  GetCurrentDirectory(size, buf);

  *output = buf;
  return size;
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

    CSVUNREACHABLE();

  other:
    for (;;) {
      t = p;
      /*!re2c
        re2c:indent:top = 3;

        *   { *op++ = *t; continue; }
        end { p = t; break; }
        dir { p = t; break; }
      */
      CSVUNREACHABLE();
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
            [a-zA-Z]":" dir+ { is_abs = 1; break; }
            dir+ { is_abs = 1; break; }
            ".." dir+ { continue; }
          */
          CSVUNREACHABLE();
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
  if (dir[n] == '/' || dir[n] == '\\') {
    return jupiter_asprintf(output, "%s%s", dir, file);
  } else {
    return jupiter_asprintf(output, "%s/%s", dir, file);
  }
}

int change_directory(const char *path)
{
  BOOL b;

  GEOM_ASSERT(path);

  errno = 0;
  b = SetCurrentDirectory(path);
  if (!b) {
    errno = err_win_to_posix(GetLastError());
    return 1;
  }
  return 0;
}

int make_directory(const char *path)
{
  BOOL b;

  GEOM_ASSERT(path);

  errno = 0;
  b = CreateDirectoryA(path, NULL);
  if (!b) {
    errno = err_win_to_posix(GetLastError());
    return 1;
  }
  return 0;
}

int make_directory_recursive(const char *path)
{
  DWORD d;
  BOOL b;
  int r;
  HANDLE owd;
  LPWSTR owdp;
  DWORD owdps;
  char *buf;
  const char *p;
  const char *t;
  const char *tt;
  int wd;
  size_t mxpart;

  int flg;
  int mrk;

  GEOM_ASSERT(path);

  mxpart = strlen(path);
  if (mxpart == 0) { /* Empty path */
    return 0;
  }
  mxpart += 1;

  r = 0;
  errno = 0;

  owd = CreateFileA(".", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS, NULL);
  if (owd == INVALID_HANDLE_VALUE) {
    errno = err_win_to_posix(GetLastError());
    return 1;
  }

  owdps = GetCurrentDirectoryW(0, NULL);
  owdp = (LPWSTR)malloc(sizeof(WCHAR) * owdps);
  buf = (char *)malloc(sizeof(char) * mxpart);
  if (!buf || !owdp) {
    errno = ENOMEM;
    goto error;
  }

  GetCurrentDirectoryW(owdps, owdp);

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
    b = CreateDirectory(buf, NULL);
    if (!b) {
      errno = err_win_to_posix(GetLastError());
      if (errno != EEXIST) goto error;
    } else {
#ifndef NDEBUG
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG, NULL,
                 "Directory `%.*s' created.", tt - path, path);
#endif
    }
    errno = 0;
    b = SetCurrentDirectoryA(buf);
    if (!b) {
      errno = err_win_to_posix(GetLastError());
      goto error;
    }

    t = p;
    mrk = 0;
    if (*tt == '\0') break; /* end */
    continue;
  }

clean:
  CloseHandle(owd);
  if (owdp) {
    SetCurrentDirectoryW(owdp);
  }
  free(owdp);
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

struct glob_entry
{
  struct geom_list list;
  char *path;
};

struct glob_data
{
  const char *pat;
  struct glob_entry *curs;
  struct glob_entry head;
  WIN32_FIND_DATA data;
  glob_error_func *func;
  void *earg;
};

#define glob_entry_data(ptr) geom_list_entry(ptr, struct glob_entry, list)

struct glob_data *glob_new(const char *pattern)
{
  struct glob_data *p;
  p = (struct glob_data *)malloc(sizeof(struct glob_data));
  if (!p) return NULL;
  p->pat = pattern;
  geom_list_init(&p->head.list);
  p->head.path = NULL;
  p->curs = &p->head;
  return p;
}

void glob_error_set(struct glob_data *p, glob_error_func *func, void *arg)
{
  GEOM_ASSERT(p);

  p->func = func;
  p->earg = arg;
}

static
void glob_free_list(struct glob_data *p)
{
  struct geom_list *lp, *ln;
  struct glob_entry *ent;

  geom_list_foreach_safe(lp, ln, &p->head.list) {
    ent = glob_entry_data(lp);
    free(ent->path);
    free(ent);
  }
}

static
int add_glob_entry(struct glob_data *p, const char *dir, const char *filename)
{
  struct glob_entry *e;
  int r;

  e = (struct glob_entry *)malloc(sizeof(struct glob_entry));
  if (!e) {
    return 1;
  }
  r = join_filenames(&e->path, dir, filename);
  if (r < 0) {
    free(e);
    return 1;
  }
  geom_list_insert_prev(&p->head.list, &e->list);
  return 0;
}

int glob_run(struct glob_data *p)
{
  /**
   * @todo In this program, "a/*foo/b" won't match any files or
   *       directories (even if it exists). This is problematic if the
   *       user used a "%n/time.dat" as the template of 'time.dat'
   *       file name.
   */

  char *dir;
  HANDLE h;
  DWORD d;
  struct glob_entry *e;
  int r;

  glob_free_list(p);

  r = extract_dirname_allocate(&dir, p->pat);
  if (r < 0) {
    errno = ENOMEM;
    if (p->func)
      p->func(p, NULL, errno, p->earg);
    return 1;
  }

  r = 0;
  h = FindFirstFile(p->pat, &p->data);
  if (h == INVALID_HANDLE_VALUE) {
    d = GetLastError();
    if (d == ERROR_FILE_NOT_FOUND) {
      return 0;
    }
    errno = err_win_to_posix(d);
    if (p->func)
      p->func(p, NULL, errno, p->earg);
    return 1;
  }

  r = add_glob_entry(p, dir, p->data.cFileName);
  if (r) {
    errno = ENOMEM;
    if (p->func)
      p->func(p, NULL, errno, p->earg);
    goto error;
  }

  while (FindNextFile(h, &p->data)) {
    r = add_glob_entry(p, dir, p->data.cFileName);
    if (r) {
      errno = ENOMEM;
      if (p->func)
        p->func(p, NULL, errno, p->earg);
      goto error;
    }
  }
  d = GetLastError();
  if (d != ERROR_NO_MORE_FILES) {
    errno = err_win_to_posix(d);
    if (p->func)
      p->func(p, NULL, errno, p->earg);
    goto error;
  }

 clean:
  CloseHandle(h);
  return r;

 error:
  r = 1;
  goto clean;
}

const char *glob_next(struct glob_data *p)
{
  p->curs = glob_entry_data(geom_list_next(&p->curs->list));
  return p->curs->path;
}

const char *glob_prev(struct glob_data *p)
{
  p->curs = glob_entry_data(geom_list_prev(&p->curs->list));
  return p->curs->path;
}

void glob_rewind(struct glob_data *p)
{
  p->curs = &p->head;
}

void glob_free(struct glob_data *p)
{
  glob_free_list(p);
  free(p);
}

double cpu_time(void)
{
  int r;
  LARGE_INTEGER time;
  LARGE_INTEGER f;
  double meas;

  r = QueryPerformanceCounter(&time);
  meas = 0.0;
  if (r != 0) {
    r = QueryPerformanceFrequency(&f);
    if (r != 0) {
      meas = (double)time.QuadPart / f.QuadPart;
    }
  }
  return meas;
}

int jupiter_sleep(unsigned int milliseconds)
{
  DWORD ms = (DWORD)milliseconds;
  if (ms < 0) {
    errno = ERANGE;
    return -1;
  }

  if (SleepEx(ms, TRUE)) {
    DWORD d = GetLastError();
    errno = err_win_to_posix(d);
    return -1;
  }
  return 0;
}

int jupiter_get_terminal_width(void)
{
  CONSOLE_SCREEN_BUFFER_INFO info;
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  if (h == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(h, &info)) {
    return 0;
  }

  return info.srWindow.Right - info.srWindow.Left + 1;
}
