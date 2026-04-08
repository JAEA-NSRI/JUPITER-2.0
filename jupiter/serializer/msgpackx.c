
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <inttypes.h>
#include <errno.h>
#include <math.h>

#include "buffer.h"
#include "msgpackx.h"
#include "byteswap.h"
#include "defs.h"
#include "error.h"

#include "../geometry/list.h"
#include "../geometry/rbtree.h"

#if CHAR_BIT != 8
#error "CHAR_BIT must be 8."
#endif

/**
 * @ingroup Serializer
 * @brief A node of `msgpackx_data`
 *
 * A `msgpackx_node` will be list of two elements, parent and child.
 *
 *            +------+
 *     parent | head | (pointer == NULL)
 *            +------+
 *              ^  |
 *              |  v
 *            +------+
 *      child | data | (pointer != NULL)
 *            +------+
 *
 * Root node is on `msgpackx_data`.
 *
 *      ----+------+----
 *          | head |      (root)
 *      ----+------+----
 *            ^  |
 *            |  v
 *        +----------+
 *        | 1st item |
 *        +----------+
 *
 * An array uses `msgpackx_array_node` at data.
 *
 *       ----+------+----
 *           | head |     (parent data)
 *       ----+------+----
 *             ^  |  (last) <-.            .------------.
 *             |  v           v            |            v
 *           +------+------------+------+  |  +------+----------+------+
 *     array | data | array head | head |  |  | data | 1st item | head |
 *           +------+------------+------+  |  +------+----------+------+
 *               ^            ^     |      |    ^  |    ^           |
 *               |            '------------'    |  |    '---- ....  |
 *               |                  |           |  |                |
 *               '------------------------------|--|----------------'
 *                                              |  v
 *                                            +------+
 *                                            | data | (child data)
 *                                            +------+
 *
 * A map uses `msgpackx_map_node` at data.
 *
 *                                        .-----> (to tree's root) ------.
 *                                        |                              |
 *           (parent data)                |     .--> (not in member      |
 *         ----+------+----               |     |     of the tree)       |
 *             | head |                   |     |                        |
 *         ----+------+----               |     |         .------> ....  |
 *               ^  |                     |     |         |  .---- ....  |
 *               |  v                     |     |         |  v           |
 *           +----------+------------+--------+--------+--------+        |
 *       map | key node | value node |  root  |  tree  |  list  |        |
 *           +----------+------------+--------+--------+--------+        |
 *                          |                   ^         ^  |           |
 *       (not connected) <--'                   |         |  |           |
 *                            (points head, .---'    .---------- .... ---'
 *                                not root) |        v    |  v
 *           +----------+------------+--------+--------+--------+
 *       map | key node | value node |  root  |  tree  |  list  |
 *           +----------+------------+--------+--------+--------+
 *               ^  |        |  ^               ^    ^    ^  |
 *               |  |        |  |               |    |    |  '---> ....
 *          .----'  |        |  |               |    |    '------- ....
 *          |  .----'        |  |               |    |  (keep insertion order)
 *          |  |             |  |               |    |
 *          |  v             v  |       .... <--'    '--> ....
 *      +----------+    +------------+
 *      | key data |    | value data | (child data)
 *      +----------+    +------------+
 *
 * Other types does not use extended struct type.
 *
 * Therefore, node_type of parents can be MSGPACKX_NODE_ROOT,
 * MSGPACKX_NODE_ARRAY, MSGPACKX_NODE_MAP_KEY, or MSGPACKX_NODE_MAP_VALUE.
 *
 * And, node_type of children can be MSGPACKX_NODE_SINGLE,
 * MSGPACKX_NODE_ARRAY (array head), or MSGPACKX_NODE_MAP_KEY (map head).
 *
 * Other combinations will be consistency error.
 */
struct msgpackx_node
{
  struct geom_list list;
  msgpackx_buffer *pointer; /* NULL is head. */
  enum msgpackx_node_type {
    MSGPACKX_NODE_SINGLE, MSGPACKX_NODE_ARRAY,
    MSGPACKX_NODE_MAP_KEY, MSGPACKX_NODE_MAP_VALUE,
    MSGPACKX_NODE_ROOT,
  } node_type;
};

#define msgpackx_node_list_entry(ptr)              \
  geom_list_entry(ptr, struct msgpackx_node, list)

#define msgpackx_node_entry(ptr, type, member) \
  geom_container_of(ptr, type, member)

/**
 * @ingroup Serializer
 * @brief Array node
 *
 * Stores single array head or arry node entry data.
 *
 * See ::msgpackx_node for detail.
 */
struct msgpackx_array_node
{
  struct msgpackx_node node;
  struct geom_list list;
  struct msgpackx_array_node *head;
};

#define msgpackx_array_node_entry(ptr)                          \
  msgpackx_node_entry(ptr, struct msgpackx_array_node, node)

#define msgpackx_array_list_entry(ptr)                      \
  geom_list_entry(ptr, struct msgpackx_array_node, list)

/**
 * @ingroup Serializer
 * @brief Map node
 *
 * Stores single map head or map node entry data.
 *
 * See ::msgpackx_node for detail.
 */
struct msgpackx_map_node
{
  struct msgpackx_node key_node;
  struct msgpackx_node value_node;
  struct geom_rbtree map_tree;
  struct geom_rbtree *root;
  struct geom_list insert_list;
};

#define msgpackx_map_key_node_entry(ptr)                        \
  msgpackx_node_entry(ptr, struct msgpackx_map_node, key_node)

#define msgpackx_map_value_node_entry(ptr)                          \
  msgpackx_node_entry(ptr, struct msgpackx_map_node, value_node)

#define msgpackx_map_tree_entry(ptr)                            \
  geom_rbtree_entry(ptr, struct msgpackx_map_node, map_tree)

#define msgpackx_map_list_entry(ptr)                            \
  geom_list_entry(ptr, struct msgpackx_map_node, insert_list)

/**
 * @ingroup Serializer
 * @brief Root node
 *
 * See ::msgpackx_node for detail.
 */
struct msgpackx_data
{
  struct msgpackx_node head;
};
#define msgpackx_data_node_entry(ptr) \
  msgpackx_node_entry(ptr, struct msgpackx_data, head)

static msgpackx_node *
msgpackx_node_do_pack_child_recursion(msgpackx_node *chld,
                                      msgpackx_buffer *buf,
                                      msgpackx_error *err);

static msgpackx_node *
msgpackx_node_do_pack(msgpackx_node *node, msgpackx_buffer *base,
                      msgpackx_error *err);

static msgpackx_array_node *
msgpackx_array_node_do_pack(msgpackx_array_node *node, msgpackx_buffer *base,
                            msgpackx_error *err);

static msgpackx_map_node *
msgpackx_map_node_do_pack(msgpackx_map_node *node, msgpackx_buffer *base,
                          msgpackx_error *err);

static uintmax_t
msgpackx_node_read_uint(msgpackx_buffer *buf, ptrdiff_t off,
                        ptrdiff_t size, msgpackx_error *err)
{
  const char *l;
  uint8_t  u8;
  uint16_t u16;
  uint32_t u32;
  uint64_t u64;

  l = msgpackx_buffer_pointer(buf);
  MSGPACKX_ASSERT(l);

  if (off + size > msgpackx_buffer_size(buf)) {
    if (err) *err = MSGPACKX_ERR_MSG_INCOMPLETE;
    return (uintmax_t)-1;
  }

  switch(size) {
  case 1:
    memcpy(&u8, l + off, size);
    return u8;
  case 2:
    memcpy(&u16, l + off, size);
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u16 = msgpackx_byteswap_2(u16);
#endif
    return u16;
  case 4:
    memcpy(&u32, l + off, size);
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u32 = msgpackx_byteswap_4(u32);
#endif
    return u32;
  case 8:
    memcpy(&u64, l + off, size);
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u64 = msgpackx_byteswap_8(u64);
#endif
    return u64;
  default:
    MSGPACKX_UNREACHABLE();
    return (uint64_t)-1;
  }
}

static ptrdiff_t
msgpackx_node_size_used(struct msgpackx_node *node)
{
  MSGPACKX_ASSERT(node);

  if (!node->pointer) return 0;
  return msgpackx_buffer_size(node->pointer);
}

static void
msgpackx_node_init(struct msgpackx_node *node)
{
  MSGPACKX_ASSERT(node);

  geom_list_init(&node->list);
  node->node_type = MSGPACKX_NODE_SINGLE;
  node->pointer = NULL;
}

msgpackx_data *
msgpackx_data_new(void)
{
  msgpackx_data *p;
  p = (msgpackx_data *)malloc(sizeof(msgpackx_data));
  if (!p) return NULL;

  msgpackx_node_init(&p->head);
  p->head.node_type = MSGPACKX_NODE_ROOT;
  return p;
}

static int
msgpackx_node_is_parent(struct msgpackx_node *node)
{
  MSGPACKX_ASSERT(node);

  return (node->pointer == NULL);
}

static msgpackx_node *
msgpackx_node_parent(msgpackx_node *node)
{
  struct geom_list *p;

  MSGPACKX_ASSERT(node);

  p = NULL;
  if (!msgpackx_node_is_parent(node)) {
    p = geom_list_next(&node->list);
  }
  if (p && p != &node->list) {
    return msgpackx_node_list_entry(p);
  }
  return NULL;
}

static msgpackx_node *
msgpackx_node_child(msgpackx_node *node)
{
  struct geom_list *p;

  MSGPACKX_ASSERT(node);

  p = NULL;
  if (msgpackx_node_is_parent(node)) {
    p = geom_list_next(&node->list);
  }
  if (p && p != &node->list) {
    return msgpackx_node_list_entry(p);
  }
  return NULL;
}

msgpackx_node *msgpackx_node_get_parent(msgpackx_node *node)
{
  msgpackx_array_node *anode;
  msgpackx_map_node *mnode;

  MSGPACKX_ASSERT(node);

  anode = msgpackx_node_get_array(node);
  if (anode)
    return msgpackx_array_node_get_parent(anode);

  mnode = msgpackx_node_get_map(node);
  if (mnode)
    return msgpackx_map_node_get_parent(mnode);

  return msgpackx_node_parent(node);
}

int msgpackx_node_is_root(msgpackx_node *node)
{
  return node->node_type == MSGPACKX_NODE_ROOT;
}

msgpackx_data *msgpackx_node_get_data(msgpackx_node *node)
{
  MSGPACKX_ASSERT(node);

  if (msgpackx_node_is_root(node))
    return msgpackx_data_node_entry(node);

  node = msgpackx_node_get_parent(node);
  if (!node)
    return NULL;

  return msgpackx_node_get_data(node);
}

static msgpackx_node *
msgpackx_node_set_child(msgpackx_node *node, msgpackx_node *child)
{
  msgpackx_node *old_chld;

  MSGPACKX_ASSERT(node);
  MSGPACKX_ASSERT(child);

  old_chld = msgpackx_node_child(node);
  if (old_chld) {
    msgpackx_node_delete(old_chld);
  }
  geom_list_insert_prev(&node->list, &child->list);
  return child;
}

static void
msgpackx_node_unlink(msgpackx_node *n)
{
  MSGPACKX_ASSERT(n);

  geom_list_delete(&n->list);
}

