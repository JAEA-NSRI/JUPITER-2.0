
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "test-util.h"
#include "serializer_test.h"

#include <jupiter/serializer/error.h>
#include <jupiter/serializer/buffer.h>
#include <jupiter/serializer/msgpackx.h>

#define E2_le(a,b) b,a
#define E4_le(a,b,c,d) d,c,b,a
#define E8_le(a,b,c,d,e,f,g,h) h,g,f,e,d,c,b,a

#define E2_be(a,b) a,b
#define E4_be(a,b,c,d) a,b,c,d
#define E8_be(a,b,c,d,e,f,g,h) a,b,c,d,e,f,g,h

#define STR    's','t','r'
#define STR6   STR,STR
#define STR24  STR6,STR6,STR6,STR6
#define STR96  STR24,STR24,STR24,STR24
#define STR384 STR96,STR96,STR96,STR96

#define BIN    'b','i','n','a','r','y'
#define BIN12  BIN,BIN
#define BIN48  BIN12,BIN12,BIN12,BIN12
#define BIN192 BIN48,BIN48,BIN48,BIN48
#define BIN768 BIN192,BIN192,BIN192,BIN192

#define U2(ed,n) E2_##ed(UINT16_C(n)>>8&0xff,UINT16_C(n)&0xff)
#define U4(ed,n) E4_##ed(UINT32_C(n)>>24&0xff,UINT32_C(n)>>16&0xff,   \
                         UINT32_C(n)>>8&0xff,UINT32_C(n)&0xff)
#define U8(ed,n) E8_##ed(UINT64_C(n)>>56&0xff,UINT64_C(n)>>48&0xff,   \
                         UINT64_C(n)>>40&0xff,UINT64_C(n)>>32&0xff,   \
                         UINT64_C(n)>>24&0xff,UINT64_C(n)>>16&0xff,   \
                         UINT64_C(n)>>8&0xff,UINT64_C(n)&0xff)

#define parse_data(ed)                                                  \
  const unsigned char parse_data_##ed[] = {                             \
    0x94, /* 4 element array */                                         \
    /* [0] */ 0xff, /* -1 */                                            \
    /* [1] */ 0xfe, /* -2 */                                            \
    /* [2] */ 0x83, /* 3 element map */                                 \
    /* [2][0].k */ 0x01, /* 1 */                                        \
    /* [2][0].v */ 0xc7,0x01,0x04,'d','a','t','a', /* ext(1): data */   \
    /* [2][1].k */ 0xc0, /* nil */                                      \
    /* [2][1].v */ 0xc3, /* true */                                     \
    /* [2][2].k */ 0x93, /* 3 element array */                          \
    /* [2][2].k[0] */ 0xa3,'s','t','r', /* "str" */                     \
    /* [2][2].k[1] */ 0xd4,96,STR96,    /* "str"*32 */                  \
    /* [2][2].k[2] */ 0xd5,U2(ed,384),STR384, /* "str"*128 */           \
    /* [2][2].v */ 0xc4,U2(ed,12),BIN12, /* "bin"*12 */                 \
    /* [3] */ 0xda,U2(ed,6), /* 6 element array (verbose def) */        \
    /* [3][0] */ 0xdf,U8(ed,4), /* 4 element map (verbose def) */       \
    /* [3][0][0].k */ 0x01, /* 1 */                                     \
    /* [3][0][0].v */ 0xd0,122, /* 122 */                               \
    /* [3][0][1].k */ 0xd0,-44, /* -44 */                               \
    /* [3][0][1].v */ 0xd1,U2(ed,284), /* 284 */                        \
    /* [3][0][2].k */ 0xcd,U2(ed,0xfeff), /* 65279 */                   \
    /* [3][0][2].v */ 0xce,U4(ed,0x11223344),                           \
    /* [3][0][3].k */ 0xcf,U8(ed,0x41235789abcdef60),                   \
    /* [3][0][3].v */ 0xcc,0x22,                                        \
    /* [3][1] */ 0xd2,U4(ed,22), /* 22 (verbose def) */                 \
    /* [3][2] */ 0xd3,U8(ed,66), /* 66 (verbose def) */                 \
    /* [3][3] */ 0xcc,0x33, /* 0x33 */                                  \
    /* [3][4] */ 0xc2, /* false */                                      \
    /* [3][5] */ 0x92, /* 2 element array */                            \
    /* [3][5][0] */ 0xca,U4(ed,0x3e200000), /* float 32 */              \
    /* [3][5][1] */ 0xcb,U8(ed,0x3ff8000000000000), /* float 64 */      \
    0xc1,0xc1 /* invalid bytes */                                       \
  }

