
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

#include "table.h"

#ifndef TABLE_NO_IO_FUNCTIONS
#if CHAR_BIT != 8
#error "CHAR_BIT must be 8"
#endif
#ifndef UINT8_MAX
#error "It seems that uint8_t is not defined"
#endif
#ifndef UINT16_MAX
#error "It seems that uint16_t is not defined"
#endif
#ifndef UINT32_MAX
#error "It seems that uint32_t is not defined"
#endif
#ifndef UINT64_MAX
#error "It seems that uint64_t is not defined"
#endif

static const uint16_t table_mp_endian_test = 0xfeff;

/**
 * # Format of binary file
 *
 * Binary data file has following format:
 * (This format inspired from MessagePack (see https://msgpack.org/ ))
 *
 * Note: If we can link to external library, HDF5 is better choice.
 *       (The spec of HDF5 is slightly complex than MessagePack,
 *        and difficult to implement by hand)
 *
 * ## Legend
 * ```
 * +------+
 * |      |
 * +------+
 * ```
 * is a byte.
 * ```
 * +======+
 * |      |
 * +======+
 * ```
 * is multiple bytes.
 *
 * ## The format
 * First three bytes must be
 * ```
 * +------+------+------+------+
 * | 0x94 | 0xfe | 0xff | 0x92 | (Big endian)
 * +------+------+------+------+
 * ```
 * or
 * ```
 * +------+------+------+------+
 * | 0x94 | 0xff | 0xfe | 0x92 | (Little endian)
 * +------+------+------+------+
 * ```
 * which renders `0xfeff` in current processor's endianess, after
 * 0x94.
 *
 * Note: Endianess conversion will not be done. You will get an error
 *       if the file has wrong endianess.
 *
 * Note:
 * Instead of 0x94 in first byte,
 * ```
 * +------+----+----+      +------+---+---+---+---+
 * | 0xdc |  0x0004 |  or  | 0xdd |   0x00000004  |
 * +------+----+----+      +------+---+---+---+---+
 *         (16bit)                 (32bit)
 * ```
 * is acceptable format (where 0x0004 is stored in correct endianess),
 * but not recommended.
 *
 * Next, flags are stored.
 * ```
 * +------+=====+=====+
 * | 0x92 | (1) | (2) |
 * +------+=====+=====+
 * ```
 * (1): Data title in string format.
 *      (Need not to end with '\0'. API adds '\0' to end of string)
 *
 * (2): "RECTILINEAR" or "SUM-CONSTANT" in string format for corresponding
 *      geometry.
 *
 * Note: Lower four bit of 0x92 (so 2 here) is number of element, and
 *       the value may increase in later version.
 *
 * Each flag data can be one of following data:
 *     (Currently only string is used)
 *   - Boolean (0xc2 for false and 0xc3 for true, 1-byte)
 *   - Nil (0xc0, 1-byte)
 *   - Positive fixint (0x00 to 0x7f, 0 to 127 each, 1-byte)
 *   - Negative fixint (0xe0 to 0xff, -32 to -1 each, 1-byte)
 *   - 8 bit signed int (0xd0 + [8bit int], -128 to 127, 2-bytes)
 *   - 16 bit signed int (0xd1 + [16bit int], -32768 to 32767, 3-bytes)
 *   - 32 bit signed int (0xd2 + [32bit int], (range omitted), 5-bytes)
 *   - 64 bit signed int (0xd3 + [64bit int], (range omitted), 9-bytes)
 *   - String, short (0xa0 to 0xbf (length, 0 to 31 bytes) + (data), 1
 *     to 32-bytes)
 *   - String, 8bit (0xd9 + [8bit unsigned int] (length) + (data),
 *     0 to 255 bytes string, 2 to 257 bytes total)
 *   - String, 16bit (0xda + [16bit unsigned int] (length) + (data),
 *     0 to 65535 bytes string, 3 to 65538 bytes total)
 *   - String, 32bit (0xdb + [32bit unsigned int] (length) + (data),
 *     0 to 2^32-1 bytes string, 5 to 2^32+4 bytes total)
 *
 * Next one byte must be:
 * ```
 * +------+
 * | 0x93 |
 * +------+
 * ```
 * Also here, number of arrays may increase in later version.
 *
 * First array stores x-positions.
 * ```
 * +------+====================+------+=========================+
 * | 0xdc | nx (in 16 bit int) | 0xcb | Array of x-pos (double) |
 * +------+====================+------+=========================+
 * ```
 * (If stored in float, change 0xcb to 0xca. Remaining part also applicable)
 *
 * Or nx is greater than 65535,
 * ```
 * +------+====================+------+=========================+
 * | 0xdd | nx (in 32 bit int) | 0xcb | Array of x-pos (double) |
 * +------+====================+------+=========================+
 * ```
 * Or nx is greater than 4294967296,
 * ```
 * +------+====================+------+=========================+
 * | 0xd7 | nx (in 64 bit int) | 0xcb | Array of x-pos (double) |
 * +------+====================+------+=========================+
 * ```
 * (Treating 0xd7 as 64bit length array is not in the spec of MessagePack,
 *  8byte extended type (such as structs, etc.) originally.)
 *
 * Or even nx is less than 16,
 * ```
 * +-----------+------+=========================+
 * | 0x91-0x9f | 0xcb | Array of x-pos (double) |
 * +-----------+------+=========================+
 * ```
 * is fine to minimize the size.
 *
 * If table_write_binary is called without initialized (when no data
 * available),
 * ```
 * +------+
 * | 0x90 |
 * +------+
 * ```
 * will be written for array.
 *
 * Second array stores y-positions:
 * ```
 * +------+====================+------+=========================+
 * | 0xdc | ny (in 16 bit int) | 0xcb | Array of y-pos (double) |
 * +------+====================+------+=========================+
 * ```
 * (0xdd for 32bit array size, 0xca for float data)
 *
 * Last array stores table data.
 * ```
 * +------+========================+------+========================+
 * | 0xdc | length (in 16 bit int) | 0xcb | Array of data (double) |
 * +------+========================+------+========================+
 * ```
 * (0xdd for 32bit array size, 0xca for float data)
 *
 * Finally, put
 * ```
 * +------+
 * | 0x00 |
 * +------+
 * ```
 * at end. Any data after here will be ignored.
 *
 * N.B. MessagePack is designed for interchangable data, so...
 *
 *    - Actual MessagePack format requires whole data are stored in
 *      big endian, which is very high cost to run in little endian
 *      system.
 *
 *    - Actual MessagePack format requires the float/double data must
 *      to be stored in IEEE-754 format, but C-language's float/double
 *      might not. We save float and double as-is for performance.
 *      (Almost of recent architectures comply to IEEE-754, though)
 *
 *      Therefore, outputted binary data is **not** interchangable and
 *      you need to regenate the binary table data if you want to run
 *      in a new system.
 *
 *    - In double array, 0xca or 0xcb for 2nd element and so far is
 *      omitted.
 */