static void
msgpackx_node_nullify(struct msgpackx_node *n)
{
  MSGPACKX_ASSERT(n);

  msgpackx_buffer_delete(n->pointer);
  n->pointer = NULL;
}

static void
msgpackx_node_clean_for_delete(struct msgpackx_node *n)
{
  MSGPACKX_ASSERT(n);

  msgpackx_node_unlink(n);
  msgpackx_node_nullify(n);
}

void msgpackx_array_node_delete_all(msgpackx_array_node *head)
{
  struct geom_list *lp, *ln, *lh;

  MSGPACKX_ASSERT(head);

  if (!msgpackx_array_node_is_head_of_array(head)) {
    do {
      head = msgpackx_array_node_next(head);
    } while (!msgpackx_array_node_is_head_of_array(head));
  }

  lh = &head->list;
  geom_list_foreach_safe(lp, ln, lh) {
    struct msgpackx_array_node *an;

    an = msgpackx_array_list_entry(lp);
    msgpackx_array_node_delete(an);
  }

  msgpackx_node_clean_for_delete(&head->node);
  free(head);
}

void msgpackx_map_node_delete_all(msgpackx_map_node *head)
{
  struct geom_list *lp, *ln;
  msgpackx_map_node *ahead;

  MSGPACKX_ASSERT(head);

  ahead = msgpackx_map_node_get_head(head);

  if (ahead) {
    geom_list_foreach_safe(lp, ln, &ahead->insert_list) {
      struct msgpackx_map_node *mp;

      mp = msgpackx_map_list_entry(lp);
      mp->root = NULL;

      msgpackx_map_node_delete(mp);
    }
  } else {
    ahead = head;
  }

  msgpackx_node_clean_for_delete(&ahead->key_node);
  msgpackx_node_clean_for_delete(&ahead->value_node);
  free(ahead);
}

void
msgpackx_data_delete(msgpackx_data *data)
{
  msgpackx_node *chld;
  chld = msgpackx_node_child(&data->head);
  if (chld) {
    msgpackx_node_delete(chld);
  }
  free(data);
}

static msgpackx_node *
msgpackx_array_node_parse(msgpackx_buffer *buffer, ptrdiff_t nelement,
                          ptrdiff_t start,
                          msgpackx_error *err, const char **eloc);

static msgpackx_node *
msgpackx_map_node_parse(msgpackx_buffer *buffer, ptrdiff_t nelement,
                        ptrdiff_t start,
                        msgpackx_error *err, const char **eloc);

static msgpackx_node *
msgpackx_node_parse(msgpackx_buffer *buffer,
                    msgpackx_error *err, const char **eloc)
{
  uint8_t u8;
  msgpackx_node *node;
  msgpackx_buffer *p;
  ptrdiff_t sz;
  ptrdiff_t noff;
  ptrdiff_t rsz;

  node = NULL;
  p = NULL;

  if (msgpackx_buffer_size(buffer) < 1) {
    if (err) *err = MSGPACKX_ERR_MSG_TYPE;
    goto error;
  }

  sz = -1;
  u8 = *(uint8_t *)msgpackx_buffer_pointer(buffer);
  if (u8 >= MSGPACKX_FIXARRAY_MIN && u8 <= MSGPACKX_FIXARRAY_MAX) {
    sz = u8 - MSGPACKX_FIXARRAY_MIN;
    return msgpackx_array_node_parse(buffer, sz, 1, err, eloc);
  }
  if (u8 >= MSGPACKX_FIXMAP_MIN && u8 <= MSGPACKX_FIXMAP_MAX) {
    sz = u8 - MSGPACKX_FIXMAP_MIN;
    return msgpackx_map_node_parse(buffer, sz, 1, err, eloc);
  }
  if ((u8 >= MSGPACKX_POSITIVE_FIXINT_MIN &&
       u8 <= MSGPACKX_POSITIVE_FIXINT_MAX) ||
      (u8 >= MSGPACKX_NEGATIVE_FIXINT_MIN &&
       u8 <= MSGPACKX_NEGATIVE_FIXINT_MAX)) {
    sz = 0;
  } else if (u8 >= MSGPACKX_FIXSTR_MIN && u8 <= MSGPACKX_FIXSTR_MAX) {
    sz = u8 - MSGPACKX_FIXSTR_MIN;
  } else {
    switch(u8) {
    case MSGPACKX_ARRAY16:
      sz = msgpackx_node_read_uint(buffer, 1, 2, err);
      noff = 3;
      goto array;
    case MSGPACKX_ARRAY32:
      sz = msgpackx_node_read_uint(buffer, 1, 4, err);
      noff = 5;
      goto array;
    case MSGPACKX_ARRAY64:
      sz = msgpackx_node_read_uint(buffer, 1, 8, err);
      noff = 9;
    array:
      if (sz < 0) goto error;
      return msgpackx_array_node_parse(buffer, sz, noff, err, eloc);

    case MSGPACKX_MAP16:
      sz = msgpackx_node_read_uint(buffer, 1, 2, err);
      noff = 3;
      goto map;
    case MSGPACKX_MAP32:
      sz = msgpackx_node_read_uint(buffer, 1, 4, err);
      noff = 5;
      goto map;
    case MSGPACKX_MAP64:
      sz = msgpackx_node_read_uint(buffer, 1, 8, err);
      noff = 9;
    map:
      if (sz < 0) goto error;
      return msgpackx_map_node_parse(buffer, sz, noff, err, eloc);

    case MSGPACKX_STR8:
      sz = 1;
      noff = 1;
      goto str_bin_ext;

    case MSGPACKX_STR16:
    case MSGPACKX_BIN16:
      sz = 2;
      noff = 1;
      goto str_bin_ext;

    case MSGPACKX_STR32:
    case MSGPACKX_BIN32:
      sz = 4;
      noff = 1;
      goto str_bin_ext;

    case MSGPACKX_BIN64:
      sz = 8;
      noff = 1;
      goto str_bin_ext;

    case MSGPACKX_EXT8:
      sz = 1;
      noff = 2;
      goto str_bin_ext;

    case MSGPACKX_EXT16:
      sz = 2;
      noff = 2;
      goto str_bin_ext;

    case MSGPACKX_EXT32:
      sz = 4;
      noff = 2;
      goto str_bin_ext;

    str_bin_ext:
      rsz = msgpackx_node_read_uint(buffer, noff, sz, err);
      if (rsz < 0) goto error;
      sz = noff + sz + rsz - 1;
      break;

    case MSGPACKX_NIL:
    case MSGPACKX_FALSE:
    case MSGPACKX_TRUE:
      sz = 0;
      break;
    case MSGPACKX_INT8:
    case MSGPACKX_UINT8:
      sz = 1;
      break;
    case MSGPACKX_INT16:
    case MSGPACKX_UINT16:
      sz = 2;
      break;
    case MSGPACKX_INT32:
    case MSGPACKX_UINT32:
      sz = 4;
      break;
    case MSGPACKX_INT64:
    case MSGPACKX_UINT64:
      sz = 8;
      break;
    case MSGPACKX_FLOAT32:
      sz = sizeof(float);
      break;
    case MSGPACKX_FLOAT64:
      sz = sizeof(double);
      break;
    }
  }

  if (sz >= 0) {
    sz += 1;
    node = msgpackx_node_new();
    p = msgpackx_buffer_substr(buffer, 0, sz, MSGPACKX_SEEK_CUR);
    if (!node || !p) {
      if (err) *err = MSGPACKX_ERR_NOMEM;
      goto error;
    }
    if (msgpackx_buffer_size(p) != sz) {
      if (err) *err = MSGPACKX_ERR_MSG_INCOMPLETE;
      goto error;
    }
    node->pointer = p;
    return node;
  }

  if (err) *err = MSGPACKX_ERR_MSG_TYPE;
  goto error;

error:
  if (node) msgpackx_node_delete(node);
  if (p) msgpackx_buffer_delete(p);
  if (eloc) *eloc = msgpackx_buffer_pointer(buffer);
  return NULL;
}

msgpackx_data *
msgpackx_data_parse(msgpackx_buffer *buffer,
                    msgpackx_error *err, ptrdiff_t *eloc)
{
  msgpackx_data *data;
  msgpackx_node *node;
  const char *base;
  const char *px;

  data = msgpackx_data_new();
  if (!data) {
    if (err) *err = MSGPACKX_ERR_NOMEM;
    if (eloc) *eloc = 0;
    return NULL;
  }

  if (eloc) base = msgpackx_buffer_pointer(buffer);
  node = msgpackx_node_parse(buffer, err, &px);
  if (!node) {
    msgpackx_data_delete(data);
    if (eloc) {
      *eloc = px - base;
    }
    return NULL;
  }

  msgpackx_node_set_child(&data->head, node);
  return data;
}

msgpackx_data *
msgpackx_data_read(FILE *fp, msgpackx_error *err, ptrdiff_t *eloc)
{
  msgpackx_data *data;
  size_t rdsz;
  size_t rssz;
  msgpackx_buffer *buffer;

  rssz = 128;
  buffer = msgpackx_buffer_new();
  if (buffer) {
    if (!msgpackx_buffer_resize(buffer, rssz)) {
      msgpackx_buffer_delete(buffer);
      buffer = NULL;
    }
  }
  if (!buffer) {
    if (err) *err = MSGPACKX_ERR_NOMEM;
    return NULL;
  }

  rdsz = fread(msgpackx_buffer_pointer(buffer), 1, rssz, fp);
  while (!feof(fp) && !ferror(fp)) {
    msgpackx_buffer_goto(buffer, rdsz, -1, MSGPACKX_SEEK_SET);
    msgpackx_buffer_resize_substr(buffer, rssz);
    rdsz += fread(msgpackx_buffer_pointer(buffer), 1, rssz, fp);
  }
  if (ferror(fp)) {
    if (err) *err = MSGPACKX_ERR_SYS;
    msgpackx_buffer_delete(buffer);
    return NULL;
  }

  msgpackx_buffer_goto(buffer, 0, rdsz, MSGPACKX_SEEK_SET);
  data = msgpackx_data_parse(buffer, err, eloc);
  msgpackx_buffer_delete(buffer);
  return data;
}

static ptrdiff_t
msgpackx_node_write_or_append(msgpackx_buffer **bufp, msgpackx_buffer *write,
                              FILE *stream)
{
  ptrdiff_t r, sz, wsz;
  const char *p, *endp, *wp;

  MSGPACKX_ASSERT(bufp);
  MSGPACKX_ASSERT(*bufp);

  r = 0;
  endp = NULL;
  p = msgpackx_buffer_pointer(*bufp);
  if (p) endp = p + msgpackx_buffer_size(*bufp);

  wp = NULL;
  if (write) {
    wp = msgpackx_buffer_pointer(write);
    wsz = msgpackx_buffer_size(write);
  }

  if (endp && endp == wp) {
    endp = wp + wsz;
    msgpackx_buffer_goto(*bufp, 0, endp - p, MSGPACKX_SEEK_CUR);
  } else {
    if (endp) {
      sz = endp - p;
      r = fwrite(p, sizeof(char), sz, stream);
      if (r != sz) {
        return -1;
      }
    }
    if (write) {
      msgpackx_buffer_relocate(*bufp, write, 0, MSGPACKX_SEEK_CUR);
      msgpackx_buffer_goto(*bufp, 0, wsz, MSGPACKX_SEEK_CUR);
    }
  }
  return r;
}

