
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "test-util.h"
#include "serializer_test.h"

#include <jupiter/serializer/error.h>
#include <jupiter/serializer/msgpackx.h>

int array_test(void)
{
  msgpackx_node *node;
  msgpackx_node *node_r;
  msgpackx_array_node *arr_head;
  msgpackx_array_node *arr_node;
  ptrdiff_t sz;
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
  node = NULL;
  arr_head = NULL;
  arr_node = NULL;

  arr_head = msgpackx_array_node_new(MSGPACKX_ARRAY_HEAD);
  if (!arr_head) goto clean;

  err = MSGPACKX_SUCCESS;
  msgpackx_array_node_pack(arr_head, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }
  node_r = msgpackx_array_node_upcast(arr_head);
  if (test_compare_node(bytes(0x90), node_r)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node, NULL), 1);
    ecnt++;
  }

  for (sz = 0; sz < 5; ++sz) {
    node = msgpackx_node_new();
    arr_node = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
    if (!node || !arr_node) {
      fprintf(stderr, "WARN: Allocation error occured. Abort.\n");
      goto clean;
    }

    err = MSGPACKX_SUCCESS;
    msgpackx_node_set_int(node, sz, &err);
    if (err != MSGPACKX_SUCCESS) {
      fprintf(stderr, "WARN: Failed to set node. Abort.\n");
      goto clean;
    }
    msgpackx_array_node_set_child_node(arr_node, node);
    msgpackx_array_node_insert_prev(arr_head, arr_node);
    node = NULL;
    arr_node = NULL;
  }

  err = MSGPACKX_SUCCESS;
  msgpackx_array_node_pack(arr_head, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  node_r = msgpackx_array_node_upcast(arr_head);
  if (test_compare_node(bytes(0x95,0x00,0x01,0x02,0x03,0x04), node_r)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 6);
    ecnt++;
  }

#if PTRDIFF_MAX > UINT8_MAX
  err = MSGPACKX_SUCCESS;
  for (sz = 0; sz < 16; ++sz) {
    node = msgpackx_node_new();
    arr_node = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
    if (!node || !arr_node) {
      err = MSGPACKX_ERR_NOMEM;
      fprintf(stderr, "WARN: Allocation error occured. Abort.\n");
      break;
    }

    err = MSGPACKX_SUCCESS;
    msgpackx_node_set_int(node, sz + 0x10, &err);
    if (err != MSGPACKX_SUCCESS) {
      fprintf(stderr, "WARN: Failed to set node. Abort.\n");
      break;
    }
    msgpackx_array_node_set_child_node(arr_node, node);
    msgpackx_array_node_insert_next(arr_head, arr_node);
    node = NULL;
    arr_node = NULL;
  }

  if (err == MSGPACKX_SUCCESS) {
    err = MSGPACKX_SUCCESS;
    msgpackx_array_node_pack(arr_head, &err);
    if (test_compare(err, MSGPACKX_SUCCESS)) {
      ecnt++;
    }

    node_r = msgpackx_array_node_upcast(arr_head);
    if (!little) {
      if (test_compare_node(bytes(0xda,0x00,0x15,0x1f,0x1e,0x1d), node_r)) {
        test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 6);
        ecnt++;
      }
    } else {
      if (test_compare_node(bytes(0xda,0x15,0x00,0x1f,0x1e,0x1d), node_r)) {
        test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 6);
        ecnt++;
      }
    }
  }
#endif

#if PTRDIFF_MAX > UINT16_MAX
  err = MSGPACKX_SUCCESS;
  for (sz = 0; sz < 65539; ++sz) {
    node = msgpackx_node_new();
    arr_node = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
    if (!node || !arr_node) {
      err = MSGPACKX_ERR_NOMEM;
      fprintf(stderr, "WARN: Allocation error occured. Abort.\n");
      break;
    }

    err = MSGPACKX_SUCCESS;
    msgpackx_node_set_int(node, sz, &err);
    if (err != MSGPACKX_SUCCESS) {
      fprintf(stderr, "WARN: Failed to set node. Abort.\n");
      break;
    }
    msgpackx_array_node_set_child_node(arr_node, node);
    msgpackx_array_node_insert_next(arr_head, arr_node);
    node = NULL;
    arr_node = NULL;
  }

  if (err == MSGPACKX_SUCCESS) {
    err = MSGPACKX_SUCCESS;
    msgpackx_array_node_pack(arr_head, &err);
    if (test_compare(err, MSGPACKX_SUCCESS)) {
      ecnt++;
    }

    node_r = msgpackx_array_node_upcast(arr_head);
    if (!little) {
      if (test_compare_node(bytes(0xdb,0x00,0x01,0x00,0x18,0xd2,0x00,0x01,0x00,0x02), node_r)) {
        test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 10);
        ecnt++;
      }
    } else {
      if (test_compare_node(bytes(0xdb,0x18,0x00,0x01,0x00,0xd2,0x02,0x00,0x01,0x00), node_r)) {
        test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 10);
        ecnt++;
      }
    }
  }
#endif

clean:
  if (node) msgpackx_node_delete(node);
  if (arr_node) msgpackx_array_node_delete(arr_node);
  if (arr_head) msgpackx_array_node_delete_all(arr_head);
  return ecnt;
}