enum table_byte_type {
  TABLE_BT_INT0   = 0x00,  ///< 0x00-0x7f
  TABLE_BT_ARR1   = 0x90,  ///< 0x90-0x9f
  TABLE_BT_STR1   = 0xa0,  ///< 0xa0-0xbf
  TABLE_BT_NIL    = 0xc0,  ///< 0xc0
  TABLE_BT_INVAL  = 0xc1,  ///< 0xc1 (never used)
  TABLE_BT_FALSE  = 0xc2,  ///< 0xc2
  TABLE_BT_TRUE   = 0xc3,  ///< 0xc3
  TABLE_BT_FLOAT  = 0xca,  ///< 0xca
  TABLE_BT_DOUBLE = 0xcb,  ///< 0xcb
  TABLE_BT_INT8   = 0xd0,  ///< 0xd0
  TABLE_BT_INT16  = 0xd1,  ///< 0xd1
  TABLE_BT_INT32  = 0xd2,  ///< 0xd2
  TABLE_BT_INT64  = 0xd3,  ///< 0xd3
  TABLE_BT_ARR64  = 0xd7,  ///< 0xd7 (extended type of 8byte in MessagePack)
  TABLE_BT_STR8   = 0xd9,  ///< 0xd9
  TABLE_BT_STR16  = 0xda,  ///< 0xda
  TABLE_BT_STR32  = 0xdb,  ///< 0xdb
  TABLE_BT_ARR16  = 0xdc,  ///< 0xdc
  TABLE_BT_ARR32  = 0xdd,  ///< 0xdd
  TABLE_BT_INTN   = 0xe0,  ///< 0xe0-0xff
};
typedef enum table_byte_type table_byte_type;