static ptrdiff_t
msgpackx_array_node_write(msgpackx_array_node *arr, msgpackx_buffer **bufp,
                          FILE *stream);

static ptrdiff_t
msgpackx_map_node_write(msgpackx_map_node *map, msgpackx_buffer **bufp,
                        FILE *stream);

static ptrdiff_t
msgpackx_node_write(msgpackx_node *node, msgpackx_buffer **bufp,
                    FILE *stream)
{
  ptrdiff_t r;
  MSGPACKX_ASSERT(node);

  r = -1;
  switch(node->node_type) {
  case MSGPACKX_NODE_ARRAY:
    {
      msgpackx_array_node *an;
      an = msgpackx_node_get_array(node);
      r = msgpackx_array_node_write(an, bufp, stream);
    }
    break;
  case MSGPACKX_NODE_MAP_KEY:
  case MSGPACKX_NODE_MAP_VALUE:
    {
      msgpackx_map_node *mn;
      mn = msgpackx_node_get_map(node);
      r = msgpackx_map_node_write(mn, bufp, stream);
    }
    break;
  case MSGPACKX_NODE_SINGLE:
    r = msgpackx_node_write_or_append(bufp, node->pointer, stream);
    break;
  default:
    MSGPACKX_UNREACHABLE();
    break;
  }
  return r;
}

ptrdiff_t
msgpackx_data_write(msgpackx_data *data, FILE *stream)
{
  msgpackx_buffer *buf;
  msgpackx_node *chld;
  ptrdiff_t r, rr;

  chld = msgpackx_data_root_node(data);
  if (!chld) return 0;

  buf = msgpackx_buffer_new();
  if (!buf) {
#ifdef ENOMEM
    errno = ENOMEM;
#endif
    return -1;
  }

  r = msgpackx_node_write(chld, &buf, stream);
  if (r < 0) goto clean;

  rr = msgpackx_node_write_or_append(&buf, NULL, stream);
  if (rr < 0) { r = rr; goto clean; }
  r += rr;

  fflush(stream);

clean:
  msgpackx_buffer_delete(buf);
  return r;
}

ptrdiff_t
msgpackx_data_write_packed(msgpackx_data *data, FILE *stream)
{
  const char *p;
  ptrdiff_t sz;
  ptrdiff_t r;
  msgpackx_node *chld;

  chld = msgpackx_data_root_node(data);
  if (!chld) return 0;

  p = msgpackx_node_get_data_pointer(chld, &sz);
  r = sz;
  if (sz > 0) {
    r = fwrite(p, sizeof(char), sz, stream);
    if (r != sz) {
      r = -1;
    }
  }
  return r;
}

msgpackx_node *
msgpackx_node_new(void)
{
  msgpackx_node *n;

  n = (msgpackx_node *)malloc(sizeof(struct msgpackx_node));
  if (!n) return NULL;

  msgpackx_node_init(n);
  n->pointer = NULL;
  return n;
}

static msgpackx_node *
msgpackx_node_set_data_concat(msgpackx_node *node,
                              const void *data1, ptrdiff_t size1,
                              const void *data2, ptrdiff_t size2,
                              msgpackx_error *err)
{
  ptrdiff_t totsz;

  MSGPACKX_ASSERT(node);
  MSGPACKX_ASSERT(data1);
  MSGPACKX_ASSERT(size1 >= 0);

  if (data2) {
    MSGPACKX_ASSERT(size2 >= 0);
  }

  totsz = size1;
  if (data2) totsz += size2;

  if (!node->pointer) {
    node->pointer = msgpackx_buffer_new();
    if (!node->pointer) {
      if (err) *err = MSGPACKX_ERR_NOMEM;
      return NULL;
    }
    if (!msgpackx_buffer_reserve(node->pointer, totsz)) {
      msgpackx_buffer_delete(node->pointer);
      node->pointer = NULL;
      if (err) *err = MSGPACKX_ERR_NOMEM;
      return NULL;
    }
    msgpackx_buffer_goto(node->pointer, 0, 0, MSGPACKX_SEEK_SET);
  }
  if (!msgpackx_buffer_resize_substr(node->pointer, totsz)) {
    if (err) *err = MSGPACKX_ERR_NOMEM;
    return NULL;
  }
  msgpackx_buffer_raw_copy(node->pointer, data1, size1, 0);
  if (data2) {
    msgpackx_buffer_goto(node->pointer, size1, size2, MSGPACKX_SEEK_CUR);
    msgpackx_buffer_raw_copy(node->pointer, data2, size2, 0);
    msgpackx_buffer_goto(node->pointer, -size1, totsz, MSGPACKX_SEEK_CUR);
  }
  return node;
}

static msgpackx_node *
msgpackx_node_set_data(msgpackx_node *node, void *data, ptrdiff_t size,
                       msgpackx_error *err)
{
  return msgpackx_node_set_data_concat(node, data, size, NULL, 0, err);
}

msgpackx_node *
msgpackx_node_set_int(msgpackx_node *node, intmax_t value,
                      msgpackx_error *err)
{
  char buf[sizeof(uint8_t) + sizeof(int64_t)];
  ptrdiff_t sz;
  uint8_t key;
  int8_t  s8;
  int16_t s16;
  int32_t s32;
  int64_t s64;

  if (value >= 0) {
    if (value <= 127) {
      key = (uint8_t)(value);
    } else if (INTMAX_MAX <= INT8_MAX || value <= INT8_MAX) {
      key = MSGPACKX_INT8;
      s8 = (int8_t)value;
    } else if (INTMAX_MAX <= INT16_MAX || value <= INT16_MAX) {
      key = MSGPACKX_INT16;
      s16 = (int16_t)value;
    } else if (INTMAX_MAX <= INT32_MAX || value <= INT32_MAX) {
      key = MSGPACKX_INT32;
      s32 = (int32_t)value;
    } else if (INTMAX_MAX <= INT64_MAX || value <= INT64_MAX) {
      key = MSGPACKX_INT64;
      s64 = (int64_t)value;
    } else {
      if (err) *err = MSGPACKX_ERR_RANGE;
      return NULL;
    }
  } else {
    if (value >= -32) {
      key = (uint8_t)(value + 32 + MSGPACKX_NEGATIVE_FIXINT_MIN);
    } else if (INTMAX_MIN >= INT8_MIN || value >= INT8_MIN) {
      key = MSGPACKX_INT8;
      s8 = (int8_t)value;
    } else if (INTMAX_MIN >= INT16_MIN || value >= INT16_MIN) {
      key = MSGPACKX_INT16;
      s16 = (int16_t)value;
    } else if (INTMAX_MIN >= INT32_MIN || value >= INT32_MIN) {
      key = MSGPACKX_INT32;
      s32 = (int32_t)value;
    } else if (INTMAX_MIN >= INT64_MIN || value >= INT64_MIN) {
      key = MSGPACKX_INT64;
      s64 = (int64_t)value;
    } else {
      if (err) *err = MSGPACKX_ERR_RANGE;
      return NULL;
    }
  }

  sz = 0;
  memcpy(&buf[sz], &key, sizeof(uint8_t));

  sz += sizeof(uint8_t);

  switch(key) {
  case MSGPACKX_INT8:
    memcpy(&buf[sz], &s8, sizeof(int8_t));
    sz += sizeof(int8_t);
    break;

  case MSGPACKX_INT16:
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    msgpackx_byteswap_type(&s16, sizeof(int16_t));
#endif
    memcpy(&buf[sz], &s16, sizeof(int16_t));
    sz += sizeof(int16_t);
    break;

  case MSGPACKX_INT32:
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    msgpackx_byteswap_type(&s32, sizeof(int32_t));
#endif
    memcpy(&buf[sz], &s32, sizeof(int32_t));
    sz += sizeof(int32_t);
    break;

  case MSGPACKX_INT64:
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    msgpackx_byteswap_type(&s64, sizeof(int64_t));
#endif
    memcpy(&buf[sz], &s64, sizeof(int64_t));
    sz += sizeof(int64_t);
    break;

  default:
    /* nop */
    break;
  }

  return msgpackx_node_set_data(node, buf, sz, err);
}

msgpackx_node *
msgpackx_node_set_uint(msgpackx_node *node, uintmax_t value,
                       msgpackx_error *err)
{
  char buf[sizeof(uint8_t) + sizeof(uint64_t)];
  ptrdiff_t sz;
  uint8_t  key;
  uint8_t  u8;
  uint16_t u16;
  uint32_t u32;
  uint64_t u64;

  if (value <= 127) {
    key = (uint8_t)(value);
  } else if (UINTMAX_MAX <= UINT8_MAX || value <= UINT8_MAX) {
    key = MSGPACKX_UINT8;
    u8 = (uint8_t)value;
  } else if (UINTMAX_MAX <= UINT16_MAX || value <= UINT16_MAX) {
    key = MSGPACKX_UINT16;
    u16 = (uint16_t)value;
  } else if (UINTMAX_MAX <= UINT32_MAX || value <= UINT32_MAX) {
    key = MSGPACKX_UINT32;
    u32 = (uint32_t)value;
  } else if (UINTMAX_MAX <= UINT64_MAX || value <= UINT64_MAX) {
    key = MSGPACKX_UINT64;
    u64 = (uint64_t)value;
  } else {
    if (err) *err = MSGPACKX_ERR_NOMEM;
    return NULL;
  }

  sz = 0;
  memcpy(&buf[sz], &key, sizeof(uint8_t));

  sz += sizeof(uint8_t);

  switch(key) {
  case MSGPACKX_UINT8:
    memcpy(&buf[sz], &u8, sizeof(uint8_t));
    sz += sizeof(uint8_t);
    break;

  case MSGPACKX_UINT16:
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u16 = msgpackx_byteswap_2(u16);
#endif
    memcpy(&buf[sz], &u16, sizeof(uint16_t));
    sz += sizeof(uint16_t);
    break;

  case MSGPACKX_UINT32:
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u32 = msgpackx_byteswap_4(u32);
#endif
    memcpy(&buf[sz], &u32, sizeof(uint32_t));
    sz += sizeof(uint32_t);
    break;

  case MSGPACKX_UINT64:
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u64 = msgpackx_byteswap_8(u64);
#endif
    memcpy(&buf[sz], &u64, sizeof(uint64_t));
    sz += sizeof(uint64_t);
    break;

  default:
    /* nop */
    break;
  }

  return msgpackx_node_set_data(node, buf, sz, err);
}

