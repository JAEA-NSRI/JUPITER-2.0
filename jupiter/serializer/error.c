
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "defs.h"
#include "error.h"

void msgpackx_assert_impl(const char *file, long line, const char *func,
                          const char *cond, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);

#ifdef JUPITER_GEOMETRY_USE_OPENMP
#pragma omp critical
#endif
  {
    fprintf(stderr, "Assertion failed%s%s%s",
            cond ? ": " : "", cond ? cond : "", cond ? ", " : "");
    fprintf(stderr, "%s at %s(%ld)%s", func, file, line, format ? ": " : "");
    if (format) {
      vfprintf(stderr, format, ap);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
  }

  va_end(ap);

#if defined(__GNUC__) && !(defined(__PGI) || defined(__NVCOMPILER))
  __builtin_trap();
#else
  abort();
#endif
}

const char *msgpackx_strerror(enum msgpackx_error eval)
{
  switch(eval) {
  case MSGPACKX_SUCCESS:
    return "No error";
  case MSGPACKX_ERR_NOMEM:
    return "Allocation failed";
  case MSGPACKX_ERR_MAP_KEY_DUPLICATE:
    return "Map key duplicated";
  case MSGPACKX_ERR_NO_HEAD:
    return "Given map/array does not have 'head' node";
  case MSGPACKX_ERR_RANGE:
    return "Out of range (too many elements or value)";
  case MSGPACKX_ERR_MSG_TYPE:
    return "Invalid message type";
  case MSGPACKX_ERR_MSG_INCOMPLETE:
    return "Incomplete message";
  case MSGPACKX_ERR_INDEX:
    return "Index out-of-range";
  case MSGPACKX_ERR_KEYNOTFOUND:
    return "Specified key not found in map";
  case MSGPACKX_ERR_SYS:
    return "Subsidiary system call failed";
  case MSGPACKX_ERR_HEADER_SIZE:
    return "Header size does not match to expected";
  case MSGPACKX_ERR_ENDIAN:
    return "Invalid endian used or failed to parse";
  case MSGPACKX_ERR_TITLE:
    return "Data title does not match to expected";
  case MSGPACKX_ERR_EOF:
    return "Invalid data EOF mark";
  }
  return "Unknown serializer error";
}