static table_byte_type table_mp_get_type(unsigned char byte);

static table_error table_read_bytes(void *dest, size_t size, size_t nmemb, FILE *fp);
static table_error table_mp_get_arr_size(FILE *fp, unsigned char byte, size_t *ret);
static table_error table_mp_get_str_size(FILE *fp, unsigned char byte, size_t *ret);
static table_error table_mp_check_arr_size(FILE *fp, size_t req);
static table_error table_mp_read_int(FILE *fp, unsigned char byte, int64_t *val);
static table_error table_mp_read_str(FILE *fp, unsigned char byte, char **ret, size_t *szret);
static table_error table_mp_read_float_array(FILE *fp, size_t n, double **ret);


table_error table_read_binary(table_data *table, const char *from)
{
  FILE *fp;
  table_error e;
  unsigned char byte;
  uint16_t end_test;
  char *buf = NULL;
  size_t s;
  int64_t ival;
  table_geometry g;
  double *xt;
  double *yt;
  double *dt;
  table_size nx;
  table_size ny;
  size_t nsx;
  size_t nsy;
  char *title = NULL;

  if (!table || !from) return TABLE_ERR_NULLP;

  xt = NULL;
  yt = NULL;
  dt = NULL;

  errno = 0;
  fp = fopen(from, "rb");
  if (!fp) {
    return TABLE_ERR_SYS;
  }
  e = TABLE_SUCCESS;

  /* Start reading */
  do {
    /* Read header. */
    /* 4 is for 2-negative fixint (0xfeff), data, and 0x00 */
    e = table_mp_check_arr_size(fp, 4);

    /* Endianness check */
    e = table_read_bytes(&end_test, sizeof(uint16_t), 1, fp);
    if (e != TABLE_SUCCESS) break;

    if (end_test != table_mp_endian_test) {
      e = TABLE_ERR_ENDIAN;
      break;
    }

    /* Data head */
    /* Flags and Array Data */
    e = table_mp_check_arr_size(fp, 2);
    if (e != TABLE_SUCCESS) break;

    /* Flags */
    e = table_mp_check_arr_size(fp, 2);
    if (e != TABLE_SUCCESS) break;

    /* Title */
    e = table_read_bytes(&byte, sizeof(unsigned char), 1, fp);
    if (e != TABLE_SUCCESS) break;

    e = table_mp_read_str(fp, byte, &title, NULL);
    if (e != TABLE_SUCCESS) {
      title = NULL;
      break;
    }

    /* Geometry */
    e = table_read_bytes(&byte, sizeof(unsigned char), 1, fp);
    if (e != TABLE_SUCCESS) break;

    e = table_mp_read_str(fp, byte, &buf, &s);
    if (e != TABLE_SUCCESS) {
      buf = NULL;
      break;
    }

    g = TABLE_GEOMETRY_INVALID;
    if (strncmp(buf, "RECTILINEAR", s) == 0) {
      g = TABLE_GEOMETRY_RECTILINEAR;
    } else if (strncmp(buf, "SUM-CONSTANT", s) == 0) {
      g = TABLE_GEOMETRY_SUM_CONSTANT;
    }
    free(buf);
    buf = NULL;
    if (g == TABLE_GEOMETRY_INVALID) {
      e = TABLE_ERR_FORMAT;
      break;
    }

    /* Array Data */
    e = table_mp_check_arr_size(fp, 3);
    if (e != TABLE_SUCCESS) break;

    /* X data */
    e = table_read_bytes(&byte, sizeof(unsigned char), 1, fp);
    if (e != TABLE_SUCCESS) break;

    e = table_mp_get_arr_size(fp, byte, &nsx);
    if (e != TABLE_SUCCESS) break;
    nx = nsx;
    if (nsx > 0) {
      e = table_mp_read_float_array(fp, nsx, &xt);
      if (e != TABLE_SUCCESS) {
        if (e == TABLE_ERR_RANGE) {
          e = TABLE_ERR_FORMAT;
        }
        break;
      }
    }

    /* Y data */
    e = table_read_bytes(&byte, sizeof(unsigned char), 1, fp);
    if (e != TABLE_SUCCESS) break;

    e = table_mp_get_arr_size(fp, byte, &nsy);
    if (e != TABLE_SUCCESS) break;
    ny = nsy;
    if (nsy > 0) {
      e = table_mp_read_float_array(fp, nsy, &yt);
      if (e != TABLE_SUCCESS) {
        if (e == TABLE_ERR_RANGE) {
          e = TABLE_ERR_FORMAT;
        }
        break;
      }
    }

    /* Data */
    e = table_read_bytes(&byte, sizeof(unsigned char), 1, fp);
    if (e != TABLE_SUCCESS) break;

    e = table_mp_get_arr_size(fp, byte, &s);
    if (e != TABLE_SUCCESS) break;
    if (nx > 0 && ny > 0) {
      if (s >= TABLE_SIZE_MAX ||
          (table_size)s != table_calc_data_size(g, nx, ny)) {
        e = TABLE_ERR_RANGE;
        break;
      }
      e = table_mp_read_float_array(fp, s, &dt);
      if (e != TABLE_SUCCESS) {
        if (e == TABLE_ERR_RANGE) {
          e = TABLE_ERR_FORMAT;
        }
        break;
      }
    } else {
      if (s != 0) {
        e = TABLE_ERR_RANGE;
        break;
      }
    }

    /* EOF */
    e = table_read_bytes(&byte, sizeof(unsigned char), 1, fp);
    if (e != TABLE_SUCCESS) break;

    e = table_mp_read_int(fp, byte, &ival);
    if (e != TABLE_SUCCESS) break;

    if (ival != 0) {
      e = TABLE_ERR_FORMAT;
      break;
    }

  } while(0);

  /* Initialize data with readed info (interpolation mode keep same) */
  if (e == TABLE_SUCCESS) {
    e = table_init(table, title, g, nx, ny, table_get_interp_mode(table),
                   xt, yt, dt);
  }

  free(xt);
  free(yt);
  free(dt);
  free(buf);
  free(title);
  fclose(fp);
  return e;
}

