
#ifndef JUPITER_SERIALIZER_MSGPACKX_H
#define JUPITER_SERIALIZER_MSGPACKX_H

#include <stdio.h>
#include <stdint.h>

#include "defs.h"

JUPITER_SERIALIZER_DECL_START

/**
 * @memberof msgpackx_data
 * @brief Create New Serializer Data
 */
JUPITER_SERIALIZER_DECL
msgpackx_data *msgpackx_data_new(void);

/**
 * @memberof msgpackx_data
 * @brief Delete Serializer Data
 * @param data Serializer data to delete
 */
JUPITER_SERIALIZER_DECL
void msgpackx_data_delete(msgpackx_data *data);

/**
 * @memberof msgpackx_data
 * @brief Parse memory as serializer data
 * @param buffer Buffer to parse
 * @param err Sets any errors if occured
 * @param eloc Location (byte offset) of error occured.
 * @return parsed data, or NULL if failed
 *
 * The memory region where @p buffer points will be shared with
 * returned data.
 */
JUPITER_SERIALIZER_DECL
msgpackx_data *msgpackx_data_parse(msgpackx_buffer *buffer, msgpackx_error *err,
                                   ptrdiff_t *eloc);

/**
 * @bmemberof msgpackx_data
 * @brief Parse stream as serializer data
 * @param fp File pointer to parse
 * @param err Sets any erros if occured
 * @param eloc Location (byte offset) of error occured.
 *
 * Currently, @p fp would be read until EOF.
 */
JUPITER_SERIALIZER_DECL
msgpackx_data *msgpackx_data_read(FILE *fp, msgpackx_error *err,
                                  ptrdiff_t *eloc);

/**
 * @memberof msgpackx_data
 * @brief Write data to stream
 * @param data Data to write
 * @param stream Stream to write to
 * @return Number of bytes written, negative value if failed.
 */
JUPITER_SERIALIZER_DECL
ptrdiff_t msgpackx_data_write(msgpackx_data *data, FILE *stream);

/**
 * @memberof msgpackx_data
 * @brief Write (already packed) data to stream
 * @param data Data to write
 * @param stream Stream to write to
 * @return Number of bytes written, negative value if failed.
 *
 * msgpackx_data_write() does not pack the data into a single buffer,
 * and does not require such condition (can write sparse data).
 *
 * But if you can assume the data is packed, for example, parsed data
 * with msgpackx_data_parse() (we use given buffer itself, its
 * equivalent to packed data), received or sent data by MPI (these
 * operations pack data implicitly), and not modified, this is faster
 * than msgpackx_data_write().
 *
 * If you can not assume the data is packed, msgpackx_data_write() is
 * usually faster than calling msgpackx_data_pack() and
 * msgpackx_data_write_packed().
 *
 * If you are going to write the same data twice or more, a single
 * call of msgpackx_data_pack() and call msgpackx_data_write_packed()
 * for required numbers may faster than multiple calls of
 * msgpackx_data_write(), because msgpackx_data_write() traverses all
 * data recursively.
 */
JUPITER_SERIALIZER_DECL
ptrdiff_t msgpackx_data_write_packed(msgpackx_data *data, FILE *stream);

/**
 * @memberof msgpackx_data
 * @brief Get root node of the data
 * @param data Data to get from.
 * @return root node, NULL if not set.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_data_root_node(msgpackx_data *data);

/**
 * @memberof msgpackx_data
 * @brief Set new root node to the data
 * @param node New node to set.
 *
 * Old data will be deleted.
 *
 * NULL is acceptable value for @p node, then this function will delete
 * the data.
 */
JUPITER_SERIALIZER_DECL
void msgpackx_data_set_root_node(msgpackx_data *data, msgpackx_node *node);

/**
 * @memberof msgpackx_data
 * @brief Pack data into a single streamable buffer.
 * @param data Data to pack.
 * @param err Sets errror information into given pointer, if an error occured
 * @return @p data if success, NULL if failed.
 */
