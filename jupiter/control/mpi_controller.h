#ifndef JUPITER_CONTROL_MPI_CONTROLLER_H
#define JUPITER_CONTROL_MPI_CONTROLLER_H

#include "defs.h"
#include "shared_object.h"

#ifdef JCNTRL_USE_MPI
#include <mpi.h>
#endif

JUPITER_CONTROL_DECL_BEGIN

/**
 * Most functions available even if MPI is not enabled. Communication functions
 * does nothing if it's not enabled.
 */

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mpi_controller);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_reduce_op);

/**
 * @note Initialized controller contains NULL communicator
 */
JUPITER_CONTROL_DECL
jcntrl_mpi_controller *jcntrl_mpi_controller_new(void);
JUPITER_CONTROL_DECL
void jcntrl_mpi_controller_delete(jcntrl_mpi_controller *controller);

/**
 * Returns controller for MPI_COMM_SELF. This object cannot be deleted.
 */
JUPITER_CONTROL_DECL
const jcntrl_mpi_controller *jcntrl_mpi_controller_self(void);

/**
 * Returns controller for MPI_COMM_WORLD. This object cannot be deleted.
 */
JUPITER_CONTROL_DECL
const jcntrl_mpi_controller *jcntrl_mpi_controller_world(void);

JUPITER_CONTROL_DECL
const jcntrl_reduce_op *jcntrl_reduce_op_sum(void);
JUPITER_CONTROL_DECL
const jcntrl_reduce_op *jcntrl_reduce_op_prod(void);
JUPITER_CONTROL_DECL
const jcntrl_reduce_op *jcntrl_reduce_op_max(void);
JUPITER_CONTROL_DECL
const jcntrl_reduce_op *jcntrl_reduce_op_min(void);
JUPITER_CONTROL_DECL
const jcntrl_reduce_op *jcntrl_reduce_op_land(void);
JUPITER_CONTROL_DECL
const jcntrl_reduce_op *jcntrl_reduce_op_lor(void);
JUPITER_CONTROL_DECL
const jcntrl_reduce_op *jcntrl_reduce_op_lxor(void);
JUPITER_CONTROL_DECL
const jcntrl_reduce_op *jcntrl_reduce_op_band(void);
JUPITER_CONTROL_DECL
const jcntrl_reduce_op *jcntrl_reduce_op_bor(void);
JUPITER_CONTROL_DECL
const jcntrl_reduce_op *jcntrl_reduce_op_bxor(void);

#ifdef JCNTRL_USE_MPI
JUPITER_CONTROL_DECL
MPI_Comm jcntrl_mpi_controller_get_comm(jcntrl_mpi_controller *controller);

/**
 * Communicator @p comm will be duplicated upon set.
 */
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_set_comm(jcntrl_mpi_controller *controller,
                                   MPI_Comm comm);
#endif

JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_nproc(const jcntrl_mpi_controller *controller);
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_rank(const jcntrl_mpi_controller *controller);

JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_mpi_controller_object(jcntrl_mpi_controller *controller);

JUPITER_CONTROL_DECL
jcntrl_mpi_controller *
jcntrl_mpi_controller_downcast(jcntrl_shared_object *object);

JUPITER_CONTROL_DECL
jcntrl_mpi_controller *
jcntrl_mpi_controller_split(const jcntrl_mpi_controller *controller, //
                            int color, int key);

JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_any(const jcntrl_mpi_controller *controller,
                              int cond);

JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_all(const jcntrl_mpi_controller *controller,
                              int cond);

/**
 * Send partial data to @p dest_rank
 */
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_send(const jcntrl_mpi_controller *controller,
                               jcntrl_data_array *array, int ntuple,
                               int dest_rank, int tag);

/**
 * Receive partial data from @p dest_rank.
 *
 * @note this function does not resize the @p array since it is explicitly
 * specified. Please resize the array prior to call this function
 * (and possibly share the result).
 *
 * @note You cannot receive data send by jcntrl_mpi_controller_senda().
 */
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_recv(const jcntrl_mpi_controller *controller,
                               jcntrl_data_array *array, int ntuple,
                               int src_rank, int tag);

/**
 * Send whole data to @p dest_rank
 */
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_senda(const jcntrl_mpi_controller *controller,
                                jcntrl_data_array *array, int dest_rank,
                                int tag);