//---
static table_error table_write_bytes(const void *src, size_t size, size_t nmemb, FILE *fp);
static table_error table_mp_write_array_header(FILE *fp, size_t n);
static table_error table_mp_write_string(FILE *fp, const char *buf, size_t n);
static table_error table_mp_write_int(FILE *fp, int64_t val);
static table_error table_mp_write_double_array(FILE *fp, size_t n, const double *data);

table_error table_write_binary(table_data *table, const char *dest)
{
  table_error e;
  FILE *fp;
  table_geometry g;
  table_size nx;
  table_size ny;
  table_size nd;
  const double *xt;
  const double *yt;
  const double *dt;
  const char *tit;

  if (!table || !dest) return TABLE_ERR_NULLP;

  g = table_get_geometry(table);
  nx = table_get_nx(table);
  ny = table_get_ny(table);
  nd = table_calc_data_size(g, nx, ny);
  xt = table_get_xdata(table);
  yt = table_get_ydata(table);
  dt = table_get_data(table);
  tit = table_get_title(table);
  if (!xt) nx = 0;
  if (!yt) ny = 0;
  if (!dt) nd = 0;
  TABLE_ASSERT(nx >= 0 && ny >= 0 && nd >= 0);

  fp = fopen(dest, "wb");
  if (!fp) {
    return TABLE_ERR_SYS;
  }

  /* Start writing */
  do {
    /* overall header */
    e = table_mp_write_array_header(fp, 4);
    if (e != TABLE_SUCCESS) break;

    /* write endian test */
    e = table_write_bytes(&table_mp_endian_test, sizeof(uint16_t), 1, fp);
    if (e != TABLE_SUCCESS) break;

    /* Flag and data */
    e = table_mp_write_array_header(fp, 2);
    if (e != TABLE_SUCCESS) break;

    /* Flag */
    e = table_mp_write_array_header(fp, 2);
    if (e != TABLE_SUCCESS) break;

    /* Title */
    e = table_mp_write_string(fp, tit, 0);
    if (e != TABLE_SUCCESS) break;

    /* Geometry */
    switch(g) {
    case TABLE_GEOMETRY_RECTILINEAR:
      e = table_mp_write_string(fp, "RECTILINEAR", 0);
      break;
    case TABLE_GEOMETRY_SUM_CONSTANT:
      e = table_mp_write_string(fp, "SUM-CONSTANT", 0);
      break;
    default:
      e = table_mp_write_string(fp, "geometry?", 0);
      break;
    }
    if (e != TABLE_SUCCESS) break;

    /* Data array */
    e = table_mp_write_array_header(fp, 3);
    if (e != TABLE_SUCCESS) break;

    e = table_mp_write_double_array(fp, nx, xt);
    if (e != TABLE_SUCCESS) break;

    e = table_mp_write_double_array(fp, ny, yt);
    if (e != TABLE_SUCCESS) break;

    e = table_mp_write_double_array(fp, nd, dt);
    if (e != TABLE_SUCCESS) break;

    /* EOF marker */
    e = table_mp_write_int(fp, 0);
    if (e != TABLE_SUCCESS) break;

  } while(0);

  fclose(fp);
  return e;
}