msgpackx_node *
msgpackx_node_set_str(msgpackx_node *node, const char *str, ptrdiff_t len,
                      msgpackx_error *err)
{
  uint8_t  buf[sizeof(uint8_t) + sizeof(uint32_t)];
  uint8_t  u8;
  uint16_t u16;
  uint32_t u32;
  ptrdiff_t shead;

  if (len < 0) {
    len = strlen(str);
  }
  if (len <= 31) {
    buf[0] = MSGPACKX_FIXSTR_MIN + len;
    shead = 1;
  } else if (len <= UINT8_MAX) {
    buf[0] = MSGPACKX_STR8;
    u8 = (uint8_t)len;
    memcpy(&buf[1], &u8, sizeof(uint8_t));
    shead = 1 + sizeof(uint8_t);
  } else if (len <= MSGPACKX_UINT16_MAX) {
    buf[0] = MSGPACKX_STR16;
    u16 = (uint16_t)len;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u16 = msgpackx_byteswap_2(u16);
#endif
    memcpy(&buf[1], &u16, sizeof(uint16_t));
    shead = 1 + sizeof(uint16_t);
  } else if (len <= MSGPACKX_UINT32_MAX) {
    buf[0] = MSGPACKX_STR32;
    u32 = (uint32_t)len;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u32 = msgpackx_byteswap_4(u32);
#endif
    memcpy(&buf[1], &u32, sizeof(uint32_t));
    shead = 1 + sizeof(uint32_t);
  } else {
    if (err) *err = MSGPACKX_ERR_RANGE;
    return NULL;
  }

  return msgpackx_node_set_data_concat(node, buf, shead, str, len, err);
}

msgpackx_node *
msgpackx_node_set_bin(msgpackx_node *node, const void *data, ptrdiff_t size,
                      msgpackx_error *err)
{
  uint8_t  buf[sizeof(uint8_t) + sizeof(uint64_t)];
  uint16_t u16;
  uint32_t u32;
  uint64_t u64;
  ptrdiff_t shead;
  size_t usz;

  MSGPACKX_ASSERT(size >= 0);
  usz = size;

  if (usz <= MSGPACKX_UINT16_MAX) {
    buf[0] = MSGPACKX_BIN16;
    u16 = (uint16_t)size;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u16 = msgpackx_byteswap_2(u16);
#endif
    memcpy(&buf[1], &u16, sizeof(uint16_t));
    shead = 1 + sizeof(uint16_t);
  } else if (usz <= MSGPACKX_UINT32_MAX) {
    buf[0] = MSGPACKX_BIN32;
    u32 = (uint32_t)size;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u32 = msgpackx_byteswap_4(u32);
#endif
    memcpy(&buf[1], &u32, sizeof(uint32_t));
    shead = 1 + sizeof(uint32_t);
  } else if (usz <= MSGPACKX_UINT64_MAX) {
    buf[0] = MSGPACKX_BIN64;
    u64 = (uint64_t)size;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u64 = msgpackx_byteswap_8(u64);
#endif
    memcpy(&buf[1], &u64, sizeof(uint64_t));
    shead = 1 + sizeof(uint64_t);
  } else {
    if (err) *err = MSGPACKX_ERR_RANGE;
    return NULL;
  }

  return msgpackx_node_set_data_concat(node, buf, shead, data, size, err);
}

msgpackx_node *
msgpackx_node_set_ext(msgpackx_node *node, int type, void *data,
                      ptrdiff_t size, msgpackx_error *err)
{
  uint8_t  buf[sizeof(uint8_t) + sizeof(int8_t) + sizeof(uint32_t)];
  uint8_t  u8;
  uint16_t u16;
  uint32_t u32;
  ptrdiff_t shead;

  MSGPACKX_ASSERT(INT8_MIN <= type && type <= INT8_MAX);
  MSGPACKX_ASSERT(size >= 0);

  buf[1] = (int8_t)type;
  if (size <= UINT8_MAX) {
    buf[0] = MSGPACKX_EXT8;
    u8 = (uint8_t)size;
    memcpy(&buf[2], &u8, sizeof(uint8_t));
    shead = 2 + sizeof(uint8_t);
  } else if (size <= UINT16_MAX) {
    buf[0] = MSGPACKX_EXT16;
    u16 = (uint16_t)size;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u16 = msgpackx_byteswap_2(u16);
#endif
    memcpy(&buf[2], &u16, sizeof(uint16_t));
    shead = 2 + sizeof(uint16_t);
  } else if (size <= UINT32_MAX) {
    buf[0] = MSGPACKX_EXT32;
    u32 = (uint32_t)size;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u32 = msgpackx_byteswap_4(u32);
#endif
    memcpy(&buf[2], &u32, sizeof(uint32_t));
    shead = 2 + sizeof(uint32_t);
  } else {
    if (err) *err = MSGPACKX_ERR_RANGE;
    return NULL;
  }

  return msgpackx_node_set_data_concat(node, buf, shead, data, size, err);
}


msgpackx_node *
msgpackx_node_set_nil(msgpackx_node *node, msgpackx_error *err)
{
  uint8_t u8;
  u8 = MSGPACKX_NIL;
  return msgpackx_node_set_data(node, &u8, sizeof(uint8_t), err);
}

msgpackx_node *
msgpackx_node_set_bool(msgpackx_node *node, int value, msgpackx_error *err)
{
  uint8_t u8;
  if (value) {
    u8 = MSGPACKX_TRUE;
  } else {
    u8 = MSGPACKX_FALSE;
  }
  return msgpackx_node_set_data(node, &u8, sizeof(uint8_t), err);
}

msgpackx_node *
msgpackx_node_set_float(msgpackx_node *node, float flt, msgpackx_error *err)
{
  enum { szbuf = sizeof(uint8_t) + sizeof(float) };
  uint8_t buf[(szbuf == sizeof(uint8_t) + 4) ? szbuf : -1];

  buf[0] = MSGPACKX_FLOAT32;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
  msgpackx_byteswap_type(&flt, sizeof(float));
#endif
  memcpy(&buf[1], &flt, sizeof(float));
  return msgpackx_node_set_data(node, buf, sizeof(buf), err);
}

msgpackx_node *
msgpackx_node_set_double(msgpackx_node *node, double dbl, msgpackx_error *err)
{
  enum { szbuf = sizeof(uint8_t) + sizeof(double) };
  uint8_t buf[(szbuf == sizeof(uint8_t) + 8) ? szbuf : -1];

  buf[0] = MSGPACKX_FLOAT64;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
  msgpackx_byteswap_type(&dbl, sizeof(double));
#endif
  memcpy(&buf[1], &dbl, sizeof(double));
  return msgpackx_node_set_data(node, buf, sizeof(buf), err);
}

const void *
msgpackx_node_get_data_pointer(msgpackx_node *node, ptrdiff_t *size_out)
{
  MSGPACKX_ASSERT(node);

  if (!node->pointer) return NULL;

  if (size_out) {
    *size_out = msgpackx_buffer_size(node->pointer);
  }
  return msgpackx_buffer_pointer(node->pointer);
}

msgpackx_buffer *
msgpackx_node_get_data_copy(msgpackx_node *node)
{
  MSGPACKX_ASSERT(node);

  if (!node->pointer) return NULL;

  return msgpackx_buffer_dup(node->pointer, MSGPACKX_DUP_SUBSTR);
}

void
msgpackx_node_delete(struct msgpackx_node *node)
{
  switch(node->node_type) {
  case MSGPACKX_NODE_ARRAY:
    {
      msgpackx_array_node *n;
      n = msgpackx_node_get_array(node);
      msgpackx_array_node_delete_all(n);
    }
    break;
  case MSGPACKX_NODE_MAP_KEY:
    {
      msgpackx_map_node *n;
      n = msgpackx_node_get_map(node);
      msgpackx_map_node_delete_all(n);
    }
    break;
  case MSGPACKX_NODE_MAP_VALUE:
    {
      msgpackx_map_node *n;
      n = msgpackx_node_get_map(node);
      msgpackx_map_node_delete_all(n);
    }
    break;
  default:
    msgpackx_node_clean_for_delete(node);
    free(node);
  }
}

msgpackx_node *
msgpackx_data_root_node(msgpackx_data *data)
{
  return msgpackx_node_child(&data->head);
}

void
msgpackx_data_set_root_node(msgpackx_data *data, msgpackx_node *node)
{
  MSGPACKX_ASSERT(data);

  msgpackx_node_set_child(&data->head, node);
}

msgpackx_data *
msgpackx_data_pack(msgpackx_data *data, msgpackx_error *err)
{
  msgpackx_buffer *buf;
  msgpackx_node *chld;

  MSGPACKX_ASSERT(data);

  chld = msgpackx_node_child(&data->head);
  if (chld) {
    buf = msgpackx_buffer_new();
    if (!buf) {
      if (err) *err = MSGPACKX_ERR_NOMEM;
      return NULL;
    }

    if (!msgpackx_buffer_reserve(buf, 1024)) {
      if (err) *err = MSGPACKX_ERR_NOMEM;
      msgpackx_buffer_delete(buf);
      return NULL;
    }
    msgpackx_buffer_goto(buf, 0, 0, MSGPACKX_SEEK_SET);

    if (!msgpackx_node_do_pack_child_recursion(chld, buf, err)) {
      msgpackx_buffer_delete(buf);
      return NULL;
    }

    /*
     * msgpackx_node_do_pack_child_recursion() sets `chld->pointer`.
     * So this buf is not required anymore.
     */
    msgpackx_buffer_delete(buf);
  }
  return data;
}


msgpackx_array_node *
msgpackx_node_get_array(msgpackx_node *node)
{
  if (node->node_type == MSGPACKX_NODE_ARRAY) {
    return msgpackx_array_node_entry(node);
  }
  return NULL;
}

msgpackx_map_node *
msgpackx_node_get_map(msgpackx_node *node)
{
  if (node->node_type == MSGPACKX_NODE_MAP_KEY) {
    return msgpackx_map_key_node_entry(node);
  }
  if (node->node_type == MSGPACKX_NODE_MAP_VALUE) {
    return msgpackx_map_value_node_entry(node);
  }
  return NULL;
}

intmax_t
msgpackx_node_get_int(msgpackx_node *node, msgpackx_error *err)
{
  const char *m;
  ptrdiff_t sz;
  uint8_t u8;

  if (node->node_type != MSGPACKX_NODE_SINGLE || !node->pointer) {
    goto error;
  }

  m = msgpackx_node_get_data_pointer(node, &sz);
  MSGPACKX_ASSERT(m);
  if (sz <= 0) goto error;

  u8 = *m;
  if (u8 >= MSGPACKX_POSITIVE_FIXINT_MIN &&
      u8 <= MSGPACKX_POSITIVE_FIXINT_MAX) {
    if (sz != 1) goto error;
    return u8;
  }
  if (u8 >= MSGPACKX_NEGATIVE_FIXINT_MIN &&
      u8 <= MSGPACKX_NEGATIVE_FIXINT_MAX) {
    intmax_t im;

    if (sz != 1) goto error;
    im = u8;
    im -= (intmax_t)MSGPACKX_NEGATIVE_FIXINT_MAX + 1;
    return im;
  }

  sz -= sizeof(uint8_t);
  m++;
  switch(u8) {
  case MSGPACKX_INT8:
    {
      int8_t s8;
      if (sz != sizeof(s8)) goto error;
      memcpy(&s8, m, sizeof(s8));
      return s8;
    }
  case MSGPACKX_INT16:
    {
      int16_t s16;
      if (sz != sizeof(s16)) goto error;
      memcpy(&s16, m, sizeof(s16));
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
      msgpackx_byteswap_type(&s16, sizeof(int16_t));
#endif
      return s16;
    }
  case MSGPACKX_INT32:
    {
      int32_t s32;
      if (sz != sizeof(s32)) goto error;
      memcpy(&s32, m, sizeof(s32));
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
      msgpackx_byteswap_type(&s32, sizeof(int32_t));
#endif
      return s32;
    }
  case MSGPACKX_INT64:
    {
      int64_t s64;
      if (sz != sizeof(s64)) goto error;
      memcpy(&s64, m, sizeof(s64));
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
      msgpackx_byteswap_type(&s64, sizeof(int64_t));
#endif
      return s64;
    }
  default:
    break;
  }

  error:
  if (err) *err = MSGPACKX_ERR_MSG_TYPE;
  return 0;
}

