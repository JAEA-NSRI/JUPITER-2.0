/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* include files in JUPITER */
#include <jupiter/csv.h>
#include <jupiter/csvutil.h>

#include "convert.h"

static void print_usage(const char *argv0);

static bin2txt_error str_to_int(const char *str, int *ret);
static bin2txt_error parse_int_arg(const char *str, int *ret);

struct domain
{
  int ne;
  int nx;
  int ny;
  int nz;
  int mpi_x;
  int mpi_y;
  int mpi_z;
};

enum process_type
{
  process_char,    /* for char */
  process_int,     /* for int */
  process_long,    /* for long */
  process_schar,   /* for signed char */
  process_uchar,   /* for unsigned char */
  process_uint,    /* for unsigned int */
  process_ulong,   /* for unsigned long */
  process_double,  /* for double */
  process_single,  /* for float */
  process_i8,      /* for int8_t */
  process_i16,     /* for int16_t */
  process_i32,     /* for int32_t */
  process_i64,     /* for int64_t */
  process_imax,    /* for intmax_t */
  process_u8,      /* for uint8_t */
  process_u16,     /* for uint16_t */
  process_u32,     /* for uint32_t */
  process_u64,     /* for uint64_t */
  process_umax,    /* for uintmax_t */
  process_ptrdiff, /* for ptrdiff_t */
  process_size,    /* for size_t */
};

union process_element_u
{
  char ch;
  signed char sch;
  unsigned char uch;
  int i;
  long l;
  unsigned u;
  unsigned long ul;
  double d;
  float s;
  intmax_t imax;
  uintmax_t umax;
  ptrdiff_t pdiff;
  size_t size;
#ifdef INT8_C
  int8_t i8;
#endif
#ifdef INT16_C
  int16_t i16;
#endif
#ifdef INT32_C
  int32_t i32;
#endif
#ifdef INT64_C
  int64_t i64;
#endif
#ifdef UINT8_C
  uint8_t u8;
#endif
#ifdef UINT16_C
  uint16_t u16;
#endif
#ifdef UINT32_C
  uint32_t u32;
#endif
#ifdef UINT64_C
  uint64_t u64;
#endif
};

struct process_element
{
  union process_element_u u;
  int (*read)(union process_element_u *unit, FILE *input);
  void (*write)(convert_fp *output, union process_element_u *unit);
};

#define DEFINE_PROCESS_READ(fname, memname)                        \
  static int fname(union process_element_u *unit, FILE *input)     \
  {                                                                \
    return fread(&unit->memname, sizeof(unit->memname), 1, input); \
  }

#define DEFINE_PROCESS_CONV(fname, memname, bintxt_func)               \
  static void fname(convert_fp *output, union process_element_u *unit) \
  {                                                                    \
    bintxt_func(output, unit->memname);                                \
  }

#define DEFINE_PROCESS_INIT(fname, reader, conv)                  \
  static int fname(struct process_element *f)                     \
  {                                                               \
    *f = (struct process_element){.read = reader, .write = conv}; \
    return reader && conv;                                        \
  }

#if CHAR_MIN < 0
DEFINE_PROCESS_READ(process_read_ch, ch)
DEFINE_PROCESS_CONV(process_conv_ch, ch, bin2txt_convert_single_i)
DEFINE_PROCESS_INIT(process_init_ch, process_read_ch, process_conv_ch)
#else
DEFINE_PROCESS_READ(process_read_ch, ch)
DEFINE_PROCESS_CONV(process_conv_ch, ch, bin2txt_convert_single_u)
DEFINE_PROCESS_INIT(process_init_ch, process_read_ch, process_conv_ch)
#endif

DEFINE_PROCESS_READ(process_read_sch, sch)
DEFINE_PROCESS_CONV(process_conv_sch, sch, bin2txt_convert_single_i)
DEFINE_PROCESS_INIT(process_init_sch, process_read_sch, process_conv_sch)