/* Implementation of supplemental functions... */

static table_byte_type table_mp_get_type(unsigned char byte)
{
  if (byte <= 0x7f)  return TABLE_BT_INT0;
  if (byte >= 0xe0)  return TABLE_BT_INTN;
  if (byte <  0x90)  return TABLE_BT_INVAL;
  if (byte <= 0xbf) {
    if (byte <= 0x9f) return TABLE_BT_ARR1;
    return TABLE_BT_STR1;
  }
  switch(byte) {
  case 0xc0: return TABLE_BT_NIL;
  case 0xc2: return TABLE_BT_TRUE;
  case 0xc3: return TABLE_BT_FALSE;
  case 0xca: return TABLE_BT_FLOAT;
  case 0xcb: return TABLE_BT_DOUBLE;
  case 0xd0: return TABLE_BT_INT8;
  case 0xd1: return TABLE_BT_INT16;
  case 0xd2: return TABLE_BT_INT32;
  case 0xd3: return TABLE_BT_INT64;
  case 0xd7: return TABLE_BT_ARR64;
  case 0xd9: return TABLE_BT_STR8;
  case 0xda: return TABLE_BT_STR16;
  case 0xdb: return TABLE_BT_STR32;
  case 0xdc: return TABLE_BT_ARR16;
  case 0xdd: return TABLE_BT_ARR32;
  default: return TABLE_BT_INVAL;
  }
}

static table_error table_read_bytes(void *dest, size_t size, size_t nmemb,
                                    FILE *fp)
{
  size_t rf;

  rf = fread(dest, size, nmemb, fp);
  if (rf != nmemb) {
    if (feof(fp)) {
      return TABLE_ERR_EOF;
    } else {
      return TABLE_ERR_SYS;
    }
  }
  return TABLE_SUCCESS;
}

static table_error table_mp_get_arr_size(FILE *fp, unsigned char byte, size_t *ret)
{
  table_error e;
  union t {
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
  } p;
  void *ds;
  size_t mb;

  if (!ret) return TABLE_ERR_NULLP;

  if (table_mp_get_type(byte) == TABLE_BT_ARR1) {
    *ret = byte & 0x0f;
    return TABLE_SUCCESS;
  }

  switch(byte) {
  case TABLE_BT_ARR16:
    ds = &p.u16;
    mb = sizeof(uint16_t);
    break;
  case TABLE_BT_ARR32:
    ds = &p.u32;
    mb = sizeof(uint32_t);
    break;
  case TABLE_BT_ARR64:
    ds = &p.u64;
    mb = sizeof(uint64_t);
    break;
  default:
    return TABLE_ERR_RANGE;
  }

  e = table_read_bytes(ds, mb, 1, fp);
  if (e != TABLE_SUCCESS) {
    return e;
  }

  switch(byte) {
  case TABLE_BT_ARR16:
    *ret = p.u16;
    break;
  case TABLE_BT_ARR32:
    *ret = p.u32;
    break;
  case TABLE_BT_ARR64:
    *ret = p.u64;
    break;
  default:
    TABLE_UNREACHABLE();
    break;
  }
  return TABLE_SUCCESS;
}

