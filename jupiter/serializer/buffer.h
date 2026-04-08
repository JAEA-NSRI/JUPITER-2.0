#ifndef JUPITER_SERIALIZER_BUFFER_H
#define JUPITER_SERIALIZER_BUFFER_H

#include <stdarg.h>

#include "defs.h"

JUPITER_SERIALIZER_DECL_START

/**
 * @memberof msgpackx_buffer
 * @brief Create New buffer
 * @return Created buffer, or NULL if allocation failed.
 */
JUPITER_SERIALIZER_DECL
msgpackx_buffer *msgpackx_buffer_new(void);

/**
 * @memberof msgpackx_buffer
 * @brief Delete buffer pointer
 * @param ptr Pointer to delete
 *
 * If @p ptr is the last pointer that points to its content,
 * corresponding memory will be also freed.
 */
JUPITER_SERIALIZER_DECL
void msgpackx_buffer_delete(msgpackx_buffer *ptr);

/**
 * @memberof msgpackx_buffer
 * @brief Delete all buffer pointers and buffer relates to @p ptr
 * @param ptr Pointer to delete
 */
JUPITER_SERIALIZER_DECL
void msgpackx_buffer_delete_all_referenced(msgpackx_buffer *ptr);

/**
 * @memberof msgpackx_buffer
 * @brief Test specific pointer is inside of the buffer
 * @param ptr Pointer to buffer
 * @param testp Pointer to test.
 * @retval MSGPACKX_POINTER_INVALID testp is invalid pointer (out-of-range)
 * @retval MSGPACKX_POINTER_VALID   testp is valid as both start and end
 * @retval MSGPACKX_POINTER_VALID_AS_END testp is only valid as end
 *
 * You can use bitwise-AND to returned value to check valid as start or end.
 *
 *     if (msgpackx_buffer_is_valid(buf, p) & MSGPACKX_POINTER_VALID_AS_START) {
 *       ...; // p is valid as range of start
 *     }
 *
 *     if (msgpackx_buffer_is_valid(buf, p) & MSGPACKX_POINTER_VALID_AS_END) {
 *       ...; // p is valid as range of end
 *     }
 *
 * If @p testp points the beginning of buffer, this function returns
 * MSGPACKX_POINTER_VALID (This is why this function does not return a
 * value only flagged MSGPACKX_POINTER_VALID_AS_START). Because
 * [testp, testp] is considered valid range for zero length buffer.
 */
JUPITER_SERIALIZER_DECL
enum msgpackx_buffer_pointer_validity
msgpackx_buffer_is_valid(msgpackx_buffer *ptr, void *testp);

/**
 * @memberof msgpackx_buffer
 * @brief Get pointer of buffer
 * @param ptr Pointer to get.
 * @return Raw pointer that @p ptr points.
 *
 * The validity of pointer is not tested. Use
 * msgpackx_buffer_is_valid() to test.
 *
 * In general purpose, you should use msgpackx_buffer_make_cstr() to
 * get data, and msgpackx_buffer_raw_copy() to set data.
 *
 * The scope of the returned pointer is undefined. It is only for
 * immediate use. An any operation on another `msgpackx_buffer`
 * pointer that references same buffer pointed by @p ptr may move
 * pointer @p ptr.
 */
JUPITER_SERIALIZER_DECL
void *msgpackx_buffer_pointer(msgpackx_buffer *ptr);

/**
 * @memberof msgpackx_buffer
 * @brief Get end pointer of buffer
 * @param ptr Pointer to get
 * @return Raw pointer that @p ptr ends.
 *
 * If @p ptr does not have the end (aka. open pointer),
 * returns end of available buffer.
 *
 * Note that end pointer is exclusive.
 *
 * The validity of pointer is not tested. Use
 * msgpackx_buffer_is_valid() to test.
 *
 * In general purpose, you should use msgpackx_buffer_make_cstr() to
 * get data, and msgpackx_buffer_raw_copy() to set data.
 *
 * The scope of the returned pointer is undefined. It is only for
 * immediate use. An any operation on another `msgpackx_buffer`
 * pointer that references same buffer pointed by @p ptr may move
 * pointer @p ptr.
 */
