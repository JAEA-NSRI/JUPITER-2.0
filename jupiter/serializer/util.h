/**
 * High level API for set/get arrays and maps
 *
 * Supports map keys in string or signed (or fixed) integers
 *
 * @note In low level API, any objects can be assigned for map key.
 *
 * These functions are built on top of low level API. Nothing improved on speed
 * than calling low level API.
 *
 * `msgpackx_[OP]_[T]()`:
 *
 * Operations:
 * - `msgpackx_node_new_[T](&node, ...)`
 *   - Allocate new node with given value set. Returns error.
 *
 * - `msgpackx_array_append_[T](ahead, ...)`
 *   - Append given value at last. Returns error.
 *
 * - `msgpackx_array_prepend_[T](ahead, ...)`
 *   - Prepend given value at first. Returns error.
 *
 * - `msgpackx_array_insert_prev_[T](anode, ...)`
 *   - Insert given value at previous of specified anode. Returns error.
 *
 * - `msgpackx_array_insert_next_[T](anode, ...)`
 *   - Insert given value at next of specified anode, Returns error.
 *
 * - `msgpackx_array_node_get_[T](anode, ..., &error)`
 *   - Get value from given specified array node.
 *
 * - `msgpackx_array_node_set_[T](anode, ...)`
 *    - Replace the value of given array node. Returns error.
 *
 * - `msgpackx_array_at_[T](ahead, index, ..., &error)`
 *   - Get value from given index of specified array.
 *     (note: array in msgpackx is more like list, so the cost is O(N))
 *
 * - `msgpackx_array_insert_at_[T](ahead, index, ...)`
 *   - Insert specified value at given index of array. Returns error.
 *
 * - `msgpackx_array_set_at_[T](ahead, index, ...)`
 *   - Replace the value of given index of array. Returns error.
 *
 * - `msgpackx_map_value_get_[T](mnode, ..., &error)`
 *   - Get the value of given map node.
 *
 * - `msgpackx_skey_get_[T](mhead, keystr, keylen, ..., &error)`
 *   - Get the value of string keystr[keylen].
 *
 * - `msgpackx_skey_set_[T](mhead, keystr, keylen, ...)`
 *   - Set or replace the value of string keystr[keylen].
 *
 * - `msgpackx_cskey_get_[T](mhead, keystr, ..., &error)`
 *   - Get the value of null-terminated string keystr.
 *
 * - `msgpackx_cskey_set_[T](mhead, keystr, ...)`
 *   - Set or replace the value of null-terminated string keystr.
 *
 * - `msgpackx_ikey_get_[T](mhead, keyvalue, ..., &error)`
 *   - Get the value of string keystr[keylen].
 *
 * - `msgpackx_ikey_set_[T](mhead, keyvalue, ...)`
 *   - Set or replace the value of string keystr[keylen].
 *
 * Types:
 *   - `node`: Set/Get node
 *     - Get: Return `msgpackx_node *`, No argument
 *     - Set: `msgpackx_node *node`
 *
 *   - `str`: Set/Get specified length string
 *     - Get: Return `const char *`, Argument `ptrdiff_t *len`
 *     - Set: `const char *str, ptrdiff_t len`
 *
 *   - `bin`: Set/Get binary
 *     - Get: Return `const void *`, Argument `ptrdiff_t *len`
 *     - Set: `const void *data, ptrdiff_t size`
 *
 *   - `ext`: Set/Get user-defined type
 *     - Get: Return `const void *`, Argument `ptrdiff_t *len, int *type`
 *     - Set: `int type, const void *data, ptrdiff_t size`
 *
 *   - `int`: Set/Get int type
 *     - Get: Return `intmax_t`, No argument
 *     - Set: `intmax_t value`
 *
 *   - `uint`: Set/Get uint type
 *     - Get: Return `uintmax_t`, No argument
 *     - Set: `uintmax_t value`
 *
 *   - `bool`: Set/Get bool type
 *     - Get: Return `int`, No argument
 *     - Set: `int value`
 *
 *   - `float`: Set/Get float type
 *     - Get: Return `float`, No argument
 *     - Set: `float value`
 *
 *   - `double`: Set/Get double type
 *     - Get: Return `double`, No argument
 *     - Set: `double value`
 *     - Note: No up-conversion performed here. So getting float value
 *       by double functions returns type error.
 *
 *   - `nil`: Sets nil/Tests whether nil
 *     - Set: No argument
 *     - Get: Return 1 if nil, 0 otherwise.
 *     - Note: function convention is `..._is_nil()` rathar than `..._get_nil`
 *
 *   - `ary`: Set new array/Get array
 *     - Get: Return `msgpackx_array_node *`, No argument
 *     - Set: `msgpackx_array_node **ahead`
 *     - Note 1: Returning pointer on get is array head.
 *     - Note 2: Allocates new empty array and set it.
 *
 *   - `map`: Set new map/Get map
 *     - Get: Return `msgpackx_map_node *`, No argument
 *     - Set: `msgpackx_map_node **mhead`
 *     - Note 1: Returning pointer on get is map head.
 *     - Note 2: Allocates new empty map and set it.
 *
 *   - `exary`: Set existing array
 *     - Set: `msgpackx_array_node *ahead`
 *     - Note: Given array should not be a part of any data.
 *
 *   - `exmap`: Set existing map
 *     - Set: `msgpackx_map_node *mhead`
 *     - Note: Given map should not be a part of any data.
 */