static table_error table_mp_get_str_size(FILE *fp, unsigned char byte, size_t *ret)
{
  table_error e;
  union t {
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
  } p;
  void *ds;
  size_t mb;

  if (!ret) return TABLE_ERR_NULLP;

  if (table_mp_get_type(byte) == TABLE_BT_STR1) {
    *ret = byte & 0x1f;
    return TABLE_SUCCESS;
  }

  switch(byte) {
  case TABLE_BT_STR8:
    ds = &p.u8;
    mb = sizeof(uint8_t);
    break;
  case TABLE_BT_STR16:
    ds = &p.u16;
    mb = sizeof(uint16_t);
    break;
  case TABLE_BT_STR32:
    ds = &p.u32;
    mb = sizeof(uint32_t);
    break;
  default:
    return TABLE_ERR_RANGE;
  }

  e = table_read_bytes(ds, mb, 1, fp);
  if (e != TABLE_SUCCESS) {
    return e;
  }

  switch(byte) {
  case TABLE_BT_STR8:
    *ret = p.u8;
    break;
  case TABLE_BT_STR16:
    *ret = p.u16;
    break;
  case TABLE_BT_STR32:
    *ret = p.u32;
    break;
  default:
    TABLE_UNREACHABLE();
  }
  return TABLE_SUCCESS;
}

static table_error table_mp_check_arr_size(FILE *fp, size_t req)
{
  table_error e;
  unsigned char byte;
  size_t comp;

  if (!fp) return TABLE_ERR_NULLP;

  e = table_read_bytes(&byte, sizeof(unsigned char), 1, fp);
  if (e != TABLE_SUCCESS) return e;

  e = table_mp_get_arr_size(fp, byte, &comp);
  if (e != TABLE_SUCCESS) {
    if (e == TABLE_ERR_RANGE) {
      e = TABLE_ERR_FORMAT;
    }
    return e;
  }

  if (comp != req) {
    return TABLE_ERR_FORMAT;
  }
  return TABLE_SUCCESS;
}

static table_error table_mp_read_int(FILE *fp, unsigned char byte, int64_t *val)
{
  table_error e;
  table_byte_type type;
  union t {
    int8_t  i8;
    int16_t i16;
    int32_t i32;
  } p;

  if (!fp || !val) return TABLE_ERR_NULLP;

  e = TABLE_SUCCESS;

  type = table_mp_get_type(byte);
  switch(type) {
  case TABLE_BT_INT0:
    *val = byte;
    break;
  case TABLE_BT_INTN:
    *val = -0x100 + byte;
    break;
  case TABLE_BT_INT8:
    e = table_read_bytes(&p.i8, sizeof(int8_t), 1, fp);
    if (e != TABLE_SUCCESS) return e;
    *val = p.i8;
    break;
  case TABLE_BT_INT16:
    e = table_read_bytes(&p.i16, sizeof(int16_t), 1, fp);
    if (e != TABLE_SUCCESS) return e;
    *val = p.i16;
    break;
  case TABLE_BT_INT32:
    e = table_read_bytes(&p.i32, sizeof(int32_t), 1, fp);
    if (e != TABLE_SUCCESS) return e;
    *val = p.i32;
    break;
  case TABLE_BT_INT64:
    e = table_read_bytes(val, sizeof(int64_t), 1, fp);
    if (e != TABLE_SUCCESS) return e;
    break;
  default:
    return TABLE_ERR_RANGE;
  }
  return TABLE_SUCCESS;
}

static table_error table_mp_read_str(FILE *fp, unsigned char byte, char **ret, size_t *szret)
{
  table_error e;
  size_t sz;
  char *buf;

  if (!fp || !ret) return TABLE_ERR_NULLP;

  e = table_mp_get_str_size(fp, byte, &sz);
  if (e != TABLE_SUCCESS) return e;

  if (sz + 1 == 0) return TABLE_ERR_OVERFLOW;

  buf = (char *)malloc(sizeof(char) * (sz + 1));
  if (!buf) return TABLE_ERR_NOMEM;

  if (sz > 0) {
    e = table_read_bytes(buf, sizeof(char), sz, fp);
    if (e != TABLE_SUCCESS) {
      free(buf);
      return e;
    }
  }

  buf[sz] = '\0';
  *ret = buf;
  if (szret) *szret = sz;
  return TABLE_SUCCESS;
}