JUPITER_SERIALIZER_DECL
void *msgpackx_buffer_endp(msgpackx_buffer *ptr);

/**
 * @memberof msgpackx_buffer
 * @brief Make C String (aka. ends with '\0') from pointer
 * @param ptr Pointer to make.
 * @return Created string
 *
 * Returned pointer uses malloc() for allocation.
 * Use free() after use.
 */
JUPITER_SERIALIZER_DECL
char *msgpackx_buffer_make_cstr(msgpackx_buffer *ptr);

/**
 * @memberof msgpackx_buffer
 * @brief Get size of puffer or substr
 * @param ptr Pointer to get
 * @return size of pointer
 *
 * If @p ptr is open pointer, returns size of @p ptr points to end of
 * possible usable buffer.
 *
 * If @p ptr is substr, return size of substr.
 */
JUPITER_SERIALIZER_DECL
ptrdiff_t msgpackx_buffer_size(msgpackx_buffer *ptr);

/**
 * @memberof msgpackx_buffer
 * @brief Get lowest pointer of the buffer
 * @param ptr Buffer to get.
 * @return the lowest pointer, or NULL if not allocated.
 *
 * This function is equivalent to (but do without allocation):
 *
 *     {
 *       char *retp;
 *       msgpackx_buffer *p;
 *       p = msgpackx_buffer_for_whole_region(ptr);
 *       retp = msgpackx_buffer_pointer(p);
 *       msgpackx_buffer_delete(p);
 *       return retp;
 *     }
 */
JUPITER_SERIALIZER_DECL
char *msgpackx_buffer_pointer_root(msgpackx_buffer *ptr);

/**
 * @memberof msgpackx_buffer
 * @brief Reserve buffer
 * @param ptr Buffer to reserve
 * @param size Size to reserve
 * @return non-NULL pointer if success, NULL if failed to allocate.
 *
 * If size is smaller than already allocated size, this function has
 * no effect.
 *
 * The size of buffer will not change in this function. Just reserve
 * memory to avoid reallocating frequently.
 *
 * The content of returned pointer (for non-NULL) is currently
 * undefined.
 */
JUPITER_SERIALIZER_DECL
char *msgpackx_buffer_reserve(msgpackx_buffer *ptr, ptrdiff_t size);

/**
 * @memberof msgpackx_buffer
 * @brief Resize buffer
 * @param ptr Buffer to resize
 * @param size Size to resize
 * @return non-NULL pointer if success, NULL if failed to allocate.
 *
 * If size is smaller than original, this function won't fail
 * (reallocation will not occur).
 *
 * The content of returned pointer (for non-NULL) is currently
 * undefined.
 *
 * The @p size is the total size of buffer.
 *
 * This function is core operation (can broke consistency). In general use,
 * msgpackx_buffer_resize_substr() can be better choice.
 */
JUPITER_SERIALIZER_DECL
char *msgpackx_buffer_resize(msgpackx_buffer *ptr, ptrdiff_t size);

/**
 * @memberof msgpackx_buffer
 * @brief Get a pointer points whole region of the buffer
 * @param ptr Pointer to get.
 * @return Pointer points whole region of the buffer, or NULL if allocation
 *         failed.
 *
 * Returned pointer is open pointer (End point is not set. i.e., if
 * the buffer extends, the size of the pointer will also increase).
 */
JUPITER_SERIALIZER_DECL
msgpackx_buffer *msgpackx_buffer_for_whole_region(msgpackx_buffer *ptr);

/**
 * @memberof msgpackx_buffer
 * @brief Test whether the pointer has end or not.
 * @param ptr Pointer to get info
 * @return 0 if pointer have end, non-0 if the pointer does not have end.
 */
JUPITER_SERIALIZER_DECL
int msgpackx_buffer_is_substr(msgpackx_buffer *ptr);

/**
 * @memberof msgpackx_buffer
 * @brief Test whether the two pointers point same buffer
 * @param a Pointer a
 * @param b Pointer b
 * @return 0 if pointers share buffer, non-0 if not.
 */
JUPITER_SERIALIZER_DECL
int msgpackx_buffer_is_shared(msgpackx_buffer *a, msgpackx_buffer *b);

