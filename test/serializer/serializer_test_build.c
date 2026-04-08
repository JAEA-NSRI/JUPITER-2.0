
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

// #define DEBUG_MEMCMP
#ifdef DEBUG_MEMCMP
#define memcmp(a, b, s) \
  ((test_print_bytes(a, s), test_print_bytes(b, s), \
    fprintf(stderr, "..... Size: %" PRIdMAX ", ", (intmax_t)s), \
    fprintf(stderr, "Comp: %d\n", memcmp(a, b, s)), memcmp(a, b, s)))
#endif

typedef msgpackx_node *node_setter_func(msgpackx_node *, void *,
                                        ptrdiff_t, msgpackx_error *);

static msgpackx_node *
node_set_str(msgpackx_node *n, void *p, ptrdiff_t s, msgpackx_error *e)
{
  return msgpackx_node_set_str(n, p, s, e);
}

static msgpackx_node *
node_set_bin(msgpackx_node *n, void *p, ptrdiff_t s, msgpackx_error *e)
{
  return msgpackx_node_set_bin(n, p, s, e);
}

static msgpackx_node *
node_set_ext(msgpackx_node *n, void *p, ptrdiff_t s, msgpackx_error *e)
{
  return msgpackx_node_set_ext(n, 1, p, s, e);
}

static int
test_bin_size(ptrdiff_t sz, node_setter_func *func, int bits, size_t compsz,
              const void *comp_data_big, const void *comp_data_little,
              const char *exprs_big, const char *exprs_little,
              msgpackx_error *err, int little,
              const char *file, long line)
{
  msgpackx_node *node_r;
  char *tmp;
  int ret;
  int stat;

  node_r = msgpackx_node_new();
  if (!node_r) {
    fprintf(stderr, "WARN: Node allocation failed\n");
    return 0;
  }

  tmp = malloc(sz);
  if (!tmp) {
    fprintf(stderr,
            "WARN: Could not allocate %" PRIdMAX " bytes.\n"
            "..... Skipping %dbits test.\n",
            (intmax_t)sz, bits);
    msgpackx_node_delete(node_r);
    return 0;
  }

  tmp[0] = 0x1b;
  if (!func(node_r, tmp, sz, err)) {
    fprintf(stderr,
            "WARN: Could not set %" PRIdMAX " bytes data.\n"
            "..... Skipping %dbits test.\n",
            (intmax_t)sz, bits);
    free(tmp);
    msgpackx_node_delete(node_r);
    return 1;
  }
  free(tmp);

  if (little) {
    stat = memcmp(comp_data_little,
                  msgpackx_node_get_data_pointer(node_r, NULL), compsz);
    ret = test_compare_fail(stat != 0, file, line, exprs_little, NULL);
  } else {
    stat = memcmp(comp_data_big, msgpackx_node_get_data_pointer(node_r, NULL),
                  compsz);
    ret = test_compare_fail(stat != 0, file, line, exprs_big, NULL);
  }
  if (ret) {
    test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL),
                     compsz + 1);
  }

  msgpackx_node_delete(node_r);
  return ret;
}

#define STAG(...) #__VA_ARGS__

#define test_size_fix(func, tag, base, err, little)             \
  test_bin_size((ptrdiff_t)tag - base, func, 8, 1,              \
                (unsigned char[]){tag}, (unsigned char[]){tag}, \
                #func " == ((char[]){" STAG(tag) "})",          \
                #func " == ((char[]){" STAG(tag) "})",          \
                err, little, __FILE__, __LINE__)

#define test_size_8(func, tag, err, little)                  \
  test_bin_size((ptrdiff_t)0x80, func, 8, 2,                 \
                (unsigned char[]){tag,0x80},                 \
                (unsigned char[]){tag,0x80},                 \
                #func " == ((char[]){" STAG(tag) ",0x80})",  \
                #func " == ((char[]){" STAG(tag) ",0x80})",  \
                err, little, __FILE__, __LINE__)

#define test_size_16(func, tag, err, little)                     \
  test_bin_size((ptrdiff_t)UINT8_MAX + 1, func, 16, 3,           \
                (unsigned char[]){tag,0x01,0x00},                \
                (unsigned char[]){tag,0x00,0x01},                \
                #func " == ((char[]){" STAG(tag) ",0x01,0x00})", \
                #func " == ((char[]){" STAG(tag) ",0x00,0x01})", \
                err, little, __FILE__, __LINE__)

#define test_size_32(func, tag, err, little)                          \
  test_bin_size((ptrdiff_t)UINT16_MAX + 1, func, 32, 5,               \
                (unsigned char[]){tag,0x00,0x01,0x00,0x00},           \
                (unsigned char[]){tag,0x00,0x00,0x01,0x00},             \
                #func " == ((char[]){" STAG(tag) ",0x00,0x01,0x00,0x00})", \
                #func " == ((char[]){" STAG(tag) ",0x00,0x00,0x01,0x00})", \
                err, little, __FILE__, __LINE__)