DEFINE_PROCESS_READ(process_read_uch, uch)
DEFINE_PROCESS_CONV(process_conv_uch, uch, bin2txt_convert_single_u)
DEFINE_PROCESS_INIT(process_init_uch, process_read_uch, process_conv_uch)

DEFINE_PROCESS_READ(process_read_i, i)
DEFINE_PROCESS_CONV(process_conv_i, i, bin2txt_convert_single_i)
DEFINE_PROCESS_INIT(process_init_i, process_read_i, process_conv_i)

DEFINE_PROCESS_READ(process_read_l, l)
DEFINE_PROCESS_CONV(process_conv_l, l, bin2txt_convert_single_i)
DEFINE_PROCESS_INIT(process_init_l, process_read_l, process_conv_l)

DEFINE_PROCESS_READ(process_read_u, u)
DEFINE_PROCESS_CONV(process_conv_u, u, bin2txt_convert_single_u)
DEFINE_PROCESS_INIT(process_init_u, process_read_u, process_conv_u)

DEFINE_PROCESS_READ(process_read_ul, ul)
DEFINE_PROCESS_CONV(process_conv_ul, ul, bin2txt_convert_single_u)
DEFINE_PROCESS_INIT(process_init_ul, process_read_ul, process_conv_ul)

DEFINE_PROCESS_READ(process_read_d, d)
DEFINE_PROCESS_CONV(process_conv_d, d, bin2txt_convert_single_d)
DEFINE_PROCESS_INIT(process_init_d, process_read_d, process_conv_d)

DEFINE_PROCESS_READ(process_read_s, s)
DEFINE_PROCESS_CONV(process_conv_s, s, bin2txt_convert_single_s)
DEFINE_PROCESS_INIT(process_init_s, process_read_s, process_conv_s)

DEFINE_PROCESS_READ(process_read_imax, imax)
DEFINE_PROCESS_CONV(process_conv_imax, imax, bin2txt_convert_single_i)
DEFINE_PROCESS_INIT(process_init_imax, process_read_imax, process_conv_imax)

DEFINE_PROCESS_READ(process_read_umax, umax)
DEFINE_PROCESS_CONV(process_conv_umax, umax, bin2txt_convert_single_u)
DEFINE_PROCESS_INIT(process_init_umax, process_read_umax, process_conv_umax)

DEFINE_PROCESS_READ(process_read_pdiff, pdiff)
DEFINE_PROCESS_CONV(process_conv_pdiff, pdiff, bin2txt_convert_single_i)
DEFINE_PROCESS_INIT(process_init_pdiff, process_read_pdiff, process_conv_pdiff)

DEFINE_PROCESS_READ(process_read_size, size)
DEFINE_PROCESS_CONV(process_conv_size, size, bin2txt_convert_single_u)
DEFINE_PROCESS_INIT(process_init_size, process_read_size, process_conv_size)

#ifdef INT8_C
DEFINE_PROCESS_READ(process_read_i8, i8)
DEFINE_PROCESS_CONV(process_conv_i8, i8, bin2txt_convert_single_i)
DEFINE_PROCESS_INIT(process_init_i8, process_read_i8, process_conv_i8)
#else
DEFINE_PROCESS_INIT(process_init_i8, NULL, NULL)
#endif

#ifdef INT16_C
DEFINE_PROCESS_READ(process_read_i16, i16)
DEFINE_PROCESS_CONV(process_conv_i16, i16, bin2txt_convert_single_i)
DEFINE_PROCESS_INIT(process_init_i16, process_read_i16, process_conv_i16)
#else
DEFINE_PROCESS_INIT(process_init_i16, NULL, NULL)
#endif

#ifdef INT32_C
DEFINE_PROCESS_READ(process_read_i32, i32)
DEFINE_PROCESS_CONV(process_conv_i32, i32, bin2txt_convert_single_i)
DEFINE_PROCESS_INIT(process_init_i32, process_read_i32, process_conv_i32)
#else
DEFINE_PROCESS_INIT(process_init_i32, NULL, NULL)
#endif