#ifndef JUPITER_SERIALIZER_UTIL_H
#define JUPITER_SERIALIZER_UTIL_H

#include "defs.h"

JUPITER_SERIALIZER_DECL_START

#define MSGPACKX_EMPTY

#define MSGPACKX_T_SARG_node(n) , msgpackx_node *n##node
#define MSGPACKX_T_SARG_str(n) , const char *n##str, ptrdiff_t n##len
#define MSGPACKX_T_SARG_bin(n) , const void *n##data, ptrdiff_t n##size
#define MSGPACKX_T_SARG_ext(n) , int n##type, void *n##data, ptrdiff_t n##size
#define MSGPACKX_T_SARG_int(n) , intmax_t n##value
#define MSGPACKX_T_SARG_uint(n) , uintmax_t n##value
#define MSGPACKX_T_SARG_bool(n) , int n##value
#define MSGPACKX_T_SARG_float(n) , float n##value
#define MSGPACKX_T_SARG_double(n) , double n##value
#define MSGPACKX_T_SARG_ary(n) , msgpackx_array_node **new_ary_head##n
#define MSGPACKX_T_SARG_map(n) , msgpackx_map_node **new_map_head##n
#define MSGPACKX_T_SARG_exary(n) , msgpackx_array_node *ary_head_set##n
#define MSGPACKX_T_SARG_exmap(n) , msgpackx_map_node *map_head_set##n
#define MSGPACKX_T_SARG_nil(n)

#define MSGPACKX_T_GARG_node(n)
#define MSGPACKX_T_GARG_str(n) , ptrdiff_t *n##len
#define MSGPACKX_T_GARG_bin(n) , ptrdiff_t *n##len
#define MSGPACKX_T_GARG_ext(n) , ptrdiff_t *n##len, int *n##type
#define MSGPACKX_T_GARG_int(n)
#define MSGPACKX_T_GARG_uint(n)
#define MSGPACKX_T_GARG_bool(n)
#define MSGPACKX_T_GARG_float(n)
#define MSGPACKX_T_GARG_double(n)
#define MSGPACKX_T_GARG_ary(n)
#define MSGPACKX_T_GARG_map(n)

#define MSGPACKX_T_GRET_node msgpackx_node *
#define MSGPACKX_T_GRET_str const char *
#define MSGPACKX_T_GRET_bin const void *
#define MSGPACKX_T_GRET_ext const void *
#define MSGPACKX_T_GRET_int intmax_t
#define MSGPACKX_T_GRET_uint uintmax_t
#define MSGPACKX_T_GRET_bool int
#define MSGPACKX_T_GRET_float float
#define MSGPACKX_T_GRET_double double
#define MSGPACKX_T_GRET_ary msgpackx_array_node *
#define MSGPACKX_T_GRET_map msgpackx_map_node *

