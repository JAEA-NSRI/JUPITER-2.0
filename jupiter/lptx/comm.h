#ifndef JUPITER_LPTX_COMM_H
#define JUPITER_LPTX_COMM_H

#include "defs.h"

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

JUPITER_LPTX_DECL_START

#ifdef JUPITER_LPTX_MPI

/**
 * @brief MPI_Error_string() supports LPTX MPI error codes.
 */
JUPITER_LPTX_DECL
int LPTX_MPI_Error_string(int errorcode, char *string, int *resultlen);

//----

/**
 * @brief Create MPI Datatype for LPTX_vector.
 * @param out Datatype to output
 * @return MPI result
 *
 * Use MPI_Datatype_free() to free the generated datatype. The datatype will be
 * committed before return, i.e., the datatype will be immediately available for
 * communication.
 *
 * You can do any communication for array of LPTX_vector, while it is plain data
 * type.
 */
JUPITER_LPTX_DECL
int LPTX_vector_mpi_type(MPI_Datatype *out);

/**
 * @brief Create MPI Datatype for LPTX_particle.
 * @param out Datatype to output
 * @return MPI result
 *
 * Use MPI_Datatype_free() to free the generated datatype. The datatype will be
 * committed before return, i.e., the datatype will be immediately available for
 * communication.
 *
 * You can do any communication for array of LPTX_particle, while it is plain
 * data type.
 *
 * @warning This is not for LPTX_particle_set. Do not confuse.
 */
JUPITER_LPTX_DECL
int LPTX_particle_mpi_type(MPI_Datatype *out);

/**
 * @brief Create MPI Datatype for LPTX_particle_data.
 * @param out Datatype to output
 * @return MPI result
 *
 * Use MPI_Datatype_free() to free the generated datatype. The datatype will be
 * committed before return, i.e., the datatype will be immediately available for
 * communication.
 *
 * You can do any communication for array of LPTX_particle_data. However, only
 * POD contents (LPTX_particle base) will be communicated.
 *
 * @warning This is not for LPTX_particle_set. Do not confuse.
 */
JUPITER_LPTX_DECL
int LPTX_particle_data_mpi_type(MPI_Datatype *out);

//----