JUPITER_SERIALIZER_DECL
msgpackx_data *msgpackx_data_pack(msgpackx_data *data, msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Create a new serializer node.
 * @return Created node, NULL if failed.
 *
 * Created node should be treat as 'child'. Returned node can be child
 * of data, array node, or map node.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_node_new(void);

/**
 * @memberof msgpackx_node
 * @brief Get the parent node
 * @param node Node to obtain
 * @return The parent node, NULL if does not exist (i.e., root or dangling)
 *
 * If @p node is array node, array head, map node or map head, this
 * function returns the parent of array head or map head respectively.
 *
 * However, we recommend to use this function for traversing upwards of the
 * 'leaf' child node. Use `msgpackx_map_node_get_parent` or
 * `msgpackx_array_node_get_parent` for maps and arrays.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_node_get_parent(msgpackx_node *node);

/**
 * @memberof msgpackx_node
 * @brief Returns whether the root of node
 * @param node Node to be tested
 * @return 1 if given node is parent of root, 0 otherwise
 *
 * @note `msgpackx_node_is_root(msgpackx_data_get_root_node(data))` will return
 *       false.
 *
 * @note This function returns 0 for dangling node (aka. no parent other).
 */
JUPITER_SERIALIZER_DECL
int msgpackx_node_is_root(msgpackx_node *node);

/**
 * @memberof msgpackx_node
 * @brief Returns overall root data of the node
 * @param node Node to obtain
 * @return pointer to data, NULL if no data exists in parents.
 *
 * This function travarses node for ancestors to reach the root data.
 */
JUPITER_SERIALIZER_DECL
msgpackx_data *msgpackx_node_get_data(msgpackx_node *node);

/**
 * @memberof msgpackx_node
 * @brief Set integer value to a node
 * @param node Node to set.
 * @param value Value to set.
 * @param err Set error information if given, and an error occured
 * @return @p node if success, NULL if failed.
 *
 * @p node must be a 'child'.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_node_set_int(msgpackx_node *node, intmax_t value,
                                     msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Set unsigned integer value to a node
 * @param node Node to set.
 * @param value Value to set.
 * @param err Set error information if given, and an error occured
 * @return @p node if success, NULL if failed.
 *
 * @p node must be a 'child'.
 *
 * If the value is between 0 and 127, the sign information will
 * lose. These values can be read with msgpackx_node_get_int(), and it
 * does not raise type mismatch error.
 *
 * Contrast to this, if the value is larger than 127, the sign
 * information will keep. These values can not be read with
 * msgpackx_node_get_int(), and raises type mismatch error, even if it
 * fits to an int(max_t) type.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_node_set_uint(msgpackx_node *node, uintmax_t value,
                                      msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Set string value to a node
 * @param node Node to set.
 * @param str Value to set.
 * @param len Length of the string (in bytes).
 * @param err Set error information if given, and an error occured
 * @return @p node if success, NULL if failed.
 *
 * @p node must be a 'child'.
 *
 * The content rule (such as encoding, treatment of NUL charactor,
 * etc.) of @p str is not defined. Usually, you should make @p str in
 * UTF-8 encoding (this implies ASCII). NUL charactor is allowed as
 * part of string, it does not mean end of the string.
 *
 * If @p len is negative, this function computes the length of @p str
 * with strlen(), but you should explicitly specify the length.
 * Especially, if you give @p str with a literal, you should specify
 * calling strlen() with it for @p len, because the compiler may
 * optimize strlen() into a integral literal, without counting number
 * of charactors (actually, bytes) by yourself.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_node_set_str(msgpackx_node *node, const char *str,
                                     ptrdiff_t len, msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Set binary value to a node
 * @param node Node to set.
 * @param data Value to set.
 * @param size Length of the binary (in bytes).
 * @param err Set error information if given, and an error occured
 * @return @p node if success, NULL if failed.
 *
 * @p node must be a 'child'.
 *
 * @p size must be positive.
 *
 * String function msgpackx_node_set_str() can not store a string
 * longer than \f$ 2^32-1 \f$ bytes, but msgpackx_node_set_bin() can.
 *
 * @note While this function supports the very large data, but this
 *       function copies it, because of memory maintainance
 *       requirement. We may want to implement set binary function
 *       with shared buffer system, ::msgpackx_buffer, which does not
 *       require copying data.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_node_set_bin(msgpackx_node *node, const void *data,
                                     ptrdiff_t size, msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Set extended typed data to a node
 * @param node Node to set.
 * @param type type of the data.
 * @param data Value to set.
 * @param size Length of the binary (in bytes).
 * @param err Set error information if given, and an error occured
 * @return @p node if success, NULL if failed.
 *
 * @p node must be a 'child'.
 *
 * @p size must be positive.
 *
 * @p type must be between -128 and 127. Negitive value of @p type is
 * reserved (but nothing implemented currently). Positive values can
 * be used with programmers' choice.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_node_set_ext(msgpackx_node *node, int type, void *data,
                                     ptrdiff_t size, msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Set nil to a node
 * @param node Node to set.
 * @param err Set error information if given, and an error occured
 * @return @p node if success, NULL if failed.
 *
 * @p node must be a 'child'.
 *
 * This is different to setting someone's child to NULL.
 *
 * You need a node with a nil for indicating no data, but exists
 * entry.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_node_set_nil(msgpackx_node *node, msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Set boolean to a node
 * @param node Node to set.
 * @param value 0 for false, non-0 for true.
 * @param err Set error information if given, and an error occured
 * @return @p node if success, NULL if failed.
 *
 * @p node must be a 'child'.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_node_set_bool(msgpackx_node *node, int value,
                                      msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Set float value to a node
 * @param node Node to set.
 * @param flt A float value.
 * @param err Set error information if given, and an error occured
 * @return @p node if success, NULL if failed.
 *
 * @p node must be a 'child'.
 *
 * For the in-memory format of float, IEEE754 binary float32 is
 * assumed (cannot be checked).
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_node_set_float(msgpackx_node *node, float flt,
                                       msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Set double value to a node
 * @param node Node to set.
 * @param dbl A double value.
 * @param err Set error information if given, and an error occured
 * @return @p node if success, NULL if failed.
 *
 * @p node must be a 'child'.
 *
 * For the in-memory format of double, IEEE754 binary float64 is
 * assumed (cannot be checked).
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_node_set_double(msgpackx_node *node, double dbl,
                                        msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Get pointer of node's buffer
 * @param node Node to get from
 * @param size_out If given, set size of the buffer.
 * @return the pointer, NULL if not set or invalid.
 *
 * If @p node is a data, array head or map head, returns the size of
 * whole of its data, array or map, but requires packed. If @p node is
 * a data, array head or map head and not packed, the retured pointer
 * will not point whole buffer and size value is invalid (but still
 * indicates the size of the available buffer now, so it will not be 0
 * or negative).
 */
JUPITER_SERIALIZER_DECL
const void *msgpackx_node_get_data_pointer(msgpackx_node *node,
                                           ptrdiff_t *size_out);

/**
 * @memberof msgpackx_node
 * @brief Get copy of node's buffer
 * @param node Node to get from.
 * @return the pointer, NULL if allocation falied.
 *
 * The retured buffer is not shared with serialized data (Do deep copy).
 *
 * If @p node is a data, array head or map head and not packed, the
 * retured pointer will not point whole buffer.
 */
JUPITER_SERIALIZER_DECL
msgpackx_buffer *msgpackx_node_get_data_copy(msgpackx_node *node);

/**
 * @memberof msgpackx_node
 * @brief Delete a node
 * @param node Node to delete.
 *
 * @p node must be a 'child'.
 *
 * If @p node has a parent, disconnect from it.
 */
JUPITER_SERIALIZER_DECL
void msgpackx_node_delete(msgpackx_node *node);

/**
 * @memberof msgpackx_node
 * @brief Downcast node to an array.
 * @param node node to get from
 * @return array node if node is an array, NULL otherwise.
 *
 * If @p node is someone's child, returned pointer will be an array
 * head.
 *
 * If @p node is someone's parent, returned pointer will be an array
 * node.
 *
 * @sa msgpackx_array_node_upcast()
 */
JUPITER_SERIALIZER_DECL
msgpackx_array_node *msgpackx_node_get_array(msgpackx_node *node);

/**
 * @memberof msgpackx_node
 * @brief Downcast node to a map.
 * @param node node to get from
 * @return map node if node is a map, NULL otherwise.
 *
 * If @p node is someone's child, returned pointer will be a map head.
 *
 * If @p node is someone's parent, returned pointer will be a map
 * node.
 *
 * @sa msgpackx_map_node_upcast()
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_node_get_map(msgpackx_node *node);

/**
 * @memberof msgpackx_node
 * @brief Get int value from node.
 * @param node node to get from
 * @param err when any errors occured, set its information to here
 * @return Node's int value, 0 if error occured.
 *
 * If node's value is between 0 and 127, this function will success
 * even if msgpackx_node_set_uint() were used when set that value,
 * because it loses sign information for these values.
 *
 * @note The return value 0 does not mean that any error occured. If
 *       you need to know that the node is properly has int value, you
 *       must give @p err and check it.
 *
 * @sa msgpackx_node_set_int()
 */
JUPITER_SERIALIZER_DECL
intmax_t msgpackx_node_get_int(msgpackx_node *node, msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Get unsigned int value from node.
 * @param node node to get from
 * @param err when any errors occured, set its information to here
 * @return Node's int value, 0 if error occured.
 *
 * If node's value is between 0 and 127, this function will success
 * even if msgpackx_node_set_int() were used when set that value,
 * because it loses sign information for these values.
 *
 * @note The return value 0 does not mean that any error occured. If
 *       you need to know that the node is properly has uint value,
 *       you must give @p err and check it.
 *
 * @sa msgpackx_node_set_uint()
 */
JUPITER_SERIALIZER_DECL
uintmax_t msgpackx_node_get_uint(msgpackx_node *node, msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Get string data from node.
 * @param node node to get from
 * @param len Length output
 * @param err when any errors occured, set its information to here
 * @return pointer to string, NULL if error occured.
 *
 * The returned pointer is part of serialized buffer.
 *
 * @warning The returned pointer will **not** be end with
 *          NUL-charactor (`'\0'`).
 *
 * @sa msgpackx_node_set_str()
 */
JUPITER_SERIALIZER_DECL
const char *msgpackx_node_get_str(msgpackx_node *node, ptrdiff_t *len,
                                  msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Get binary data from node.
 * @param node node to get from
 * @param len Length output
 * @param err when any errors occured, set its information to here
 * @return pointer to data, NULL if error occured.
 *
 * The returned pointer is part of serialized buffer.
 *
 * @sa msgpackx_node_set_bin()
 */
JUPITER_SERIALIZER_DECL
const void *msgpackx_node_get_bin(msgpackx_node *node, ptrdiff_t *len,
                                  msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Get extended data from node.
 * @param node node to get from
 * @param len Length output
 * @param type Type output
 * @param err when any errors occured, set its information to here
 * @return pointer to data, NULL if error occured.
 *
 * The returned pointer is part of serialized buffer.
 *
 * @sa msgpackx_node_set_ext()
 */
JUPITER_SERIALIZER_DECL
const void *msgpackx_node_get_ext(msgpackx_node *node, ptrdiff_t *len,
                                  int *type, msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Get float32 data from node.
 * @param node node to get from
 * @param err when any errors occured, set its information to here
 * @return the value, or HUGE_VALF if error occured.
 *
 * @note The return value HUGE_VALF does not mean that any error
 *       occured. If you need to know that the node is properly has
 *       float value, you must give @p err and check it.
 *
 * @sa msgpackx_node_set_float()
 */
JUPITER_SERIALIZER_DECL
float msgpackx_node_get_float(msgpackx_node *node, msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Get float32 data from node.
 * @param node node to get from
 * @param err when any errors occured, set its information to here
 * @return the value, or HUGE_VALF if error occured.
 *
 * @note The return value HUGE_VAL does not mean that any error
 *       occured. If you need to know that the node is properly has
 *       float value, you must give @p err and check it.
 *
 * @sa msgpackx_node_set_double()
 */
JUPITER_SERIALIZER_DECL
double msgpackx_node_get_double(msgpackx_node *node, msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Get bool data from node.
 * @param node node to get from
 * @param err when any errors occured, set its information to here
 * @return 0 if false (or error occured), 1 if true
 *
 * @note The return value 0 does not mean that any error occured. If
 *       you need to know that the node is properly has bool value,
 *       you must give @p err and check it.
 *
 * @sa msgpackx_node_set_bool()
 */
JUPITER_SERIALIZER_DECL
int msgpackx_node_get_bool(msgpackx_node *node, msgpackx_error *err);

/**
 * @memberof msgpackx_node
 * @brief Test whether node is nil or not
 * @param node node to get from
 * @retval  0 the given @p node is not nil
 * @retval  1 the given @p node is nil
 * @retval -1 an error occured (node data is not set).
 *
 * @sa msgpackx_node_set_nil()
 */
JUPITER_SERIALIZER_DECL
int msgpackx_node_is_nil(msgpackx_node *node);

/**
 * @memberof msgpackx_array_node
 * @brief Create new array node
 * @param type Node type of array (head or node)
 * @return created node or NULL if allocation failed.
 */
JUPITER_SERIALIZER_DECL
msgpackx_array_node *
msgpackx_array_node_new(enum msgpackx_array_node_type type);

/**
 * @memberof msgpackx_array_node
 * @brief Deletes an array node (and its children)
 * @param node Array node to delete
 *
 * @p node must not be a head.
 */
JUPITER_SERIALIZER_DECL
void msgpackx_array_node_delete(msgpackx_array_node *node);

/**
 * @memberof msgpackx_array_node
 * @brief Deletes an array head and its nodes (and their children)
 * @param node Array head to delete
 *
 * @p head should be an array head. It's ok that @p head is an array
 * node, but strongly recommended.
 */
JUPITER_SERIALIZER_DECL
void msgpackx_array_node_delete_all(msgpackx_array_node *head);

/**
 * @memberof msgpackx_array_node
 * @brief Next array node of the given node
 * @param node Array node (or head) to get
 * @return the next node of @p node.
 *
 * If @p node is the last node of the array, this function returns the
 * array head.
 *
 * If @p node is the array head, this function returns the first node
 * of the array.
 *
 * If returned pointer is equal to @p node, no other nodes is present
 * in the array, or the array is empty if @p node is the array head.
 */
JUPITER_SERIALIZER_DECL
msgpackx_array_node *msgpackx_array_node_next(msgpackx_array_node *node);

/**
 * @memberof msgpackx_array_node
 * @brief Previous array node of the given node
 * @param node Array node (or head) to get
 * @return the previous node of @p node.
 *
 * If @p node is the first node of the array, this function returns
 * the array head.
 *
 * If @p node is the array head, this function returns the last node
 * of the array.
 *
 * If returned pointer is equal to @p node, no other nodes is present
 * in the array, or the array is empty if @p node is the array head.
 */
JUPITER_SERIALIZER_DECL
msgpackx_array_node *msgpackx_array_node_prev(msgpackx_array_node *node);

/**
 * @memberof msgpackx_array_node
 * @brief Insert an array node into next of another node
 * @param prev The array node (or head) that becomes previous of the new node
 * @param ins An array node to insert.
 *
 * Insert the new array node between the @p prev and @p prev 's next.
 *
 *     --->[prev]<------->[prev's next]<---
 *                   ^
 *                   '-- Inserts the new node here.
 */
JUPITER_SERIALIZER_DECL
msgpackx_array_node *msgpackx_array_node_insert_next(msgpackx_array_node *prev,
                                                     msgpackx_array_node *ins);

/**
 * @memberof msgpackx_array_node
 * @brief Insert an array node into previous of another node
 * @param next The array node (or head) that becomes next of the new node
 * @param ins An array node to insert.
 *
 * Insert the new array node between the @p next and @p next 's previous.
 *
 *         --->[next's previous]<------->[next]<---
 *                                  ^
 *     Inserts the new node here. --'
 */
JUPITER_SERIALIZER_DECL
msgpackx_array_node *msgpackx_array_node_insert_prev(msgpackx_array_node *next,
                                                     msgpackx_array_node *ins);

/**
 * @memberof msgpackx_array_node
 * @brief Set the child node
 * @param array_node The array node to set to.
 * @param node Node to set to.
 * @return @p array_node if success, NULL if failed
 *
 * @p node must be a 'child'.
 *
 * To set an array or a map as a child, use
 * msgpackx_array_node_upcast() or msgpackx_map_node_upcast() to **head**,
 * and give it.
 *
 * The API specifies return value NULL when failed, but currently this
 * function won't fail.
 *
 * @sa msgpackx_array_node_get_child_node()
 */
JUPITER_SERIALIZER_DECL
msgpackx_array_node *
msgpackx_array_node_set_child_node(msgpackx_array_node *array_node,
                                   msgpackx_node *node);

/**
 * @memberof msgpackx_array_node
 * @brief Get the child node of an array node
 * @param array_node The array node to get from.
 * @return The child node, or NULL if it's not set.
 *
 * This function also return NULL if given @p array_node is actually
 * an array head.
 *
 * @sa msgpackx_array_node_set_child_node()
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *
msgpackx_array_node_get_child_node(msgpackx_array_node *array_node);

/**
 * @memberof msgpackx_array_node
 * @brief Get the parent node of the array
 * @param array_node The array node (or head) to get from.
 * @return The parent node, or NULL if it's not set.
 *
 * The parent node will be one of (overall) data, array entry or map
 * entry.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_array_node_get_parent(msgpackx_array_node *array_node);

/**
 * @memberof msgpackx_array_node
 * @brief Get the array head of the array
 * @param array_node The array node to get from.
 * @return The array head, or NULL if it's not in the members.
 */
JUPITER_SERIALIZER_DECL
msgpackx_array_node *
msgpackx_array_node_get_head(msgpackx_array_node *array_node);

/**
 * @memberof msgpackx_array_node
 * @brief Get the pointer to generic node of the array
 * @param array_node The array head or node to get from.
 * @return Pointer to @p array_node, but as ::msgpackx_node.
 *
 * @sa msgpackx_node_get_array()
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_array_node_upcast(msgpackx_array_node *array_node);

/**
 * @memberof msgpackx_array_node
 * @brief Test whether a ::msgpackx_array_node pointer is a head or a node
 * @param node A pointer to test its context is head or node.
 * @return 0 if @p node is an array node, non-0 if a @p is an array head.
 */
JUPITER_SERIALIZER_DECL
int msgpackx_array_node_is_head_of_array(msgpackx_array_node *node);

/**
 * @memberof msgpackx_array_node
 * @brief Pack the array members into a single buffer
 * @param head The array head to perform packing
 * @param err Sets error informations when an error occuered, if given.
 * @return @p head if success, NULL if failed
 */
JUPITER_SERIALIZER_DECL
msgpackx_array_node *msgpackx_array_node_pack(msgpackx_array_node *head,
                                              msgpackx_error *err);

/**
 * @memberof msgpackx_map_node
 * @brief Create a new map head or node
 * @param type The map type (head or node) to create
 * @return Created data, or NULL if failed
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_new(enum msgpackx_map_node_type type);

/**
 * @memberof msgpackx_map_node
 * @brief Delete a map node (and its children)
 * @param node The map node to delete
 *
 * @p node must not be a map head.
 */
JUPITER_SERIALIZER_DECL
void msgpackx_map_node_delete(msgpackx_map_node *node);

/**
 * @memberof msgpackx_map_node
 * @brief Delete whole map head and node (and their children)
 * @param head The map head to delete
 *
 * @p head should be a map head. It's ok that @p head is a map node,
 * but strongly recommended.
 */
JUPITER_SERIALIZER_DECL
void msgpackx_map_node_delete_all(msgpackx_map_node *head);

/**
 * @memberof msgpackx_map_node
 * @brief Test whether a ::msgpackx_map_node pointer is a head or a node
 * @param node A pointer to test its context is head or node.
 * @return 0 if @p node is a map node, non-0 if a @p is a map head.
 */
JUPITER_SERIALIZER_DECL
int msgpackx_map_node_is_head_of_map(msgpackx_map_node *node);

/**
 * @memberof msgpackx_array_node
 * @brief Get the parent node of the map
 * @param map_node The map node (or head) to get from.
 * @return The parent node, or NULL if it's not set.
 *
 * The parent node will be one of (overall) data, array entry or map
 * entry.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_map_node_get_parent(msgpackx_map_node *map_node);

/**
 * @memberof msgpackx_map_node
 * @brief Find map entry
 * @param node Map head to find
 * @param find Search for
 * @param err Error info for failure.
 * @return found node if found, head node if not found, NULL if failed.
 *
 * Because of returning value, you should give head node (use
 * msgpackx_map_node_get_head() to get) as @p node.
 *
 *     ret = msgpackx_map_node_find(head, ...);
 *     if (!ret) goto error;
 *     if (ret == head) {
 *        // not found
 *     } else {
 *        // found
 *     }
 *
 * If you are going to lookup a key which is an array or a map, make
 * sure that @p find is packed. If not, you may get unexpected result.
 *
 * @warning This function does not set MSGPACKX_ERR_KEYNOTFOUND to @p err
 *          for historical reason. This error is only for 'High-level APIs'
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_find(msgpackx_map_node *node,
                                          msgpackx_node *find,
                                          msgpackx_error *err);

/**
 * @memberof msgpackx_map_node
 * @brief Find map node by string
 * @param node Map head to find
 * @param str String to find for
 * @param len Length of the string
 * @param err Error info for failure.
 * @return found node if found, head node if not found, NULL if failed.
 *
 * This is a shortcut function that avoids allocating new array,
 * setting a string (which might cause an error).
 *
 * @sa msgpackx_map_node_find()
 *
 * @warning This function does not set MSGPACKX_ERR_KEYNOTFOUND to @p err
 *          for historical reason. This error is only for 'High-level APIs'
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_find_by_str(msgpackx_map_node *node,
                                                 const char *str, ptrdiff_t len,
                                                 msgpackx_error *err);

/**
 * @memberof msgpackx_map_node
 * @brief Find map node by signed integer value
 * @param node Map head to find
 * @param value Integer value to find for
 * @param err Error info for failure.
 * @return found node if found, head node if not found, NULL if failed.
 *
 * This is a shortcut function that avoids allocating new array,
 * setting an int (which might cause an error).
 *
 * @sa msgpackx_map_node_find()
 *
 * @warning This function does not set MSGPACKX_ERR_KEYNOTFOUND to @p err
 *          for historical reason. This error is only for 'High-level APIs'
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_find_by_int(msgpackx_map_node *node,
                                                 intmax_t value,
                                                 msgpackx_error *err);

/**
 * @memberof msgpackx_map_node
 * @brief Find map node by unsigned integer value
 * @param node Map head to find
 * @param value Integer value to find for
 * @param err Error info for failure.
 * @return found node if found, head node if not found, NULL if failed.
 *
 * This is a shortcut function that avoids allocating new array,
 * setting a uint (which might cause an error).
 *
 * @sa msgpackx_map_node_find()
 *
 * @warning This function does not set MSGPACKX_ERR_KEYNOTFOUND to @p err
 *          for historical reason. This error is only for 'High-level APIs'
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_find_by_uint(msgpackx_map_node *node,
                                                  uintmax_t value,
                                                  msgpackx_error *err);

/**
 * @memberof msgpackx_map_node
 * @brief Insert map entry
 * @param head map head to insert destination
 * @param insert map node to insert
 * @param err Set error info if occured.
 * @return node (map of head if given node is not head) if success,
 *         NULL otherwise.
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_insert(msgpackx_map_node *head,
                                            msgpackx_map_node *insert,
                                            msgpackx_error *err);

/**
 * @memberof msgpackx_map_node
 * @brief Get map head
 * @param node map node
 * @return map head, or NULL if @p node is not a member of any map.
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_get_head(msgpackx_map_node *node);

/**
 * @memberof msgpackx_map_node
 * @brief Get key child node
 * @param node map node to get from
 * @return key child node, or NULL if not set or @p node is a map head.
 *
 * @note You can not modify it after insert to map. If you want to
 *       change key, you can set a new key by
 *       msgpackx_map_node_set_key().
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_map_node_get_key(msgpackx_map_node *node);

/**
 * @memberof msgpackx_map_node
 * @brief Get value child node
 * @param node map node to get from.
 * @return value child node, or NULL if not set or @p node is a map head.
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_map_node_get_value(msgpackx_map_node *node);

/**
 * @memberof msgpackx_map_node
 * @brief Set map key node
 * @param node map node to set
 * @param key the node to be used as key
 * @param err Sets error information when an error occuered, if given.
 *
 * You must make sure that the @p key is packed if you are going to
 * use map or array data as a key.
 *
 * You cannot modify the data after setting key.
 *
 * @p node must be a node.
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_set_key(msgpackx_map_node *node,
                                             msgpackx_node *key,
                                             msgpackx_error *err);

/**
 * @memberof msgpackx_map_node
 * @brief Set map value node
 * @param node map node to set (node must not be a head)
 * @param value the node to be used as value
 *
 * @p node must be a node.
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_set_value(msgpackx_map_node *node,
                                               msgpackx_node *value);

/**
 * @memberof msgpackx_map_node
 * @brief Get the pointer to generic node of the array
 * @param map_node The map head or node to get from.
 * @return Pointer to @p map_node, but as ::msgpackx_node.
 *
 * @sa msgpackx_node_get_map()
 */
JUPITER_SERIALIZER_DECL
msgpackx_node *msgpackx_map_node_upcast(msgpackx_map_node *map_node);

/**
 * @memberof msgpackx_map_node
 * @brief Next map node
 * @param node map node (or head) to get from.
 * @return next map node in insertion order.
 *
 * If @p node is a map head, returns the first inserted node.
 *
 * If @p node is the last inserted node, returns the map head.
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_next(msgpackx_map_node *node);

/**
 * @memberof msgpackx_map_node
 * @brief Previous map node
 * @param node map node (or head) to get from.
 * @return previous map node in insertion order.
 *
 * If @p node is a map head, returns the last inserted node.
 *
 * If @p node is the first inserted node, returns the map head.
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_prev(msgpackx_map_node *node);

/**
 * @memberof msgpackx_map_node
 * @brief Next map node in sequence of key
 * @param node map node (or head) to get from.
 * @return next map node in sorted key order.
 *
 * If @p node is a map head, returns the node with minimum key value
 *
 * If @p node has the maximum key value, returns NULL.
 *
 * Use msgpackx_map_node_next() for general iterative purpose (it's
 * also faster than this function)
 *
 * @note The sorted order is in order that serialized representation of
 *       the key. So,
 *         * All positive integer values are in its order. (0 is the
 *           smallest key value among all integer and non-integer values).
 *         * Negative integer values are somewhat randamic (-1 is the
 *           largest key value among all integer and non-integer values).
 *         * String and binary data has priority in its length and then
 *           its contents (So, "b" is before "aa", "bb" is after "aa").
 *         * nil, false and true are between 127 and 128, in this order.
 *         * Extended data is sorted by rough length, type, actual length and
 *           then its contents.
 *
 * @warning If you decide to use Little Endian to store data, the result is
 *          different to Big Endian case.
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_next_sorted(msgpackx_map_node *node);

/**
 * @memberof msgpackx_map_node
 * @brief Previous map node in sequence of key
 * @param node map node (or head) to get from.
 * @return prev map node in sorted key order.
 *
 * If @p node is a map head, returns the node with maximum key value
 *
 * If @p node has the minimum key value, returns NULL.
 *
 * Use msgpackx_map_node_prev() for general iterative purpose (it's
 * also faster than this function)
 *
 * @note The sorted order is in order that serialized representation of
 *       the key. So,
 *         * All positive integer values are in its order. (0 is the
 *           smallest key value among all integer and non-integer values).
 *         * Negative integer values are somewhat randamic (-1 is the
 *           largest key value among all integer and non-integer values).
 *         * String and binary data has priority in its length and then
 *           its contents (So, "b" is before "aa", "bb" is after "aa").
 *         * nil, false and true are between 127 and 128, in this order.
 *         * Extended data is sorted by rough length, type, actual length and
 *           then its contents.
 *
 * @warning If you decide to use Little Endian to store data, the result is
 *          different to Big Endian case.
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_prev_sorted(msgpackx_map_node *node);

/**
 * @memberof msgpackx_map_node
 * @brief Pack the map members into a single buffer
 * @param head The map head to perform packing
 * @param err Sets error informations when an error occuered, if given.
 * @return @p head if success, NULL if failed
 */
JUPITER_SERIALIZER_DECL
msgpackx_map_node *msgpackx_map_node_pack(msgpackx_map_node *head,
                                          msgpackx_error *err);

JUPITER_SERIALIZER_DECL_END

#endif