/**
 * @memberof msgpackx_buffer
 * @brief Test whether any other pointers shares the buffer with given one
 * @param a Pointer to test
 * @return 0 if no pointers shares the buffer with given one, non-0 if not.
 */
JUPITER_SERIALIZER_DECL
int msgpackx_buffer_any_shared(msgpackx_buffer *a);

/**
 * @memberof msgpackx_buffer
 * @brief Test whether the two pointers point same buffer
 * @param a Pointer a
 * @param b Pointer b
 * @return 0 if pointers overlaps buffer, non-0 if not.
 */
JUPITER_SERIALIZER_DECL
int msgpackx_buffer_is_overlapped(msgpackx_buffer *a, msgpackx_buffer *b);

/**
 * @memberof msgpackx_buffer
 * @brief Test whether any other pointers overlap the substr of given one
 * @param a Pointer to test
 * @return 0 if no pointers shares the buffer with given one, non-0 if not.
 */
JUPITER_SERIALIZER_DECL
int msgpackx_buffer_any_overlapped(msgpackx_buffer *a);

/**
 * @memberof msgpackx_buffer
 * @brief Get substr pointer of the buffer
 * @param ptr Pointer to get.
 * @param offset Offset value
 * @param size Size of substr (-1 for create no-End pointer)
 * @param seek Location of the base of offset.
 * @return New substr pointer, or NULL if allocation failed.
 *
 * If @p seek is `MSGPACKX_SEEK_SET`, @p offset will be offset from
 * the beginning of the buffer.
 *
 * If @p seek is `MSGPACKX_SEEK_CUR`, @p offset will be offset from
 * the @ptr points.
 *
 * If @p seek is `MSGPACKX_SEEK_END`, @p offset will be offset from
 * the end of the buffer.
 */
JUPITER_SERIALIZER_DECL
msgpackx_buffer *msgpackx_buffer_substr(msgpackx_buffer *ptr, ptrdiff_t offset,
                                        ptrdiff_t size,
                                        enum msgpackx_buffer_seek_mode seek);

/**
 * @memberof msgpackx_buffer
 * @brief Move pointer.
 * @param ptr Pointer to move.
 * @param offset Offset value
 * @param size Size of substr (-1 for create no-End pointer)
 * @param seek Location of the base of offset.
 * @return @p ptr.
 *
 * This function is equivalent to msgpackx_buffer_substr(), except for
 * moving @p ptr itself instead of creating a new pointer.
 *
 * This function won't fail.
 */
JUPITER_SERIALIZER_DECL
msgpackx_buffer *msgpackx_buffer_goto(msgpackx_buffer *ptr, ptrdiff_t offset,
                                      ptrdiff_t size,
                                      enum msgpackx_buffer_seek_mode seek);

/**
 * @memberof msgpackx_buffer
 * @brief Move pointer to another buffer
 * @param moving_p      Pointer to move
 * @param destination_p Destination buffer
 * @param offset Offset to @p destination_p
 * @param seek Location of the base of offset
 * @return @p moving_p.
 *
 * If @p moving_p is the last pointer which refers corresponding
 * buffer, that buffer will be freed.
 *
 * The status that open or substr pointer will not be change.
 *
 * If @p moving_p is an open pointer, the result of
 * msgpackx_buffer_size() may change.
 *
 * This function won't fail.
 */
JUPITER_SERIALIZER_DECL
msgpackx_buffer *msgpackx_buffer_relocate(msgpackx_buffer *moving_p,
                                          msgpackx_buffer *destination_p,
                                          ptrdiff_t offset,
                                          enum msgpackx_buffer_seek_mode seek);

/**
 * @memberof msgpackx_buffer
 * @brief Move pointer right by 1.
 * @param ptr Pointer to get.
 * @return @p ptr.
 *
 * Size (aka. end-point) will not be change.
 *
 * Use msgpackx_buffer_goto() if 2 or more offset at once, this
 * function has O(log n) (Here, n is the number of pointers which
 * points same buffer that @p ptr points) computation time at the
 * worst case.
 */
JUPITER_SERIALIZER_DECL
msgpackx_buffer *msgpackx_buffer_increment(msgpackx_buffer *ptr);