#ifdef INT64_C
DEFINE_PROCESS_READ(process_read_i64, i64)
DEFINE_PROCESS_CONV(process_conv_i64, i64, bin2txt_convert_single_i)
DEFINE_PROCESS_INIT(process_init_i64, process_read_i64, process_conv_i64)
#else
DEFINE_PROCESS_INIT(process_init_i64, NULL, NULL)
#endif

#ifdef UINT8_C
DEFINE_PROCESS_READ(process_read_u8, u8)
DEFINE_PROCESS_CONV(process_conv_u8, u8, bin2txt_convert_single_u)
DEFINE_PROCESS_INIT(process_init_u8, process_read_u8, process_conv_u8)
#else
DEFINE_PROCESS_INIT(process_init_u8, NULL, NULL)
#endif

#ifdef UINT16_C
DEFINE_PROCESS_READ(process_read_u16, u16)
DEFINE_PROCESS_CONV(process_conv_u16, u16, bin2txt_convert_single_u)
DEFINE_PROCESS_INIT(process_init_u16, process_read_u16, process_conv_u16)
#else
DEFINE_PROCESS_INIT(process_init_u16, NULL, NULL)
#endif

#ifdef UINT32_C
DEFINE_PROCESS_READ(process_read_u32, u32)
DEFINE_PROCESS_CONV(process_conv_u32, u32, bin2txt_convert_single_u)
DEFINE_PROCESS_INIT(process_init_u32, process_read_u32, process_conv_u32)
#else
DEFINE_PROCESS_INIT(process_init_u32, NULL, NULL)
#endif

#ifdef UINT64_C
DEFINE_PROCESS_READ(process_read_u64, u64)
DEFINE_PROCESS_CONV(process_conv_u64, u64, bin2txt_convert_single_u)
DEFINE_PROCESS_INIT(process_init_u64, process_read_u64, process_conv_u64)
#else
DEFINE_PROCESS_INIT(process_init_u64, NULL, NULL)
#endif

/*!re2c
  re2c:define:YYCTYPE = "unsigned char";
  re2c:define:YYCURSOR = "cur";
  re2c:define:YYMARKER = "mrk";
  re2c:yyfill:enable = 0;
  re2c:indent:string = "  ";
  re2c:flags:8 = 1;
*/

static int process_element_init(struct process_element *e,
                                enum process_type type)
{
  switch (type) {
  case process_char:
    return process_init_ch(e);
  case process_schar:
    return process_init_sch(e);
  case process_uchar:
    return process_init_uch(e);
  case process_int:
    return process_init_i(e);
  case process_long:
    return process_init_l(e);
  case process_uint:
    return process_init_u(e);
  case process_ulong:
    return process_init_ul(e);
  case process_single:
    return process_init_s(e);
  case process_double:
    return process_init_d(e);
  case process_imax:
    return process_init_imax(e);
  case process_umax:
    return process_init_umax(e);
  case process_ptrdiff:
    return process_init_pdiff(e);
  case process_size:
    return process_init_size(e);
  case process_i8:
    return process_init_i8(e);
  case process_i16:
    return process_init_i16(e);
  case process_i32:
    return process_init_i32(e);
  case process_i64:
    return process_init_i64(e);
  case process_u8:
    return process_init_u8(e);
  case process_u16:
    return process_init_u16(e);
  case process_u32:
    return process_init_u32(e);
  case process_u64:
    return process_init_u64(e);
  }
  return 0;
}