static parse_data(be);
static parse_data(le);

static msgpackx_node *
parse_node_test(msgpackx_node *node, int *ecnt)
{
  if (test_compare_c(node != NULL)) {
    ++*ecnt;
    return NULL;
  }
  return node;
}

static msgpackx_node *
parse_arr_test(msgpackx_array_node *arr, int *ecnt)
{
  msgpackx_node *node_r;
  node_r = msgpackx_array_node_get_child_node(arr);
  return parse_node_test(node_r, ecnt);
}

static msgpackx_node *
parse_map_key_test(msgpackx_map_node *map, int *ecnt)
{
  msgpackx_node *node_r;
  node_r = msgpackx_map_node_get_key(map);
  return parse_node_test(node_r, ecnt);
}

static msgpackx_node *
parse_map_value_test(msgpackx_map_node *map, int *ecnt)
{
  msgpackx_node *node_r;
  node_r = msgpackx_map_node_get_value(map);
  return parse_node_test(node_r, ecnt);
}

static int
parse_node_test_bool(msgpackx_node *node, int value,
                     const char *comp, const char *file, long line)
{
  int val;
  msgpackx_error err;

  MSGPACKX_ASSERT(node);

  err = MSGPACKX_SUCCESS;
  val = msgpackx_node_get_bool(node, &err);
  if (test_compare_f(err, MSGPACKX_SUCCESS,
                     "Failed to get bool: %s (%s:%ld)",
                     msgpackx_strerror(err), file, line)) {
    return 1;
  }

  return test_compare_fail((!!val) != (!!value), file, line, comp, NULL);
}
#define parse_node_test_bool(node, value)                               \
  parse_node_test_bool(node, value,                                     \
                       value ? #node " == true" : #node " == false",    \
                       __FILE__, __LINE__)