/**
 * @memberof msgpackx_buffer
 * @brief Resize substr
 * @param ptr substr to resize
 * @param size new substr size
 * @return ptr if success, NULL otherwise.
 *
 * Shrink or expand buffer at @p ptr refers, and resize substr.
 *
 * If given @p size is larger than the substr size @p ptr, inserts
 * bytes at @p ptr points and make the substr size @p ptr to @p size.
 *
 * If given @p size is smaller than the substr size @p ptr, shrinks
 * bytes at @p ptr points and make the substr size @p ptr to @p size.
 */
JUPITER_SERIALIZER_DECL
msgpackx_buffer *msgpackx_buffer_resize_substr(msgpackx_buffer *ptr,
                                               ptrdiff_t size);

/**
 * @memberof msgpackx_buffer
 * @brief Move pointer left by 1.
 * @param ptr Pointer to get.
 * @return @p ptr.
 *
 * Size (aka. end-point) will not be change.
 *
 * Use msgpackx_buffer_goto() if 2 or more offset at once, this
 * function has O(log n) (Here, n is the number of pointers which
 * points same buffer that @p ptr points) computation time at the
 * worst case.
 */
JUPITER_SERIALIZER_DECL
msgpackx_buffer *msgpackx_buffer_decrement(msgpackx_buffer *ptr);

/**
 * @memberof msgpackx_buffer
 * @brief Copy from plain buffer
 * @param dest Destination pointer
 * @param data Source data
 * @param size size of @p data
 * @param mode Copy mode.
 * @return @p ptr if success, NULL if size <= 0
 *
 * See msgpackx_buffer_copy() for description of @p mode.
 *
 * This function does not test whether @p data overlaps @p dest.
 *
 * Copying is safe (using memmove()), but shrinking may be
 * unsafe. Shrinking may destruct @p data if overlapped.
 */
JUPITER_SERIALIZER_DECL
msgpackx_buffer *msgpackx_buffer_raw_copy(msgpackx_buffer *dest,
                                          const void *data, ptrdiff_t size,
                                          enum msgpackx_buffer_copy_mode mode);

/**
 * @memberof msgpackx_buffer
 * @brief Copy from another pointer
 * @param dest Destination pointer
 * @param from Source pointer
 * @param mode copy mode
 * @return @p dest if success, NULL if failed (allocation error etc.)
 *
 * If @p mode is flagged `MSGACKX_COPY_CREATE`, a new buffer will be
 * created on @p dest and copied to there.
 *
 * If the size @p dest is larger than the size of @p from and ...:
 *
 *   - If @p mode is flagged `MSGPACKX_COPY_SHRINK`, shrink buffer at
 *     @p dest points, and make the size of @p dest to the size of @p
 *     from.
 *
 *   - Else If @p mode is flagged `MSGPACKX_COPY_ZERO`, remained
 *     region will be filled with 0.
 *
 *   - If @p mode is not flagged both of `MSGPACKX_COPY_SHRINK` or
 *     `MSGPACKX_COPY_ZERO`, remained region will be unmodified.
 *
 * If the size @p from is larger than the size of @p dest and ...:
 *
 *   - If @p mode is flagged `MSGPACKX_COPY_EXPAND`, expand buffer at
 *     @p dest points, and make the size of @p dest to the size of @p
 *     from.
 *
 *   - If @p mode is not flagged `MSGPACKX_COPY_EXPAND`, the copy
 *     count will be limited to the size of @p dest. (Use
 *     `MSGPACKX_COPY_TRUNC` to be explicit on call, but note that
 *     `MSGPACKX_COPY_EXPAND` has priority when both flagged)
 *
 * The preset mode `MSGPACKX_COPY_FLEXIBLE` is bitwise-OR of
 * `MSGPACKX_COPY_EXPAND` and `MSGPACKX_COPY_SHRINK`.
 * (Expand if source is long, shrink if source is short)
 *
 * The preset mode `MSGPCAKX_COPY_FIXED` is bitwise-OR of
 * `MSGPACKX_COPY_TRUNC` and and `MSGPACKX_COPY_ZERO`.
 * (Truncate if source is long, fill zero if source is short)
 */