static table_error table_mp_read_float_array(FILE *fp, size_t n, double **ret)
{
  table_error e;
  unsigned char byte;
  table_byte_type bt;
  float *flt;
  double *dbl;
  size_t is;

  if (!fp || !ret) return TABLE_ERR_NULLP;
  if (n == 0) {
    *ret = NULL;
    return TABLE_SUCCESS;
  }

  e = table_read_bytes(&byte, sizeof(unsigned char), 1, fp);
  if (e != TABLE_SUCCESS) return e;

  bt = table_mp_get_type(byte);
  if (bt == TABLE_BT_FLOAT) {
    flt = (float *)calloc(sizeof(float), n);
    if (!flt) {
      free(flt);
      return TABLE_ERR_NOMEM;
    }
    e = table_read_bytes(flt, sizeof(float), n, fp);
    if (e != TABLE_SUCCESS) {
      free(flt);
      return e;
    }
    dbl = (double *)calloc(sizeof(double), n);
    if (!dbl) {
      free(flt);
      return TABLE_ERR_NOMEM;
    }
    for (is = 0; is < n; ++is) {
      dbl[is] = flt[is];
    }
    free(flt);

    e = TABLE_SUCCESS;

  } else if (bt == TABLE_BT_DOUBLE) {
    dbl = (double *)calloc(sizeof(double), n);
    if (!dbl) {
      return TABLE_ERR_NOMEM;
    }
    e = table_read_bytes(dbl, sizeof(double), n, fp);
    if (e != TABLE_SUCCESS) {
      free(dbl);
      return e;
    }

    e = TABLE_SUCCESS;

  } else {
    e = TABLE_ERR_RANGE;
  }

  if (e == TABLE_SUCCESS) {
    *ret = dbl;
  }

  return e;
}


static table_error table_write_bytes(const void *src, size_t size, size_t nmemb, FILE *fp)
{
  size_t n;

  if (!src || !fp) return TABLE_ERR_NULLP;

  n = fwrite(src, size, nmemb, fp);
  if (n != nmemb) {
    return TABLE_ERR_SYS;
  }

  return TABLE_SUCCESS;
}

static table_error table_mp_write_array_header(FILE *fp, size_t n)
{
  unsigned char byte;
  uint16_t u16;
  uint32_t u32;
  uint64_t u64;
  size_t wrsz;
  unsigned char buf[sizeof(uint64_t) + 1];

  if (!fp) return TABLE_ERR_NULLP;

  wrsz = 1;
  if (n <= 0xf) {
    byte = (unsigned char)TABLE_BT_ARR1;
    byte = byte | n;
    buf[0] = byte;
  } else if (n <= UINT16_MAX) {
    buf[0] = (unsigned char)TABLE_BT_ARR16;
    u16 = n;
    memcpy(&buf[1], &u16, sizeof(uint16_t));
    wrsz += sizeof(uint16_t);
  } else if (n <= UINT32_MAX) {
    buf[0] = (unsigned char)TABLE_BT_ARR32;
    u32 = n;
    memcpy(&buf[1], &u32, sizeof(uint32_t));
    wrsz += sizeof(uint32_t);
  } else if (n <= UINT64_MAX) {
    buf[0] = (unsigned char)TABLE_BT_ARR64;
    u64 = n;
    memcpy(&buf[1], &u64, sizeof(uint64_t));
    wrsz += sizeof(uint64_t);
  } else {
    return TABLE_ERR_RANGE;
  }

  return table_write_bytes(buf, sizeof(unsigned char), wrsz, fp);
}