/**
 * @brief Allocate particles with number_of_particles in @p dest rank.
 * @return Any MPI Error
 * @retval LPTX_MPI_ERR_REMOTE Memory alloaction failed at receiver's rank.
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_replicate_send(const LPTX_particle_set *set,
                                     LPTX_idtype number_of_particles, int dest,
                                     int tag, MPI_Comm comm);

/**
 * @brief Allocate particles from acoording to the set in @p source rank
 *        specification.
 * @return Any MPI Error
 * @retval LPTX_MPI_ERR_NO_MEM Memory allocation failed
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_replicate_recv(LPTX_particle_set **outset, int source,
                                     int tag, MPI_Comm comm);

/**
 * @brief Allocate particles according @p number_of_particles which
 *        specified in @p root rank.
 * @param set Vector information source (required in @p set_root)
 * @param number_of_paritcles Number of particles to allocate (required in @p
 *        np_root)
 * @param np_root Rank number which broadcasts the number of particles
 * @param set_root Rank number which broadcasts the vector info (see below)
 * @return Any MPI Error
 * @retval LPTX_MPI_ERR_NO_MEM Memory allocation failed
 *
 * @p set_root is allowed to be MPI_ANY_SOURCE. Then, first tries the @p set
 * specified in @p np_root rank. If it's NULL, @p set_root will be chosen
 * from the first rank that @p set is not NULL.
 *
 * If @p allocate_in_root is LPTX_false in set_root, skips to allocate new
 * particle set in @p np_root rank. In this case @p *outset will be set to NULL.
 *
 * In the normal particle communication flow, @p np_root should be equal to @p
 * set_root, as broadcasting a particle set.
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_replicate_bcast(LPTX_particle_set **outset,
                                      const LPTX_particle_set *set,
                                      LPTX_idtype number_of_particles,
                                      int np_root, int set_root,
                                      LPTX_bool allocate_in_root,
                                      MPI_Comm comm);

/**
 * @brief Allocate particles in @p root rank with total number of
 *        @p number_of_particles from each rank.
 * @param gather_root Rank number which gathers onto
 * @param set_root Rank number which the source of the vector info (see below)
 * @return Any MPI Error
 * @retval LPTX_MPI_ERR_NO_MEM   Memory allocation failed (in @p root rank)
 * @retval LPTX_MPI_ERR_REMOTE   Memory allocation failed at @p root rank
 *                               (not in @p root rank)
 * @retval LPTX_MPI_ERR_OVERFLOW Too many particles to allocate
 *
 * @warning The content of @p outset is undefined for the ranks other than @p
 *          gather_root.
 *
 * @p set_root is allowed to be MPI_ANY_SOURCE. Then, first tries the @p set
 * specified in @p gather_root rank. If it's NULL, @p set_root will be chosen
 * from the first rank that @p set is not NULL.
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_replicate_gather(LPTX_particle_set **outset,
                                       const LPTX_particle_set *set,
                                       LPTX_idtype number_of_particles,
                                       int gather_root, int set_root,
                                       MPI_Comm comm);

/**
 * @brief Same as LPTX_particle_set_replicate_gather() but allocates in
 *        all ranks.
 * @param set_root Rank number which the source of the vector info (see below)
 * @return Any MPI Error
 * @retval LPTX_MPI_ERR_NO_MEM   Memory allocation failed
 * @retval LPTX_MPI_ERR_REMOTE   Memory allocation failed at any other rank
 * @retval LPTX_MPI_ERR_OVERFLOW Too many particles to allocate
 *
 * @p set_root is allowed to be MPI_ANY_SOURCE. Then, first checks the @p set is
 * provided in all ranks. If @p set is provided in all ranks, we use it (and
 * does not check consistency). Otherwise @p set_root will be chosen from the
 * first rank that @p set is not NULL.
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_replicate_allgather(LPTX_particle_set **outset,
                                          const LPTX_particle_set *set,
                                          LPTX_idtype number_of_particles,
                                          int set_root, MPI_Comm comm);

/**
 * @brief Allocate particles according @p set and @p numbers_of_particles which
 *        speciefied in @p root rank.
 * @param set_root Rank number which the source of the vector info (see below)
 * @return Any MPI Error
 * @retval LPTX_MPI_ERR_NO_MEM Memory allocation failed
 * @retval LPTX_MPI_ERR_REMOTE Memory allocation failed at any other rank
 *
 * @p numbers_of_particles in @p scatter_root rank specifies allocate sizes for
 * each ranks; requires the array of comm size. @p numbers_of_particles other
 * than @p scatter_root rank is ignored.
 *
 * @p set_root is allowed to be MPI_ANY_SOURCE. Then, first tries the @p set
 * specified in @p scatter_root rank. If it's NULL, @p set_root will be chosen
 * from the first rank that @p set is not NULL.
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_replicate_scatter(LPTX_particle_set **outset,
                                        const LPTX_particle_set *set,
                                        const LPTX_idtype *numbers_of_particles,
                                        int scatter_root, int set_root,
                                        MPI_Comm comm);

/**
 * @brief Allocate particles according @p set and @p numbers_of_particles which
 *        specified in total of each ranks.
 * @param set_root Rank number which the source of the vector info (see below)
 * @return Any MPI Error
 * @retval LPTX_MPI_ERR_NO_MEM   Memory allocation failed
 * @retval LPTX_MPI_ERR_REMOTE   Error on other rank(s)
 * @retval LPTX_MPI_ERR_OVERFLOW Too many particles to allocate
 *
 * @p set_root is allowed to be MPI_ANY_SOURCE. Then, first checks the @p set is
 * provided in all ranks. If @p set is provided in all ranks, we use it (and
 * does not check consistency). Otherwise @p set_root will be chosen from the
 * first rank that @p set is not NULL.
 *
 * @p numbers_of_particles specifies allocate sizes for each ranks; requires the
 * array of comm size.
 *
 * Example:
 *
 *     number_of_particles [0] [1] [2] [3] | Allocates:
 *     rank 0               4   1   5   8  |    8
 *     rank 1               3   2   1   4  |    9
 *     rank 2               1   4   6   2  |   12
 *     rank 3               0   2   0   4  |   18
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_replicate_alltoall(
  LPTX_particle_set **outset, const LPTX_particle_set *set,
  const LPTX_idtype *numbers_of_particles, int set_root, MPI_Comm comm);

//----

/**
 * @brief Test communicatability in this rank's @p set and @p peer rank's one.
 * @param set Particle set to test
 * @param peer Peer rank
 * @param tag Tag value while communication
 * @param comm MPI communicator
 * @param ier Returns MPI error
 *
 * The condition is mostly same as LPTX_particle_set_are_mergeable(), excepted
 * for returns LPTX_false if either peer of @p set is NULL.
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_particle_set_is_communicatable(const LPTX_particle_set *set,
                                              int peer, int tag, MPI_Comm comm,
                                              int *ier);

/**
 * @brief Test communicatability in all ranks' @p set.
 * @param set Particle set to test
 * @param comm MPI communicator
 * @param ier Returns MPI error
 *
 * The condition is mostly same as LPTX_particle_set_are_mergeable(), excepted
 * for returns LPTX_false if any rank of @p set is NULL.
 */
