
#ifndef JUPITER_TABLE_FUNC_H
#define JUPITER_TABLE_FUNC_H

#include "table.h"

JUPITER_TABLE_DECL_START

struct table_search_functions
{
  /**
   * Provide function to allocate algorithm specific data structure.
   */
  void *(*allocate)(void);

  /**
   * Provide function to deallocate algorithm specific data structure.
   */
  void (*deallocate)(void *d);

  /**
   * Provide function to initialize algorithm specific data structure.
   */
  table_error (*init)(void *d, table_geometry g, table_size nx, table_size ny,
                      const double *x, const double *y);

  /**
   * @brief Provide function to find the node.
   * @param d Passes data allocated by allocate function
   * @param x Passes x position to find
   * @param y Passes y position to find
   * @param node Passes pointer to return result.
   * @return TABLE_SUCCESS if found, TABLE_ERR_RANGE if outside of table.
   *
   * This function must be thread safe.
   */
  table_error (*find)(void *d, double x, double y, table_node *node);
};
typedef struct table_search_functions table_search_functions;

JUPITER_TABLE_DECL_END

#endif