static int process_element_init_arg(struct process_element *e,
                                    const char *str)
{
  const char *cur;
  const char *mrk;

  cur = str;

  for (;;) {
    /*!re2c
      re2c:indent:top = 2;
      end = "\x00";

      * { break; }

      'CHAR'/end      { return process_element_init(e, process_char); }
      'SCHAR'/end     { return process_element_init(e, process_schar); }
      'UCHAR'/end     { return process_element_init(e, process_uchar); }

      'INT'/end       { return process_element_init(e, process_int); }
      'LONG'/end      { return process_element_init(e, process_long); }
      'UINT'/end      { return process_element_init(e, process_uint); }
      'ULONG'/end     { return process_element_init(e, process_ulong); }

      'DOUBLE'/end    { return process_element_init(e, process_double); }
      'SINGLE'/end    { return process_element_init(e, process_single); }

      'INTMAX_T'/end  { return process_element_init(e, process_imax); }
      'UINTMAX_T'/end { return process_element_init(e, process_umax); }
      'PTRDIFF_T'/end { return process_element_init(e, process_ptrdiff); }
      'SIZE_T'/end    { return process_element_init(e, process_size); }

      'INT8'/end      { return process_element_init(e, process_i8); }
      'INT16'/end     { return process_element_init(e, process_i16); }
      'INT32'/end     { return process_element_init(e, process_i32); }
      'INT64'/end     { return process_element_init(e, process_i64); }

      'UINT8'/end     { return process_element_init(e, process_u8); }
      'UINT16'/end    { return process_element_init(e, process_u16); }
      'UINT32'/end    { return process_element_init(e, process_u32); }
      'UINT64'/end    { return process_element_init(e, process_u64); }
     */
  }
  return 0;
}

static int process_element_read(struct process_element *e, FILE *fp)
{
  return e->read(&e->u, fp);
}

static void process_element_conv(convert_fp *fp, struct process_element *e)
{
  e->write(fp, &e->u);
}