#define MSGPACKX_TARG_X(v, type, n) MSGPACKX_T_##v##_##type(n)

#define MSGPACKX_TARG(v, type, n) MSGPACKX_TARG_X(v, type, n)

#define DEFINE_MSGPACKX_SETTER_B(fn, type) \
  msgpackx_error msgpackx_##fn##_##type(   \
    MSGPACKX_ARGS_##fn MSGPACKX_TARG(SARG, type, MSGPACKX_EMPTY))

#define DEFINE_MSGPACKX_GETTER_B(fn, type)                        \
  MSGPACKX_T_GRET_##type msgpackx_##fn##_##type(                  \
    MSGPACKX_ARGS_##fn MSGPACKX_TARG(GARG, type, MSGPACKX_EMPTY), \
    msgpackx_error *error)

#define DEFINE_MSGPACKX_TESTER_B(fn, type) \
  int msgpackx_##fn##_##type(MSGPACKX_ARGS_##fn, msgpackx_error *error)

#define DEFINE_MSGPACKX_PAIR_SETTER_B(fn, t1, t2)   \
  msgpackx_error msgpackx_##fn##_##t2(              \
    MSGPACKX_ARGS_##fn MSGPACKX_TARG(SARG, t1, key) \
      MSGPACKX_TARG(SARG, t2, MSGPACKX_EMPTY))

#define DEFINE_MSGPACKX_PAIR_GETTER_B(fn, t1, t2)                        \
  MSGPACKX_T_GRET_##t2                                                   \
    msgpackx_##fn##_##t2(MSGPACKX_ARGS_##fn MSGPACKX_TARG(SARG, t1, key) \
                           MSGPACKX_TARG(GARG, t2, MSGPACKX_EMPTY),      \
                         msgpackx_error *error)

#define DEFINE_MSGPACKX_PAIR_TESTER_B(fn, t1, t2)                           \
  int msgpackx_##fn##_##t2(MSGPACKX_ARGS_##fn MSGPACKX_TARG(SARG, t1, key), \
                           msgpackx_error *error)

#define DEFINE_MSGPACKX_SETTER(fn, type) \
  JUPITER_SERIALIZER_DECL DEFINE_MSGPACKX_SETTER_B(fn, type)

#define DEFINE_MSGPACKX_GETTER(fn, type) \
  JUPITER_SERIALIZER_DECL DEFINE_MSGPACKX_GETTER_B(fn, type)

#define DEFINE_MSGPACKX_TESTER(fn, type) \
  JUPITER_SERIALIZER_DECL DEFINE_MSGPACKX_TESTER_B(fn, type)

#define DEFINE_MSGPACKX_PAIR_SETTER(fn, t1, t2) \
  JUPITER_SERIALIZER_DECL DEFINE_MSGPACKX_PAIR_SETTER_B(fn, t1, t2)

#define DEFINE_MSGPACKX_PAIR_GETTER(fn, t1, t2) \
  JUPITER_SERIALIZER_DECL DEFINE_MSGPACKX_PAIR_GETTER_B(fn, t1, t2)

#define DEFINE_MSGPACKX_PAIR_TESTER(fn, t1, t2) \
  JUPITER_SERIALIZER_DECL DEFINE_MSGPACKX_PAIR_TESTER_B(fn, t1, t2)

//---- Create new node and set

#define MSGPACKX_ARGS_node_new msgpackx_node **node
DEFINE_MSGPACKX_SETTER(node_new, str);
DEFINE_MSGPACKX_SETTER(node_new, bin);
DEFINE_MSGPACKX_SETTER(node_new, ext);
DEFINE_MSGPACKX_SETTER(node_new, int);
DEFINE_MSGPACKX_SETTER(node_new, uint);
DEFINE_MSGPACKX_SETTER(node_new, bool);
DEFINE_MSGPACKX_SETTER(node_new, float);
DEFINE_MSGPACKX_SETTER(node_new, double);
DEFINE_MSGPACKX_SETTER(node_new, nil);