JUPITER_SERIALIZER_DECL
msgpackx_buffer *msgpackx_buffer_copy(msgpackx_buffer *dest,
                                      msgpackx_buffer *from,
                                      enum msgpackx_buffer_copy_mode mode);

/**
 * @memberof msgpackx_buffer
 * @brief Duplicate buffer
 * @param src Source
 * @param mode Duplication mode
 * @return pointer to new buffer if success, NULL otherwise.
 *
 * Duplicates the buffer @p src and returns a new buffer pointer.
 *
 * If @p mode is MSGPACKX_DUP_WHOLE, copies whole buffer that @p src
 * points to.
 *
 * If @p mode is MSGPACKX_DUP_SUBSTR, copies substring region that @p
 * src points to.
 */
JUPITER_SERIALIZER_DECL
msgpackx_buffer *msgpackx_buffer_dup(msgpackx_buffer *src,
                                     enum msgpackx_buffer_dup_mode mode);

/**
 * @memberof msgpackx_buffer
 * @brief C language format print onto the buffer
 * @param dest Destination pointer
 * @param mode insertion mode
 * @param format format string to be used
 * @param ... arguments to format
 * @return number of bytes (NUL is not included) written, nagative
 *         value if failed
 *
 * See msgpackx_buffer_copy() for usage of @p mode argument.
 *
 * Usually, make a pointer with size == 0, and `MSGPACKX_COPY_EXPAND`
 * to insert a text.
 *
 *     // Make size to 0
 *     msgpackx_buffer_goto(dest, 0, 0, MSGPACKX_SEEK_CUR);
 *     msgpackx_buffer_printf(dest, MSGPACKX_COPY_EXPAND, "...", ...);
 *
 * Use `MSGPACKX_COPY_FLEXIBLE` to overwrite substr @p dest points.
 *
 * We use snprintf() to print the text. So if you use
 * MSGPACKX_COPY_TRUNC as insertion @p mode, NUL charactor is written
 * on the last byte inside buffer. See the snprintf(3) for more info.
 *
 * In @p mode of `MSGPACKX_COPY_EXPAND` or `MSGPACKX_COPY_SHRINK` (and
 * bitwise-OR of those, `MSGPACKX_COPY_FLEXIBLE`), the last NUL
 * charactor will be trimmed out.
 *
 * The size of the text is not easily predictable (as meaning of this
 * function, but we can use snprintf() (in C99 standard) to get the
 * size, FYI).
 *
 *     msgpackx_buffer_printf(dest, MSGPACKX_COPY_FLEXIBLE, "...", ...);
 */
JUPITER_SERIALIZER_DECL
int msgpackx_buffer_printf(msgpackx_buffer *dest,
                           enum msgpackx_buffer_copy_mode mode,
                           const char *format, ...);

/**
 * @memberof msgpackx_buffer
 * @brief C language format print onto the buffer
 * @param dest Destination pointer
 * @param mode insertion mode
 * @param format format string to be used
 * @param ap arguments to format
 * @return number of bytes (NUL is not included) written, nagative
 *         value if failed
 *
 * Same as msgpackx_buffer_printf(), except for giving a va_list for
 * passing list of arguments.
 */
JUPITER_SERIALIZER_DECL
int msgpackx_buffer_vprintf(msgpackx_buffer *dest,
                            enum msgpackx_buffer_copy_mode mode,
                            const char *format, va_list ap);

/**
 * @memberof msgpackx_buffer
 * @brief Make a string of tree image.
 * @param dump Buffer to dump
 * @param use_offset Use offset from buffer base (in decimal) instead
 *        of address.
 * @param prefix Prefix text on each line.
 * @return Formatted text if success, NULL if failed
 *
 * This function is for debug.
 *
 * The returned pointer is allocated with malloc() (see
 * msgpackx_buffer_make_cstr()), so free by free() after use.
 *
 * This function uses msgpackx_buffer to create dump text, by the way.
 */
JUPITER_SERIALIZER_DECL
char *msgpackx_buffer_tree_dump(msgpackx_buffer *dump, int use_offset,
                                const char *prefix);

JUPITER_SERIALIZER_DECL_END

#endif