int main(int argc, char **argv)
{
  struct domain dom;
  char *bin_file;
  char *txt_file;
  struct process_element e;
  convert_fp *fp;
  FILE *ifp;
  size_t n, is;
  bin2txt_error b2t_err;
  int ds_switch = 0;
  int stat = 0;

  if (argc < 7) {
    csvperrorf("<command line>", -1, -1, CSV_EL_FATAL, NULL,
               "7 arguments required at least");
    print_usage(argv[0]);
    return 1;
  }

  bin_file = argv[1];
  dom.mpi_x = 1;
  dom.mpi_y = 1;
  dom.mpi_z = 1;

  b2t_err = parse_int_arg(argv[2], &dom.ne);
  if (b2t_err != BIN2TXT_SUCCESS) {
    stat = 1;
  } else if (dom.ne <= 0) {
    csvperrorf("<command line>", -1, -1, CSV_EL_ERROR, NULL,
               "Number of elements per cell must be positive: %d", dom.ne);
    stat = 1;
  }

  b2t_err = parse_int_arg(argv[3], &dom.nx);
  if (b2t_err != BIN2TXT_SUCCESS) {
    stat = 1;
  } else if (dom.nx <= 0) {
    stat = 1;
    csvperrorf("<command line>", -1, -1, CSV_EL_ERROR, NULL,
               "Number of X size must be positive: %d", dom.nx);
  }

  b2t_err = parse_int_arg(argv[4], &dom.ny);
  if (b2t_err != BIN2TXT_SUCCESS) {
    stat = 1;
  } else if (dom.ny <= 0) {
    stat = 1;
    csvperrorf("<command line>", -1, -1, CSV_EL_ERROR, NULL,
               "Number of Y size must be positive: %d", dom.ny);
  }

  b2t_err = parse_int_arg(argv[5], &dom.nz);
  if (b2t_err != BIN2TXT_SUCCESS) {
    stat = 1;
  } else if (dom.nz <= 0) {
    stat = 1;
    csvperrorf("<command line>", -1, -1, CSV_EL_ERROR, NULL,
               "Number of Z size must be positive: %d", dom.nz);
  }

  txt_file = argv[6];
  if (argc > 7) {
    ds_switch = 7;
  }

  if (!bin_file || bin_file[0] == '\0') {
    bin_file = NULL;
    csvperrorf("<command line>", 0, 0, CSV_EL_FATAL, NULL,
               "No binary input file specified (or empty)");
  }
  if (!txt_file || txt_file[0] == '\0') {
    txt_file = NULL;
    csvperrorf("<command line>", 0, 0, CSV_EL_FATAL, NULL,
               "No text output file specified (or empty)");
  }

  process_element_init(&e, process_double);
  if (ds_switch > 0) {
    if (!process_element_init_arg(&e, argv[ds_switch])) {
      stat = 1;
      csvperrorf("<command line>", 0, 0, CSV_EL_FATAL, NULL,
                 "Invalid type name: %s", argv[ds_switch]);
    }
  }

  if (stat || !txt_file || !bin_file) {
    print_usage(argv[0]);
    return 1;
  }

  ifp = fopen(bin_file, "rb");
  if (!ifp) {
    if (errno != 0) {
      csvperror(bin_file, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0,
                NULL);
    } else {
      csvperror(bin_file, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_FOPEN, 0, 0, NULL);
    }
    return 1;
  }

  b2t_err = BIN2TXT_SUCCESS;
  fp = convert_file_open(txt_file, &b2t_err);
  if (!fp) {
    csv_error err;
    switch (b2t_err) {
    case BIN2TXT_ERR_NOMEM:
      err = CSV_ERR_NOMEM;
      break;
    case BIN2TXT_ERR_FOPEN:
      err = CSV_ERR_FOPEN;
      break;
    case BIN2TXT_ERR_SYS:
      err = CSV_ERR_SYS;
      break;
    default:
      err = CSV_ERR_FOPEN;
      break;
    }
    csvperror(txt_file, 0, 0, CSV_EL_FATAL, NULL, err, errno, 0, NULL);
    fclose(ifp);
    return 1;
  }

  dom.nx *= dom.mpi_x;
  dom.ny *= dom.mpi_y;
  dom.nz *= dom.mpi_z;
  n = dom.nx;
  n *= dom.ny;
  n *= dom.nz;
  n *= dom.ne;

  for (is = 0; is < n; ++is) {
    size_t nout;
    size_t i, j, k, ie;

    k = is / (dom.nx * dom.ny * dom.ne);
    ie = is % (dom.nx * dom.ny * dom.ne);
    j = ie / (dom.nx * dom.ne);
    ie = ie % (dom.nx * dom.ne);
    i = ie / dom.ne;
    ie = ie % dom.ne;

    errno = 0;
    process_element_read(&e, ifp);
    if (feof(ifp)) {
      csvperrorf(bin_file, 0, 0, CSV_EL_ERROR, NULL,
                 "Short data, requires %" PRIdMAX " elements", (intmax_t)n);
      break;
    }
    if (ferror(ifp)) {
      if (errno != 0) {
        csvperror(bin_file, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0,
                  NULL);
      } else {
        csvperrorf(bin_file, 0, 0, CSV_EL_ERROR, NULL, "Read error occured");
      }
      break;
    }

    nout = bin2txt_convert_nout(fp);
    if ((i == 0 && ie == 0) || nout > 512) {
      bin2txt_convert_newline(fp);
    }

    process_element_conv(fp, &e);
  }

  bin2txt_convert_finalize(fp);

  fclose(ifp);
  convert_file_close(fp);

  return 0;
}

