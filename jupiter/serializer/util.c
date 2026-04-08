#include "util.h"
#include "defs.h"
#include "error.h"
#include "msgpackx.h"

#include <string.h>
#include <stdint.h>
#include <stddef.h>

//--- setters

struct msgpackx_set_data
{
  msgpackx_error (*setter)(msgpackx_node *n, void *arg);
  void *arg;
};

//--- str

struct msgpackx_set_str_data
{
  struct msgpackx_set_data p;
  const char *str;
  ptrdiff_t len;
};

static msgpackx_error msgpackx_set_str(msgpackx_node *n, void *arg)
{
  struct msgpackx_set_str_data *p;
  msgpackx_error err = MSGPACKX_SUCCESS;
  p = (struct msgpackx_set_str_data *)arg;
  msgpackx_node_set_str(n, p->str, p->len, &err);
  return err;
}

static struct msgpackx_set_data *
msgpackx_set_str_f(const char *str, ptrdiff_t len,
                   struct msgpackx_set_str_data *p)
{
  *p = (struct msgpackx_set_str_data){
    .str = str,
    .len = len,
    .p = {.setter = msgpackx_set_str, .arg = p},
  };
  return &p->p;
}

#define msgpackx_set_str(str, len) \
  msgpackx_set_str_f(str, len, &(struct msgpackx_set_str_data){{NULL}})

//---- bin

struct msgpackx_set_bin_data
{
  struct msgpackx_set_data p;
  const void *data;
  ptrdiff_t size;
};

static msgpackx_error msgpackx_set_bin(msgpackx_node *n, void *arg)
{
  struct msgpackx_set_bin_data *p;
  msgpackx_error err = MSGPACKX_SUCCESS;
  p = (struct msgpackx_set_bin_data *)arg;
  msgpackx_node_set_bin(n, p->data, p->size, &err);
  return err;
}

static struct msgpackx_set_data *
msgpackx_set_bin_f(const void *data, ptrdiff_t size,
                   struct msgpackx_set_bin_data *p)
{
  *p = (struct msgpackx_set_bin_data){
    .data = data,
    .size = size,
    .p = {.setter = msgpackx_set_bin, .arg = p},
  };
  return &p->p;
}

#define msgpackx_set_bin(data, size) \
  msgpackx_set_bin_f(data, size, &(struct msgpackx_set_bin_data){{NULL}})

//---- ext

struct msgpackx_set_ext_data
{
  struct msgpackx_set_data p;
  int type;
  void *data;
  ptrdiff_t size;
};

static msgpackx_error msgpackx_set_ext(msgpackx_node *n, void *arg)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  struct msgpackx_set_ext_data *p;
  p = (struct msgpackx_set_ext_data *)arg;
  msgpackx_node_set_ext(n, p->type, p->data, p->size, &err);
  return err;
}

static struct msgpackx_set_data *
msgpackx_set_ext_f(int type, void *data, ptrdiff_t size,
                   struct msgpackx_set_ext_data *p)
{
  *p = (struct msgpackx_set_ext_data){
    .type = type,
    .data = data,
    .size = size,
    .p = {.setter = msgpackx_set_ext, .arg = p},
  };
  return &p->p;
}

#define msgpackx_set_ext(type, data, size) \
  msgpackx_set_ext_f(type, data, size, &(struct msgpackx_set_ext_data){{NULL}})

//---- int

struct msgpackx_set_int_data
{
  struct msgpackx_set_data p;
  intmax_t value;
};

static msgpackx_error msgpackx_set_int(msgpackx_node *n, void *arg)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  struct msgpackx_set_int_data *p;
  p = (struct msgpackx_set_int_data *)arg;
  msgpackx_node_set_int(n, p->value, &err);
  return err;
}

static struct msgpackx_set_data *
msgpackx_set_int_f(intmax_t value, struct msgpackx_set_int_data *p)
{
  *p = (struct msgpackx_set_int_data){
    .value = value,
    .p = {.setter = msgpackx_set_int, .arg = p},
  };
  return &p->p;
}

#define msgpackx_set_int(value) \
  msgpackx_set_int_f(value, &(struct msgpackx_set_int_data){{NULL}})

//---- uint

struct msgpackx_set_uint_data
{
  struct msgpackx_set_data p;
  uintmax_t value;
};

static msgpackx_error msgpackx_set_uint(msgpackx_node *n, void *arg)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  struct msgpackx_set_uint_data *p;
  p = (struct msgpackx_set_uint_data *)arg;
  msgpackx_node_set_uint(n, p->value, &err);
  return err;
}

static struct msgpackx_set_data *
msgpackx_set_uint_f(uintmax_t value, struct msgpackx_set_uint_data *p)
{
  *p = (struct msgpackx_set_uint_data){
    .value = value,
    .p = {.setter = msgpackx_set_uint, .arg = p},
  };
  return &p->p;
}

#define msgpackx_set_uint(value) \
  msgpackx_set_uint_f(value, &(struct msgpackx_set_uint_data){{NULL}})

//---- bool

struct msgpackx_set_bool_data
{
  struct msgpackx_set_data p;
  int value;
};

static msgpackx_error msgpackx_set_bool(msgpackx_node *n, void *arg)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  struct msgpackx_set_bool_data *p;
  p = (struct msgpackx_set_bool_data *)arg;
  msgpackx_node_set_bool(n, p->value, &err);
  return err;
}

static struct msgpackx_set_data *
msgpackx_set_bool_f(int value, struct msgpackx_set_bool_data *p)
{
  *p = (struct msgpackx_set_bool_data){
    .value = value,
    .p = {.setter = msgpackx_set_bool, .arg = p},
  };
  return &p->p;
}

#define msgpackx_set_bool(value) \
  msgpackx_set_bool_f(value, &(struct msgpackx_set_bool_data){{NULL}})

//---- float

struct msgpackx_set_float_data
{
  struct msgpackx_set_data p;
  float value;
};

static msgpackx_error msgpackx_set_float(msgpackx_node *n, void *arg)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  struct msgpackx_set_float_data *p;
  p = (struct msgpackx_set_float_data *)arg;
  msgpackx_node_set_float(n, p->value, &err);
  return err;
}

static struct msgpackx_set_data *
msgpackx_set_float_f(float value, struct msgpackx_set_float_data *p)
{
  *p = (struct msgpackx_set_float_data){
    .value = value,
    .p = {.setter = msgpackx_set_float, .arg = p},
  };
  return &p->p;
}

#define msgpackx_set_float(value) \
  msgpackx_set_float_f(value, &(struct msgpackx_set_float_data){{NULL}})

//---- double

struct msgpackx_set_double_data
{
  struct msgpackx_set_data p;
  double value;
};

static msgpackx_error msgpackx_set_double(msgpackx_node *n, void *arg)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  struct msgpackx_set_double_data *p;
  p = (struct msgpackx_set_double_data *)arg;
  msgpackx_node_set_double(n, p->value, &err);
  return err;
}

static struct msgpackx_set_data *
msgpackx_set_double_f(double value, struct msgpackx_set_double_data *p)
{
  *p = (struct msgpackx_set_double_data){
    .value = value,
    .p = {.setter = msgpackx_set_double, .arg = p},
  };
  return &p->p;
}

#define msgpackx_set_double(value) \
  msgpackx_set_double_f(value, &(struct msgpackx_set_double_data){{NULL}})

//---- nil

struct msgpackx_set_nil_data
{
  struct msgpackx_set_data p;
};

static msgpackx_error msgpackx_set_nil(msgpackx_node *n, void *arg)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  msgpackx_node_set_nil(n, &err);
  return err;
}

static struct msgpackx_set_data *
msgpackx_set_nil_f(struct msgpackx_set_nil_data *p)
{
  *p = (struct msgpackx_set_nil_data){
    .p = {.setter = msgpackx_set_nil, .arg = NULL},
  };
  return &p->p;
}

#define msgpackx_set_nil() \
  msgpackx_set_nil_f(&(struct msgpackx_set_nil_data){{NULL}})

//----

struct msgpackx_aset_data
{
  msgpackx_error (*gennode)(msgpackx_node **node, void *arg);
  void (*failclean)(msgpackx_node *node, void *arg);
  void *arg;
};

static msgpackx_error msgpackx_aset_node(msgpackx_node **node,
                                         struct msgpackx_aset_data *p)
{
  return p->gennode(node, p->arg);
}

static void msgpackx_aset_clean(msgpackx_node *node,
                                struct msgpackx_aset_data *p)
{
  if (p->failclean)
    p->failclean(node, p->arg);
}

//----

static msgpackx_error msgpackx_node_new_general(msgpackx_node **node,
                                                struct msgpackx_set_data *p)
{
  msgpackx_error err;
  msgpackx_node *n;
  n = msgpackx_node_new();

  if (!n)
    return MSGPACKX_ERR_NOMEM;

  err = p->setter(n, p->arg);
  if (err != MSGPACKX_SUCCESS) {
    msgpackx_node_delete(n);
    return err;
  }

  *node = n;
  return err;
}

struct msgpackx_aset_general
{
  struct msgpackx_aset_data p;
  struct msgpackx_set_data *sp;
};

static msgpackx_error msgpackx_aset_general_node(msgpackx_node **node,
                                                 void *arg)
{
  struct msgpackx_aset_general *p;
  p = (struct msgpackx_aset_general *)arg;

  *node = NULL;
  return msgpackx_node_new_general(node, p->sp);
}

static void msgpackx_aset_general_clean(msgpackx_node *node, void *arg)
{
  if (node)
    msgpackx_node_delete(node);
}

static struct msgpackx_aset_data *
msgpackx_aset_general(struct msgpackx_set_data *p,
                      struct msgpackx_aset_general *gp)
{
  *gp = (struct msgpackx_aset_general){
    .sp = p,
    .p =
      {
        .gennode = msgpackx_aset_general_node,
        .failclean = msgpackx_aset_general_clean,
        .arg = gp,
      },
  };
  return &gp->p;
}

#define msgpackx_aset_general(p) \
  msgpackx_aset_general((p), &(struct msgpackx_aset_general){{NULL}})

#define msgpackx_aset_str(str, len) \
  msgpackx_aset_general(msgpackx_set_str(str, len))

#define msgpackx_aset_bin(data, size) \
  msgpackx_aset_general(msgpackx_set_bin(data, size))

#define msgpackx_aset_ext(type, data, size) \
  msgpackx_aset_general(msgpackx_set_ext(type, data, size))

#define msgpackx_aset_int(value) msgpackx_aset_general(msgpackx_set_int(value))

#define msgpackx_aset_uint(value) \
  msgpackx_aset_general(msgpackx_set_uint(value))

#define msgpackx_aset_bool(value) \
  msgpackx_aset_general(msgpackx_set_bool(value))

#define msgpackx_aset_float(value) \
  msgpackx_aset_general(msgpackx_set_float(value))

#define msgpackx_aset_double(value) \
  msgpackx_aset_general(msgpackx_set_double(value))

#define msgpackx_aset_nil() msgpackx_aset_general(msgpackx_set_nil())

//---- ary

struct msgpackx_aset_ary
{
  struct msgpackx_aset_data p;
  msgpackx_array_node **aheadp;
};

static msgpackx_error msgpackx_aset_ary_node(msgpackx_node **node, void *arg)
{
  msgpackx_array_node *ahead;
  struct msgpackx_aset_ary *p;
  p = (struct msgpackx_aset_ary *)arg;

  ahead = msgpackx_array_node_new(MSGPACKX_ARRAY_HEAD);
  if (!ahead)
    return MSGPACKX_ERR_NOMEM;

  *p->aheadp = ahead;
  *node = msgpackx_array_node_upcast(ahead);
  return MSGPACKX_SUCCESS;
}