JUPITER_LPTX_DECL
LPTX_bool LPTX_particle_set_is_collectable(const LPTX_particle_set *set,
                                           MPI_Comm comm, int *ier);

//----

/**
 * @brief Send particle range to another rank
 * @return Any LPTX MPI error
 *
 * You can send particles for receiving by LPTX_particle_recv() or
 * LPTX_particle_recvv().
 */
JUPITER_LPTX_DECL
int LPTX_particle_send(const LPTX_particle_set *set, LPTX_idtype start,
                       int count, int dest, int tag, MPI_Comm comm);

/**
 * @brief Receive particle range from another rank
 * @return Any LPTX MPI error
 *
 * You can receive particles sent by LPTX_particle_send() or
 * LPTX_particle_sendv().
 */
JUPITER_LPTX_DECL
int LPTX_particle_recv(LPTX_particle_set *set, LPTX_idtype start, int count,
                       int source, int tag, MPI_Comm comm);

/**
 * @brief Send particle at specified (random-ordered) indices to another rank
 * @return Any LPTX MPI error
 *
 * You can send particles for receiving by LPTX_particle_recv() or
 * LPTX_particle_recvv().
 */
JUPITER_LPTX_DECL
int LPTX_particle_sendv(const LPTX_particle_set *set,
                        LPTX_idtype size_of_indices, const LPTX_idtype *indices,
                        int dest, int tag, MPI_Comm comm);

/**
 * @brief Receive particle at specified (random-ordered) indices from another
 * rank
 * @return Any MPI error
 * @return Any LPTX MPI error
 *
 * You can receive particles sent by LPTX_particle_send() or
 * LPTX_particle_sendv().
 */
JUPITER_LPTX_DECL
int LPTX_particle_recvv(LPTX_particle_set *set, LPTX_idtype size_of_indices,
                        const LPTX_idtype *indices, int source, int tag,
                        MPI_Comm comm);

//----