uintmax_t
msgpackx_node_get_uint(msgpackx_node *node, msgpackx_error *err)
{
  const char *m;
  ptrdiff_t sz;
  uint8_t u8;

  if (node->node_type != MSGPACKX_NODE_SINGLE || !node->pointer) {
    goto error;
  }

  m = msgpackx_node_get_data_pointer(node, &sz);
  MSGPACKX_ASSERT(m);
  if (sz <= 0) goto error;

  u8 = *m;
  if (u8 >= MSGPACKX_POSITIVE_FIXINT_MIN &&
      u8 <= MSGPACKX_POSITIVE_FIXINT_MAX) {
    if (sz != 1) goto error;
    return u8;
  }

  sz -= sizeof(uint8_t);
  m++;
  switch(u8) {
  case MSGPACKX_UINT8:
    {
      if (sz != sizeof(u8)) goto error;
      memcpy(&u8, m, sizeof(u8));
      return u8;
    }
  case MSGPACKX_UINT16:
    {
      uint16_t u16;
      if (sz != sizeof(u16)) goto error;
      memcpy(&u16, m, sizeof(u16));
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
      u16 = msgpackx_byteswap_2(u16);
#endif
      return u16;
    }
  case MSGPACKX_UINT32:
    {
      uint32_t u32;
      if (sz != sizeof(u32)) goto error;
      memcpy(&u32, m, sizeof(u32));
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
      u32 = msgpackx_byteswap_4(u32);
#endif
      return u32;
    }
  case MSGPACKX_UINT64:
    {
      uint64_t u64;
      if (sz != sizeof(u64)) goto error;
      memcpy(&u64, m, sizeof(u64));
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
      u64 = msgpackx_byteswap_8(u64);
#endif
      return u64;
    }
  default:
    break;
  }

error:
  if (err) *err = MSGPACKX_ERR_MSG_TYPE;
  return (uintmax_t)-1;
}

const char *
msgpackx_node_get_str(msgpackx_node *node, ptrdiff_t *len,
                      msgpackx_error *err)
{
  const char *m;
  ptrdiff_t sz;
  uint8_t u8;

  if (node->node_type != MSGPACKX_NODE_SINGLE || !node->pointer) {
    goto error;
  }

  m = msgpackx_node_get_data_pointer(node, &sz);
  MSGPACKX_ASSERT(m);
  if (sz <= 0) goto error;

  u8 = *m;
  if (u8 >= MSGPACKX_FIXSTR_MIN && u8 <= MSGPACKX_FIXSTR_MAX) {
    if (len) *len = u8 - MSGPACKX_FIXSTR_MIN;
    ++m;
  } else {
    ptrdiff_t bsz, rsz;

    switch(u8) {
    case MSGPACKX_STR8:
      bsz = 1;
      break;
    case MSGPACKX_STR16:
      bsz = 2;
      break;
    case MSGPACKX_STR32:
      bsz = 4;
      break;
    default:
      goto error;
    }

    if (len) {
      rsz = msgpackx_node_read_uint(node->pointer, 1, bsz, err);
      if (rsz < 0) return NULL;
      *len = rsz;
    }

    ++m;
    m += bsz;
  }
  return m;

error:
  if (err) *err = MSGPACKX_ERR_MSG_TYPE;
  return NULL;
}

const void *
msgpackx_node_get_bin(msgpackx_node *node, ptrdiff_t *len,
                      msgpackx_error *err)
{
  const char *m;
  ptrdiff_t sz, bsz, rsz;
  uint8_t u8;

  if (node->node_type != MSGPACKX_NODE_SINGLE || !node->pointer) {
    goto error;
  }

  m = msgpackx_node_get_data_pointer(node, &sz);
  MSGPACKX_ASSERT(m);
  if (sz <= 0) goto error;

  u8 = *m;
  switch(u8) {
  case MSGPACKX_BIN16:
    bsz = 2;
    break;
  case MSGPACKX_BIN32:
    bsz = 4;
    break;
  case MSGPACKX_BIN64:
    bsz = 8;
    break;
  default:
    goto error;
  }

  if (len) {
    rsz = msgpackx_node_read_uint(node->pointer, 1, bsz, err);
    if (rsz < 0) return NULL;
    *len = rsz;
  }

  ++m;
  m += bsz;
  return m;

error:
  if (err) *err = MSGPACKX_ERR_MSG_TYPE;
  return NULL;
}

const void *
msgpackx_node_get_ext(msgpackx_node *node, ptrdiff_t *len, int *type,
                      msgpackx_error *err)
{
  const char *m;
  ptrdiff_t sz, bsz, rsz;
  uint8_t u8;

  if (node->node_type != MSGPACKX_NODE_SINGLE || !node->pointer) {
    goto error;
  }

  m = msgpackx_node_get_data_pointer(node, &sz);
  MSGPACKX_ASSERT(m);
  if (sz <= 1) goto error;

  u8 = *m;
  switch(u8) {
  case MSGPACKX_EXT8:
    bsz = 1;
    break;
  case MSGPACKX_EXT16:
    bsz = 2;
    break;
  case MSGPACKX_EXT32:
    bsz = 4;
    break;
  default:
    goto error;
  }

  if (len) {
    rsz = msgpackx_node_read_uint(node->pointer, 2, bsz, err);
    if (rsz < 0) return NULL;
    *len = rsz;
  }

  ++m;
  if (type) *type = *m;
  m += bsz + 1;

  return m;

error:
  if (err) *err = MSGPACKX_ERR_MSG_TYPE;
  return NULL;
}

float
msgpackx_node_get_float(msgpackx_node *node, msgpackx_error *err)
{
  const char *m;
  ptrdiff_t sz;
  float f;
  uint8_t u8;

  if (node->node_type != MSGPACKX_NODE_SINGLE || !node->pointer) {
    goto error;
  }

  m = msgpackx_node_get_data_pointer(node, &sz);
  MSGPACKX_ASSERT(m);
  if (sz <= 0 || (size_t)sz <= sizeof(float)) goto error;

  u8 = *m;
  if (u8 != MSGPACKX_FLOAT32) goto error;

  ++m;
  sz--;
  if (sz != sizeof(float)) goto error;

  memcpy(&f, m, sizeof(float));
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
  msgpackx_byteswap_type(&f, sizeof(float));
#endif

  return f;

error:
  if (err) *err = MSGPACKX_ERR_MSG_TYPE;
  return HUGE_VALF;
}

double
msgpackx_node_get_double(msgpackx_node *node, msgpackx_error *err)
{
  const char *m;
  ptrdiff_t sz;
  double f;
  uint8_t u8;

  if (node->node_type != MSGPACKX_NODE_SINGLE || !node->pointer) {
    goto error;
  }

  m = msgpackx_node_get_data_pointer(node, &sz);
  MSGPACKX_ASSERT(m);
  if (sz <= 0 || (size_t)sz <= sizeof(double)) goto error;

  u8 = *m;
  if (u8 != MSGPACKX_FLOAT64) goto error;

  ++m;
  sz--;
  if (sz != sizeof(double)) goto error;

  memcpy(&f, m, sizeof(double));
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
  msgpackx_byteswap_type(&f, sizeof(double));
#endif

  return f;

error:
  if (err) *err = MSGPACKX_ERR_MSG_TYPE;
  return HUGE_VAL;
}

int
msgpackx_node_get_bool(msgpackx_node *node, msgpackx_error *err)
{
  const char *m;
  ptrdiff_t sz;
  uint8_t u8;

  if (node->node_type != MSGPACKX_NODE_SINGLE || !node->pointer) {
    goto error;
  }

  m = msgpackx_node_get_data_pointer(node, &sz);
  MSGPACKX_ASSERT(m);
  if (sz <= 0) goto error;

  u8 = *m;
  switch(u8) {
  case MSGPACKX_TRUE:
    return 1;
  case MSGPACKX_FALSE:
    return 0;
  default:
    break;
  }

error:
  if (err) *err = MSGPACKX_ERR_MSG_TYPE;
  return 0;
}

int
msgpackx_node_is_nil(msgpackx_node *node)
{
  const char *m;
  ptrdiff_t sz;
  uint8_t u8;

  if (!node->pointer) {
    return -1;
  }

  if (node->node_type != MSGPACKX_NODE_SINGLE) {
    return 0;
  }

  m = msgpackx_node_get_data_pointer(node, &sz);
  MSGPACKX_ASSERT(m);
  if (sz <= 0) return -1;

  u8 = *m;
  switch(u8) {
  case MSGPACKX_NIL:
    return 1;
  default:
    return 0;
  }
}

static msgpackx_node *
msgpackx_node_do_pack(msgpackx_node *node, msgpackx_buffer *base,
                      msgpackx_error *err)
{
  MSGPACKX_ASSERT(node);
  MSGPACKX_ASSERT(node->pointer);

  if (!base) return node;

  if (!msgpackx_buffer_copy(base, node->pointer, MSGPACKX_COPY_EXPAND)) {
    return NULL;
  }
  msgpackx_buffer_relocate(node->pointer, base, 0, MSGPACKX_SEEK_CUR);
  return node;
}

msgpackx_array_node *
msgpackx_array_node_new(enum msgpackx_array_node_type type)
{
  uint8_t u8;
  struct msgpackx_array_node *a;
  a = (struct msgpackx_array_node *)malloc(sizeof(struct msgpackx_array_node));
  if (!a) return NULL;

  geom_list_init(&a->list);
  msgpackx_node_init(&a->node);
  a->node.node_type = MSGPACKX_NODE_ARRAY;
  a->head = NULL;

  if (type == MSGPACKX_ARRAY_HEAD) {
    u8 = MSGPACKX_FIXARRAY_MIN;
    if (!msgpackx_node_set_data(&a->node, &u8, sizeof(uint8_t), NULL)) {
      free(a);
      return NULL;
    }
    a->head = a;
  }
  return a;
}

void
msgpackx_array_node_delete(msgpackx_array_node *node)
{
  struct msgpackx_node *chld;

  chld = msgpackx_node_child(&node->node);

  geom_list_delete(&node->list);
  if (chld) {
    msgpackx_node_delete(chld);
  }

  msgpackx_node_clean_for_delete(&node->node);
  free(node);
}

msgpackx_array_node *
msgpackx_array_node_next(msgpackx_array_node *node)
{
  struct geom_list *lp;
  lp = geom_list_next(&node->list);
  return msgpackx_array_list_entry(lp);
}

msgpackx_array_node *
msgpackx_array_node_prev(msgpackx_array_node *node)
{
  struct geom_list *lp;
  lp = geom_list_prev(&node->list);
  return msgpackx_array_list_entry(lp);
}

msgpackx_array_node *
msgpackx_array_node_insert_next(msgpackx_array_node *prev,
                                msgpackx_array_node *ins)
{
  geom_list_insert_next(&prev->list, &ins->list);
  ins->head = prev->head;
  return ins;
}