static void msgpackx_aset_ary_clean(msgpackx_node *node, void *arg)
{
  msgpackx_array_node *ahead;

  if (!node)
    return;

  /* ((struct msgpackx_aset_ary *)arg)->aheadp is not reliable */
  ahead = msgpackx_node_get_array(node);
  MSGPACKX_ASSERT(ahead && msgpackx_array_node_is_head_of_array(ahead));
  msgpackx_array_node_delete_all(ahead);
}

static struct msgpackx_aset_data *
msgpackx_aset_ary(msgpackx_array_node **aheadp, struct msgpackx_aset_ary *p)
{
  *p = (struct msgpackx_aset_ary){
    .aheadp = aheadp,
    .p =
      {
        .gennode = msgpackx_aset_ary_node,
        .failclean = msgpackx_aset_ary_clean,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_aset_ary(aheadp) \
  msgpackx_aset_ary(aheadp, &(struct msgpackx_aset_ary){{NULL}})

//---- map

struct msgpackx_aset_map
{
  struct msgpackx_aset_data p;
  msgpackx_map_node **mheadp;
};

static msgpackx_error msgpackx_aset_map_node(msgpackx_node **node, void *arg)
{
  msgpackx_map_node *mhead;
  struct msgpackx_aset_map *p;
  p = (struct msgpackx_aset_map *)arg;

  mhead = msgpackx_map_node_new(MSGPACKX_MAP_HEAD);
  if (!mhead)
    return MSGPACKX_ERR_NOMEM;

  *p->mheadp = mhead;
  *node = msgpackx_map_node_upcast(mhead);
  return MSGPACKX_SUCCESS;
}

static void msgpackx_aset_map_clean(msgpackx_node *node, void *arg)
{
  msgpackx_map_node *mhead;

  if (!node)
    return;

  mhead = msgpackx_node_get_map(node);
  MSGPACKX_ASSERT(mhead && msgpackx_map_node_is_head_of_map(mhead));
  msgpackx_map_node_delete_all(mhead);
}

static struct msgpackx_aset_data *msgpackx_aset_map(msgpackx_map_node **mheadp,
                                                    struct msgpackx_aset_map *p)
{
  *p = (struct msgpackx_aset_map){
    .mheadp = mheadp,
    .p =
      {
        .gennode = msgpackx_aset_map_node,
        .failclean = msgpackx_aset_map_clean,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_aset_map(mheadp) \
  msgpackx_aset_map(mheadp, &(struct msgpackx_aset_map){{NULL}})

//---- exary

struct msgpackx_aset_exary
{
  struct msgpackx_aset_data p;
  msgpackx_array_node *ahead;
};

static msgpackx_error msgpackx_aset_exary_node(msgpackx_node **node, void *arg)
{
  struct msgpackx_aset_exary *p;
  p = (struct msgpackx_aset_exary *)arg;

  *node = msgpackx_array_node_upcast(p->ahead);
  return MSGPACKX_SUCCESS;
}

static struct msgpackx_aset_data *
msgpackx_aset_exary(msgpackx_array_node *ahead, struct msgpackx_aset_exary *p)
{
  MSGPACKX_ASSERT(ahead && msgpackx_array_node_is_head_of_array(ahead));
  *p = (struct msgpackx_aset_exary){
    .ahead = ahead,
    .p =
      {
        .gennode = msgpackx_aset_exary_node,
        .failclean = NULL,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_aset_exary(ahead) \
  msgpackx_aset_exary(ahead, &(struct msgpackx_aset_exary){{NULL}})

//---- exmap

struct msgpackx_aset_exmap
{
  struct msgpackx_aset_data p;
  msgpackx_map_node *mhead;
};

static msgpackx_error msgpackx_aset_exmap_node(msgpackx_node **node, void *arg)
{
  struct msgpackx_aset_exmap *p;
  p = (struct msgpackx_aset_exmap *)arg;

  *node = msgpackx_map_node_upcast(p->mhead);
  return MSGPACKX_SUCCESS;
}

static struct msgpackx_aset_data *
msgpackx_aset_exmap(msgpackx_map_node *mhead, struct msgpackx_aset_exmap *p)
{
  MSGPACKX_ASSERT(mhead && msgpackx_map_node_is_head_of_map(mhead));
  *p = (struct msgpackx_aset_exmap){
    .mhead = mhead,
    .p =
      {
        .gennode = msgpackx_aset_exmap_node,
        .failclean = NULL,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_aset_exmap(mhead) \
  msgpackx_aset_exmap(mhead, &(struct msgpackx_aset_exmap){{NULL}})

//---- getters

struct msgpackx_get_data
{
  void (*getter)(msgpackx_node *node, void *arg, msgpackx_error *err);
  void *arg;
};

static void *msgpackx_get_base(msgpackx_node *node, struct msgpackx_get_data *p,
                               msgpackx_error *error)
{
  if (!node) {
    if (error && *error == MSGPACKX_SUCCESS)
      *error = MSGPACKX_ERR_MSG_TYPE;
    return p->arg;
  }

  p->getter(node, p->arg, error);
  return p->arg;
}

#define msgpackx_get_base(node, error, type, ...) \
  msgpackx_get_##type##_value(                    \
    msgpackx_get_base(node, msgpackx_get_##type##_s(__VA_ARGS__), error))

//---- str

struct msgpackx_get_str_data
{
  struct msgpackx_get_data p;
  const char *str;
  ptrdiff_t *len;
};

static const char *msgpackx_get_str_value(void *arg)
{
  return ((struct msgpackx_get_str_data *)arg)->str;
}

static void msgpackx_get_str_getter(msgpackx_node *node, void *arg,
                                    msgpackx_error *error)
{
  struct msgpackx_get_str_data *p;
  p = (struct msgpackx_get_str_data *)arg;
  p->str = msgpackx_node_get_str(node, p->len, error);
}

static struct msgpackx_get_data *
msgpackx_get_str_s(ptrdiff_t *len, struct msgpackx_get_str_data *p)
{
  *p = (struct msgpackx_get_str_data){
    .str = NULL,
    .len = len,
    .p =
      {
        .getter = msgpackx_get_str_getter,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_get_str_s(len) \
  msgpackx_get_str_s(len, &(struct msgpackx_get_str_data){{NULL}})

#define msgpackx_get_str(node, error, len) \
  msgpackx_get_base(node, error, str, len)

//---- bin

struct msgpackx_get_bin_data
{
  struct msgpackx_get_data p;
  const void *data;
  ptrdiff_t *len;
};

static const void *msgpackx_get_bin_value(void *arg)
{
  return ((struct msgpackx_get_bin_data *)arg)->data;
}

static void msgpackx_get_bin_getter(msgpackx_node *node, void *arg,
                                    msgpackx_error *error)
{
  struct msgpackx_get_bin_data *p;
  p = (struct msgpackx_get_bin_data *)arg;
  p->data = msgpackx_node_get_bin(node, p->len, error);
}

static struct msgpackx_get_data *
msgpackx_get_bin_s(ptrdiff_t *len, struct msgpackx_get_bin_data *p)
{
  *p = (struct msgpackx_get_bin_data){
    .data = NULL,
    .len = len,
    .p =
      {
        .getter = msgpackx_get_bin_getter,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_get_bin_s(len) \
  msgpackx_get_bin_s(len, &(struct msgpackx_get_bin_data){{NULL}})

#define msgpackx_get_bin(node, error, len) \
  msgpackx_get_base(node, error, bin, len)

//---- ext

struct msgpackx_get_ext_data
{
  struct msgpackx_get_data p;
  const void *data;
  ptrdiff_t *len;
  int *type;
};

static const void *msgpackx_get_ext_value(void *arg)
{
  return ((struct msgpackx_get_ext_data *)arg)->data;
}

static void msgpackx_get_ext_getter(msgpackx_node *node, void *arg,
                                    msgpackx_error *error)
{
  struct msgpackx_get_ext_data *p;
  p = (struct msgpackx_get_ext_data *)arg;
  p->data = msgpackx_node_get_ext(node, p->len, p->type, error);
}

static struct msgpackx_get_data *
msgpackx_get_ext_s(ptrdiff_t *len, int *type, struct msgpackx_get_ext_data *p)
{
  *p = (struct msgpackx_get_ext_data){
    .data = NULL,
    .len = len,
    .type = type,
    .p =
      {
        .getter = msgpackx_get_ext_getter,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_get_ext_s(len, type) \
  msgpackx_get_ext_s(len, type, &(struct msgpackx_get_ext_data){{NULL}})

#define msgpackx_get_ext(node, error, len, type) \
  msgpackx_get_base(node, error, ext, len, type)

//---- int

struct msgpackx_get_int_data
{
  struct msgpackx_get_data p;
  intmax_t value;
};

static intmax_t msgpackx_get_int_value(void *arg)
{
  return ((struct msgpackx_get_int_data *)arg)->value;
}

static void msgpackx_get_int_getter(msgpackx_node *node, void *arg,
                                    msgpackx_error *error)
{
  struct msgpackx_get_int_data *p;
  p = (struct msgpackx_get_int_data *)arg;
  p->value = msgpackx_node_get_int(node, error);
}

static struct msgpackx_get_data *
msgpackx_get_int_s(struct msgpackx_get_int_data *p)
{
  *p = (struct msgpackx_get_int_data){
    .value = 0,
    .p =
      {
        .getter = msgpackx_get_int_getter,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_get_int_s() \
  msgpackx_get_int_s(&(struct msgpackx_get_int_data){{NULL}})

#define msgpackx_get_int(node, error) \
  msgpackx_get_base(node, error, int, MSGPACKX_EMPTY)

//---- uint

struct msgpackx_get_uint_data
{
  struct msgpackx_get_data p;
  uintmax_t value;
};

static uintmax_t msgpackx_get_uint_value(void *arg)
{
  return ((struct msgpackx_get_uint_data *)arg)->value;
}

static void msgpackx_get_uint_getter(msgpackx_node *node, void *arg,
                                     msgpackx_error *error)
{
  struct msgpackx_get_uint_data *p;
  p = (struct msgpackx_get_uint_data *)arg;
  p->value = msgpackx_node_get_uint(node, error);
}

static struct msgpackx_get_data *
msgpackx_get_uint_s(struct msgpackx_get_uint_data *p)
{
  *p = (struct msgpackx_get_uint_data){
    .value = 0,
    .p =
      {
        .getter = msgpackx_get_uint_getter,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_get_uint_s() \
  msgpackx_get_uint_s(&(struct msgpackx_get_uint_data){{NULL}})

#define msgpackx_get_uint(node, error) \
  msgpackx_get_base(node, error, uint, MSGPACKX_EMPTY)

//---- bool

struct msgpackx_get_bool_data
{
  struct msgpackx_get_data p;
  int value;
};

static int msgpackx_get_bool_value(void *arg)
{
  return ((struct msgpackx_get_bool_data *)arg)->value;
}

static void msgpackx_get_bool_getter(msgpackx_node *node, void *arg,
                                     msgpackx_error *error)
{
  struct msgpackx_get_bool_data *p;
  p = (struct msgpackx_get_bool_data *)arg;
  p->value = msgpackx_node_get_bool(node, error);
}

static struct msgpackx_get_data *
msgpackx_get_bool_s(struct msgpackx_get_bool_data *p)
{
  *p = (struct msgpackx_get_bool_data){
    .value = 0,
    .p =
      {
        .getter = msgpackx_get_bool_getter,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_get_bool_s() \
  msgpackx_get_bool_s(&(struct msgpackx_get_bool_data){{NULL}})

#define msgpackx_get_bool(node, error) \
  msgpackx_get_base(node, error, bool, MSGPACKX_EMPTY)

//---- float

struct msgpackx_get_float_data
{
  struct msgpackx_get_data p;
  float value;
};

static float msgpackx_get_float_value(void *arg)
{
  return ((struct msgpackx_get_float_data *)arg)->value;
}

static void msgpackx_get_float_getter(msgpackx_node *node, void *arg,
                                      msgpackx_error *error)
{
  struct msgpackx_get_float_data *p;
  p = (struct msgpackx_get_float_data *)arg;
  p->value = msgpackx_node_get_float(node, arg);
}

static struct msgpackx_get_data *
msgpackx_get_float_s(struct msgpackx_get_float_data *p)
{
  *p = (struct msgpackx_get_float_data){
    .value = 0.0f,
    .p =
      {
        .getter = msgpackx_get_float_getter,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_get_float_s() \
  msgpackx_get_float_s(&(struct msgpackx_get_float_data){{NULL}})

#define msgpackx_get_float(node, error) \
  msgpackx_get_base(node, error, float, MSGPACKX_EMPTY)

//---- double

struct msgpackx_get_double_data
{
  struct msgpackx_get_data p;
  double value;
};

static double msgpackx_get_double_value(void *arg)
{
  return ((struct msgpackx_get_double_data *)arg)->value;
}

static void msgpackx_get_double_getter(msgpackx_node *node, void *arg,
                                       msgpackx_error *error)
{
  struct msgpackx_get_double_data *p;
  p = (struct msgpackx_get_double_data *)arg;
  p->value = msgpackx_node_get_double(node, error);
}

static struct msgpackx_get_data *
msgpackx_get_double_s(struct msgpackx_get_double_data *p)
{
  *p = (struct msgpackx_get_double_data){
    .value = 0.0,
    .p =
      {
        .getter = msgpackx_get_double_getter,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_get_double_s() \
  msgpackx_get_double_s(&(struct msgpackx_get_double_data){{NULL}})

#define msgpackx_get_double(node, error) \
  msgpackx_get_base(node, error, double, MSGPACKX_EMPTY)

//---- ary

struct msgpackx_get_ary_data
{
  struct msgpackx_get_data p;
  msgpackx_array_node *ahead;
};

static msgpackx_array_node *msgpackx_get_ary_value(void *arg)
{
  return ((struct msgpackx_get_ary_data *)arg)->ahead;
}

static void msgpackx_get_ary_getter(msgpackx_node *node, void *arg,
                                    msgpackx_error *error)
{
  struct msgpackx_get_ary_data *p;
  p = (struct msgpackx_get_ary_data *)arg;
  p->ahead = msgpackx_node_get_array(node);
  if (p->ahead) {
    MSGPACKX_ASSERT(msgpackx_array_node_is_head_of_array(p->ahead));
  } else {
    if (error)
      *error = MSGPACKX_ERR_MSG_TYPE;
  }
}

static struct msgpackx_get_data *
msgpackx_get_ary_s(struct msgpackx_get_ary_data *p)
{
  *p = (struct msgpackx_get_ary_data){
    .ahead = NULL,
    .p =
      {
        .getter = msgpackx_get_ary_getter,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_get_ary_s() \
  msgpackx_get_ary_s(&(struct msgpackx_get_ary_data){{NULL}})

#define msgpackx_get_ary(node, error) \
  msgpackx_get_base(node, error, ary, MSGPACKX_EMPTY)

//---- map

struct msgpackx_get_map_data
{
  struct msgpackx_get_data p;
  msgpackx_map_node *mhead;
};

static msgpackx_map_node *msgpackx_get_map_value(void *arg)
{
  return ((struct msgpackx_get_map_data *)arg)->mhead;
}

static void msgpackx_get_map_getter(msgpackx_node *node, void *arg,
                                    msgpackx_error *error)
{
  struct msgpackx_get_map_data *p;
  p = (struct msgpackx_get_map_data *)arg;
  p->mhead = msgpackx_node_get_map(node);
  if (p->mhead) {
    MSGPACKX_ASSERT(msgpackx_map_node_is_head_of_map(p->mhead));
  } else {
    if (error)
      *error = MSGPACKX_ERR_MSG_TYPE;
  }
}

static struct msgpackx_get_data *
msgpackx_get_map_s(struct msgpackx_get_map_data *p)
{
  *p = (struct msgpackx_get_map_data){
    .mhead = NULL,
    .p =
      {
        .getter = msgpackx_get_map_getter,
        .arg = p,
      },
  };
  return &p->p;
}

#define msgpackx_get_map_s() \
  msgpackx_get_map_s(&(struct msgpackx_get_map_data){{NULL}})

#define msgpackx_get_map(node, error) \
  msgpackx_get_base(node, error, map, MSGPACKX_EMPTY)

//-----------------------------------------------

msgpackx_error msgpackx_node_new_str(msgpackx_node **node, const char *str,
                                     ptrdiff_t len)
{
  return msgpackx_node_new_general(node, msgpackx_set_str(str, len));
}

msgpackx_error msgpackx_node_new_bin(msgpackx_node **node, const void *data,
                                     ptrdiff_t size)
{
  return msgpackx_node_new_general(node, msgpackx_set_bin(data, size));
}

msgpackx_error msgpackx_node_new_ext(msgpackx_node **node, int type, void *data,
                                     ptrdiff_t size)
{
  return msgpackx_node_new_general(node, msgpackx_set_ext(type, data, size));
}

msgpackx_error msgpackx_node_new_int(msgpackx_node **node, intmax_t value)
{
  return msgpackx_node_new_general(node, msgpackx_set_int(value));
}

msgpackx_error msgpackx_node_new_uint(msgpackx_node **node, uintmax_t value)
{
  return msgpackx_node_new_general(node, msgpackx_set_uint(value));
}

msgpackx_error msgpackx_node_new_bool(msgpackx_node **node, int value)
{
  return msgpackx_node_new_general(node, msgpackx_set_bool(value));
}

msgpackx_error msgpackx_node_new_float(msgpackx_node **node, float value)
{
  return msgpackx_node_new_general(node, msgpackx_set_float(value));
}

msgpackx_error msgpackx_node_new_double(msgpackx_node **node, double value)
{
  return msgpackx_node_new_general(node, msgpackx_set_double(value));
}

msgpackx_error msgpackx_node_new_nil(msgpackx_node **node)
{
  return msgpackx_node_new_general(node, msgpackx_set_nil());
}

//---- Array append functions

static msgpackx_error msgpackx_array_append_general(msgpackx_array_node *ahead,
                                                    msgpackx_node *node)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  msgpackx_array_node *anode;

  MSGPACKX_ASSERT(msgpackx_array_node_is_head_of_array(ahead));

  anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
  if (!anode)
    return MSGPACKX_ERR_NOMEM;

  msgpackx_array_node_set_child_node(anode, node);
  msgpackx_array_node_insert_prev(ahead, anode);
  return err;
}

static msgpackx_error msgpackx_array_append_gp(msgpackx_array_node *ahead,
                                               struct msgpackx_aset_data *p)
{
  msgpackx_error err;
  msgpackx_node *node;

  err = msgpackx_aset_node(&node, p);
  if (err != MSGPACKX_SUCCESS)
    return err;

  err = msgpackx_array_append_general(ahead, node);
  if (err != MSGPACKX_SUCCESS)
    msgpackx_aset_clean(node, p);
  return err;
}

msgpackx_error msgpackx_array_append_node(msgpackx_array_node *ahead,
                                          msgpackx_node *node)
{
  return msgpackx_array_append_general(ahead, node);
}

msgpackx_error msgpackx_array_append_str(msgpackx_array_node *ahead,
                                         const char *str, ptrdiff_t len)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_str(str, len));
}

msgpackx_error msgpackx_array_append_bin(msgpackx_array_node *ahead,
                                         const void *data, ptrdiff_t size)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_bin(data, size));
}

msgpackx_error msgpackx_array_append_ext(msgpackx_array_node *ahead, int type,
                                         void *data, ptrdiff_t size)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_ext(type, data, size));
}

msgpackx_error msgpackx_array_append_int(msgpackx_array_node *ahead,
                                         intmax_t value)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_int(value));
}

msgpackx_error msgpackx_array_append_uint(msgpackx_array_node *ahead,
                                          uintmax_t value)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_uint(value));
}

msgpackx_error msgpackx_array_append_bool(msgpackx_array_node *ahead, int value)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_bool(value));
}

msgpackx_error msgpackx_array_append_float(msgpackx_array_node *ahead,
                                           float value)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_float(value));
}

msgpackx_error msgpackx_array_append_double(msgpackx_array_node *ahead,
                                            double value)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_double(value));
}

msgpackx_error msgpackx_array_append_nil(msgpackx_array_node *ahead)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_nil());
}

msgpackx_error msgpackx_array_append_ary(msgpackx_array_node *ahead,
                                         msgpackx_array_node **new_ary_head)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_ary(new_ary_head));
}

msgpackx_error msgpackx_array_append_map(msgpackx_array_node *ahead,
                                         msgpackx_map_node **new_map_head)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_map(new_map_head));
}

msgpackx_error msgpackx_array_append_exary(msgpackx_array_node *ahead,
                                           msgpackx_array_node *ary_head_set)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_exary(ary_head_set));
}

msgpackx_error msgpackx_array_append_exmap(msgpackx_array_node *ahead,
                                           msgpackx_map_node *map_head_set)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_exmap(map_head_set));
}

//---- Array prepend functions

static msgpackx_error msgpackx_array_prepend_general(msgpackx_array_node *ahead,
                                                     msgpackx_node *node)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  msgpackx_array_node *anode;

  MSGPACKX_ASSERT(msgpackx_array_node_is_head_of_array(ahead));

  anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
  if (!anode)
    return MSGPACKX_ERR_NOMEM;

  msgpackx_array_node_set_child_node(anode, node);
  msgpackx_array_node_insert_next(ahead, anode);
  return err;
}

static msgpackx_error msgpackx_array_prepend_gp(msgpackx_array_node *ahead,
                                                struct msgpackx_aset_data *p)
{
  msgpackx_error err;
  msgpackx_node *node;

  err = msgpackx_aset_node(&node, p);
  if (err != MSGPACKX_SUCCESS)
    return err;

  err = msgpackx_array_prepend_general(ahead, node);
  if (err != MSGPACKX_SUCCESS)
    msgpackx_aset_clean(node, p);
  return err;
}

msgpackx_error msgpackx_array_prepend_node(msgpackx_array_node *ahead,
                                           msgpackx_node *node)
{
  return msgpackx_array_prepend_general(ahead, node);
}

msgpackx_error msgpackx_array_prepend_str(msgpackx_array_node *ahead,
                                          const char *str, ptrdiff_t len)
{
  return msgpackx_array_prepend_gp(ahead, msgpackx_aset_str(str, len));
}

msgpackx_error msgpackx_array_prepend_bin(msgpackx_array_node *ahead,
                                          const void *data, ptrdiff_t size)
{
  return msgpackx_array_prepend_gp(ahead, msgpackx_aset_bin(data, size));
}

msgpackx_error msgpackx_array_prepend_ext(msgpackx_array_node *ahead, int type,
                                          void *data, ptrdiff_t size)
{
  return msgpackx_array_prepend_gp(ahead, msgpackx_aset_ext(type, data, size));
}

msgpackx_error msgpackx_array_prepend_int(msgpackx_array_node *ahead,
                                          intmax_t value)
{
  return msgpackx_array_prepend_gp(ahead, msgpackx_aset_int(value));
}

msgpackx_error msgpackx_array_prepend_uint(msgpackx_array_node *ahead,
                                           uintmax_t value)
{
  return msgpackx_array_prepend_gp(ahead, msgpackx_aset_uint(value));
}

msgpackx_error msgpackx_array_prepend_bool(msgpackx_array_node *ahead,
                                           int value)
{
  return msgpackx_array_prepend_gp(ahead, msgpackx_aset_bool(value));
}

msgpackx_error msgpackx_array_prepend_float(msgpackx_array_node *ahead,
                                            float value)
{
  return msgpackx_array_prepend_gp(ahead, msgpackx_aset_float(value));
}

msgpackx_error msgpackx_array_prepend_double(msgpackx_array_node *ahead,
                                             double value)
{
  return msgpackx_array_prepend_gp(ahead, msgpackx_aset_double(value));
}

msgpackx_error msgpackx_array_prepend_nil(msgpackx_array_node *ahead)
{
  return msgpackx_array_prepend_gp(ahead, msgpackx_aset_nil());
}

msgpackx_error msgpackx_array_prepend_ary(msgpackx_array_node *ahead,
                                          msgpackx_array_node **new_ary_head)
{
  return msgpackx_array_prepend_gp(ahead, msgpackx_aset_ary(new_ary_head));
}

msgpackx_error msgpackx_array_prepend_map(msgpackx_array_node *ahead,
                                          msgpackx_map_node **new_map_head)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_map(new_map_head));
}

msgpackx_error msgpackx_array_prepend_exary(msgpackx_array_node *ahead,
                                            msgpackx_array_node *ary_head_set)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_exary(ary_head_set));
}

msgpackx_error msgpackx_array_prepend_exmap(msgpackx_array_node *ahead,
                                            msgpackx_map_node *map_head_set)
{
  return msgpackx_array_append_gp(ahead, msgpackx_aset_exmap(map_head_set));
}

//---- Array insert_prev functions

static msgpackx_error msgpackx_array_iprev_general(msgpackx_array_node *anext,
                                                   msgpackx_node *node)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  msgpackx_array_node *anode;

  MSGPACKX_ASSERT(!msgpackx_array_node_is_head_of_array(anext));

  anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
  if (!anode)
    return MSGPACKX_ERR_NOMEM;

  msgpackx_array_node_set_child_node(anode, node);
  msgpackx_array_node_insert_prev(anext, anode);
  return err;
}

static msgpackx_error msgpackx_array_iprev_gp(msgpackx_array_node *anext,
                                              struct msgpackx_aset_data *p)
{
  msgpackx_error err;
  msgpackx_node *node;

  err = msgpackx_aset_node(&node, p);
  if (err != MSGPACKX_SUCCESS)
    return err;

  err = msgpackx_array_iprev_general(anext, node);
  if (err != MSGPACKX_SUCCESS)
    msgpackx_aset_clean(node, p);
  return err;
}

msgpackx_error msgpackx_array_insert_prev_node(msgpackx_array_node *anode,
                                               msgpackx_node *node)
{
  return msgpackx_array_iprev_general(anode, node);
}

msgpackx_error msgpackx_array_insert_prev_str(msgpackx_array_node *anode,
                                              const char *str, ptrdiff_t len)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_str(str, len));
}

msgpackx_error msgpackx_array_insert_prev_bin(msgpackx_array_node *anode,
                                              const void *data, ptrdiff_t size)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_bin(data, size));
}

msgpackx_error msgpackx_array_insert_prev_ext(msgpackx_array_node *anode,
                                              int type, void *data,
                                              ptrdiff_t size)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_ext(type, data, size));
}

msgpackx_error msgpackx_array_insert_prev_int(msgpackx_array_node *anode,
                                              intmax_t value)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_int(value));
}