#define test_size_64(func, tag, err, little)                            \
  test_bin_size((ptrdiff_t)UINT32_MAX + 1, func, 64, 9,                 \
                (unsigned char[]){tag,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00}, \
                (unsigned char[]){tag,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00}, \
                #func " == ((char[]){" STAG(tag)                        \
                ",0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00})",           \
                #func " == ((char[]){" STAG(tag)                        \
                ",0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00})",           \
                err, little, __FILE__, __LINE__)

static int
test_node_bool_imp(msgpackx_node *node, int boolv, unsigned char tag,
                   const char *expr, const char *file, long line)
{
  msgpackx_error err;
  msgpackx_node *node_r;
  int stat;
  int ret;

  err = MSGPACKX_SUCCESS;

  node_r = msgpackx_node_set_bool(node, boolv, &err);
  if (test_compare(node_r, node) || err != MSGPACKX_SUCCESS) {
    fprintf(stderr, "Failed to set bool: %s", msgpackx_strerror(err));
    return 1;
  }
  stat = memcmp(&tag, msgpackx_node_get_data_pointer(node, NULL), 1);
  ret = test_compare_fail(stat != 0, file, line, expr, NULL);
  if (ret) {
    test_print_bytes(msgpackx_node_get_data_pointer(node, NULL), 1);
  }
  return ret;
}

#define test_node_bool(node, boolv, tag) \
  test_node_bool_imp(node, boolv, tag, #node " == ((char[]){" #tag "})", \
                     __FILE__, __LINE__)

#define CMA(x,y) x,y