msgpackx_array_node *
msgpackx_array_node_insert_prev(msgpackx_array_node *next,
                                msgpackx_array_node *ins)
{
  geom_list_insert_prev(&next->list, &ins->list);
  ins->head = next->head;
  return ins;
}

msgpackx_array_node *
msgpackx_array_node_set_child_node(msgpackx_array_node *array_node,
                                   msgpackx_node *node)
{
  MSGPACKX_ASSERT(array_node);
  MSGPACKX_ASSERT(!msgpackx_array_node_is_head_of_array(array_node));
  MSGPACKX_ASSERT(node);

  if (msgpackx_node_set_child(&array_node->node, node)) {
    return array_node;
  }
  return NULL;
}

msgpackx_node *
msgpackx_array_node_get_child_node(msgpackx_array_node *array_node)
{
  if (!msgpackx_array_node_is_head_of_array(array_node)) {
    return msgpackx_node_child(&array_node->node);
  }
  return NULL;
}

msgpackx_node *
msgpackx_array_node_get_parent(msgpackx_array_node *array_node)
{
  if (!msgpackx_array_node_is_head_of_array(array_node)) {
    array_node = msgpackx_array_node_get_head(array_node);
    if (!array_node) return NULL;
  }
  return msgpackx_node_parent(&array_node->node);
}

msgpackx_array_node *
msgpackx_array_node_get_head(msgpackx_array_node *array_node)
{
  return array_node->head;
}

msgpackx_node *
msgpackx_array_node_upcast(msgpackx_array_node *array_node)
{
  return &array_node->node;
}

int
msgpackx_array_node_is_head_of_array(msgpackx_array_node *node)
{
  return node->head == node;
}

static msgpackx_node *
msgpackx_node_do_pack_child_recursion(msgpackx_node *chld,
                                      msgpackx_buffer *buf,
                                      msgpackx_error *err)
{
  msgpackx_array_node *asub;
  msgpackx_map_node   *msub;

  asub = msgpackx_node_get_array(chld);
  if (asub) {
    if (!msgpackx_array_node_do_pack(asub, buf, err)) {
      return NULL;
    }

  } else {
    msub = msgpackx_node_get_map(chld);
    if (msub) {
      if (!msgpackx_map_node_do_pack(msub, buf, err)) {
        return NULL;
      }

    } else {
      if (!msgpackx_node_do_pack(chld, buf, err)) {
        return NULL;
      }
    }
  }
  return chld;
}

static ptrdiff_t
msgpackx_array_node_make_header(msgpackx_array_node *head,
                                msgpackx_buffer *buf,
                                msgpackx_error *err)
{
  uint8_t data[sizeof(uint8_t) + sizeof(uint64_t)];
  struct geom_list *lp;
  ptrdiff_t nelements;
  ptrdiff_t shead;
  uint8_t   u8;
  uint16_t  u16;
  uint32_t  u32;
  uint64_t  u64;

  nelements = 0;
  geom_list_foreach(lp, &head->list) {
    msgpackx_array_node *an;
    msgpackx_node *chld;

    an = msgpackx_array_list_entry(lp);
    chld = msgpackx_array_node_get_child_node(an);
    if (!chld) continue;

    nelements++;
    if (nelements < 0) return -1;
  }

  shead = 0;
  if (nelements <= 15) {
    u8 = MSGPACKX_FIXARRAY_MIN + nelements;
    data[shead] = u8;
    shead++;

  } else if (nelements <= MSGPACKX_UINT16_MAX) {
    u8 = MSGPACKX_ARRAY16;
    data[shead] = u8;
    shead++;

    u16 = (uint16_t)nelements;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u16 = msgpackx_byteswap_2(u16);
#endif
    memcpy(&data[shead], &u16, sizeof(uint16_t));
    shead += sizeof(uint16_t);

  } else if (nelements <= MSGPACKX_UINT32_MAX) {
    u8 = MSGPACKX_ARRAY32;
    data[shead] = u8;
    shead++;

    u32 = (uint32_t)nelements;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u32 = msgpackx_byteswap_4(u32);
#endif
    memcpy(&data[shead], &u32, sizeof(uint32_t));
    shead += sizeof(uint32_t);

  } else if (nelements <= MSGPACKX_UINT64_MAX) {
    u8 = MSGPACKX_ARRAY64;
    data[shead] = u8;
    shead++;

    u64 = (uint64_t)nelements;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u64 = msgpackx_byteswap_8(u64);
#endif
    memcpy(&data[shead], &u64, sizeof(uint64_t));

  } else {
    if (err) *err = MSGPACKX_ERR_RANGE;
    return -1;
  }

  if (!msgpackx_buffer_raw_copy(buf, data, shead, MSGPACKX_COPY_FLEXIBLE)) {
    if (err) *err = MSGPACKX_ERR_NOMEM;
    return -1;
  }

  return nelements;
}

static ptrdiff_t
msgpackx_array_node_write(msgpackx_array_node *arr, msgpackx_buffer **bufp,
                          FILE *stream)
{
  msgpackx_buffer *buf;
  msgpackx_error err;
  ptrdiff_t n;
  ptrdiff_t r;
  struct geom_list *lp;

  MSGPACKX_ASSERT(msgpackx_array_node_is_head_of_array(arr));

  err = MSGPACKX_SUCCESS;
  buf = msgpackx_buffer_new();
  n = -1;
  if (buf) {
    n = msgpackx_array_node_make_header(arr, buf, &err);
  } else {
    err = MSGPACKX_ERR_NOMEM;
  }
  if (n < 0) {
    if (buf) msgpackx_buffer_delete(buf);
#ifdef ENOMEM
    if (err == MSGPACKX_ERR_NOMEM) errno = ENOMEM;
#endif
    if (err == MSGPACKX_ERR_RANGE) errno = ERANGE;
    return -1;
  }

  r = msgpackx_node_write_or_append(bufp, buf, stream);
  msgpackx_buffer_delete(buf);
  if (r < 0) return r;

  geom_list_foreach(lp, &arr->list) {
    msgpackx_array_node *an;
    msgpackx_node *chld;

    an = msgpackx_array_list_entry(lp);
    chld = msgpackx_node_child(&an->node);

    n = msgpackx_node_write(chld, bufp, stream);
    if (n < 0) return n;

    r += n;
  }
  return r;
}

static msgpackx_array_node *
msgpackx_array_node_do_pack(msgpackx_array_node *node, msgpackx_buffer *base,
                            msgpackx_error *err)
{
  msgpackx_buffer *buf;
  struct geom_list *lp;
  ptrdiff_t nelements;
  ptrdiff_t shead;

  MSGPACKX_ASSERT(node);
  MSGPACKX_ASSERT(msgpackx_array_node_is_head_of_array(node));

  if (!base) {
    buf = msgpackx_buffer_new();
    if (!buf) {
      if (err) *err = MSGPACKX_ERR_NOMEM;
      return NULL;
    }
  } else {
    buf = msgpackx_buffer_substr(base, 0, 0, MSGPACKX_SEEK_CUR);
    if (!buf) {
      if (err) *err = MSGPACKX_ERR_NOMEM;
      return NULL;
    }
  }

  nelements = msgpackx_array_node_make_header(node, buf, err);
  if (nelements < 0) {
    goto error;
  }

  shead = msgpackx_buffer_size(buf);
  msgpackx_buffer_goto(buf, shead, 0, MSGPACKX_SEEK_CUR);

  if (nelements > 0) {
    geom_list_foreach(lp, &node->list) {
      msgpackx_array_node *an;
      msgpackx_node *chld;
      ptrdiff_t sz;

      an = msgpackx_array_list_entry(lp);
      chld = msgpackx_array_node_get_child_node(an);
      MSGPACKX_ASSERT(chld);

      if (!msgpackx_node_do_pack_child_recursion(chld, buf, err)) {
        goto error;
      }
      sz = msgpackx_node_size_used(chld);
      msgpackx_buffer_goto(buf, sz, 0, MSGPACKX_SEEK_CUR);
    }
  }

  if (base) {
    shead = (char *)msgpackx_buffer_pointer(buf)
          - (char *)msgpackx_buffer_pointer(base);
    msgpackx_buffer_goto(buf, -shead, shead, MSGPACKX_SEEK_CUR);
  } else {
    msgpackx_buffer_goto(buf, 0, -1, MSGPACKX_SEEK_SET);
    msgpackx_buffer_goto(buf, 0, msgpackx_buffer_size(buf), MSGPACKX_SEEK_SET);
  }

  MSGPACKX_ASSERT(node->node.pointer);

  msgpackx_buffer_delete(node->node.pointer);
  node->node.pointer = buf;
  return node;

error:
  msgpackx_buffer_delete(buf);
  return NULL;
}

msgpackx_array_node *
msgpackx_array_node_pack(msgpackx_array_node *head, msgpackx_error *err)
{
  return msgpackx_array_node_do_pack(head, NULL, err);
}

msgpackx_map_node *
msgpackx_map_node_new(enum msgpackx_map_node_type type)
{
  uint8_t u8;
  struct msgpackx_map_node *m;
  m = (struct msgpackx_map_node *)malloc(sizeof(struct msgpackx_map_node));
  if (!m) return NULL;

  geom_rbtree_init(&m->map_tree);
  msgpackx_node_init(&m->key_node);
  msgpackx_node_init(&m->value_node);
  geom_list_init(&m->insert_list);

  m->key_node.node_type = MSGPACKX_NODE_MAP_KEY;
  m->value_node.node_type = MSGPACKX_NODE_MAP_VALUE;
  m->root = NULL;

  if (type == MSGPACKX_MAP_HEAD) {
    u8 = MSGPACKX_FIXMAP_MIN;
    if (!msgpackx_node_set_data(&m->key_node, &u8, sizeof(uint8_t), NULL)) {
      free(m);
      return NULL;
    }
  }
  return m;
}

void
msgpackx_map_node_delete(msgpackx_map_node *node)
{
  struct geom_rbtree *root;
  struct msgpackx_node *chld;
  struct msgpackx_map_node *head;

  MSGPACKX_ASSERT(node);

  root = node->root;
  head = NULL;
  if (root && root != &node->map_tree) {
    head = msgpackx_map_node_get_head(node);
  }

  chld = msgpackx_node_child(&node->key_node);
  if (chld) msgpackx_node_delete(chld);
  chld = msgpackx_node_child(&node->value_node);
  if (chld) msgpackx_node_delete(chld);

  if (head) {
    head->root = geom_rbtree_delete(head->root, &node->map_tree, NULL);
  }

  geom_list_delete(&node->insert_list);
  msgpackx_node_clean_for_delete(&node->key_node);
  msgpackx_node_clean_for_delete(&node->value_node);

  free(node);
}

int
msgpackx_map_node_is_head_of_map(msgpackx_map_node *node)
{
  return !msgpackx_node_is_parent(&node->key_node);
}

msgpackx_node *
msgpackx_map_node_get_parent(msgpackx_map_node *map_node)
{
  if (!msgpackx_map_node_is_head_of_map(map_node)) {
    map_node = msgpackx_map_node_get_head(map_node);
    if (!map_node)
      return NULL;
  }
  return msgpackx_node_parent(&map_node->key_node);
}