msgpackx_error msgpackx_array_insert_prev_uint(msgpackx_array_node *anode,
                                               uintmax_t value)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_uint(value));
}

msgpackx_error msgpackx_array_insert_prev_bool(msgpackx_array_node *anode,
                                               int value)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_bool(value));
}

msgpackx_error msgpackx_array_insert_prev_float(msgpackx_array_node *anode,
                                                float value)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_float(value));
}

msgpackx_error msgpackx_array_insert_prev_double(msgpackx_array_node *anode,
                                                 double value)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_double(value));
}

msgpackx_error msgpackx_array_insert_prev_nil(msgpackx_array_node *anode)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_nil());
}

msgpackx_error
msgpackx_array_insert_prev_ary(msgpackx_array_node *anode,
                               msgpackx_array_node **new_ary_head)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_ary(new_ary_head));
}

msgpackx_error msgpackx_array_insert_prev_map(msgpackx_array_node *anode,
                                              msgpackx_map_node **new_map_head)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_map(new_map_head));
}

msgpackx_error
msgpackx_array_insert_prev_exary(msgpackx_array_node *anode,
                                 msgpackx_array_node *ary_head_set)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_exary(ary_head_set));
}

msgpackx_error msgpackx_array_insert_prev_exmap(msgpackx_array_node *anode,
                                                msgpackx_map_node *map_head_set)
{
  return msgpackx_array_iprev_gp(anode, msgpackx_aset_exmap(map_head_set));
}

