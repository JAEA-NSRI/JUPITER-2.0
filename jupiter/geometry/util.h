
#ifndef JUPITER_GEOMETRY_UTIL_H
#define JUPITER_GEOMETRY_UTIL_H

/**
 * @ingroup Geometry
 * @brief Get container of specific pointer.
 * @param ptr
 * @param type
 * @param member
 * @return pointer to the container
 *
 * This macro is used to implement `geom_list_entry` and
 * `geom_rbtree_entry`. Do not use this macro directly.
 *
 * If you are going to implement another data structure algorithm,
 * which uses the same concept to `geom_list` or `geom_rbtree`, you
 * may need this macro.
 */
#define geom_container_of(ptr, type, member)            \
  ((type *) ((char *) ptr - offsetof(type, member)))

#endif