/**
 * Receive whole data from @p dest_rank.
 *
 * Resize to sender's size (even if it's already enough)
 *
 * @note You cannot receive data send by jcntrl_mpi_controller_send().
 */
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_recva(const jcntrl_mpi_controller *controller,
                                jcntrl_data_array *array, int src_rank,
                                int tag);

/**
 * Broadcast given size to others from @p root
 */
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_bcast(const jcntrl_mpi_controller *controller,
                                jcntrl_data_array *ary, int ntuple, int root);

/**
 * Broadcast the whole array to others from @p root. @p ary on receiver
 * will be resized to desired size.
 */
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_bcasta(const jcntrl_mpi_controller *controller,
                                 jcntrl_data_array *ary, int root);

/**
 * Perform in-place operation if @p dest == @p src. However, since this check
 * has been done by comparing the pointers to the array object, @p src must not
 * refer @p dest.
 *
 * @p dest will be resized if not enough.
 *
 * @p ntuple must be equal among processes.
 */
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_gather(const jcntrl_mpi_controller *controller,
                                 jcntrl_data_array *dest,
                                 jcntrl_data_array *src, int ntuple, int root);

JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_allgather(const jcntrl_mpi_controller *controller,
                                    jcntrl_data_array *dest,
                                    jcntrl_data_array *src, int ntuple);

/**
 * Perform in-place operation if @p dest == @p src. However, since this check
 * has been done by comparing the pointers to the array object, @p src must not
 * refer @p dest.
 *
 * @p dest will be resized if not enough. To get transfered size, resize to 0
 * prior to call.
 *
 * This function supports differently-sized @p ntuple.
 */
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_gatherv(const jcntrl_mpi_controller *controller,
                                  jcntrl_data_array *dest,
                                  jcntrl_data_array *src, int ntuple, int root);

JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_allgatherv(const jcntrl_mpi_controller *controller,
                                     jcntrl_data_array *dest,
                                     jcntrl_data_array *src, int ntuple);

/**
 * Perform in-place operation if @p dest == @p src. However, since this check
 * has been done by comparing the pointers to the array object, @p src must not
 * refer @p dest.
 *
 * @p dest will be resized if not enough. To get transfered size, resize to 0
 * prior to call.
 *
 * This function supports differently-sized @p src.
 */
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_gatherva(const jcntrl_mpi_controller *controller,
                                   jcntrl_data_array *dest,
                                   jcntrl_data_array *src, int root);

JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_allgatherva(const jcntrl_mpi_controller *controller,
                                      jcntrl_data_array *dest,
                                      jcntrl_data_array *src);

/**
 * Collects the values from @p src at indices spcified by @p srcidx and stores
 * them at indices specified by @p dstidx. @p dstidx must be the array of the
 * sizes of number of processes: jcntrl_mpi_controller_nproc(controller).
 *
 * @p dstidx should not overlap among ranks, the value at overlapped index will
 * be undefined.
 *
 * Perform in-place operation if @p dest == @p src. However, since this check
 * has been done by comparing the pointers to the array object, @p src must not
 * refer @p dest.
 *
 * The element type of @p dstidx and @p srcidx shall be jcntrl_aint_array.
 * If not, converts it to jcntrl_aint_array.
 */
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_gathervi(const jcntrl_mpi_controller *controller,
                                   jcntrl_data_array *dest,
                                   jcntrl_data_array **dstidx,
                                   jcntrl_data_array *src,
                                   jcntrl_data_array *srcidx, int root);

JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_allgathervi(const jcntrl_mpi_controller *controller,
                                      jcntrl_data_array *dest,
                                      jcntrl_data_array **dstidx,
                                      jcntrl_data_array *src,
                                      jcntrl_data_array *srcidx);

/**
 * reduce. Perform in-place operation if @p dest == @p src.
 */
JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_reduce(const jcntrl_mpi_controller *controller,
                                 jcntrl_data_array *dest,
                                 jcntrl_data_array *src,
                                 const jcntrl_reduce_op *op, int root);

JUPITER_CONTROL_DECL
int jcntrl_mpi_controller_allreduce(const jcntrl_mpi_controller *controller,
                                    jcntrl_data_array *dest,
                                    jcntrl_data_array *src,
                                    const jcntrl_reduce_op *op);

JUPITER_CONTROL_DECL_END

#endif