//---- Array insert_next functions

static msgpackx_error msgpackx_array_inext_general(msgpackx_array_node *aprev,
                                                   msgpackx_node *node)
{
  msgpackx_error err = MSGPACKX_SUCCESS;
  msgpackx_array_node *anode;

  MSGPACKX_ASSERT(!msgpackx_array_node_is_head_of_array(aprev));

  anode = msgpackx_array_node_new(MSGPACKX_ARRAY_ENTRY);
  if (!anode)
    return MSGPACKX_ERR_NOMEM;

  msgpackx_array_node_set_child_node(anode, node);
  msgpackx_array_node_insert_next(aprev, anode);
  return err;
}

static msgpackx_error msgpackx_array_inext_gp(msgpackx_array_node *aprev,
                                              struct msgpackx_aset_data *p)
{
  msgpackx_error err;
  msgpackx_node *node;

  err = msgpackx_aset_node(&node, p);
  if (err != MSGPACKX_SUCCESS)
    return err;

  err = msgpackx_array_inext_general(aprev, node);
  if (err != MSGPACKX_SUCCESS)
    msgpackx_aset_clean(node, p);
  return err;
}

msgpackx_error msgpackx_array_insert_next_node(msgpackx_array_node *anode,
                                               msgpackx_node *node)
{
  return msgpackx_array_inext_general(anode, node);
}

msgpackx_error msgpackx_array_insert_next_str(msgpackx_array_node *anode,
                                              const char *str, ptrdiff_t len)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_str(str, len));
}

msgpackx_error msgpackx_array_insert_next_bin(msgpackx_array_node *anode,
                                              const void *data, ptrdiff_t size)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_bin(data, size));
}

msgpackx_error msgpackx_array_insert_next_ext(msgpackx_array_node *anode,
                                              int type, void *data,
                                              ptrdiff_t size)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_ext(type, data, size));
}

msgpackx_error msgpackx_array_insert_next_int(msgpackx_array_node *anode,
                                              intmax_t value)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_int(value));
}

msgpackx_error msgpackx_array_insert_next_uint(msgpackx_array_node *anode,
                                               uintmax_t value)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_uint(value));
}

msgpackx_error msgpackx_array_insert_next_bool(msgpackx_array_node *anode,
                                               int value)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_bool(value));
}

msgpackx_error msgpackx_array_insert_next_float(msgpackx_array_node *anode,
                                                float value)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_float(value));
}

msgpackx_error msgpackx_array_insert_next_double(msgpackx_array_node *anode,
                                                 double value)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_double(value));
}

msgpackx_error msgpackx_array_insert_next_nil(msgpackx_array_node *anode)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_nil());
}

msgpackx_error
msgpackx_array_insert_next_ary(msgpackx_array_node *anode,
                               msgpackx_array_node **new_ary_head)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_ary(new_ary_head));
}

msgpackx_error msgpackx_array_insert_next_map(msgpackx_array_node *anode,
                                              msgpackx_map_node **new_map_head)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_map(new_map_head));
}

msgpackx_error
msgpackx_array_insert_next_exary(msgpackx_array_node *anode,
                                 msgpackx_array_node *ary_head_set)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_exary(ary_head_set));
}

msgpackx_error msgpackx_array_insert_next_exmap(msgpackx_array_node *anode,
                                                msgpackx_map_node *map_head_set)
{
  return msgpackx_array_inext_gp(anode, msgpackx_aset_exmap(map_head_set));
}

//---- Array get from node

static msgpackx_node *msgpackx_array_node_get(msgpackx_array_node *anode)
{
  return msgpackx_array_node_get_child_node(anode);
}

const char *msgpackx_array_node_get_str(msgpackx_array_node *anode,
                                        ptrdiff_t *len, msgpackx_error *error)
{
  return msgpackx_get_str(msgpackx_array_node_get(anode), error, len);
}

const void *msgpackx_array_node_get_bin(msgpackx_array_node *anode,
                                        ptrdiff_t *len, msgpackx_error *error)
{
  return msgpackx_get_bin(msgpackx_array_node_get(anode), error, len);
}

const void *msgpackx_array_node_get_ext(msgpackx_array_node *anode,
                                        ptrdiff_t *len, int *type,
                                        msgpackx_error *error)
{
  return msgpackx_get_ext(msgpackx_array_node_get(anode), error, len, type);
}

intmax_t msgpackx_array_node_get_int(msgpackx_array_node *anode,
                                     msgpackx_error *error)
{
  return msgpackx_get_int(msgpackx_array_node_get(anode), error);
}

uintmax_t msgpackx_array_node_get_uint(msgpackx_array_node *anode,
                                       msgpackx_error *error)
{
  return msgpackx_get_uint(msgpackx_array_node_get(anode), error);
}

int msgpackx_array_node_get_bool(msgpackx_array_node *anode,
                                 msgpackx_error *error)
{
  return msgpackx_get_bool(msgpackx_array_node_get(anode), error);
}

float msgpackx_array_node_get_float(msgpackx_array_node *anode,
                                    msgpackx_error *error)
{
  return msgpackx_get_float(msgpackx_array_node_get(anode), error);
}

double msgpackx_array_node_get_double(msgpackx_array_node *anode,
                                      msgpackx_error *error)
{
  return msgpackx_get_double(msgpackx_array_node_get(anode), error);
}

msgpackx_array_node *msgpackx_array_node_get_ary(msgpackx_array_node *anode,
                                                 msgpackx_error *error)
{
  return msgpackx_get_ary(msgpackx_array_node_get(anode), error);
}

msgpackx_map_node *msgpackx_array_node_get_map(msgpackx_array_node *anode,
                                               msgpackx_error *error)
{
  return msgpackx_get_map(msgpackx_array_node_get(anode), error);
}

static int msgpackx_array_node_get_tester(msgpackx_array_node *anode,
                                          int (*tester)(msgpackx_node *n))
{
  msgpackx_node *n = msgpackx_array_node_get(anode);
  if (!n)
    return 0;
  return tester(n);
}

int msgpackx_array_node_is_nil(msgpackx_array_node *anode,
                               msgpackx_error *error)
{
  return msgpackx_array_node_get_tester(anode, msgpackx_node_is_nil);
}

//---- Array node_set functions

static msgpackx_error
msgpackx_array_node_set_general(msgpackx_array_node *anode, msgpackx_node *node)
{
  anode = msgpackx_array_node_set_child_node(anode, node);
  MSGPACKX_ASSERT(anode);
  return MSGPACKX_SUCCESS;
}