static void print_usage(const char *argv0)
{
  fprintf(stderr,
          "Usage: %s BINARY_INPUT NE NX NY NZ TEXT_OUTPUT [DOUBLE|SINGLE|...]\n"
          "\n"
          "Arguments:\n"
          "   BINARY_INPUT   Binary input file name\n"
          "   TEXT_OUTPUT    Text output file name\n"
          "   CHAR           Read binary files as default char\n"
          "   SCHAR          Read binary files as default signed char\n"
          "   INT            Read binary files as default int\n"
          "   LONG           Read binary files as default long\n"
          "   UCHAR          Read binary files as default unsigned char\n"
          "   UINT           Read binary files as default unsigned int\n"
          "   ULONG          Read binary files as default unsigned long\n"
          "   DOUBLE         Read binary files as double-precision\n"
          "   SINGLE         Read binary files as single-precision\n"
#ifdef INT8_C
          "   INT8           Read binary files as 8-bit int\n"
#endif
#ifdef INT16_C
          "   INT16          Read binary files as 16-bit int\n"
#endif
#ifdef INT32_C
          "   INT32          Read binary files as 32-bit int\n"
#endif
#ifdef INT64_C
          "   INT64          Read binary files as 64-bit int\n"
#endif
#ifdef UINT8_C
          "   UINT8          Read binary files as 8-bit unsigned int\n"
#endif
#ifdef UINT16_C
          "   UINT16         Read binary files as 16-bit unsigned int\n"
#endif
#ifdef UINT32_C
          "   UINT32         Read binary files as 32-bit unsigned int\n"
#endif
#ifdef UINT64_C
          "   UINT64         Read binary files as 64-bit unsigned int\n"
#endif
          "   INTMAX_T       Read binary files as system's intmax_t\n"
          "   UINTMAX_T      Read binary files as system's uintmax_t\n"
          "   PTRDIFF_T      Read binary files as system's ptrdiff_t\n"
          "   SIZE_T         Read binary files as system's size_t\n"
          "                  (If ommited, DOUBLE is assumed)\n"
          "   NE (integer)   Number of elements per cell\n"
          "   NX (integer)   X (I) size of binary data\n"
          "   NY (integer)   Y (J) size of binary data\n"
          "   NZ (integer)   Z (K) size of binary data\n"
          "\n"
          "This program is for testing use, so "
          "it'll be hard to read text output file.\n"
          "   0-9: The value is integer 0 to 9 respectively.\n"
          "   A-F: The value is integer 10 to 15 respectively.\n"
          "     N: The value is NaN\n"
          "     H: The value is Positive Infinity (or HUGE_VAL)\n"
          "     L: The value is Negative Infinity (or HUGE_VAL)\n"
          "     R: Same as previous value (Repeat)\n"
          " [+|-][(1)[/(2)]][.(3)/(4)]*: \n"
          "        [+|-]: Signess (always present)\n"
          "          (1): Integer part of float in hex\n"
          "          (2): Integer exponential in hex\n"
          "          (3): Fractional numerator in hex\n"
          "          (4): Fractional denominator in hex in 2-power\n"
          "       ex) 1 -> 1, 0 -> 0, A -> 10\n"
          "           +110*  -> 272\n"
          "           +11/9* -> 8704 (17*2^9)\n"
          "           +.1/1* -> 0.5 (1*2^-1)\n"
          "           +33.f/5* -> 51.46875 (51+15*2^-5)\n"
          "     note: integer exponential is used only if the\n"
          "           number of trailing '0's is two or more:\n"
          "              8191.0 -> +1fff*, 256.0 -> +1/8*\n"
          "\n"
          "note: Giving SINGLE to double version and just using\n"
          "      single version is different. The double\n"
          "      version always processes values in double,\n"
          "      where single version processes values in single.\n"
          "\n",
          argv0);
}

static bin2txt_error parse_int_arg(const char *str, int *ret)
{
  bin2txt_error e;
  e = str_to_int(str, ret);
  if (e != BIN2TXT_SUCCESS) {
    if (e == BIN2TXT_ERR_CONV) {
      csvperrorf("<command line>", -1, -1, CSV_EL_ERROR, str,
                 "Failed to parse value: %s", strerror(errno));
    } else {
      csvperrorf("<command line>", -1, -1, CSV_EL_ERROR, str,
                 "Failed to parse value");
    }
  }
  return e;
}

static bin2txt_error str_to_int(const char *str, int *ret)
{
  char *p;
  long l;
  int i;

  CSVASSERT(str);
  CSVASSERT(ret);

  errno = 0;
  l = strtol(str, &p, 10);
  if (*p != '\0') {
    if (errno != 0)
      return BIN2TXT_ERR_SYS;
    return BIN2TXT_ERR_CONV;
  }
  i = l;
  if (i != l) {
#ifdef EDOM
    errno = EDOM;
#endif
    if (errno != 0)
      return BIN2TXT_ERR_SYS;
    return BIN2TXT_ERR_CONV;
  }
  *ret = i;
  return BIN2TXT_SUCCESS;
}
