
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "test-util.h"
#include "serializer_test.h"

#include <jupiter/serializer/error.h>
#include <jupiter/serializer/msgpackx.h>

static const char *lst[] = {
  "Eridanus", "Crux", "Taurus", "Cepheus", "Ursa Major", "Grus", "Hydra",
  "Corona Borealis", "Andromeda", "Pisces", "Aquila", "Phoenix", "Vulpecula",
  "Scorpius", "Bootes", "Lepus", "Triangulum Australe", "Carina", "Auriga",
  "Canes Venatici", "Fornax", "Cygnus", "Capricornus", "Cetus", "Draco",
  "Pegasus", "Piscis Austrinus", "Corvus", "Aries", "Sagittarius", "Equuleus",
  "Hercules", "Corona Australis", "Perseus", "Puppis", "Pavo", "Columba",
  "Ursa Minor", "Gemini", "Leo Minor", "Canis Minor", "Ophiuchus", "Leo",
  "Orion", "Centaurus", "Delphinus", "Aquarius", "Cassiopeia", "Canis Major",
  "Virgo", "Cancer", "Serpens", "Lyra", "Libra", "Antlia", "Apus", "Caelum",
  "Chamaeleon", "Circinus", "Dorado", "Horologium", "Indus", "Lacerta",
  "Lupus", "Lynx", "Mensa", "Musca", "Pictor", "Pyxis", "Reticulum",
  "Sculptor", "Scutum", "Sextans", "Telescopium", "Tucana", "Ara",
  "Coma Berenices", "Hydrus", "Monoceros", "Triangulum", "Volans",
  "Camelopardalis", "Microscopium", "Sagitta", "Norma", "Vela", "Crater",
  "Octans",
};
static const ptrdiff_t lstsz = sizeof(lst) / sizeof(lst[0]);