//---- Array append functions

#define MSGPACKX_ARGS_array_append msgpackx_array_node *ahead
DEFINE_MSGPACKX_SETTER(array_append, node);
DEFINE_MSGPACKX_SETTER(array_append, str);
DEFINE_MSGPACKX_SETTER(array_append, bin);
DEFINE_MSGPACKX_SETTER(array_append, ext);
DEFINE_MSGPACKX_SETTER(array_append, int);
DEFINE_MSGPACKX_SETTER(array_append, uint);
DEFINE_MSGPACKX_SETTER(array_append, bool);
DEFINE_MSGPACKX_SETTER(array_append, float);
DEFINE_MSGPACKX_SETTER(array_append, double);
DEFINE_MSGPACKX_SETTER(array_append, nil);
DEFINE_MSGPACKX_SETTER(array_append, ary);
DEFINE_MSGPACKX_SETTER(array_append, map);
DEFINE_MSGPACKX_SETTER(array_append, exary);
DEFINE_MSGPACKX_SETTER(array_append, exmap);

//---- Array prepend functions

#define MSGPACKX_ARGS_array_prepend msgpackx_array_node *ahead
DEFINE_MSGPACKX_SETTER(array_prepend, node);
DEFINE_MSGPACKX_SETTER(array_prepend, str);
DEFINE_MSGPACKX_SETTER(array_prepend, bin);
DEFINE_MSGPACKX_SETTER(array_prepend, ext);
DEFINE_MSGPACKX_SETTER(array_prepend, int);
DEFINE_MSGPACKX_SETTER(array_prepend, uint);
DEFINE_MSGPACKX_SETTER(array_prepend, bool);
DEFINE_MSGPACKX_SETTER(array_prepend, float);
DEFINE_MSGPACKX_SETTER(array_prepend, double);
DEFINE_MSGPACKX_SETTER(array_prepend, nil);
DEFINE_MSGPACKX_SETTER(array_prepend, ary);
DEFINE_MSGPACKX_SETTER(array_prepend, map);
DEFINE_MSGPACKX_SETTER(array_prepend, exary);
DEFINE_MSGPACKX_SETTER(array_prepend, exmap);

//---- Array insert_prev functions

#define MSGPACKX_ARGS_array_insert_prev msgpackx_array_node *anode
DEFINE_MSGPACKX_SETTER(array_insert_prev, node);
DEFINE_MSGPACKX_SETTER(array_insert_prev, str);
DEFINE_MSGPACKX_SETTER(array_insert_prev, bin);
DEFINE_MSGPACKX_SETTER(array_insert_prev, ext);
DEFINE_MSGPACKX_SETTER(array_insert_prev, int);
DEFINE_MSGPACKX_SETTER(array_insert_prev, uint);
DEFINE_MSGPACKX_SETTER(array_insert_prev, bool);
DEFINE_MSGPACKX_SETTER(array_insert_prev, float);
DEFINE_MSGPACKX_SETTER(array_insert_prev, double);
DEFINE_MSGPACKX_SETTER(array_insert_prev, nil);
DEFINE_MSGPACKX_SETTER(array_insert_prev, ary);
DEFINE_MSGPACKX_SETTER(array_insert_prev, map);
DEFINE_MSGPACKX_SETTER(array_insert_prev, exary);
DEFINE_MSGPACKX_SETTER(array_insert_prev, exmap);

//---- Array insert_next functions

#define MSGPACKX_ARGS_array_insert_next msgpackx_array_node *anode
DEFINE_MSGPACKX_SETTER(array_insert_next, node);
DEFINE_MSGPACKX_SETTER(array_insert_next, str);
DEFINE_MSGPACKX_SETTER(array_insert_next, bin);
DEFINE_MSGPACKX_SETTER(array_insert_next, ext);
DEFINE_MSGPACKX_SETTER(array_insert_next, int);
DEFINE_MSGPACKX_SETTER(array_insert_next, uint);
DEFINE_MSGPACKX_SETTER(array_insert_next, bool);
DEFINE_MSGPACKX_SETTER(array_insert_next, float);
DEFINE_MSGPACKX_SETTER(array_insert_next, double);
DEFINE_MSGPACKX_SETTER(array_insert_next, nil);
DEFINE_MSGPACKX_SETTER(array_insert_next, ary);
DEFINE_MSGPACKX_SETTER(array_insert_next, map);
DEFINE_MSGPACKX_SETTER(array_insert_next, exary);
DEFINE_MSGPACKX_SETTER(array_insert_next, exmap);