static msgpackx_error msgpackx_array_node_set_gp(msgpackx_array_node *anode,
                                                 struct msgpackx_aset_data *p)
{
  msgpackx_error err;
  msgpackx_node *node;

  err = msgpackx_aset_node(&node, p);
  if (err != MSGPACKX_SUCCESS)
    return err;

  err = msgpackx_array_node_set_general(anode, node);
  if (err != MSGPACKX_SUCCESS)
    msgpackx_aset_clean(node, p);
  return err;
}

msgpackx_error msgpackx_array_node_set_node(msgpackx_array_node *anode,
                                            msgpackx_node *node)
{
  return msgpackx_array_node_set_general(anode, node);
}

msgpackx_error msgpackx_array_node_set_str(msgpackx_array_node *anode,
                                           const char *str, ptrdiff_t len)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_str(str, len));
}

msgpackx_error msgpackx_array_node_set_bin(msgpackx_array_node *anode,
                                           const void *data, ptrdiff_t size)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_bin(data, size));
}

msgpackx_error msgpackx_array_node_set_ext(msgpackx_array_node *anode, int type,
                                           void *data, ptrdiff_t size)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_ext(type, data, size));
}

msgpackx_error msgpackx_array_node_set_int(msgpackx_array_node *anode,
                                           intmax_t value)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_int(value));
}

msgpackx_error msgpackx_array_node_set_uint(msgpackx_array_node *anode,
                                            uintmax_t value)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_uint(value));
}

msgpackx_error msgpackx_array_node_set_bool(msgpackx_array_node *anode,
                                            int value)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_bool(value));
}

msgpackx_error msgpackx_array_node_set_nil(msgpackx_array_node *anode)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_nil());
}

msgpackx_error msgpackx_array_node_set_float(msgpackx_array_node *anode,
                                             float value)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_float(value));
}

msgpackx_error msgpackx_array_node_set_double(msgpackx_array_node *anode,
                                              double value)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_double(value));
}

msgpackx_error msgpackx_array_node_set_ary(msgpackx_array_node *anode,
                                           msgpackx_array_node **new_ary_head)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_ary(new_ary_head));
}

msgpackx_error msgpackx_array_node_set_map(msgpackx_array_node *anode,
                                           msgpackx_map_node **new_map_head)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_map(new_map_head));
}

msgpackx_error msgpackx_array_node_set_exary(msgpackx_array_node *anode,
                                             msgpackx_array_node *ary_head_set)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_exary(ary_head_set));
}

msgpackx_error msgpackx_array_node_set_exmap(msgpackx_array_node *anode,
                                             msgpackx_map_node *map_head_set)
{
  return msgpackx_array_node_set_gp(anode, msgpackx_aset_exmap(map_head_set));
}

//---- Array at functions

static msgpackx_array_node *msgpackx_array_at_anode(msgpackx_array_node *ahead,
                                                    ptrdiff_t index,
                                                    msgpackx_error *error)
{
  msgpackx_array_node *anode = NULL;

  MSGPACKX_ASSERT(ahead && msgpackx_array_node_is_head_of_array(ahead));
  if (index >= 0) {
    anode = msgpackx_array_node_next(ahead);
    for (; index > 0 && anode != ahead; --index)
      anode = msgpackx_array_node_next(anode);
  } else {
    anode = msgpackx_array_node_prev(ahead);
    ++index;
    for (; index < 0 && anode != ahead; ++index)
      anode = msgpackx_array_node_prev(anode);
  }
  if (!anode || anode == ahead) {
    if (error)
      *error = MSGPACKX_ERR_INDEX;
    return NULL;
  }
  return anode;
}

msgpackx_node *msgpackx_array_at_node(msgpackx_array_node *ahead,
                                      ptrdiff_t index, msgpackx_error *error)
{
  msgpackx_array_node *anode = msgpackx_array_at_anode(ahead, index, error);
  if (anode)
    return msgpackx_array_node_get_child_node(anode);
  return NULL;
}

const char *msgpackx_array_at_str(msgpackx_array_node *ahead, ptrdiff_t index,
                                  ptrdiff_t *len, msgpackx_error *error)
{
  msgpackx_node *n = msgpackx_array_at_node(ahead, index, error);
  return msgpackx_get_str(n, error, len);
}

const void *msgpackx_array_at_bin(msgpackx_array_node *ahead, ptrdiff_t index,
                                  ptrdiff_t *len, msgpackx_error *error)
{
  msgpackx_node *n = msgpackx_array_at_node(ahead, index, error);
  return msgpackx_get_bin(n, error, len);
}

const void *msgpackx_array_at_ext(msgpackx_array_node *ahead, ptrdiff_t index,
                                  ptrdiff_t *len, int *type,
                                  msgpackx_error *error)
{
  msgpackx_node *n = msgpackx_array_at_node(ahead, index, error);
  return msgpackx_get_ext(n, error, len, type);
}

intmax_t msgpackx_array_at_int(msgpackx_array_node *ahead, ptrdiff_t index,
                               msgpackx_error *error)
{
  return msgpackx_get_int(msgpackx_array_at_node(ahead, index, error), error);
}

uintmax_t msgpackx_array_at_uint(msgpackx_array_node *ahead, ptrdiff_t index,
                                 msgpackx_error *error)
{
  return msgpackx_get_uint(msgpackx_array_at_node(ahead, index, error), error);
}

int msgpackx_array_at_bool(msgpackx_array_node *ahead, ptrdiff_t index,
                           msgpackx_error *error)
{
  return msgpackx_get_bool(msgpackx_array_at_node(ahead, index, error), error);
}

float msgpackx_array_at_float(msgpackx_array_node *ahead, ptrdiff_t index,
                              msgpackx_error *error)
{
  return msgpackx_get_float(msgpackx_array_at_node(ahead, index, error), error);
}

double msgpackx_array_at_double(msgpackx_array_node *ahead, ptrdiff_t index,
                                msgpackx_error *error)
{
  return msgpackx_get_double(msgpackx_array_at_node(ahead, index, error),
                             error);
}

msgpackx_array_node *msgpackx_array_at_ary(msgpackx_array_node *ahead,
                                           ptrdiff_t index,
                                           msgpackx_error *error)
{
  return msgpackx_get_ary(msgpackx_array_at_node(ahead, index, error), error);
}

msgpackx_map_node *msgpackx_array_at_map(msgpackx_array_node *ahead,
                                         ptrdiff_t index, msgpackx_error *error)
{
  return msgpackx_get_map(msgpackx_array_at_node(ahead, index, error), error);
}

//---- Array insert_at functions

static msgpackx_error msgpackx_array_insat_general(msgpackx_array_node *ahead,
                                                   ptrdiff_t index,
                                                   msgpackx_node *node)
{
  msgpackx_array_node *anode;
  msgpackx_error error = MSGPACKX_SUCCESS;
  anode = msgpackx_array_at_anode(ahead, index, &error);
  if (error != MSGPACKX_SUCCESS)
    return error;

  MSGPACKX_ASSERT(anode);
  if (index >= 0)
    return msgpackx_array_iprev_general(anode, node);
  return msgpackx_array_inext_general(anode, node);
}

static msgpackx_error msgpackx_array_insat_gp(msgpackx_array_node *ahead,
                                              ptrdiff_t index,
                                              struct msgpackx_aset_data *p)
{
  msgpackx_error err;
  msgpackx_node *node;

  err = msgpackx_aset_node(&node, p);
  if (err != MSGPACKX_SUCCESS)
    return err;

  err = msgpackx_array_insat_general(ahead, index, node);
  if (err != MSGPACKX_SUCCESS)
    msgpackx_aset_clean(node, p);
  return err;
}

msgpackx_error msgpackx_array_insert_at_node(msgpackx_array_node *ahead,
                                             ptrdiff_t index,
                                             msgpackx_node *node)
{
  return msgpackx_array_insat_general(ahead, index, node);
}

msgpackx_error msgpackx_array_insert_at_str(msgpackx_array_node *ahead,
                                            ptrdiff_t index, const char *str,
                                            ptrdiff_t len)
{
  return msgpackx_array_insat_gp(ahead, index, msgpackx_aset_str(str, len));
}

msgpackx_error msgpackx_array_insert_at_bin(msgpackx_array_node *ahead,
                                            ptrdiff_t index, const void *data,
                                            ptrdiff_t size)
{
  return msgpackx_array_insat_gp(ahead, index, msgpackx_aset_bin(data, size));
}

msgpackx_error msgpackx_array_insert_at_ext(msgpackx_array_node *ahead,
                                            ptrdiff_t index, int type,
                                            void *data, ptrdiff_t size)
{
  return msgpackx_array_insat_gp(ahead, index,
                                 msgpackx_aset_ext(type, data, size));
}

msgpackx_error msgpackx_array_insert_at_int(msgpackx_array_node *ahead,
                                            ptrdiff_t index, intmax_t value)
{
  return msgpackx_array_insat_gp(ahead, index, msgpackx_aset_int(value));
}

msgpackx_error msgpackx_array_insert_at_uint(msgpackx_array_node *ahead,
                                             ptrdiff_t index, uintmax_t value)
{
  return msgpackx_array_insat_gp(ahead, index, msgpackx_aset_uint(value));
}

msgpackx_error msgpackx_array_insert_at_bool(msgpackx_array_node *ahead,
                                             ptrdiff_t index, int value)
{
  return msgpackx_array_insat_gp(ahead, index, msgpackx_aset_bool(value));
}

msgpackx_error msgpackx_array_insert_at_nil(msgpackx_array_node *ahead,
                                            ptrdiff_t index)
{
  return msgpackx_array_insat_gp(ahead, index, msgpackx_aset_nil());
}

msgpackx_error msgpackx_array_insert_at_float(msgpackx_array_node *ahead,
                                              ptrdiff_t index, float value)
{
  return msgpackx_array_insat_gp(ahead, index, msgpackx_aset_float(value));
}

msgpackx_error msgpackx_array_insert_at_double(msgpackx_array_node *ahead,
                                               ptrdiff_t index, double value)
{
  return msgpackx_array_insat_gp(ahead, index, msgpackx_aset_double(value));
}

msgpackx_error msgpackx_array_insert_at_ary(msgpackx_array_node *ahead,
                                            ptrdiff_t index,
                                            msgpackx_array_node **new_ary_head)
{
  return msgpackx_array_insat_gp(ahead, index, msgpackx_aset_ary(new_ary_head));
}

msgpackx_error msgpackx_array_insert_at_map(msgpackx_array_node *ahead,
                                            ptrdiff_t index,
                                            msgpackx_map_node **new_map_head)
{
  return msgpackx_array_insat_gp(ahead, index, msgpackx_aset_map(new_map_head));
}

msgpackx_error msgpackx_array_insert_at_exary(msgpackx_array_node *ahead,
                                              ptrdiff_t index,
                                              msgpackx_array_node *ary_head_set)
{
  return msgpackx_array_insat_gp(ahead, index,
                                 msgpackx_aset_exary(ary_head_set));
}

msgpackx_error msgpackx_array_insert_at_exmap(msgpackx_array_node *ahead,
                                              ptrdiff_t index,
                                              msgpackx_map_node *map_head_set)
{
  return msgpackx_array_insat_gp(ahead, index,
                                 msgpackx_aset_exmap(map_head_set));
}

//---- Array set_at functions