static int
msgpackx_map_node_comp(struct geom_rbtree *at, struct geom_rbtree *bt)
{
  msgpackx_map_node *am;
  msgpackx_map_node *bm;
  msgpackx_node *ak;
  msgpackx_node *bk;
  const void *ap;
  const void *bp;
  ptrdiff_t asz;
  ptrdiff_t bsz;
  ptrdiff_t szcomp;
  int ret;

  am = msgpackx_map_tree_entry(at);
  bm = msgpackx_map_tree_entry(bt);

  ak = msgpackx_map_node_get_key(am);
  bk = msgpackx_map_node_get_key(bm);

  if (ak == bk) return 0;
  if (!ak) return -1;
  if (!bk) return  1;

  ap = msgpackx_node_get_data_pointer(ak, &asz);
  bp = msgpackx_node_get_data_pointer(bk, &bsz);
  if (ap == bp && asz == bsz) return 0;

  szcomp = (asz < bsz) ? asz : bsz;
  ret = memcmp(ap, bp, szcomp);
  if (ret == 0 && asz != bsz) {
    if (asz < bsz) {
      ret = -1;
    } else {
      ret =  1;
    }
  }
  return ret;
}

msgpackx_map_node *
msgpackx_map_node_find(msgpackx_map_node *node, msgpackx_node *find,
                       msgpackx_error *err)
{
  struct geom_rbtree *root, *entry;
  struct msgpackx_node *parent;
  struct msgpackx_map_node find_map;
  struct msgpackx_map_node *pchk;

  MSGPACKX_ASSERT(find);
  MSGPACKX_ASSERT(!msgpackx_node_is_parent(find));

  if (!msgpackx_map_node_is_head_of_map(node)) {
    node = msgpackx_map_node_get_head(node);
    if (!node) return NULL;
  }

  parent = msgpackx_node_parent(find);

  if (parent) {
    /*
     * We temporarily delinks the parent for search. But if the
     * searching node is one of the keys that belongs to this map,
     * returning it without actual search. Since delinking will cause
     * not found, because delinking it will get excluded of this map.
     */
    pchk = msgpackx_node_get_map(parent);
    if (pchk) {
      if (msgpackx_map_node_get_head(pchk) == node) {
        return pchk;
      }
    }

    msgpackx_node_unlink(find);
  }

  msgpackx_node_init(&find_map.key_node);
  msgpackx_node_set_child(&find_map.key_node, find);

  root = node->root;
  entry = geom_rbtree_find(root, &find_map.map_tree, msgpackx_map_node_comp);
  if (entry) {
    node = msgpackx_map_tree_entry(entry);
  }
  msgpackx_node_unlink(find);
  if (parent) {
    msgpackx_node_set_child(parent, find);
  }
  return node;
}

msgpackx_map_node *
msgpackx_map_node_find_by_str(msgpackx_map_node *node,
                              const char *str, ptrdiff_t len,
                              msgpackx_error *err)
{
  struct msgpackx_node find;

  msgpackx_node_init(&find);

  if (!msgpackx_node_set_str(&find, str, len, err)) {
    return NULL;
  }

  node = msgpackx_map_node_find(node, &find, err);
  msgpackx_node_clean_for_delete(&find);
  return node;
}

msgpackx_map_node *
msgpackx_map_node_find_by_int(msgpackx_map_node *node, intmax_t value,
                              msgpackx_error *err)
{
  struct msgpackx_node find;

  msgpackx_node_init(&find);

  if (!msgpackx_node_set_int(&find, value, err)) {
    return NULL;
  }

  node = msgpackx_map_node_find(node, &find, err);
  msgpackx_node_clean_for_delete(&find);
  return node;
}

msgpackx_map_node *
msgpackx_map_node_find_by_uint(msgpackx_map_node *node, uintmax_t value,
                               msgpackx_error *err)
{
  struct msgpackx_node find;

  msgpackx_node_init(&find);

  if (!msgpackx_node_set_uint(&find, value, err)) {
    return NULL;
  }

  node = msgpackx_map_node_find(node, &find, err);
  msgpackx_node_clean_for_delete(&find);
  return node;
}

msgpackx_map_node *
msgpackx_map_node_insert(msgpackx_map_node *head,
                         msgpackx_map_node *insert, msgpackx_error *err)
{
  struct geom_rbtree *root;

  MSGPACKX_ASSERT(head);
  MSGPACKX_ASSERT(insert);
  MSGPACKX_ASSERT(msgpackx_map_node_get_key(insert));
  MSGPACKX_ASSERT(msgpackx_map_node_is_head_of_map(head));

  root = head->root;
  root = geom_rbtree_insert(root, &insert->map_tree,
                            msgpackx_map_node_comp, NULL);
  if (!root) {
    if (err) *err = MSGPACKX_ERR_MAP_KEY_DUPLICATE;
    return NULL;
  }
  head->root = root;

  geom_list_insert_prev(&head->insert_list, &insert->insert_list);
  return head;
}

msgpackx_map_node *
msgpackx_map_node_get_head(msgpackx_map_node *node)
{
  MSGPACKX_ASSERT(node);

  if (msgpackx_map_node_is_head_of_map(node)) {
    return node;
  }
  if (!node->root) return NULL;
  return msgpackx_map_tree_entry(node->root);
}

msgpackx_node *
msgpackx_map_node_get_key(msgpackx_map_node *node)
{
  MSGPACKX_ASSERT(node);
  MSGPACKX_ASSERT(!msgpackx_map_node_is_head_of_map(node));

  return msgpackx_node_child(&node->key_node);
}

msgpackx_node *
msgpackx_map_node_get_value(msgpackx_map_node *node)
{
  MSGPACKX_ASSERT(node);
  MSGPACKX_ASSERT(!msgpackx_map_node_is_head_of_map(node));

  return msgpackx_node_child(&node->value_node);
}

msgpackx_map_node *
msgpackx_map_node_set_key(msgpackx_map_node *node, msgpackx_node *key,
                          msgpackx_error *err)
{
  struct msgpackx_map_node *head;
  struct geom_rbtree *root;
  struct msgpackx_node *nret;
  struct msgpackx_node *ochld;

  MSGPACKX_ASSERT(node);
  MSGPACKX_ASSERT(!msgpackx_map_node_is_head_of_map(node));
  MSGPACKX_ASSERT(key);
  MSGPACKX_ASSERT(!msgpackx_node_is_parent(key));
  MSGPACKX_ASSERT(msgpackx_node_get_data_pointer(key, NULL));

  head = msgpackx_map_node_get_head(node);
  ochld = msgpackx_node_child(&node->key_node);
  node->key_node.pointer = NULL;
  nret = msgpackx_node_set_child(&node->key_node, key);

  /*
   * Deletion does not use comparison, tree fixing is done after set.
   */
  if (nret && head) {
    struct geom_rbtree *nroot;

    root = head->root;
    root = geom_rbtree_delete(root, &node->map_tree, NULL);
    nroot = geom_rbtree_insert(root, &node->map_tree,
                               msgpackx_map_node_comp, NULL);

    if (!nroot && ochld) {
      node->key_node.pointer = NULL;
      msgpackx_node_set_child(&node->key_node, ochld);
      nroot = geom_rbtree_insert(root, &node->map_tree,
                                 msgpackx_map_node_comp, NULL);
      MSGPACKX_ASSERT_X(nroot, "Map key reversion failed");

      ochld = NULL;
      nret = NULL;
      if (err) *err = MSGPACKX_ERR_MAP_KEY_DUPLICATE;
    }
    head->root = nroot;
  }

  if (ochld) {
    msgpackx_node_delete(ochld);
  }

  if (!nret) return NULL;
  return node;
}

msgpackx_map_node *
msgpackx_map_node_set_value(msgpackx_map_node *node, msgpackx_node *value)
{
  MSGPACKX_ASSERT(node);
  MSGPACKX_ASSERT(!msgpackx_map_node_is_head_of_map(node));
  MSGPACKX_ASSERT(value);
  MSGPACKX_ASSERT(!msgpackx_node_is_parent(value));
  MSGPACKX_ASSERT(msgpackx_node_get_data_pointer(value, NULL));

  if (!msgpackx_node_set_child(&node->value_node, value)) {
    return NULL;
  }
  return node;
}

msgpackx_node *
msgpackx_map_node_upcast(msgpackx_map_node *map_node);

static ptrdiff_t
msgpackx_map_node_make_header(msgpackx_map_node *head, msgpackx_buffer *buf,
                              msgpackx_error *err)
{
  uint8_t data[sizeof(uint8_t) + sizeof(uint64_t)];
  struct geom_list *lp;
  ptrdiff_t nelements;
  ptrdiff_t shead;
  uint8_t   u8;
  uint16_t  u16;
  uint32_t  u32;
  uint64_t  u64;

  nelements = 0;
  geom_list_foreach(lp, &head->insert_list) {
    nelements += 1;
    if (nelements < 0) {
      if (err) *err = MSGPACKX_ERR_RANGE;
      return -1;
    }
  }

  shead = 0;
  if (nelements <= 15) {
    u8 = MSGPACKX_FIXMAP_MIN + nelements;
    data[shead] = u8;
    shead++;

  } else if (nelements <= MSGPACKX_UINT16_MAX) {
    u8 = MSGPACKX_MAP16;
    data[shead] = u8;
    shead++;

    u16 = (uint16_t)nelements;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u16 = msgpackx_byteswap_2(u16);
#endif
    memcpy(&data[shead], &u16, sizeof(uint16_t));
    shead += sizeof(uint16_t);

  } else if (nelements <= MSGPACKX_UINT32_MAX) {
    u8 = MSGPACKX_MAP32;
    data[shead] = u8;
    shead++;

    u32 = (uint32_t)nelements;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u32 = msgpackx_byteswap_4(u32);
#endif
    memcpy(&data[shead], &u32, sizeof(uint32_t));
    shead += sizeof(uint32_t);

  } else if (nelements <= MSGPACKX_UINT64_MAX) {
    u8 = MSGPACKX_MAP64;
    data[shead] = u8;
    shead++;

    u64 = (uint64_t)nelements;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
    u64 = msgpackx_byteswap_8(u64);
#endif
    memcpy(&data[shead], &u64, sizeof(uint64_t));
    shead += sizeof(uint64_t);

  } else {
    if (err) *err = MSGPACKX_ERR_RANGE;
    return -1;
  }

  if (!msgpackx_buffer_raw_copy(buf, data, shead, MSGPACKX_COPY_FLEXIBLE)) {
    if (err) *err = MSGPACKX_ERR_NOMEM;
    return -1;
  }

  return nelements;
}

static ptrdiff_t
msgpackx_map_node_write(msgpackx_map_node *map, msgpackx_buffer **bufp,
                        FILE *stream)
{
  msgpackx_buffer *buf;
  msgpackx_error err;
  ptrdiff_t n;
  ptrdiff_t r;
  struct geom_list *lp;

  MSGPACKX_ASSERT(msgpackx_map_node_is_head_of_map(map));

  err = MSGPACKX_SUCCESS;

  buf = msgpackx_buffer_new();
  n = -1;
  if (buf) {
    n = msgpackx_map_node_make_header(map, buf, &err);
  } else {
    err = MSGPACKX_ERR_NOMEM;
  }
  if (n < 0) {
    if (buf) msgpackx_buffer_delete(buf);
#ifdef ENOMEM
    if (err == MSGPACKX_ERR_NOMEM) errno = ENOMEM;
#endif
    if (err == MSGPACKX_ERR_RANGE) errno = ERANGE;
    return -1;
  }

  r = msgpackx_node_write_or_append(bufp, buf, stream);
  msgpackx_buffer_delete(buf);

  if (r < 0) return r;

  geom_list_foreach(lp, &map->insert_list) {
    msgpackx_map_node *mn;
    msgpackx_node *chld;

    mn = msgpackx_map_list_entry(lp);
    chld = msgpackx_node_child(&mn->key_node);

    n = msgpackx_node_write(chld, bufp, stream);
    if (n < 0) return n;
    r += n;

    chld = msgpackx_node_child(&mn->value_node);

    n = msgpackx_node_write(chld, bufp, stream);
    if (n < 0) return n;
    r += n;
  }
  return r;
}