//---- Array get from node

#define MSGPACKX_ARGS_array_node_get msgpackx_array_node *anode
DEFINE_MSGPACKX_GETTER(array_node_get, str);
DEFINE_MSGPACKX_GETTER(array_node_get, bin);
DEFINE_MSGPACKX_GETTER(array_node_get, ext);
DEFINE_MSGPACKX_GETTER(array_node_get, int);
DEFINE_MSGPACKX_GETTER(array_node_get, uint);
DEFINE_MSGPACKX_GETTER(array_node_get, bool);
DEFINE_MSGPACKX_GETTER(array_node_get, float);
DEFINE_MSGPACKX_GETTER(array_node_get, double);
DEFINE_MSGPACKX_GETTER(array_node_get, ary);
DEFINE_MSGPACKX_GETTER(array_node_get, map);

#define MSGPACKX_ARGS_array_node_is msgpackx_array_node *anode
DEFINE_MSGPACKX_TESTER(array_node_is, nil);

//---- Array node_set functions

#define MSGPACKX_ARGS_array_node_set msgpackx_array_node *anode
DEFINE_MSGPACKX_SETTER(array_node_set, node);
DEFINE_MSGPACKX_SETTER(array_node_set, str);
DEFINE_MSGPACKX_SETTER(array_node_set, bin);
DEFINE_MSGPACKX_SETTER(array_node_set, ext);
DEFINE_MSGPACKX_SETTER(array_node_set, int);
DEFINE_MSGPACKX_SETTER(array_node_set, uint);
DEFINE_MSGPACKX_SETTER(array_node_set, bool);
DEFINE_MSGPACKX_SETTER(array_node_set, nil);
DEFINE_MSGPACKX_SETTER(array_node_set, float);
DEFINE_MSGPACKX_SETTER(array_node_set, double);
DEFINE_MSGPACKX_SETTER(array_node_set, ary);
DEFINE_MSGPACKX_SETTER(array_node_set, map);
DEFINE_MSGPACKX_SETTER(array_node_set, exary);
DEFINE_MSGPACKX_SETTER(array_node_set, exmap);

//---- Array at functions (Note: Internally, arrays cannot be accessed randomly)

#define MSGPACKX_ARGS_array_at msgpackx_array_node *ahead, ptrdiff_t index
DEFINE_MSGPACKX_GETTER(array_at, node);
DEFINE_MSGPACKX_GETTER(array_at, str);
DEFINE_MSGPACKX_GETTER(array_at, bin);
DEFINE_MSGPACKX_GETTER(array_at, ext);
DEFINE_MSGPACKX_GETTER(array_at, int);
DEFINE_MSGPACKX_GETTER(array_at, uint);
DEFINE_MSGPACKX_GETTER(array_at, bool);
DEFINE_MSGPACKX_GETTER(array_at, float);
DEFINE_MSGPACKX_GETTER(array_at, double);
DEFINE_MSGPACKX_GETTER(array_at, ary);
DEFINE_MSGPACKX_GETTER(array_at, map);

//---- Array insert_at functions

#define MSGPACKX_ARGS_array_insert_at \
  msgpackx_array_node *ahead, ptrdiff_t index