static table_error table_mp_write_string(FILE *fp, const char *buf, size_t n)
{
  table_error e;
  unsigned char byte;
  union t {
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
  } p;
  void *ds;
  size_t mb;

  if (!fp) return TABLE_ERR_NULLP;

  byte = (unsigned char)TABLE_BT_STR1;

  if (buf && n == 0) {
    n = strlen(buf);
  }

  if (n <= 0x1f) {
    byte = byte | n;
    ds = NULL;
  } else if (n <= UINT8_MAX) {
    byte = TABLE_BT_STR8;
    p.u8 = n;
    ds = &p.u8;
    mb = sizeof(uint8_t);
  } else if (n <= UINT16_MAX) {
    byte = TABLE_BT_STR16;
    p.u16 = n;
    ds = &p.u16;
    mb = sizeof(uint16_t);
  } else if (n <= UINT64_MAX) {
    byte = TABLE_BT_STR32;
    p.u32 = n;
    ds = &p.u32;
    mb = sizeof(uint32_t);
  } else {
    return TABLE_ERR_RANGE;
  }

  e = table_write_bytes(&byte, sizeof(unsigned char), 1, fp);
  if (e != TABLE_SUCCESS) return e;

  if (ds) {
    e = table_write_bytes(ds, mb, 1, fp);
    if (e != TABLE_SUCCESS) return e;
  }

  if (buf && n > 0) {
    e = table_write_bytes(buf, sizeof(unsigned char), n, fp);
  }

  return e;
}

static table_error table_mp_write_int(FILE *fp, int64_t val)
{
  union t {
    int8_t  i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
  } p;

  table_error e;
  unsigned char byte;
  void *pt;
  size_t ws;
  uint64_t u64;
  table_byte_type bt;
  uint64_t full = (uint64_t)-1;

  if (val >= 0) {
    if (val < 0x80) {
      bt = TABLE_BT_INT0;
      p.i8 = val;
    } else if (val <= 0xff) {
      bt = TABLE_BT_INT8;
      p.i8 = val;
    } else if (val <= 0xffff) {
      bt = TABLE_BT_INT16;
      p.i16 = val;
    } else if (val <= 0xffffffff) {
      bt = TABLE_BT_INT32;
      p.i32 = val;
    } else {
      bt = TABLE_BT_INT64;
      p.i64 = val;
    }
  } else {
    u64 = val;
    if ((u64 | 0x1f) == full) {
      bt = TABLE_BT_INTN;
      p.i8 = u64;
    } else if ((u64 | 0xff) == full) {
      bt = TABLE_BT_INT8;
      p.i8 = u64;
    } else if ((u64 | 0xffff) == full) {
      bt = TABLE_BT_INT16;
      p.i16 = u64;
    } else if ((u64 | 0xffffffff) == full) {
      bt = TABLE_BT_INT32;
      p.i32 = u64;
    } else {
      bt = TABLE_BT_INT64;
      p.i64 = u64;
    }
  }

  switch(bt) {
  case TABLE_BT_INT0:
  case TABLE_BT_INTN:
    return table_write_bytes(&p.i8, sizeof(int8_t), 1, fp);

  case TABLE_BT_INT8:
    byte = bt;
    ws = sizeof(int8_t);
    pt = &p.i8;
    break;

  case TABLE_BT_INT16:
    byte = bt;
    ws = sizeof(int16_t);
    pt = &p.i16;
    break;

  case TABLE_BT_INT32:
    byte = bt;
    ws = sizeof(int32_t);
    pt = &p.i32;
    break;

  case TABLE_BT_INT64:
    byte = bt;
    ws = sizeof(int64_t);
    pt = &p.i64;
    break;

  default:
    return TABLE_ERR_RANGE;
  }

  e = table_write_bytes(&byte, sizeof(unsigned char), 1, fp);
  if (e != TABLE_SUCCESS) return e;

  return table_write_bytes(pt, ws, 1, fp);
}

static table_error table_mp_write_double_array(FILE *fp, size_t n, const double *data)
{
  table_error e;
  unsigned char byte;

  if (!fp) return TABLE_ERR_NULLP;

  if (!data) n = 0;

  e = table_mp_write_array_header(fp, n);
  if (e != TABLE_SUCCESS) return e;

  e = TABLE_SUCCESS;
  if (data) {
    byte = (unsigned char)TABLE_BT_DOUBLE;
    e = table_write_bytes(&byte, sizeof(unsigned char), 1, fp);
    if (e != TABLE_SUCCESS) return e;

    e = table_write_bytes(data, sizeof(double), n, fp);
    if (e != TABLE_SUCCESS) return e;
  }
  return e;
}

#endif /* of TABLE_NO_IO_FUNCTIONS */