/**
 * @brief Sends all particles (including free particle) in a set to given rank
 * @param set particle set to send
 * @param dest Destination rank
 * @return MPI communication result
 * @retval LPTX_MPI_ERR_NO_MEM Allocation failed
 * @retval LPTX_MPI_ERR_REMOTE Allocation failed on receiver's rank
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_send(LPTX_particle_set *set, int dest, int tag,
                           MPI_Comm comm);

/**
 * @brief Sends particles in a set to given rank
 * @param set particle set to send
 * @param skip_unused Whether skip unused (free) particles
 * @param send_sorted Send particles in sorted order
 * @param cond Custom particle send conditions
 * @param cond_arg Extra argument for cond_arg
 * @param dest Destination rank
 * @return MPI communication result
 * @retval LPTX_MPI_ERR_NO_MEM Allocation failed
 * @retval LPTX_MPI_ERR_REMOTE Allocation failed on receiver's rank
 *
 * If @p skip_unused or @p send_sorted is LPTX_true or @p cond is passed,
 * this function will build particle indices to send.
 *
 * LPTX_particle_set_send() is same as
 * LPTX_particle_set_sendc(set, LPTX_false, LPTX_false, NULL, NULL, ...);
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_sendc(LPTX_particle_set *set, LPTX_bool skip_unused,
                            LPTX_bool send_sorted, LPTX_cb_particle_if *cond,
                            void *cond_arg, int dest, int tag, MPI_Comm comm);

/**
 * @brief Sends particles at specified index in a set to given rank
 * @param set particle set to send
 * @param size_of_indices Number of index array.
 * @param indices index array to send
 * @param dest Destination rank
 * @return MPI communication result
 * @retval LPTX_MPI_ERR_NO_MEM Allocation failed
 * @retval LPTX_MPI_ERR_REMOTE Allocation failed on receiver's rank
 *
 * If @p skip_unused or @p send_sorted is LPTX_true or @p cond is passed,
 * this function will build particle indices to send.
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_sendv(LPTX_particle_set *set, LPTX_idtype size_of_indices,
                            LPTX_idtype *indices, int dest, int tag,
                            MPI_Comm comm);

/**
 * @brief Receives particles as a new set from given rank
 * @return MPI communication result
 * @retval MPI_SUCCESS success
 * @retval LPTX_MPI_ERR_NO_MEM Allocation failed
 * @retval LPTX_MPI_ERR_REMOTE Allocation failed on receiver's rank
 *
 * @p *set will always allocate new set to store particles with received
 * size. Particles are always stored from index 0 without gap.
 *
 * Receives particles from LPTX_particle_set_send(), LPTX_particle_set_sendc()
 * or LPTX_particle_set_sendv().
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_recv(LPTX_particle_set **set, int source, int tag,
                           MPI_Comm comm);

/**
 * @brief Gathers particles from each rank
 * @param out Result output particle
 * @param inp Particle set to gather
 * @param root Rank to gather to
 * @param rank Array of destination ranks for each particle.
 *
 * @p *out will always allocate new set to store particles.
 *
 * @note @p *out in other than @p root rank is undefined.
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_gather(LPTX_particle_set **out, LPTX_particle_set *inp,
                             int root, MPI_Comm comm);

/**
 * @brief Gathers particles to all ranks from each rank
 * @param out Result output particle
 * @param inp Particle set to gather
 * @param rank Array of destination ranks for each particle.
 *
 * @p *out will always allocate new set to store particles.
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_allgather(LPTX_particle_set **out, LPTX_particle_set *inp,
                                MPI_Comm comm);

/**
 * @brief Scatter particles to each rank
 * @param out Result output particle
 * @param inp Particle set to scatter
 * @param start Starting index for @p inp
 * @param root Rank to scatter from
 * @param size_of_rank Size of @p rank
 * @param rank Array of destination ranks for each particle.
 *
 * Scatters @p size_of_rank particles from @p start, to the rank given by @p
 * rank; similar to MPI_Scatterv for particles.
 *
 * You can pass NULL to @p rank if the calling rank is not @p root.
 *
 * @p *out will always allocate new set to store particles.
 */
JUPITER_LPTX_DECL
int LPTX_particle_set_scatter(LPTX_particle_set **out,
                              const LPTX_particle_set *inp, LPTX_idtype start,
                              int root, int size_of_rank, const int *rank,
                              MPI_Comm comm);

JUPITER_LPTX_DECL
int LPTX_particle_set_alltoall(LPTX_particle_set **out,
                               const LPTX_particle_set *inp, LPTX_idtype start,
                               int size_of_rank, const int *rank,
                               MPI_Comm comm);
#endif

JUPITER_LPTX_DECL_END

#endif