int build_test(void)
{
  msgpackx_data *data;
  msgpackx_node *node;
  msgpackx_node *node_r;
  msgpackx_map_node *map_head;
  msgpackx_map_node *map_node;
  msgpackx_array_node *arr_head;
  msgpackx_array_node *arr_node;
  msgpackx_error err;
  int ecnt;
  int little;

#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
  little = 0;
#else
  little = test_is_little_endian();
  if (little) {
    fprintf(stderr, "Note: You are using little endian.\n");
  }
#endif

  ecnt = 0;
  data = msgpackx_data_new();
  if (!data) return 1;

  map_head = NULL;
  map_node = NULL;
  arr_head = NULL;
  arr_node = NULL;

  node = msgpackx_node_new();
  if (!node) goto clean;

  err = MSGPACKX_SUCCESS;

  if (test_size_fix(node_set_str, 0xa5, 0xa0, &err, little)) {
    ecnt++;
  }
  if (test_size_8(node_set_str, 0xd4, &err, little)) {
    ecnt++;
  }
#if PTRDIFF_MAX > UINT8_MAX
  if (test_size_16(node_set_str, 0xd5, &err, little)) {
    ecnt++;
  }
#endif
#if PTRDIFF_MAX > UINT16_MAX
  if (test_size_32(node_set_str, 0xd6, &err, little)) {
    ecnt++;
  }
#endif
#if PTRDIFF_MAX > UINT8_MAX
  if (test_size_16(node_set_bin, 0xc4, &err, little)) {
    ecnt++;
  }
#endif
#if PTRDIFF_MAX > UINT16_MAX
  if (test_size_32(node_set_bin, 0xc5, &err, little)) {
    ecnt++;
  }
#endif
#if PTRDIFF_MAX > UINT32_MAX
  err = MSGPACKX_SUCCESS;
  if (test_size_64(node_set_bin, 0xc6, &err, little)) {
    ecnt++;
  }
#endif
  if (test_size_8(node_set_ext, CMA(0xc7,1), &err, little)) {
    ecnt++;
  }
#if PTRDIFF_MAX > UINT8_MAX
  if (test_size_16(node_set_ext, CMA(0xc8,1), &err, little)) {
    ecnt++;
  }
#endif
#if PTRDIFF_MAX > UINT16_MAX
  if (test_size_32(node_set_ext, CMA(0xc9,1), &err, little)) {
    ecnt++;
  }
#endif

  err = MSGPACKX_SUCCESS;
  node_r = msgpackx_node_set_nil(node, &err);
  if (test_compare(node_r, node) || err != MSGPACKX_SUCCESS) {
    fprintf(stderr, "Failed to nil: %s", msgpackx_strerror(err));
  }
  if (test_compare_node(bytes(0xc0), node)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node, NULL), 1);
    ecnt++;
  }

  if (test_node_bool(node, 0, 0xc2)) {
    ecnt++;
  }
  if (test_node_bool(node, 1, 0xc3)) {
    ecnt++;
  }
  msgpackx_node_delete(node);
  node = NULL;

  map_head = msgpackx_map_node_new(MSGPACKX_MAP_HEAD);
  map_node = msgpackx_map_node_new(MSGPACKX_MAP_ENTRY);
  node = msgpackx_node_new();
  if (!map_head || !map_node || !node) {
    fprintf(stderr, "..... Allocation failed\n");
    goto clean;
  }

  msgpackx_node_set_int(node, -1, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  if (test_compare_node(bytes(0xff), node)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node, NULL), 1);
    ecnt++;
  }

  msgpackx_map_node_set_key(map_node, node, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  node = msgpackx_node_new();
  if (!node) {
    fprintf(stderr, "..... Allocation failed\n");
    goto clean;
  }

  msgpackx_node_set_int(node, 9999, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  msgpackx_map_node_set_value(map_node, node);

  msgpackx_map_node_insert(map_head, map_node, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }
  map_node = NULL;

  arr_head = msgpackx_array_node_new(MSGPACKX_ARRAY_HEAD);
  arr_node = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
  map_node = msgpackx_map_node_new(MSGPACKX_MAP_ENTRY);
  node = msgpackx_node_new();
  if (!arr_head || !arr_node || !node || !map_node) {
    fprintf(stderr, "..... Allocation failed\n");
    goto clean;
  }

  msgpackx_node_set_int(node, -32, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  if (test_compare_node(bytes(0xe0), node)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node, NULL), 1);
    ecnt++;
  }

  msgpackx_array_node_set_child_node(arr_node, node);
  msgpackx_array_node_insert_prev(arr_head, arr_node);
  msgpackx_array_node_pack(arr_head, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  node_r = msgpackx_array_node_upcast(arr_head);
  msgpackx_map_node_set_key(map_node, node_r, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  node = msgpackx_node_new();
  msgpackx_node_set_ext(node, 1, "data", 4, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  if (test_compare_node(bytes(0xc7,1,0x04,'d','a','t','a'), node)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node, NULL), 1);
    ecnt++;
  }

  msgpackx_map_node_set_value(map_node, node);

  msgpackx_map_node_insert(map_head, map_node, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  node_r = msgpackx_map_node_upcast(map_head);
  msgpackx_data_set_root_node(data, node_r);

  msgpackx_data_pack(data, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  /*
   * The representation in YAML is like below.
   *
   *     ---
   *     -1: 9999
   *     [-32]: !1 "data"
   */
  node_r = msgpackx_data_root_node(data);
  if (!little) {
    if (test_compare_node(bytes(0x82,/* map of two */
                                0xff,/* key: -1 */
                                0xd1,0x27,0x0f,/* val: 9999 */
                                0x91,/* key: array of one item */
                                0xe0,/* [1]: -32 */
                                0xc7,1,0x04,'d','a','t','a'),/* val: data */
                          node_r)) {
      test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 14);
      ecnt++;
    }
  } else {
    if (test_compare_node(bytes(0x82,/* map of two */
                                0xff,/* key: -1 */
                                0xd1,0x0f,0x27,/* val: 9999 */
                                0x91,/* key: array of one item */
                                0xe0,/* [1]: -32 */
                                0xc7,1,0x04,'d','a','t','a'),/* val: data */
                          node_r)) {
      test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 14);
      ecnt++;
    }
  }

  {
    FILE *fp;
    msgpackx_buffer *buf;
    buf = msgpackx_buffer_new();

    fp = tmpfile();
    if (fp) {
      ptrdiff_t r;

      r = msgpackx_data_write(data, fp);

      fseek(fp, 0, SEEK_SET);

      if (r >= 0) {
        char *p;
        p = NULL;
        if (buf) {
          p = msgpackx_buffer_resize(buf, r);
        }
        if (p) {
          p = msgpackx_buffer_pointer(buf);
          r = fread(p, sizeof(char), r, fp);
          if (!little) {
            if (test_compare_bytes(bytes(0x82,/* map of two */
                                         0xff,/* key: -1 */
                                         0xd1,0x27,0x0f,/* val: 9999 */
                                         0x91,/* key: array of one item */
                                         0xe0,/* [1]: -32 */
                                         0xc7,1,0x04,'d','a','t','a'),/* val */
                                   p)) {
              test_print_bytes(p, r);
              ecnt++;
            }
          } else {
            if (test_compare_bytes(bytes(0x82,/* map of two */
                                         0xff,/* key: -1 */
                                         0xd1,0x0f,0x27,/* val: 9999 */
                                         0x91,/* key: array of one item */
                                         0xe0,/* [1]: -32 */
                                         0xc7,1,0x04,'d','a','t','a'),/* val */
                                   p)) {
              test_print_bytes(p, r);
              ecnt++;
            }
          }
        } else {
          fprintf(stderr, "..... Could not allocate memory\n");
        }
      } else {
        perror("..... Could not write data");
      }
      fclose(fp);
    } else {
      perror("..... Could not open temporary file");
    }
    if (buf) {
      msgpackx_buffer_delete(buf);
    }
  }

  map_node = NULL;
  map_head = NULL;
  arr_node = NULL;
  arr_head = NULL;

clean:
  if (node) msgpackx_node_delete(node);
  if (map_node) msgpackx_map_node_delete(map_node);
  if (map_head) msgpackx_map_node_delete_all(map_head);
  if (arr_node) msgpackx_array_node_delete(arr_node);
  if (arr_head) msgpackx_array_node_delete_all(arr_head);
  msgpackx_data_delete(data);
  return ecnt;
}