int map_test(void)
{
  char stbuf[100];
  msgpackx_node *node;
  msgpackx_node *node_r;
  msgpackx_map_node *map_head;
  msgpackx_map_node *map_node;
  ptrdiff_t sz;
  msgpackx_error err;
  int ecnt;
  int little;

  ecnt = 0;
  node = NULL;
  map_head = NULL;
  map_node = NULL;

#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
  little = 0;
#else
  little = test_is_little_endian();
  if (little) {
    fprintf(stderr, "Note: You are using little endian.\n");
  }
#endif

  map_head = msgpackx_map_node_new(MSGPACKX_MAP_HEAD);
  if (!map_head) {
    fprintf(stderr, "ERROR Could not allocate map memory");
    goto clean;
  }

  for (sz = 0; sz < 5; ++sz) {
    int i;

    node = msgpackx_node_new();
    map_node = msgpackx_map_node_new(MSGPACKX_MAP_ENTRY);
    if (!node || !map_node) {
      fprintf(stderr, "WARN: Allocation error occured. Abort.\n");
      goto clean;
    }

    err = MSGPACKX_SUCCESS;
    msgpackx_node_set_uint(node,
                           (uint16_t)((uint16_t)(sz + 1) * 123456 + 33221),
                           &err);
    if (err != MSGPACKX_SUCCESS) {
      fprintf(stderr, "WARN: Failed to set node. Abort.\n");
      goto clean;
    }

    msgpackx_map_node_set_key(map_node, node, &err);
    if (test_compare(err, MSGPACKX_SUCCESS)) {
      ecnt++;
      msgpackx_map_node_delete(map_node);
      msgpackx_node_delete(node);
      continue;
    }

    node = msgpackx_node_new();
    if (!node) {
      fprintf(stderr, "WARN: Allocation error occured. Abort.\n");
      goto clean;
    }

    for (i = 0; i < sz + 1; ++i) {
      stbuf[i] = sz + i + 65;
    }

    msgpackx_node_set_str(node, stbuf, sz + 1, &err);
    if (err != MSGPACKX_SUCCESS) {
      fprintf(stderr, "WARN: Failed to set node. Abort.\n");
      goto clean;
    }

    msgpackx_map_node_set_value(map_node, node);
    msgpackx_map_node_insert(map_head, map_node, &err);
    if (test_compare(err, MSGPACKX_SUCCESS)) {
      ecnt++;
      break;
    }
    node = NULL;
    map_node = NULL;
  }

  msgpackx_map_node_pack(map_head, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  /*
   * key:   1 * 123456 + 33221 = 156677 = 0x2[6405]
   * value: 65 = 0x42
   */
  node_r = msgpackx_map_node_upcast(map_head);
  if (!little) {
    if (test_compare_node(bytes(0x85,0xcd,0x64,0x05,0xa1,0x41), node_r)) {
      test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 11);
      ecnt++;
    }
  } else {
    if (test_compare_node(bytes(0x85,0xcd,0x05,0x64,0xa1,0x41), node_r)) {
      test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 11);
      ecnt++;
    }
  }

  /*
   * key: 1 * 123456 + 33221 = 156677 = 0x2[6405]
   * key: 2 * 123456 + 33221 = 280133 = 0x4[4645]
   * key: 3 * 123456 + 33221 = 403589 = 0x6[2885]
   * key: 4 * 123456 + 33221 = 527045 = 0x8[0AC5]
   * key: 5 * 123456 + 33221 = 650501 = 0x9[ED05]
   * --> so minimum is 0x0AC5 in big, 0x6405 in little
   */
  map_node = msgpackx_map_node_next_sorted(map_head);
  node_r = msgpackx_map_node_get_key(map_node);
  if (!little) {
    if (test_compare_node(bytes(0xcd,0x0a,0xc5), node_r)) {
      test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 3);
      ecnt++;
    }
  } else {
    if (test_compare_node(bytes(0xcd,0x05,0x64), node_r)) {
      test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 3);
      ecnt++;
    }
  }

  /* --> and second is 0x2885 in big, 0xED05 in little  */
  map_node = msgpackx_map_node_next_sorted(map_node);
  node_r = msgpackx_map_node_get_key(map_node);
  if (!little) {
    if (test_compare_node(bytes(0xcd,0x28,0x85), node_r)) {
      test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 3);
      ecnt++;
    }
  } else {
    if (test_compare_node(bytes(0xcd,0x05,0xed), node_r)) {
      test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 3);
      ecnt++;
    }
  }

  err = MSGPACKX_SUCCESS;
  map_node = msgpackx_map_node_find_by_uint(map_head, 0xED05, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  node_r = msgpackx_map_node_get_key(map_node);
  if (!little) {
    if (test_compare_node(bytes(0xcd,0xed,0x05), node_r)) {
      test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 3);
      ecnt++;
    }
  } else {
    if (test_compare_node(bytes(0xcd,0x05,0xed), node_r)) {
      test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 3);
      ecnt++;
    }
  }

  err = MSGPACKX_SUCCESS;
  map_node = msgpackx_map_node_find_by_int(map_head, 0xED05, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }
  if (test_compare(map_node, map_head)) {
    ecnt++;
  }

  for (sz = 0; sz < lstsz; ++sz) {
    int i;

    node = msgpackx_node_new();
    map_node = msgpackx_map_node_new(MSGPACKX_MAP_ENTRY);
    if (!node || !map_node) {
      fprintf(stderr, "WARN: Allocation error occured. Abort.\n");
      goto clean;
    }

    msgpackx_node_set_str(node, lst[sz], strlen(lst[sz]), &err);
    if (err != MSGPACKX_SUCCESS) {
      fprintf(stderr, "WARN: Failed to set node. Abort.\n");
      goto clean;
    }

    msgpackx_map_node_set_key(map_node, node, &err);
    if (test_compare(err, MSGPACKX_SUCCESS)) {
      ecnt++;
      msgpackx_map_node_delete(map_node);
      msgpackx_node_delete(node);
      continue;
    }

    node = msgpackx_node_new();
    if (!node) {
      fprintf(stderr, "WARN: Allocation error occured. Abort.\n");
      goto clean;
    }

    msgpackx_node_set_int(node, sz + 1, &err);
    if (err != MSGPACKX_SUCCESS) {
      fprintf(stderr, "WARN: Failed to set node. Abort.\n");
      goto clean;
    }
    msgpackx_map_node_set_value(map_node, node);
    msgpackx_map_node_insert(map_head, map_node, &err);
    if (test_compare(err, MSGPACKX_SUCCESS)) {
      ecnt++;
      break;
    }
    node = NULL;
    map_node = NULL;
  }

  /*
   * Length of text has priority.
   * (because type key is included in comparison)
   *
   * So, the first item is "Ara" (in ASCII).
   */
  map_node = msgpackx_map_node_next_sorted(map_head);
  node_r = msgpackx_map_node_get_key(map_node);
  if (test_compare_node(bytes(0xa3,'A','r','a'), node_r)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 4);
    ecnt++;
  }

  /* --> and second is "Leo". */
  map_node = msgpackx_map_node_next_sorted(map_node);
  node_r = msgpackx_map_node_get_key(map_node);
  if (test_compare_node(bytes(0xa3,'L','e','o'), node_r)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 4);
    ecnt++;
  }

  /* --> Next of "Leo" in insertion order is "Orion" */
  map_node = msgpackx_map_node_next(map_node);
  node_r = msgpackx_map_node_get_key(map_node);
  if (test_compare_node(bytes(0xa5,'O','r','i','o','n'), node_r)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 6);
    ecnt++;
  }

  /* --> Alphabetical next (and same length) of "Orion" will be "Pyxis" */
  map_node = msgpackx_map_node_next_sorted(map_node);
  node_r = msgpackx_map_node_get_key(map_node);
  if (test_compare_node(bytes(0xa5,'P','y','x','i','s'), node_r)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 6);
    ecnt++;
  }

  /* --> Alphabetical next (and same length) of "Pyxis" will be "Virgo" */
  map_node = msgpackx_map_node_next_sorted(map_node);
  node_r = msgpackx_map_node_get_key(map_node);
  if (test_compare_node(bytes(0xa5,'V','i','r','g','o'), node_r)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 6);
    ecnt++;
  }

  /* --> Alphabetical next of "Virgo" will be "Antlia" */
  map_node = msgpackx_map_node_next_sorted(map_node);
  node_r = msgpackx_map_node_get_key(map_node);
  if (test_compare_node(bytes(0xa6,'A','n','t','l','i','a'), node_r)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 7);
    ecnt++;
  }

  err = MSGPACKX_SUCCESS;
  map_node = msgpackx_map_node_find_by_str(map_head, "Draco", -1, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }
  node_r = msgpackx_map_node_get_key(map_node);
  if (test_compare_node(bytes(0xa5,'D','r','a','c','o'), node_r)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 6);
    ecnt++;
  }

  node_r = msgpackx_map_node_get_value(map_node);
  if (test_compare_node(bytes(25), node_r)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 1);
    ecnt++;
  }

  err = MSGPACKX_SUCCESS;
  map_node = msgpackx_map_node_find_by_str(map_head, "Sagittae", -1, &err);
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }
  if (test_compare(map_node, map_head)) {
    ecnt++;
  }

  /* --> minimum uint is 0x0AC5 in big, 0x6405 in little */
  if (!little) {
    map_node = msgpackx_map_node_find_by_uint(map_head, 0x0ac5, &err);
  } else {
    map_node = msgpackx_map_node_find_by_uint(map_head, 0x6405, &err);
  }
  if (test_compare(err, MSGPACKX_SUCCESS)) {
    ecnt++;
  }

  /* Longest name is "Triangulum Australe" */
  map_node = msgpackx_map_node_prev_sorted(map_node);
  node_r = msgpackx_map_node_get_key(map_node);
  if (test_compare_node(bytes(0xb3,'T','r','i','a','n','g','u','l','u','m',' ','A','u','s','t','r','a','l','e'), node_r)) {
    test_print_bytes(msgpackx_node_get_data_pointer(node_r, NULL), 20);
    ecnt++;
  }

  map_node = NULL;

clean:
  if (node) msgpackx_node_delete(node);
  if (map_node) msgpackx_map_node_delete(map_node);
  if (map_head) msgpackx_map_node_delete_all(map_head);
  return ecnt;
}