DEFINE_MSGPACKX_SETTER(array_insert_at, node);
DEFINE_MSGPACKX_SETTER(array_insert_at, str);
DEFINE_MSGPACKX_SETTER(array_insert_at, bin);
DEFINE_MSGPACKX_SETTER(array_insert_at, ext);
DEFINE_MSGPACKX_SETTER(array_insert_at, int);
DEFINE_MSGPACKX_SETTER(array_insert_at, uint);
DEFINE_MSGPACKX_SETTER(array_insert_at, bool);
DEFINE_MSGPACKX_SETTER(array_insert_at, nil);
DEFINE_MSGPACKX_SETTER(array_insert_at, float);
DEFINE_MSGPACKX_SETTER(array_insert_at, double);
DEFINE_MSGPACKX_SETTER(array_insert_at, ary);
DEFINE_MSGPACKX_SETTER(array_insert_at, map);
DEFINE_MSGPACKX_SETTER(array_insert_at, exary);
DEFINE_MSGPACKX_SETTER(array_insert_at, exmap);

//---- Array set_at functions

#define MSGPACKX_ARGS_array_set_at msgpackx_array_node *ahead, ptrdiff_t index
DEFINE_MSGPACKX_SETTER(array_set_at, node);
DEFINE_MSGPACKX_SETTER(array_set_at, str);
DEFINE_MSGPACKX_SETTER(array_set_at, bin);
DEFINE_MSGPACKX_SETTER(array_set_at, ext);
DEFINE_MSGPACKX_SETTER(array_set_at, int);
DEFINE_MSGPACKX_SETTER(array_set_at, uint);
DEFINE_MSGPACKX_SETTER(array_set_at, bool);
DEFINE_MSGPACKX_SETTER(array_set_at, nil);
DEFINE_MSGPACKX_SETTER(array_set_at, float);
DEFINE_MSGPACKX_SETTER(array_set_at, double);
DEFINE_MSGPACKX_SETTER(array_set_at, ary);
DEFINE_MSGPACKX_SETTER(array_set_at, map);
DEFINE_MSGPACKX_SETTER(array_set_at, exary);
DEFINE_MSGPACKX_SETTER(array_set_at, exmap);

//---- Map node value getter

#define MSGPACKX_ARGS_map_value_get msgpackx_map_node *mnode
DEFINE_MSGPACKX_GETTER(map_value_get, node);
DEFINE_MSGPACKX_GETTER(map_value_get, str);
DEFINE_MSGPACKX_GETTER(map_value_get, bin);
DEFINE_MSGPACKX_GETTER(map_value_get, ext);
DEFINE_MSGPACKX_GETTER(map_value_get, int);
DEFINE_MSGPACKX_GETTER(map_value_get, uint);
DEFINE_MSGPACKX_GETTER(map_value_get, bool);
DEFINE_MSGPACKX_GETTER(map_value_get, float);
DEFINE_MSGPACKX_GETTER(map_value_get, double);
DEFINE_MSGPACKX_GETTER(map_value_get, ary);
DEFINE_MSGPACKX_GETTER(map_value_get, map);

#define MSGPACKX_ARGS_map_value_is msgpackx_map_node *mnode
DEFINE_MSGPACKX_TESTER(map_value_is, nil);

//---- Map string key getter

#define MSGPACKX_ARGS_map_skey_get msgpackx_map_node *mhead
#define MSGPACKX_ARGS_map_skey_is msgpackx_map_node *mhead
DEFINE_MSGPACKX_PAIR_GETTER(map_skey_get, str, node);
DEFINE_MSGPACKX_PAIR_GETTER(map_skey_get, str, str);
DEFINE_MSGPACKX_PAIR_GETTER(map_skey_get, str, bin);
DEFINE_MSGPACKX_PAIR_GETTER(map_skey_get, str, ext);
DEFINE_MSGPACKX_PAIR_GETTER(map_skey_get, str, int);
DEFINE_MSGPACKX_PAIR_GETTER(map_skey_get, str, uint);
DEFINE_MSGPACKX_PAIR_GETTER(map_skey_get, str, bool);
DEFINE_MSGPACKX_PAIR_GETTER(map_skey_get, str, float);
DEFINE_MSGPACKX_PAIR_GETTER(map_skey_get, str, double);
DEFINE_MSGPACKX_PAIR_GETTER(map_skey_get, str, ary);
DEFINE_MSGPACKX_PAIR_GETTER(map_skey_get, str, map);
DEFINE_MSGPACKX_PAIR_TESTER(map_skey_is, str, nil);

