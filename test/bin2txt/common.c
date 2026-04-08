
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

#include "convert.h"

convert_fp *convert_file_bind(FILE *fp)
{
  convert_fp *retp;

  if (!fp) return NULL;

  retp = (convert_fp *)calloc(sizeof(convert_fp), 1);
  if (!retp) {
    return NULL;
  }
  retp->close = 0;
  retp->fp = fp;
  retp->nout = 0;
  retp->iout = 0;
  return retp;
}

/* Open in binary mode (to avoid newline conversion) */
convert_fp *convert_file_open(const char *filename, bin2txt_error *err)
{
  convert_fp *retp;

  errno = 0;
  retp = (convert_fp *)calloc(sizeof(convert_fp), 1);
  if (!retp) {
    if (!err) *err = BIN2TXT_ERR_NOMEM;
    return NULL;
  }

  retp->fp = fopen(filename, "wb");
  if (!retp->fp) {
    int tmp = errno;
    free(retp);
    errno = tmp;
    if (err) {
      if (errno != 0) {
        *err = BIN2TXT_ERR_SYS;
      } else {
        *err = BIN2TXT_ERR_FOPEN;
      }
    }
    return NULL;
  }

  retp->close = 1;
  retp->iout = 0;
  retp->nout = 0;
  return retp;
}

void convert_file_close(convert_fp *stream)
{
  if (!stream) return;

  if (stream->close) {
    if (stream->fp) fclose(stream->fp);
  }
  free(stream);
}

size_t bin2txt_convert_nout(convert_fp *stream)
{
  return stream->nout;
}

int bin2txt_fprintf(convert_fp *stream, const char *format, ...)
{
  int ret;
  va_list ap;

  ret = -1;
  if (stream->fp) {
    va_start(ap, format);
    ret = vfprintf(stream->fp, format, ap);
    va_end(ap);
  }
  return ret;
}

void bin2txt_convert_newline(convert_fp *stream)
{
  if (stream->nout != 0) {
    bin2txt_fprintf(stream, "\n");
  }
  bin2txt_fprintf(stream, "%08x: ", stream->iout);
  stream->nout = 0;
}

void bin2txt_convert_finalize(convert_fp *stream)
{
  bin2txt_fprintf(stream, "\n%08x\n", stream->iout);
}