static msgpackx_error msgpackx_array_set_at_general(msgpackx_array_node *ahead,
                                                    ptrdiff_t index,
                                                    msgpackx_node *node)
{
  msgpackx_array_node *anode;
  msgpackx_error error = MSGPACKX_SUCCESS;

  anode = msgpackx_array_at_anode(ahead, index, &error);
  if (error != MSGPACKX_SUCCESS)
    return error;

  MSGPACKX_ASSERT(anode);
  return msgpackx_array_node_set_general(anode, node);
}

static msgpackx_error msgpackx_array_set_at_gp(msgpackx_array_node *ahead,
                                               ptrdiff_t index,
                                               struct msgpackx_aset_data *p)
{
  msgpackx_error error;
  msgpackx_node *node;

  error = msgpackx_aset_node(&node, p);
  if (error != MSGPACKX_SUCCESS)
    return error;

  error = msgpackx_array_set_at_general(ahead, index, node);
  if (error != MSGPACKX_SUCCESS)
    msgpackx_aset_clean(node, p);
  return error;
}

msgpackx_error msgpackx_array_set_at_node(msgpackx_array_node *ahead,
                                          ptrdiff_t index, msgpackx_node *node)
{
  return msgpackx_array_set_at_general(ahead, index, node);
}

msgpackx_error msgpackx_array_set_at_str(msgpackx_array_node *ahead,
                                         ptrdiff_t index, const char *str,
                                         ptrdiff_t len)
{
  return msgpackx_array_set_at_gp(ahead, index, msgpackx_aset_str(str, len));
}

msgpackx_error msgpackx_array_set_at_bin(msgpackx_array_node *ahead,
                                         ptrdiff_t index, const void *data,
                                         ptrdiff_t size)
{
  return msgpackx_array_set_at_gp(ahead, index, msgpackx_aset_bin(data, size));
}

msgpackx_error msgpackx_array_set_at_ext(msgpackx_array_node *ahead,
                                         ptrdiff_t index, int type, void *data,
                                         ptrdiff_t size)
{
  return msgpackx_array_set_at_gp(ahead, index,
                                  msgpackx_aset_ext(type, data, size));
}

msgpackx_error msgpackx_array_set_at_int(msgpackx_array_node *ahead,
                                         ptrdiff_t index, intmax_t value)
{
  return msgpackx_array_set_at_gp(ahead, index, msgpackx_aset_int(value));
}

msgpackx_error msgpackx_array_set_at_uint(msgpackx_array_node *ahead,
                                          ptrdiff_t index, uintmax_t value)
{
  return msgpackx_array_set_at_gp(ahead, index, msgpackx_aset_uint(value));
}

msgpackx_error msgpackx_array_set_at_bool(msgpackx_array_node *ahead,
                                          ptrdiff_t index, int value)
{
  return msgpackx_array_set_at_gp(ahead, index, msgpackx_aset_bool(value));
}

msgpackx_error msgpackx_array_set_at_nil(msgpackx_array_node *ahead,
                                         ptrdiff_t index)
{
  return msgpackx_array_set_at_gp(ahead, index, msgpackx_aset_nil());
}

msgpackx_error msgpackx_array_set_at_float(msgpackx_array_node *ahead,
                                           ptrdiff_t index, float value)
{
  return msgpackx_array_set_at_gp(ahead, index, msgpackx_aset_float(value));
}

msgpackx_error msgpackx_array_set_at_double(msgpackx_array_node *ahead,
                                            ptrdiff_t index, double value)
{
  return msgpackx_array_set_at_gp(ahead, index, msgpackx_aset_double(value));
}

msgpackx_error msgpackx_array_set_at_ary(msgpackx_array_node *ahead,
                                         ptrdiff_t index,
                                         msgpackx_array_node **new_ary_head)
{
  return msgpackx_array_set_at_gp(ahead, index,
                                  msgpackx_aset_ary(new_ary_head));
}

msgpackx_error msgpackx_array_set_at_map(msgpackx_array_node *ahead,
                                         ptrdiff_t index,
                                         msgpackx_map_node **new_map_head)
{
  return msgpackx_array_set_at_gp(ahead, index,
                                  msgpackx_aset_map(new_map_head));
}

msgpackx_error msgpackx_array_set_at_exary(msgpackx_array_node *ahead,
                                           ptrdiff_t index,
                                           msgpackx_array_node *ary_head_set)
{
  return msgpackx_array_set_at_gp(ahead, index,
                                  msgpackx_aset_exary(ary_head_set));
}

msgpackx_error msgpackx_array_set_at_exmap(msgpackx_array_node *ahead,
                                           ptrdiff_t index,
                                           msgpackx_map_node *map_head_set)
{
  return msgpackx_array_set_at_gp(ahead, index,
                                  msgpackx_aset_exmap(map_head_set));
}

//---- Map node value getter

static msgpackx_node *msgpackx_map_node_g(msgpackx_map_node *mnode,
                                          msgpackx_error *error)
{
  if (mnode) {
    MSGPACKX_ASSERT(!msgpackx_map_node_is_head_of_map(mnode));
    return msgpackx_map_node_get_value(mnode);
  }

  if (error && error == MSGPACKX_SUCCESS)
    *error = MSGPACKX_ERR_MSG_TYPE;
  return NULL;
}

msgpackx_node *msgpackx_map_value_get_node(msgpackx_map_node *mnode,
                                           msgpackx_error *error)
{
  return msgpackx_map_node_g(mnode, error);
}

const char *msgpackx_map_value_get_str(msgpackx_map_node *mnode, ptrdiff_t *len,
                                       msgpackx_error *error)
{
  return msgpackx_get_str(msgpackx_map_node_g(mnode, error), error, len);
}

const void *msgpackx_map_value_get_bin(msgpackx_map_node *mnode, ptrdiff_t *len,
                                       msgpackx_error *error)
{
  return msgpackx_get_bin(msgpackx_map_node_g(mnode, error), error, len);
}

const void *msgpackx_map_value_get_ext(msgpackx_map_node *mnode, ptrdiff_t *len,
                                       int *type, msgpackx_error *error)
{
  return msgpackx_get_ext(msgpackx_map_node_g(mnode, error), error, len, type);
}

intmax_t msgpackx_map_value_get_int(msgpackx_map_node *mnode,
                                    msgpackx_error *error)
{
  return msgpackx_get_int(msgpackx_map_node_g(mnode, error), error);
}

uintmax_t msgpackx_map_value_get_uint(msgpackx_map_node *mnode,
                                      msgpackx_error *error)
{
  return msgpackx_get_uint(msgpackx_map_node_g(mnode, error), error);
}

int msgpackx_map_value_get_bool(msgpackx_map_node *mnode, msgpackx_error *error)
{
  return msgpackx_get_bool(msgpackx_map_node_g(mnode, error), error);
}

float msgpackx_map_value_get_float(msgpackx_map_node *mnode,
                                   msgpackx_error *error)
{
  return msgpackx_get_float(msgpackx_map_node_g(mnode, error), error);
}

double msgpackx_map_value_get_double(msgpackx_map_node *mnode,
                                     msgpackx_error *error)
{
  return msgpackx_get_double(msgpackx_map_node_g(mnode, error), error);
}

msgpackx_array_node *msgpackx_map_value_get_ary(msgpackx_map_node *mnode,
                                                msgpackx_error *error)
{
  return msgpackx_get_ary(msgpackx_map_node_g(mnode, error), error);
}

msgpackx_map_node *msgpackx_map_value_get_map(msgpackx_map_node *mnode,
                                              msgpackx_error *error)
{
  return msgpackx_get_map(msgpackx_map_node_g(mnode, error), error);
}

static int msgpackx_map_node_tester(msgpackx_map_node *mnode,
                                    int (*tester)(msgpackx_node *node))
{
  msgpackx_error error;
  msgpackx_node *node;

  node = msgpackx_map_node_g(mnode, &error);
  if (error != MSGPACKX_SUCCESS)
    return 0;

  return tester(node);
}

int msgpackx_map_value_is_nil(msgpackx_map_node *mnode, msgpackx_error *error)
{
  return msgpackx_map_node_tester(mnode, msgpackx_node_is_nil);
}

//---- Map string key getter

static msgpackx_map_node *msgpackx_map_skey_mnode(msgpackx_map_node *mhead,
                                                  const char *keystr,
                                                  ptrdiff_t keylen,
                                                  msgpackx_error *error)
{
  msgpackx_map_node *mnode;

  if (!mhead)
    return NULL;

  MSGPACKX_ASSERT(msgpackx_map_node_is_head_of_map(mhead));
  mnode = msgpackx_map_node_find_by_str(mhead, keystr, keylen, error);
  if (mnode == mhead) {
    if (error)
      *error = MSGPACKX_ERR_KEYNOTFOUND;
    return NULL;
  }
  return mnode;
}

static msgpackx_node *msgpackx_map_skey_g(msgpackx_map_node *mhead,
                                          const char *keystr, ptrdiff_t keylen,
                                          msgpackx_error *error)
{
  msgpackx_map_node *mnode;
  mnode = msgpackx_map_skey_mnode(mhead, keystr, keylen, error);
  return msgpackx_map_node_g(mnode, error);
}

msgpackx_node *msgpackx_map_skey_get_node(msgpackx_map_node *mhead,
                                          const char *keystr, ptrdiff_t keylen,
                                          msgpackx_error *error)
{
  return msgpackx_map_skey_g(mhead, keystr, keylen, error);
}

const char *msgpackx_map_skey_get_str(msgpackx_map_node *mhead,
                                      const char *keystr, ptrdiff_t keylen,
                                      ptrdiff_t *len, msgpackx_error *error)
{
  return msgpackx_get_str(msgpackx_map_skey_g(mhead, keystr, keylen, error),
                          error, len);
}

const void *msgpackx_map_skey_get_bin(msgpackx_map_node *mhead,
                                      const char *keystr, ptrdiff_t keylen,
                                      ptrdiff_t *len, msgpackx_error *error)
{
  return msgpackx_get_bin(msgpackx_map_skey_g(mhead, keystr, keylen, error),
                          error, len);
}

const void *msgpackx_map_skey_get_ext(msgpackx_map_node *mhead,
                                      const char *keystr, ptrdiff_t keylen,
                                      ptrdiff_t *len, int *type,
                                      msgpackx_error *error)
{
  return msgpackx_get_ext(msgpackx_map_skey_g(mhead, keystr, keylen, error),
                          error, len, type);
}

intmax_t msgpackx_map_skey_get_int(msgpackx_map_node *mhead, const char *keystr,
                                   ptrdiff_t keylen, msgpackx_error *error)
{
  return msgpackx_get_int(msgpackx_map_skey_g(mhead, keystr, keylen, error),
                          error);
}

uintmax_t msgpackx_map_skey_get_uint(msgpackx_map_node *mhead,
                                     const char *keystr, ptrdiff_t keylen,
                                     msgpackx_error *error)
{
  return msgpackx_get_uint(msgpackx_map_skey_g(mhead, keystr, keylen, error),
                           error);
}

int msgpackx_map_skey_get_bool(msgpackx_map_node *mhead, const char *keystr,
                               ptrdiff_t keylen, msgpackx_error *error)
{
  return msgpackx_get_bool(msgpackx_map_skey_g(mhead, keystr, keylen, error),
                           error);
}

float msgpackx_map_skey_get_float(msgpackx_map_node *mhead, const char *keystr,
                                  ptrdiff_t keylen, msgpackx_error *error)
{
  return msgpackx_get_float(msgpackx_map_skey_g(mhead, keystr, keylen, error),
                            error);
}

double msgpackx_map_skey_get_double(msgpackx_map_node *mhead,
                                    const char *keystr, ptrdiff_t keylen,
                                    msgpackx_error *error)
{
  return msgpackx_get_double(msgpackx_map_skey_g(mhead, keystr, keylen, error),
                             error);
}