static msgpackx_map_node *
msgpackx_map_node_do_pack(msgpackx_map_node *node, msgpackx_buffer *base,
                          msgpackx_error *err)
{
  msgpackx_buffer *buf;
  struct geom_list *lp;
  ptrdiff_t nelements;
  ptrdiff_t shead;

  MSGPACKX_ASSERT(node);
  MSGPACKX_ASSERT(msgpackx_map_node_is_head_of_map(node));

  if (!base) {
    buf = msgpackx_buffer_new();
    if (!buf) {
      if (err) *err = MSGPACKX_ERR_NOMEM;
      return NULL;
    }
  } else {
    buf = msgpackx_buffer_substr(base, 0, 0, MSGPACKX_SEEK_CUR);
    if (!buf) {
      if (err) *err = MSGPACKX_ERR_NOMEM;
      return NULL;
    }
  }

  nelements = msgpackx_map_node_make_header(node, buf, err);
  if (nelements < 0) {
    goto error;
  }

  shead = msgpackx_buffer_size(buf);
  msgpackx_buffer_goto(buf, shead, 0, MSGPACKX_SEEK_CUR);

  if (nelements > 0) {
    geom_list_foreach(lp, &node->insert_list) {
      msgpackx_map_node   *mn;
      msgpackx_node *key_chld, *val_chld;
      ptrdiff_t sz;

      mn = msgpackx_map_list_entry(lp);
      key_chld = msgpackx_map_node_get_key(mn);
      val_chld = msgpackx_map_node_get_value(mn);
      MSGPACKX_ASSERT(key_chld && val_chld);

      if (!msgpackx_node_do_pack_child_recursion(key_chld, buf, err)) {
        goto error;
      }
      sz = msgpackx_node_size_used(key_chld);
      msgpackx_buffer_goto(buf, sz, 0, MSGPACKX_SEEK_CUR);

      if (!msgpackx_node_do_pack_child_recursion(val_chld, buf, err)) {
        goto error;
      }
      sz = msgpackx_node_size_used(val_chld);
      msgpackx_buffer_goto(buf, sz, 0, MSGPACKX_SEEK_CUR);
    }
  }

  if (base) {
    shead = (char *)msgpackx_buffer_pointer(buf)
          - (char *)msgpackx_buffer_pointer(base);
    msgpackx_buffer_goto(buf, -shead, shead, MSGPACKX_SEEK_CUR);
  } else {
    msgpackx_buffer_goto(buf, 0, -1, MSGPACKX_SEEK_SET);
    msgpackx_buffer_goto(buf, 0, msgpackx_buffer_size(buf), MSGPACKX_SEEK_SET);
  }

  MSGPACKX_ASSERT(node->key_node.pointer);

  msgpackx_buffer_delete(node->key_node.pointer);
  node->key_node.pointer = buf;
  return node;

error:
  msgpackx_buffer_delete(buf);
  return NULL;
}

msgpackx_node *
msgpackx_map_node_upcast(msgpackx_map_node *map_node)
{
  return &map_node->key_node;
}

msgpackx_map_node *
msgpackx_map_node_next(msgpackx_map_node *node)
{
  return msgpackx_map_list_entry(geom_list_next(&node->insert_list));
}

msgpackx_map_node *
msgpackx_map_node_prev(msgpackx_map_node *node)
{
  return msgpackx_map_list_entry(geom_list_prev(&node->insert_list));
}

msgpackx_map_node *
msgpackx_map_node_next_sorted(msgpackx_map_node *node)
{
  struct geom_rbtree *t;
  t = NULL;
  if (msgpackx_map_node_is_head_of_map(node)) {
    if (node->root) {
      t = geom_rbtree_minimum(node->root);
    }
  } else {
    t = geom_rbtree_succ_next(&node->map_tree);
  }
  if (t) {
    return msgpackx_map_tree_entry(t);
  } else {
    return NULL;
  }
}

msgpackx_map_node *
msgpackx_map_node_prev_sorted(msgpackx_map_node *node)
{
  struct geom_rbtree *t;
  t = NULL;
  if (msgpackx_map_node_is_head_of_map(node)) {
    if (node->root) {
      t = geom_rbtree_maximum(node->root);
    }
  } else {
    t = geom_rbtree_predec_next(&node->map_tree);
  }
  if (t) {
    return msgpackx_map_tree_entry(t);
  } else {
    return NULL;
  }
}

msgpackx_map_node *
msgpackx_map_node_pack(msgpackx_map_node *head, msgpackx_error *err)
{
  return msgpackx_map_node_do_pack(head, NULL, err);
}

static msgpackx_node *
msgpackx_array_node_parse(msgpackx_buffer *buffer, ptrdiff_t nelement,
                          ptrdiff_t start,
                          msgpackx_error *err, const char **eloc)
{
  msgpackx_array_node *ahead;
  msgpackx_array_node *anode;
  msgpackx_node *node;
  msgpackx_buffer *cur;
  ptrdiff_t nlim;
  ptrdiff_t sz;

  MSGPACKX_ASSERT(buffer);
  MSGPACKX_ASSERT(nelement >= 0);
  MSGPACKX_ASSERT(start >= 0);

  cur = msgpackx_buffer_substr(buffer, 0, -1, MSGPACKX_SEEK_CUR);
  ahead = msgpackx_array_node_new(MSGPACKX_ARRAY_HEAD);
  anode = NULL;
  node = NULL;
  if (!cur || !ahead) {
    if (err) *err = MSGPACKX_ERR_NOMEM;
    goto error;
  }

  msgpackx_node_nullify(&ahead->node);

  nlim = msgpackx_buffer_size(buffer);
  nlim -= start;
  if (nlim < 0) {
    if (err) *err = MSGPACKX_ERR_MSG_INCOMPLETE;
    goto error;
  }
  msgpackx_buffer_goto(cur, start, nlim, MSGPACKX_SEEK_CUR);
  sz = start;

  for (; nelement > 0; nelement--) {
    node = msgpackx_node_parse(cur, err, eloc);
    if (!node) {
      eloc = NULL;
      goto error;
    }

    anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
    if (!anode) {
      if (err) *err = MSGPACKX_ERR_NOMEM;
      goto error;
    }

    start = msgpackx_node_size_used(node);
    sz += start;
    nlim -= start;
    if (nlim < 0) {
      if (err) *err = MSGPACKX_ERR_MSG_INCOMPLETE;
      eloc = NULL;
      goto error;
    }
    msgpackx_buffer_goto(cur, start, nlim, MSGPACKX_SEEK_CUR);

    msgpackx_array_node_set_child_node(anode, node);
    msgpackx_array_node_insert_prev(ahead, anode);
    node = NULL;
    anode = NULL;
  }

  msgpackx_buffer_relocate(cur, buffer, 0, MSGPACKX_SEEK_CUR);
  msgpackx_buffer_goto(cur, 0, sz, MSGPACKX_SEEK_CUR);
  ahead->node.pointer = cur;
  cur = NULL;

clean:
  if (anode) msgpackx_array_node_delete(anode);
  if (cur)   msgpackx_buffer_delete(cur);
  if (node)  msgpackx_node_delete(node);
  if (ahead) {
    return msgpackx_array_node_upcast(ahead);
  }
  return NULL;

error:
  msgpackx_array_node_delete_all(ahead);
  ahead = NULL;
  if (eloc) *eloc = msgpackx_buffer_pointer(buffer);
  goto clean;
}

static msgpackx_node *
msgpackx_map_node_parse(msgpackx_buffer *buffer, ptrdiff_t nelement,
                        ptrdiff_t start,
                        msgpackx_error *err, const char **eloc)
{
  msgpackx_map_node *mhead;
  msgpackx_map_node *mnode;
  msgpackx_node *node;
  msgpackx_buffer *cur;
  ptrdiff_t nlim;
  ptrdiff_t sz;
  int i;

  MSGPACKX_ASSERT(buffer);
  MSGPACKX_ASSERT(nelement >= 0);
  MSGPACKX_ASSERT(start >= 0);

  cur = msgpackx_buffer_substr(buffer, 0, -1, MSGPACKX_SEEK_CUR);
  mhead = msgpackx_map_node_new(MSGPACKX_MAP_HEAD);
  mnode = NULL;
  node = NULL;
  if (!cur || !mhead) {
    if (err) *err = MSGPACKX_ERR_NOMEM;
    goto error;
  }

  nlim = msgpackx_buffer_size(buffer);
  nlim -= start;
  if (nlim < 0) {
    if (err) *err = MSGPACKX_ERR_MSG_INCOMPLETE;
    goto error;
  }
  msgpackx_buffer_goto(cur, start, nlim, MSGPACKX_SEEK_CUR);
  sz = start;

  for (; nelement > 0; nelement--) {
    mnode = msgpackx_map_node_new(MSGPACKX_MAP_ENTRY);
    if (!mnode) {
      if (err) *err = MSGPACKX_ERR_NOMEM;
      goto error;
    }

    for (i = 0; i < 2; ++i) {
      node = msgpackx_node_parse(cur, err, eloc);
      if (!node) {
        eloc = NULL;
        goto error;
      }

      start = msgpackx_node_size_used(node);
      sz += start;
      nlim -= start;
      if (nlim < 0) {
        if (err) *err = MSGPACKX_ERR_MSG_INCOMPLETE;
        eloc = NULL;
        goto error;
      }
      msgpackx_buffer_goto(cur, start, nlim, MSGPACKX_SEEK_CUR);

      if (i == 0) {
        msgpackx_map_node_set_key(mnode, node, NULL);
      } else {
        msgpackx_map_node_set_value(mnode, node);
      }
      node = NULL;
    }

    if (!msgpackx_map_node_insert(mhead, mnode, err)) {
      buffer = cur;
      goto error;
    }
    mnode = NULL;
  }

  msgpackx_node_nullify(&mhead->key_node);
  msgpackx_buffer_relocate(cur, buffer, 0, MSGPACKX_SEEK_CUR);
  msgpackx_buffer_goto(cur, 0, sz, MSGPACKX_SEEK_CUR);
  mhead->key_node.pointer = cur;
  cur = NULL;

clean:
  if (mnode) msgpackx_map_node_delete(mnode);
  if (cur)   msgpackx_buffer_delete(cur);
  if (node)  msgpackx_node_delete(node);
  if (mhead) {
    return msgpackx_map_node_upcast(mhead);
  }
  return NULL;

error:
  msgpackx_map_node_delete_all(mhead);
  mhead = NULL;
  if (eloc) *eloc = msgpackx_buffer_pointer(buffer);
  goto clean;
}
