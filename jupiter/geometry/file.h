
#ifndef GEOMETRY_FILE_H
#define GEOMETRY_FILE_H

#include "defs.h"
#include "svector.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @brief Create new file data.
 * @param parent Data element to belong
 * @param e If non-NULL value given, set error infomation.
 * @return Created file data, or NULL if allocation failed.
 */
JUPITER_GEOMETRY_DECL
geom_file_data *geom_file_data_new(geom_data_element *parent, geom_error *e);

/**
 * @brief Get data element that `file` belongs to.
 * @param data File data to get.
 * @return data element
 */
JUPITER_GEOMETRY_DECL
geom_data_element *geom_file_data_parent(geom_file_data *data);

/**
 * @brief Get master data thas `file` belongs to.
 * @param data File data to get.
 * @return geometry data.
 */
JUPITER_GEOMETRY_DECL
geom_data *geom_file_data_master(geom_file_data *data);

/**
 * @brief Delete allocated file data
 * @param data File data to delete.
 */
JUPITER_GEOMETRY_DECL
void geom_file_data_delete(geom_file_data *data);

/**
 * @brief Set filename of the data
 * @param data File data to set.
 * @param fname File name to set.
 * @param altname Alternative file name to set.
 *
 * Geometry library does not actually do any work with these provided
 * file names or reading mode.
 *
 * Therefore, Geometry library does not provide meaning of `fname` vs
 * `altname`, just you can set two distinct files independently.
 *
 * File data does not maintain provided pointers (does not `copy` or
 * `free`). If you need, copy before call this function, and use
 * `geom_data_add_pointer` to transfer the deallocation responsibility
 * of the copied pointer to `geom_data` structure.
 *
 * A typical use case is:
 *  * `fname` is user provided original file name, and `altname` is
 *    actual filename to read or write, where `fname` contains
 *    formatting directives with other parameters (such as MPI-rank index)
 */
JUPITER_GEOMETRY_DECL
void geom_file_data_set_file_path(geom_file_data *data, const char *fname,
                                  const char *altname);

/**
 * @brief Set size of file data (with vector type)
 * @param data File data to set.
 * @param size Size of the file.
 */
JUPITER_GEOMETRY_DECL
void geom_file_data_set_sizev(geom_file_data *data, geom_svec3 size);

/**
 * @brief Set repetition offset of file data (with vector type)
 * @param data File data to set.
 * @param offset Repetiton offset of the file.
 */
JUPITER_GEOMETRY_DECL
void geom_file_data_set_offsetv(geom_file_data *data, geom_svec3 offset);

/**
 * @brief Set repetition number of file data (with vector type)
 * @param data File data to set
 * @param rep Repetition number of the file.
 */
JUPITER_GEOMETRY_DECL
void geom_file_data_set_repeatv(geom_file_data *data, geom_svec3 rep);

/**
 * @brief Set origin of file data (with vector type)
 * @param data File data to set
 * @param ori Origin of the file.
 */
JUPITER_GEOMETRY_DECL
void geom_file_data_set_originv(geom_file_data *data, geom_svec3 ori);

/**
 * @biref Obtain file path from the file data.
 * @param data File data to get.
 * @return pointer to the file path
 */
JUPITER_GEOMETRY_DECL
const char *geom_file_data_get_file_path(geom_file_data *data);

/**
 * @brief Obtain alternative file path from the file data.
 * @param data File data to get.
 * @return pointer to the alternative file path.
 */
JUPITER_GEOMETRY_DECL
const char *geom_file_data_get_alt_file_path(geom_file_data *data);

/**
 * @brief Obtain size info of file data
 * @param data File data to get.
 * @return Size info
 */
JUPITER_GEOMETRY_DECL
geom_svec3 geom_file_data_get_size(geom_file_data *data);

/**
 * @brief Obtain repetition offset info of file data
 * @param data File data to get.
 * @return Repetition offset info
 */
JUPITER_GEOMETRY_DECL
geom_svec3 geom_file_data_get_offset(geom_file_data *data);

/**
 * @brief Obtain repetition number of file data
 * @param data File data to get.
 * @return Repetition number info
 */
JUPITER_GEOMETRY_DECL
geom_svec3 geom_file_data_get_repeat(geom_file_data *data);

/**
 * @brief Obtain size info of file data
 * @param data File data to get.
 * @return Size info
 */
JUPITER_GEOMETRY_DECL
geom_svec3 geom_file_data_get_origin(geom_file_data *data);

/**
 * @brief Set custom data
 * @memberof geom_file_data
 *
 * @param data Destination file data
 * @param extra_data Data to be set
 * @param dealloc Deallocator function for given `data`
 * @retval GEOM_SUCCESS No error
 * @retval GEOM_ERR_NOMEM Cannot allocate memory
 *
 * Same as `geom_data_set_extra_data()` but set data for contained by
 * `geom_file_data`.
 *
 * @sa geom_data_set_extra_data()
 */
JUPITER_GEOMETRY_DECL
geom_error geom_file_data_set_extra_data(geom_file_data *data, void *extra_data,
                                         geom_deallocator *dealloc);

/**
 * @brief Get custom data.
 * @memberof geom_file_data
 * @param data Data to get from.
 * @return user-defined custom data
 */
JUPITER_GEOMETRY_DECL
const geom_user_defined_data *
geom_file_data_get_extra_data(geom_file_data *data);

JUPITER_GEOMETRY_DECL_END

#endif
