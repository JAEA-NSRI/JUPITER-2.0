#ifndef BIN2TXT_CONVERT_H
#define BIN2TXT_CONVERT_H

#include <stdint.h>
#include <stdio.h>

struct convert_fp
{
  FILE *fp;
  int close;
  size_t nout;
  size_t iout;
  float  last_valf;
  double last_vald;
  intmax_t last_vali;
  uintmax_t last_valu;
};
typedef struct convert_fp convert_fp;

enum bin2txt_error
{
  BIN2TXT_SUCCESS = 0,
  BIN2TXT_ERR_FOPEN,
  BIN2TXT_ERR_SYS,
  BIN2TXT_ERR_NOMEM,
  BIN2TXT_ERR_CONV,
};
typedef enum bin2txt_error bin2txt_error;

convert_fp *convert_file_bind(FILE *fp);
convert_fp *convert_file_open(const char *filename, bin2txt_error *err);
void convert_file_close(convert_fp *stream);

size_t bin2txt_convert_nout(convert_fp *stream);
void bin2txt_convert_single_s(convert_fp *stream, float value);
void bin2txt_convert_single_d(convert_fp *stream, double value);
void bin2txt_convert_single_i(convert_fp *stream, intmax_t value);
void bin2txt_convert_single_u(convert_fp *stream, uintmax_t value);
void bin2txt_convert_newline(convert_fp *stream);
void bin2txt_convert_finalize(convert_fp *stream);

int bin2txt_convert_s(float *input, int mx, int my, int mz, convert_fp *stream);
int bin2txt_convert_d(double *input, int mx, int my, int mz, convert_fp *stream);
int bin2txt_convert_i(int *input, int mx, int my, int mz, convert_fp *stream);
int bin2txt_convert_l(long *input, int mx, int my, int mz, convert_fp *stream);
int bin2txt_convert_u(unsigned *input, int mx, int my, int mz, convert_fp *stream);

float     bin2txt_convert_last_s(convert_fp *stream);
double    bin2txt_convert_last_d(convert_fp *stream);
intmax_t  bin2txt_convert_last_i(convert_fp *stream);
uintmax_t bin2txt_convert_last_u(convert_fp *stream);

int bin2txt_fprintf(convert_fp *stream, const char *format, ...);

#ifdef JUPITER_DOUBLE
typedef double bin2txt_type;
#define BIN2TXT_PRECISION_POSTP _d
#define bin2txt_frexp (frexp)
#define bin2txt_modf  (modf)
#else
typedef float  bin2txt_type;
#define BIN2TXT_PRECISION_POSTP _s
#define bin2txt_frexp (frexpf)
#define bin2txt_modf  (modff)
#endif

#define bin2txt_func_x(f, m, ...) ((bin2txt_##f##m)(__VA_ARGS__))
#define bin2txt_func_c(f, m, ...) bin2txt_func_x(f, m, __VA_ARGS__)

#define bin2txt_convert_single(s, v) \
  bin2txt_func_c(convert_single, BIN2TXT_PRECISION_POSTP, s, v)

#define bin2txt_convert(i, x, y, z, s) \
  bin2txt_func_c(convert, BIN2TXT_PRECISION_POSTP, i, x, y, z, s)

#define bin2txt_convert_last(s) \
  bin2txt_func_c(convert_last, BIN2TXT_PRECISION_POSTP, s)

#endif