msgpackx_array_node *msgpackx_map_skey_get_ary(msgpackx_map_node *mhead,
                                               const char *keystr,
                                               ptrdiff_t keylen,
                                               msgpackx_error *error)
{
  return msgpackx_get_ary(msgpackx_map_skey_g(mhead, keystr, keylen, error),
                          error);
}

msgpackx_map_node *msgpackx_map_skey_get_map(msgpackx_map_node *mhead,
                                             const char *keystr,
                                             ptrdiff_t keylen,
                                             msgpackx_error *error)
{
  return msgpackx_get_map(msgpackx_map_skey_g(mhead, keystr, keylen, error),
                          error);
}

static int msgpackx_map_skey_tester(msgpackx_map_node *mhead,
                                    const char *keystr, ptrdiff_t keylen,
                                    int (*tester)(msgpackx_node *n))
{
  msgpackx_error error = MSGPACKX_SUCCESS;
  msgpackx_node *node;

  node = msgpackx_map_skey_get_node(mhead, keystr, keylen, &error);
  if (error != MSGPACKX_SUCCESS)
    return 0;

  MSGPACKX_ASSERT(node);
  return tester(node);
}

int msgpackx_map_skey_is_nil(msgpackx_map_node *mhead, const char *keystr,
                             ptrdiff_t keylen, msgpackx_error *error)
{
  return msgpackx_map_skey_tester(mhead, keystr, keylen, msgpackx_node_is_nil);
}

//---- Map string key setter

static msgpackx_error msgpackx_map_key_gp(msgpackx_map_node **mnode,
                                          struct msgpackx_aset_data *p)
{
  msgpackx_error error;
  msgpackx_map_node *mtmp;
  msgpackx_node *node;

  error = msgpackx_aset_node(&node, p);
  if (error != MSGPACKX_SUCCESS)
    return error;

  mtmp = msgpackx_map_node_new(MSGPACKX_MAP_ENTRY);
  if (!mtmp) {
    msgpackx_aset_clean(node, p);
    return MSGPACKX_ERR_NOMEM;
  }

  msgpackx_map_node_set_key(mtmp, node, &error);
  if (error != MSGPACKX_SUCCESS) {
    msgpackx_aset_clean(node, p);
    msgpackx_map_node_delete(mtmp);
    return error;
  }

  *mnode = mtmp;
  return MSGPACKX_SUCCESS;
}

static msgpackx_error msgpackx_map_skey_smnode(msgpackx_map_node *mhead,
                                               const char *keystr,
                                               ptrdiff_t keylen,
                                               msgpackx_node *node)
{
  msgpackx_map_node *mnode;
  msgpackx_error error = MSGPACKX_SUCCESS;

  mnode = msgpackx_map_node_find_by_str(mhead, keystr, keylen, &error);
  if (!mnode) {
    MSGPACKX_ASSERT(error != MSGPACKX_SUCCESS);
    return error;
  }

  if (mnode == mhead) {
    error = msgpackx_map_key_gp(&mnode, msgpackx_aset_str(keystr, keylen));
    if (error != MSGPACKX_SUCCESS)
      return error;

    msgpackx_map_node_insert(mhead, mnode, &error);
    if (error != MSGPACKX_SUCCESS) {
      msgpackx_map_node_delete(mnode);
      return error;
    }
  }

  mnode = msgpackx_map_node_set_value(mnode, node);
  MSGPACKX_ASSERT(mnode);
  return error;
}

static msgpackx_error msgpackx_map_skey_set_gp(msgpackx_map_node *mhead,
                                               const char *keystr,
                                               ptrdiff_t keylen,
                                               struct msgpackx_aset_data *p)
{
  msgpackx_error error;
  msgpackx_node *node;

  error = msgpackx_aset_node(&node, p);
  if (error != MSGPACKX_SUCCESS)
    return error;

  error = msgpackx_map_skey_smnode(mhead, keystr, keylen, node);
  if (error != MSGPACKX_SUCCESS)
    msgpackx_aset_clean(node, p);
  return error;
}

msgpackx_error msgpackx_map_skey_set_node(msgpackx_map_node *mhead,
                                          const char *keystr, ptrdiff_t keylen,
                                          msgpackx_node *node)
{
  return msgpackx_map_skey_smnode(mhead, keystr, keylen, node);
}

msgpackx_error msgpackx_map_skey_set_str(msgpackx_map_node *mhead,
                                         const char *keystr, ptrdiff_t keylen,
                                         const char *str, ptrdiff_t len)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen,
                                  msgpackx_aset_str(str, len));
}

msgpackx_error msgpackx_map_skey_set_bin(msgpackx_map_node *mhead,
                                         const char *keystr, ptrdiff_t keylen,
                                         const void *data, ptrdiff_t size)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen,
                                  msgpackx_aset_bin(data, size));
}

msgpackx_error msgpackx_map_skey_set_ext(msgpackx_map_node *mhead,
                                         const char *keystr, ptrdiff_t keylen,
                                         int type, void *data, ptrdiff_t size)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen,
                                  msgpackx_aset_ext(type, data, size));
}

msgpackx_error msgpackx_map_skey_set_int(msgpackx_map_node *mhead,
                                         const char *keystr, ptrdiff_t keylen,
                                         intmax_t value)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen,
                                  msgpackx_aset_int(value));
}

msgpackx_error msgpackx_map_skey_set_uint(msgpackx_map_node *mhead,
                                          const char *keystr, ptrdiff_t keylen,
                                          uintmax_t value)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen,
                                  msgpackx_aset_uint(value));
}

msgpackx_error msgpackx_map_skey_set_bool(msgpackx_map_node *mhead,
                                          const char *keystr, ptrdiff_t keylen,
                                          int value)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen,
                                  msgpackx_aset_bool(value));
}

msgpackx_error msgpackx_map_skey_set_float(msgpackx_map_node *mhead,
                                           const char *keystr, ptrdiff_t keylen,
                                           float value)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen,
                                  msgpackx_aset_float(value));
}

msgpackx_error msgpackx_map_skey_set_double(msgpackx_map_node *mhead,
                                            const char *keystr,
                                            ptrdiff_t keylen, double value)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen,
                                  msgpackx_aset_double(value));
}

msgpackx_error msgpackx_map_skey_set_ary(msgpackx_map_node *mhead,
                                         const char *keystr, ptrdiff_t keylen,
                                         msgpackx_array_node **new_ary_head)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen,
                                  msgpackx_aset_ary(new_ary_head));
}

msgpackx_error msgpackx_map_skey_set_map(msgpackx_map_node *mhead,
                                         const char *keystr, ptrdiff_t keylen,
                                         msgpackx_map_node **new_map_head)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen,
                                  msgpackx_aset_map(new_map_head));
}

msgpackx_error msgpackx_map_skey_set_exary(msgpackx_map_node *mhead,
                                           const char *keystr, ptrdiff_t keylen,
                                           msgpackx_array_node *ary_head_set)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen,
                                  msgpackx_aset_exary(ary_head_set));
}

msgpackx_error msgpackx_map_skey_set_exmap(msgpackx_map_node *mhead,
                                           const char *keystr, ptrdiff_t keylen,
                                           msgpackx_map_node *map_head_set)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen,
                                  msgpackx_aset_exmap(map_head_set));
}

msgpackx_error msgpackx_map_skey_set_nil(msgpackx_map_node *mhead,
                                         const char *keystr, ptrdiff_t keylen)
{
  return msgpackx_map_skey_set_gp(mhead, keystr, keylen, msgpackx_aset_nil());
}

//---- Map int key getter

static msgpackx_map_node *msgpackx_map_ikey_mnode(msgpackx_map_node *mhead,
                                                  intmax_t keyvalue,
                                                  msgpackx_error *error)
{
  msgpackx_map_node *mnode;

  if (!mhead)
    return NULL;

  MSGPACKX_ASSERT(msgpackx_map_node_is_head_of_map(mhead));
  mnode = msgpackx_map_node_find_by_int(mhead, keyvalue, error);
  if (mnode == mhead) {
    if (error)
      *error = MSGPACKX_ERR_KEYNOTFOUND;
    return NULL;
  }
  return mnode;
}

static msgpackx_node *msgpackx_map_ikey_g(msgpackx_map_node *mhead,
                                          intmax_t keyvalue,
                                          msgpackx_error *error)
{
  msgpackx_map_node *mnode;
  mnode = msgpackx_map_ikey_mnode(mhead, keyvalue, error);
  return msgpackx_map_node_g(mnode, error);
}

msgpackx_node *msgpackx_map_ikey_get_node(msgpackx_map_node *mhead,
                                          intmax_t keyvalue,
                                          msgpackx_error *error)
{
  return msgpackx_map_ikey_g(mhead, keyvalue, error);
}

const char *msgpackx_map_ikey_get_str(msgpackx_map_node *mhead,
                                      intmax_t keyvalue, ptrdiff_t *len,
                                      msgpackx_error *error)
{
  return msgpackx_get_str(msgpackx_map_ikey_g(mhead, keyvalue, error), error,
                          len);
}

const void *msgpackx_map_ikey_get_bin(msgpackx_map_node *mhead,
                                      intmax_t keyvalue, ptrdiff_t *len,
                                      msgpackx_error *error)
{
  return msgpackx_get_bin(msgpackx_map_ikey_g(mhead, keyvalue, error), error,
                          len);
}

const void *msgpackx_map_ikey_get_ext(msgpackx_map_node *mhead,
                                      intmax_t keyvalue, ptrdiff_t *len,
                                      int *type, msgpackx_error *error)
{
  return msgpackx_get_ext(msgpackx_map_ikey_g(mhead, keyvalue, error), error,
                          len, type);
}

intmax_t msgpackx_map_ikey_get_int(msgpackx_map_node *mhead, intmax_t keyvalue,
                                   msgpackx_error *error)
{
  return msgpackx_get_int(msgpackx_map_ikey_g(mhead, keyvalue, error), error);
}

uintmax_t msgpackx_map_ikey_get_uint(msgpackx_map_node *mhead,
                                     intmax_t keyvalue, msgpackx_error *error)
{
  return msgpackx_get_uint(msgpackx_map_ikey_g(mhead, keyvalue, error), error);
}

int msgpackx_map_ikey_get_bool(msgpackx_map_node *mhead, intmax_t keyvalue,
                               msgpackx_error *error)
{
  return msgpackx_get_bool(msgpackx_map_ikey_g(mhead, keyvalue, error), error);
}

float msgpackx_map_ikey_get_float(msgpackx_map_node *mhead, intmax_t keyvalue,
                                  msgpackx_error *error)
{
  return msgpackx_get_float(msgpackx_map_ikey_g(mhead, keyvalue, error), error);
}

double msgpackx_map_ikey_get_double(msgpackx_map_node *mhead, intmax_t keyvalue,
                                    msgpackx_error *error)
{
  return msgpackx_get_double(msgpackx_map_ikey_g(mhead, keyvalue, error),
                             error);
}

msgpackx_array_node *msgpackx_map_ikey_get_ary(msgpackx_map_node *mhead,
                                               intmax_t keyvalue,
                                               msgpackx_error *error)
{
  return msgpackx_get_ary(msgpackx_map_ikey_g(mhead, keyvalue, error), error);
}

msgpackx_map_node *msgpackx_map_ikey_get_map(msgpackx_map_node *mhead,
                                             intmax_t keyvalue,
                                             msgpackx_error *error)
{
  return msgpackx_get_map(msgpackx_map_ikey_g(mhead, keyvalue, error), error);
}