static int
parse_node_test_int(msgpackx_node *node, intmax_t value,
                    const char *comp, const char *file, long line)
{
  intmax_t im;
  msgpackx_error err;

  MSGPACKX_ASSERT(node);

  err = MSGPACKX_SUCCESS;
  im = msgpackx_node_get_int(node, &err);
  if (test_compare_f(err, MSGPACKX_SUCCESS,
                     "Failed to get int: %s (%s:%ld)",
                     msgpackx_strerror(err), file, line)) {
    return 1;
  }

  if (test_compare_fail(im != value, file, line, comp, NULL)) {
    fprintf(stderr, "..... Got %" PRIdMAX "\n", im);
    return 1;
  }
  return 0;
}
#define parse_node_test_int(node, value) \
  parse_node_test_int(node, value, #node " == " #value, __FILE__, __LINE__)

static int
parse_node_test_uint(msgpackx_node *node, uintmax_t value,
                     const char *comp, const char *file, long line)
{
  uintmax_t im;
  msgpackx_error err;

  MSGPACKX_ASSERT(node);

  err = MSGPACKX_SUCCESS;
  im = msgpackx_node_get_uint(node, &err);
  if (test_compare_f(err, MSGPACKX_SUCCESS,
                     "Failed to get uint: %s (%s:%ld)",
                     msgpackx_strerror(err), file, line)) {
    return 1;
  }

  if (test_compare_fail(im != value, file, line, comp, NULL)) {
    fprintf(stderr, "..... Got %" PRIuMAX " (%#" PRIxMAX ")\n", im, im);
    return 1;
  }
  return 0;
}
#define parse_node_test_uint(node, value)                               \
  parse_node_test_uint(node, value, #node " == " #value, __FILE__, __LINE__)

static int
parse_node_test_str(msgpackx_node *node, const char *data,
                    ptrdiff_t len, const char *comp,
                    const char *file, long line)
{
  const char *m;
  msgpackx_error err;
  ptrdiff_t len_ret;

  MSGPACKX_ASSERT(node);

  if (len < 0) {
    len = strlen(data);
  }

  err = MSGPACKX_SUCCESS;
  m = msgpackx_node_get_str(node, &len_ret, &err);
  if (test_compare_f(err, MSGPACKX_SUCCESS,
                     "Failed to get str: %s (%s:%ld)",
                     msgpackx_strerror(err), file, line)) {
    return 1;
  }

  if (test_compare_f(len_ret, len,
                     "Size does not match: %" PRIdMAX " (got) == %" PRIdMAX
                     " (expect) (%s:%ld)", (intmax_t)len_ret,
                     (intmax_t)len, file, line)) {
    return 1;
  }

  if (test_compare_fail(memcmp(m, data, len) != 0, file, line, comp, NULL)) {
    test_print_bytes(m, len);
    return 1;
  }
  return 0;
}
#define parse_node_test_str(node, value, len)                \
  parse_node_test_str(node, value, len, #node " == " #value, \
                      __FILE__, __LINE__)

static int
parse_node_test_bin(msgpackx_node *node, const char *data,
                    ptrdiff_t len, const char *comp,
                    const char *file, long line)
{
  const char *m;
  msgpackx_error err;
  ptrdiff_t len_ret;

  MSGPACKX_ASSERT(node);

  err = MSGPACKX_SUCCESS;
  m = msgpackx_node_get_bin(node, &len_ret, &err);
  if (test_compare_f(err, MSGPACKX_SUCCESS,
                     "Failed to get bin: %s (%s:%ld)",
                     msgpackx_strerror(err), file, line)) {
    return 1;
  }

  if (test_compare_f(len_ret, len,
                     "Size does not match: %" PRIdMAX " (got) == %" PRIdMAX
                     " (expect) (%s:%ld)", (intmax_t)len_ret,
                     (intmax_t)len, file, line)) {
    return 1;
  }

  if (test_compare_fail(memcmp(m, data, len) != 0, file, line, comp, NULL)) {
    test_print_bytes(m, len);
    return 1;
  }

  return 0;
}
#define parse_node_test_bin(node, value, len)                   \
  parse_node_test_bin(node, value, len, #node " == " #value,    \
                      __FILE__, __LINE__)


static int
parse_node_test_ext(msgpackx_node *node, int type, const char *data,
                    ptrdiff_t len, const char *comp,
                    const char *file, long line)
{
  const char *m;
  msgpackx_error err;
  ptrdiff_t len_ret;
  int type_ret;

  MSGPACKX_ASSERT(node);

  if (len < 0) {
    len = strlen(data);
  }

  err = MSGPACKX_SUCCESS;
  m = msgpackx_node_get_ext(node, &len_ret, &type_ret, &err);
  if (test_compare_f(err, MSGPACKX_SUCCESS,
                     "Failed to get ext: %s (%s:%ld)",
                     msgpackx_strerror(err), file, line)) {
    return 1;
  }

  if (test_compare_f(len_ret, len,
                     "Size does not match: %" PRIdMAX " (got) == %" PRIdMAX
                     " (expect) (%s:%ld)", (intmax_t)len_ret,
                     (intmax_t)len, file, line)) {
    return 1;
  }

  if (test_compare_f(type_ret, type,
                     "Type does not match: %d (got) == %d (expect) (%s:%ld)",
                     type_ret, type, file, line)) {
    return 1;
  }

  if (test_compare_fail(memcmp(m, data, len) != 0, file, line, comp, NULL)) {
    test_print_bytes(m, len);
    return 1;
  }
  return 0;
}
#define parse_node_test_ext(node, type, value, len)                     \
  parse_node_test_ext(node, type, value, len,                           \
                      #node " == " #value " (" #type ")", __FILE__, __LINE__)

static int
parse_node_test_float(msgpackx_node *node, float value,
                      const char *comp, const char *file, long line)
{
  static int fok = -1;
  float fgot;
  msgpackx_error err;

  MSGPACKX_ASSERT(node);

#if defined(__STDC_IEC_559__)
  fok = 1;
#else
  if (fok < 0) {
    float f;
    const unsigned char *fref;
    if (sizeof(float) == 4) {
      f = 0.15625f;
      if (test_is_little_endian()) {
        fref = (char[]){U4(le,0x3e200000)};
      } else {
        fref = (char[]){U4(be,0x3e200000)};
      }
      if (memcmp(&f, fref, sizeof(float)) == 0) {
        fok = 1;
      } else {
        fok = 0;
      }
    } else {
      fok = 0;
    }
    if (!fok) {
      fprintf(stderr, "WARN: It seems that your system does not use "
              "IEEE754 float32 for float type.\n"
              "..... Or, endianess differs to integers.\n");
    }
  }
#endif

  err = MSGPACKX_SUCCESS;
  fgot = msgpackx_node_get_float(node, &err);
  if (test_compare_f(err, MSGPACKX_SUCCESS,
                     "Failed to get float: %s (%s:%ld)",
                     msgpackx_strerror(err), file, line)) {
    return 1;
  }

  if (!fok || test_compare_fail(fgot != value, file, line, comp, NULL)) {
    if (!fok) {
      fprintf(stderr, "SKIP: %s\n", comp);
    }
    fprintf(stderr, "..... Got %g\n", fgot);
    test_print_bytes(&fgot, sizeof(float));
    return 1;
  }
  return 0;
}
#define parse_node_test_float(node, value)                                \
  parse_node_test_float(node, value, #node " == " #value, __FILE__, __LINE__)

static int
parse_node_test_double(msgpackx_node *node, double value,
                       const char *comp, const char *file, long line)
{
  static int fok = -1;
  double fgot;
  msgpackx_error err;

  MSGPACKX_ASSERT(node);

#if defined(__STDC_IEC_559__)
  fok = 1;
#else
  if (fok < 0) {
    double f;
    const unsigned char *fref;
    if (sizeof(double) == 8) {
      f = 1.5;
      if (test_is_little_endian()) {
        fref = (unsigned char[]){U8(le,0x3ff8000000000000)};
      } else {
        fref = (unsigned char[]){U8(be,0x3ff8000000000000)};
      }
      if (memcmp(&f, fref, sizeof(double)) == 0) {
        fok = 1;
      } else {
        fok = 0;
      }
    } else {
      fok = 0;
    }
    if (!fok) {
      fprintf(stderr, "WARN: It seems that your system does not use "
              "IEEE754 float64 for double type.\n"
              "..... Or, endianess differs to integers.\n");
    }
  }
#endif

  err = MSGPACKX_SUCCESS;
  fgot = msgpackx_node_get_double(node, &err);
  if (test_compare_f(err, MSGPACKX_SUCCESS,
                     "Failed to get double: %s (%s:%ld)",
                     msgpackx_strerror(err), file, line)) {
    return 1;
  }

  if (!fok || test_compare_fail(fgot != value, file, line, comp, NULL)) {
    if (!fok) {
      fprintf(stderr, "SKIP: %s\n", comp);
    }
    fprintf(stderr, "..... Got %g\n", fgot);
    test_print_bytes(&fgot, sizeof(double));
    return 1;
  }
  return 0;
}
#define parse_node_test_double(node, value)                             \
  parse_node_test_double(node, value, #node " == " #value, __FILE__, __LINE__)

int parse_test(void)
{
  int ecnt;
  int little;
  const unsigned char *p;
  ptrdiff_t psz;
  msgpackx_buffer *buf;
  msgpackx_data *data;
  msgpackx_node *node_r;
  msgpackx_map_node *map_head;
  msgpackx_map_node *map_node;
  msgpackx_array_node *arr_head;
  msgpackx_array_node *arr_node;
  msgpackx_error err;

  ecnt = 0;
  data = NULL;

#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
  little = 0;
#else
  little = test_is_little_endian();
  if (little) {
    fprintf(stderr, "Note: You are using little endian.\n");
  }
#endif

  fprintf(stderr, "..... Parsing data:\n");
  if (little) {
    p = parse_data_le;
    psz = sizeof(parse_data_le);
  } else {
    p = parse_data_be;
    psz = sizeof(parse_data_be);
  }
  test_print_bytes(p, psz);

  buf = msgpackx_buffer_new();
  if (!buf || !msgpackx_buffer_raw_copy(buf, p, psz, MSGPACKX_COPY_EXPAND)) {
    if (buf) msgpackx_buffer_delete(buf);
    fprintf(stderr, "ERROR Allocation failed\n");
    return 1;
  }

  err = MSGPACKX_SUCCESS;
  data = msgpackx_data_parse(buf, &err, &psz);
  msgpackx_buffer_delete(buf);

  if (test_compare(err, MSGPACKX_SUCCESS)) {
    fprintf(stderr, "..... Error: %s, at %" PRIdMAX " (%p)\n",
            msgpackx_strerror(err), (intmax_t)psz, (void *)(p + psz));
    ecnt++;
  }

  node_r = msgpackx_data_root_node(data);
  arr_head = msgpackx_node_get_array(node_r);
  if (test_compare_c(arr_head != NULL)) {
    ecnt++;
    goto clean;
  }

  if (test_compare_c(msgpackx_array_node_is_head_of_array(arr_head) != 0)) {
    ecnt++;
  }

  arr_node = msgpackx_array_node_next(arr_head);
  node_r = parse_arr_test(arr_node, &ecnt);
  if (node_r) ecnt += parse_node_test_int(node_r, -1);

  arr_node = msgpackx_array_node_next(arr_node);
  node_r = parse_arr_test(arr_node, &ecnt);
  if (node_r) ecnt += parse_node_test_int(node_r, -2);

  map_head = NULL;
  arr_node = msgpackx_array_node_next(arr_node);
  node_r = msgpackx_array_node_get_child_node(arr_node);
  if (test_compare_c(node_r != NULL)) {
    ecnt++;
  } else {
    map_head = msgpackx_node_get_map(node_r);
    if (test_compare_c(map_head != NULL)) {
      ecnt++;
    }
  }

  if (map_head) {
    map_node = msgpackx_map_node_next(map_head);
    node_r = parse_map_key_test(map_node, &ecnt);
    if (node_r) ecnt += parse_node_test_int(node_r, 1);

    node_r = parse_map_value_test(map_node, &ecnt);
    if (node_r) ecnt += parse_node_test_ext(node_r, 1, "data", 4);

    map_node = msgpackx_map_node_next(map_node);
    node_r = parse_map_key_test(map_node, &ecnt);
    if (node_r) {
      if (test_compare(msgpackx_node_is_nil(node_r), 1)) {
        ecnt++;
      }
    }

    node_r = parse_map_value_test(map_node, &ecnt);
    if (node_r) ecnt += parse_node_test_bool(node_r, 1);

    map_node = msgpackx_map_node_next(map_node);
    node_r = parse_map_key_test(map_node, &ecnt);
    if (node_r) {
      msgpackx_array_node *anh, *ann;

      anh = msgpackx_node_get_array(node_r);
      if (test_compare_c(anh != NULL)) {
        ecnt++;
      } else {
        ann = msgpackx_array_node_next(anh);
        node_r = parse_arr_test(ann, &ecnt);
        if (node_r) ecnt += parse_node_test_str(node_r, "str", 3);

        ann = msgpackx_array_node_next(ann);
        node_r = parse_arr_test(ann, &ecnt);
        if (node_r) ecnt += parse_node_test_str(node_r, (char[]){STR96}, 96);

        ann = msgpackx_array_node_next(ann);
        node_r = parse_arr_test(ann, &ecnt);
        if (node_r) ecnt += parse_node_test_str(node_r, (char[]){STR384}, 384);

        ann = msgpackx_array_node_next(ann);
        if (test_compare(ann, anh)) {
          ecnt++;
        }
      }
    }

    node_r = parse_map_value_test(map_node, &ecnt);
    if (node_r) ecnt += parse_node_test_bin(node_r, (char[]){BIN12}, 12);

    map_node = msgpackx_map_node_next(map_node);
    if (test_compare(map_node, map_head)) {
      ecnt++;
    }
  }

  arr_node = msgpackx_array_node_next(arr_node);
  node_r = parse_arr_test(arr_node, &ecnt);
  if (node_r) {
    msgpackx_array_node *anh, *ann;

    anh = msgpackx_node_get_array(node_r);
    if (test_compare_c(anh != NULL)) {
      ecnt++;
    } else {
      ann = msgpackx_array_node_next(anh);
      node_r = parse_arr_test(ann, &ecnt);
      if (node_r) {
        map_head = msgpackx_node_get_map(node_r);
        if (test_compare_c(map_head != NULL)) {
          ecnt++;
        } else {
          map_node = msgpackx_map_node_next(map_head);
          node_r = parse_map_key_test(map_node, &ecnt);
          if (node_r) ecnt += parse_node_test_uint(node_r, 1);

          node_r = parse_map_value_test(map_node, &ecnt);
          if (node_r) ecnt += parse_node_test_int(node_r, 122);

          map_node = msgpackx_map_node_next(map_node);
          node_r = parse_map_key_test(map_node, &ecnt);
          if (node_r) ecnt += parse_node_test_int(node_r, -44);

          node_r = parse_map_value_test(map_node, &ecnt);
          if (node_r) ecnt += parse_node_test_int(node_r, 284);

          map_node = msgpackx_map_node_next(map_node);
          node_r = parse_map_key_test(map_node, &ecnt);
          if (node_r) ecnt += parse_node_test_uint(node_r, 65279);

          node_r = parse_map_value_test(map_node, &ecnt);
          if (node_r) ecnt += parse_node_test_uint(node_r, UINTMAX_C(0x11223344));

          map_node = msgpackx_map_node_next(map_node);
          node_r = parse_map_key_test(map_node, &ecnt);
          if (node_r) ecnt += parse_node_test_uint(node_r, UINTMAX_C(0x41235789abcdef60));

          node_r = parse_map_value_test(map_node, &ecnt);
          if (node_r) ecnt += parse_node_test_uint(node_r, 0x22);

          map_node = msgpackx_map_node_next(map_node);
          if (test_compare(map_node, map_head)) {
            ecnt++;
          }
        }
      }

      ann = msgpackx_array_node_next(ann);
      node_r = parse_arr_test(ann, &ecnt);
      if (node_r) ecnt += parse_node_test_int(node_r, 22);

      ann = msgpackx_array_node_next(ann);
      node_r = parse_arr_test(ann, &ecnt);
      if (node_r) ecnt += parse_node_test_int(node_r, 66);

      ann = msgpackx_array_node_next(ann);
      node_r = parse_arr_test(ann, &ecnt);
      if (node_r) ecnt += parse_node_test_uint(node_r, 0x33);

      ann = msgpackx_array_node_next(ann);
      node_r = parse_arr_test(ann, &ecnt);
      if (node_r) ecnt += parse_node_test_bool(node_r, 0);

      ann = msgpackx_array_node_next(ann);
      node_r = parse_arr_test(ann, &ecnt);
      if (node_r) {
        msgpackx_array_node *annh, *annn;

        annh = msgpackx_node_get_array(node_r);
        if (test_compare_c(annh != NULL)) {
          ecnt++;
        } else {
          annn = msgpackx_array_node_next(annh);
          node_r = parse_arr_test(annn, &ecnt);
          if (node_r) ecnt += parse_node_test_float(node_r, 0.15625f);

          annn = msgpackx_array_node_next(annn);
          node_r = parse_arr_test(annn, &ecnt);
          if (node_r) ecnt += parse_node_test_double(node_r, 1.5);
        }
      }

      ann = msgpackx_array_node_next(ann);
      if (test_compare(ann, anh)) {
        ecnt++;
      }
    }
  }

  arr_node = msgpackx_array_node_next(arr_node);
  if (test_compare(arr_node, arr_head)) {
    ecnt++;
  }

clean:
  msgpackx_data_delete(data);
  return ecnt;
}