//---- Map string key setter

#define MSGPACKX_ARGS_map_skey_set msgpackx_map_node *mhead
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, node);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, str);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, bin);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, ext);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, int);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, uint);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, bool);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, float);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, double);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, ary);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, map);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, exary);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, exmap);
DEFINE_MSGPACKX_PAIR_SETTER(map_skey_set, str, nil);

//---- Map int key getter

#define MSGPACKX_ARGS_map_ikey_get msgpackx_map_node *mhead
#define MSGPACKX_ARGS_map_ikey_is msgpackx_map_node *mhead
DEFINE_MSGPACKX_PAIR_GETTER(map_ikey_get, int, node);
DEFINE_MSGPACKX_PAIR_GETTER(map_ikey_get, int, str);
DEFINE_MSGPACKX_PAIR_GETTER(map_ikey_get, int, bin);
DEFINE_MSGPACKX_PAIR_GETTER(map_ikey_get, int, ext);
DEFINE_MSGPACKX_PAIR_GETTER(map_ikey_get, int, int);
DEFINE_MSGPACKX_PAIR_GETTER(map_ikey_get, int, uint);
DEFINE_MSGPACKX_PAIR_GETTER(map_ikey_get, int, bool);
DEFINE_MSGPACKX_PAIR_GETTER(map_ikey_get, int, float);
DEFINE_MSGPACKX_PAIR_GETTER(map_ikey_get, int, double);
DEFINE_MSGPACKX_PAIR_GETTER(map_ikey_get, int, ary);
DEFINE_MSGPACKX_PAIR_GETTER(map_ikey_get, int, map);
DEFINE_MSGPACKX_PAIR_TESTER(map_ikey_is, int, nil);

//---- Map int key setter

#define MSGPACKX_ARGS_map_ikey_set msgpackx_map_node *mhead
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, node);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, str);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, bin);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, ext);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, int);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, uint);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, bool);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, float);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, double);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, ary);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, map);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, exary);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, exmap);
DEFINE_MSGPACKX_PAIR_SETTER(map_ikey_set, int, nil);

//---- Generating composite data

/**
 * @memberof msgpack_data
 * @brief Generates common header data for writing.
 * @param data Data to set to
 * @param title Title of data
 * @param head Sets array head of returned node if given
 * @param err Error information will be set if given
 * @return Returns array node that points EOF mark, NULL if failed
 *
 * @p title can be NULL. If so, writing title is omited.
 *
 * Generates array of 4 (3 if title is not given) elements of
 * following contents:
 *
 *   1. -2 (0xfe) for using big endian, -1 (0xff) for using little endian
 *   2. -1 (0xff) for using big endian, -2 (0xfe) for using little endian
 *   3. Title (string), if given
 *      (user data should be inserted here)
 *   4. 0 (for EOF mark)
 *
 * Because The number of array elements (3, 4 or 5) is tested when
 * reading, the user data must be nested if it is an array.
 *
 * @note @p title must not include information which cannot be known
 *       unless reading the content, because it is checked.
 */
JUPITER_SERIALIZER_DECL
msgpackx_array_node *msgpackx_make_header_data(msgpackx_data *data,
                                               const char *title,
                                               msgpackx_array_node **head,
                                               msgpackx_error *err);

/**
 * @memberof msgpack_data
 * @brief Parse common header dat
 * @param data Data to set to
 * @param title Title of data
 * @param need_swap sets 1 if byteswap should be performed, if given
 * @param err Error information will be set if given
 * @return Returns array node that points the user data, NULL if not
 *         found, or failed.
 *
 * @p title can be NULL. If so, the data assumed the title is not
 * written.
 */
JUPITER_SERIALIZER_DECL
msgpackx_array_node *msgpackx_read_header_data(msgpackx_data *data,
                                               const char *title,
                                               int *need_swap,
                                               msgpackx_error *err);


//---- misc

/**
 * Expand string literal for str() setter.
 */
#define MCSTR(lit) ("" lit), strlen("" lit)

JUPITER_SERIALIZER_DECL_END

#endif