static int msgpackx_map_ikey_tester(msgpackx_map_node *mhead, intmax_t keyvalue,
                                    int (*tester)(msgpackx_node *n))
{
  msgpackx_error error = MSGPACKX_SUCCESS;
  msgpackx_node *node;

  node = msgpackx_map_ikey_get_node(mhead, keyvalue, &error);
  if (error != MSGPACKX_SUCCESS)
    return 0;

  MSGPACKX_ASSERT(node);
  return tester(node);
}

int msgpackx_map_ikey_is_nil(msgpackx_map_node *mhead, intmax_t keyvalue,
                             msgpackx_error *error)
{
  return msgpackx_map_ikey_tester(mhead, keyvalue, msgpackx_node_is_nil);
}

//---- Map int key setter

static msgpackx_error msgpackx_map_ikey_smnode(msgpackx_map_node *mhead,
                                               intmax_t keyvalue,
                                               msgpackx_node *node)
{
  msgpackx_map_node *mnode;
  msgpackx_error error = MSGPACKX_SUCCESS;

  mnode = msgpackx_map_node_find_by_int(mhead, keyvalue, &error);
  if (!mnode) {
    MSGPACKX_ASSERT(error != MSGPACKX_SUCCESS);
    return error;
  }

  if (mnode == mhead) {
    error = msgpackx_map_key_gp(&mnode, msgpackx_aset_int(keyvalue));
    if (error != MSGPACKX_SUCCESS)
      return error;

    msgpackx_map_node_insert(mhead, mnode, &error);
    if (error != MSGPACKX_SUCCESS) {
      msgpackx_map_node_delete(mnode);
      return error;
    }
  }

  mnode = msgpackx_map_node_set_value(mnode, node);
  MSGPACKX_ASSERT(mnode);
  return error;
}

static msgpackx_error msgpackx_map_ikey_set_gp(msgpackx_map_node *mhead,
                                               intmax_t keyvalue,
                                               struct msgpackx_aset_data *p)
{
  msgpackx_error error;
  msgpackx_node *node;

  error = msgpackx_aset_node(&node, p);
  if (error != MSGPACKX_SUCCESS)
    return error;

  error = msgpackx_map_ikey_smnode(mhead, keyvalue, node);
  if (error != MSGPACKX_SUCCESS)
    msgpackx_aset_clean(node, p);
  return error;
}

msgpackx_error msgpackx_map_ikey_set_node(msgpackx_map_node *mhead,
                                          intmax_t keyvalue,
                                          msgpackx_node *node)
{
  return msgpackx_map_ikey_smnode(mhead, keyvalue, node);
}

msgpackx_error msgpackx_map_ikey_set_str(msgpackx_map_node *mhead,
                                         intmax_t keyvalue, const char *str,
                                         ptrdiff_t len)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue, msgpackx_aset_str(str, len));
}

msgpackx_error msgpackx_map_ikey_set_bin(msgpackx_map_node *mhead,
                                         intmax_t keyvalue, const void *data,
                                         ptrdiff_t size)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue,
                                  msgpackx_aset_bin(data, size));
}

msgpackx_error msgpackx_map_ikey_set_ext(msgpackx_map_node *mhead,
                                         intmax_t keyvalue, int type,
                                         void *data, ptrdiff_t size)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue,
                                  msgpackx_aset_ext(type, data, size));
}

msgpackx_error msgpackx_map_ikey_set_int(msgpackx_map_node *mhead,
                                         intmax_t keyvalue, intmax_t value)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue, msgpackx_aset_int(value));
}

msgpackx_error msgpackx_map_ikey_set_uint(msgpackx_map_node *mhead,
                                          intmax_t keyvalue, uintmax_t value)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue, msgpackx_aset_uint(value));
}

msgpackx_error msgpackx_map_ikey_set_bool(msgpackx_map_node *mhead,
                                          intmax_t keyvalue, int value)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue, msgpackx_aset_bool(value));
}

msgpackx_error msgpackx_map_ikey_set_float(msgpackx_map_node *mhead,
                                           intmax_t keyvalue, float value)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue, msgpackx_aset_float(value));
}

msgpackx_error msgpackx_map_ikey_set_double(msgpackx_map_node *mhead,
                                            intmax_t keyvalue, double value)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue, msgpackx_aset_double(value));
}

msgpackx_error msgpackx_map_ikey_set_ary(msgpackx_map_node *mhead,
                                         intmax_t keyvalue,
                                         msgpackx_array_node **new_ary_head)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue,
                                  msgpackx_aset_ary(new_ary_head));
}

msgpackx_error msgpackx_map_ikey_set_map(msgpackx_map_node *mhead,
                                         intmax_t keyvalue,
                                         msgpackx_map_node **new_map_head)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue,
                                  msgpackx_aset_map(new_map_head));
}

msgpackx_error msgpackx_map_ikey_set_exary(msgpackx_map_node *mhead,
                                           intmax_t keyvalue,
                                           msgpackx_array_node *ary_head_set)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue,
                                  msgpackx_aset_exary(ary_head_set));
}

msgpackx_error msgpackx_map_ikey_set_exmap(msgpackx_map_node *mhead,
                                           intmax_t keyvalue,
                                           msgpackx_map_node *map_head_set)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue,
                                  msgpackx_aset_exmap(map_head_set));
}

msgpackx_error msgpackx_map_ikey_set_nil(msgpackx_map_node *mhead,
                                         intmax_t keyvalue)
{
  return msgpackx_map_ikey_set_gp(mhead, keyvalue, msgpackx_aset_nil());
}

//----

msgpackx_array_node *msgpackx_make_header_data(msgpackx_data *data,
                                               const char *title,
                                               msgpackx_array_node **head,
                                               msgpackx_error *err)
{
  msgpackx_node *node;
  msgpackx_array_node *ahead, *anode;
  msgpackx_error xerr;
  int8_t bom[2];
  int i;

  MSGPACKX_ASSERT(data);

#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
  /* forms 0xfeff; Ensures it's in big endian */
  bom[0] = -2;
  bom[1] = -1;
#else
  {
    uint_fast16_t ul16;
    ul16 = 0xfeff;
    if (*(uint8_t *)&ul16 == 0xff) {
      /* forms 0xfffe */
      bom[0] = -1;
      bom[1] = -2;
    } else {
      /* forms 0xfeff */
      bom[0] = -2;
      bom[1] = -1;
    }
  }
#endif

  xerr = MSGPACKX_SUCCESS;
  ahead = msgpackx_array_node_new(MSGPACKX_ARRAY_HEAD);
  if (!ahead) {
    if (err)
      *err = MSGPACKX_ERR_NOMEM;
    return NULL;
  }

  msgpackx_data_set_root_node(data, msgpackx_array_node_upcast(ahead));
  if (head)
    *head = ahead;

  for (i = 0; i < 2; ++i) {
    xerr = msgpackx_array_append_int(ahead, bom[i]);
    if (xerr != MSGPACKX_SUCCESS) {
      if (err)
        *err = xerr;
      return NULL;
    }
  }

  if (title) {
    xerr = msgpackx_array_append_str(ahead, title, strlen(title));
    if (xerr != MSGPACKX_SUCCESS) {
      if (err)
        *err = xerr;
      return NULL;
    }
  }

  xerr = msgpackx_array_append_int(ahead, 0);
  if (xerr != MSGPACKX_SUCCESS) {
    if (err)
      *err = xerr;
    return NULL;
  }

  return msgpackx_array_at_anode(ahead, -1, err);
}

msgpackx_array_node *msgpackx_read_header_data(msgpackx_data *data,
                                               const char *title,
                                               int *need_swap,
                                               msgpackx_error *err)
{
  msgpackx_node *node[4];
  msgpackx_array_node *ahead, *anode, *alast;
  msgpackx_error xerr;
  intmax_t bom[2];
  int swap;
  ptrdiff_t i;

  MSGPACKX_ASSERT(data);

  node[0] = msgpackx_data_root_node(data);
  ahead = msgpackx_node_get_array(node[0]);
  if (!ahead) {
    if (err)
      *err = MSGPACKX_ERR_MSG_TYPE;
    return NULL;
  }

  anode = msgpackx_array_node_next(ahead);
  for (i = 0; i < 2; ++i) {
    if (anode == ahead) {
      if (err)
        *err = MSGPACKX_ERR_HEADER_SIZE;
      return NULL;
    }

    node[i] = msgpackx_array_node_get_child_node(anode);
    anode = msgpackx_array_node_next(anode);
  }
  if (title) {
    if (anode == ahead) {
      if (err)
        *err = MSGPACKX_ERR_HEADER_SIZE;
      return NULL;
    }

    node[2] = msgpackx_array_node_get_child_node(anode);
    anode = msgpackx_array_node_next(anode);
  }
  if (anode == ahead) {
    if (err)
      *err = MSGPACKX_ERR_HEADER_SIZE;
    return NULL;
  }

  alast = msgpackx_array_node_prev(ahead);
  node[3] = msgpackx_array_node_get_child_node(alast);

  alast = msgpackx_array_node_prev(alast);
  if (alast != anode) {
    if (err)
      *err = MSGPACKX_ERR_HEADER_SIZE;
    return NULL;
  }

  xerr = MSGPACKX_SUCCESS;
  bom[0] = msgpackx_node_get_int(node[0], &xerr);
  bom[1] = msgpackx_node_get_int(node[1], &xerr);
  if (xerr != MSGPACKX_SUCCESS ||
      (!((bom[0] == -1 && bom[1] == -2) || (bom[0] == -2 && bom[1] == -1)))) {
    if (err)
      *err = MSGPACKX_ERR_ENDIAN;
    return NULL;
  }

  swap = 0;
  {
    uint_fast16_t u16;
    u16 = 0xfeff;
    if (*((uint8_t *)&u16) == 0xff) {
      if (bom[0] == -2)
        swap = 1;
    } else {
      if (bom[1] == -2)
        swap = 1;
    }
  }
  if (need_swap)
    *need_swap = swap;
#ifdef JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN
  if (bom[0] == -1) { /* Data is little endian */
    if (err)
      *err = MSGPACKX_ERR_ENDIAN;
    return NULL;
  }
  MSGPACKX_ASSERT(swap);
  /*
   * Configured to be use Big endian **with swapping bytes**. In big
   * endian system, you must not define
   * JUPITER_SERIALIZER_SHOULD_SWAP_ENDIAN.
   */
  if (!swap) {
    if (err)
      *err = MSGPACKX_ERR_ENDIAN;
    return NULL;
  }
#else
  if (swap) {
    if (err)
      *err = MSGPACKX_ERR_ENDIAN;
    return NULL;
  }
#endif

  if (!node[3]) {
    if (err)
      *err = MSGPACKX_ERR_EOF;
    return NULL;
  }
  bom[0] = msgpackx_node_get_int(node[3], &xerr);
  if (xerr != MSGPACKX_SUCCESS || bom[0] != 0) {
    if (err)
      *err = MSGPACKX_ERR_EOF;
    return NULL;
  }

  if (title) {
    const char *xtitle;
    ptrdiff_t len;

    if (!node[2]) {
      if (err)
        *err = MSGPACKX_ERR_TITLE;
      return NULL;
    }

    xtitle = msgpackx_node_get_str(node[2], &len, NULL);
    if (!xtitle || len != strlen(title)) {
      if (err)
        *err = MSGPACKX_ERR_TITLE;
      return NULL;
    }

    if (strncmp(title, xtitle, len) != 0) {
      if (err)
        *err = MSGPACKX_ERR_TITLE;
      return NULL;
    }
  }

  return anode;
}
