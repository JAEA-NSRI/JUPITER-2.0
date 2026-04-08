#include "mpi_controller.h"
#include "data_array.h"
#include "defs.h"
#include "error.h"
#include "overflow.h"
#include "reduce_op.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "static_array.h"
#include "subarray.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef JCNTRL_USE_MPI
#include <mpi.h>
#endif

struct jcntrl_mpi_controller
{
  jcntrl_shared_object object;
#ifdef JCNTRL_USE_MPI
  MPI_Comm comm;
#endif
};
#define jcntrl_mpi_controller__ancestor jcntrl_shared_object
#define jcntrl_mpi_controller__dnmem object
JCNTRL_VTABLE_NONE(jcntrl_mpi_controller);

static int jcntrl_safe_mpi(int ret, const char *file, int line,
                           const char *expr)
{
#ifdef JCNTRL_USE_MPI
  if (ret == MPI_SUCCESS)
    return 1;
#endif

  jcntrl_raise_mpi_error(file, line, ret, expr);
  return 0;
}

#define jcntrl_safe_mpi(func) jcntrl_safe_mpi(func, __FILE__, __LINE__, #func)

static jcntrl_mpi_controller *
jcntrl_mpi_controller_downcast_impl(jcntrl_shared_object *object)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_mpi_controller, object);
}

static void *jcntrl_mpi_controller_downcast_v(jcntrl_shared_object *object)
{
  return jcntrl_mpi_controller_downcast_impl(object);
}

static int jcntrl_mpi_controller_initializer(jcntrl_shared_object *object)
{
  jcntrl_mpi_controller *controller;
  controller = jcntrl_mpi_controller_downcast_impl(object);
#ifdef JCNTRL_USE_MPI
  controller->comm = MPI_COMM_NULL;
#endif
  return 1;
}

static void jcntrl_mpi_controller_destructor(jcntrl_shared_object *object)
{
  jcntrl_mpi_controller *controller;
  controller = jcntrl_mpi_controller_downcast_impl(object);
#ifdef JCNTRL_USE_MPI
  /**
   * @note the destructor for the controller of MPI_COMM_WORLD or MPI_COMM_SELF
   * will usually not to be called. This condition exists for errornously called
   * it.
   */
  if (!jcntrl_shared_object_is_static(
        jcntrl_mpi_controller_object(controller)) &&
      controller->comm != MPI_COMM_NULL) {
    jcntrl_safe_mpi(MPI_Comm_free(&controller->comm));

    // MPI_Comm_free is assumed to set comm to MPI_COMM_NULL. Explicitly set to
    // ensure.
    controller->comm = MPI_COMM_NULL;
  }
#endif
  (void)controller;
}

static jcntrl_shared_object *jcntrl_mpi_controller_allocator(void)
{
  jcntrl_mpi_controller *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_mpi_controller);
  return p ? jcntrl_mpi_controller_object(p) : NULL;
}

static void jcntrl_mpi_controller_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static void jcntrl_mpi_controller_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_mpi_controller_downcast_v;
  p->initializer = jcntrl_mpi_controller_initializer;
  p->destructor = jcntrl_mpi_controller_destructor;
  p->allocator = jcntrl_mpi_controller_allocator;
  p->deleter = jcntrl_mpi_controller_deleter;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mpi_controller,
                                   jcntrl_mpi_controller_init_func)

jcntrl_mpi_controller *jcntrl_mpi_controller_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mpi_controller);
}

void jcntrl_mpi_controller_delete(jcntrl_mpi_controller *controller)
{
  jcntrl_shared_object_delete(jcntrl_mpi_controller_object(controller));
}

const jcntrl_mpi_controller *jcntrl_mpi_controller_self(void)
{
  static jcntrl_mpi_controller jcntrl_mpi_self_controller;
  static int jcntrl_mpi_self_controller_inited = 0;
  if (!jcntrl_mpi_self_controller_inited) {
    jcntrl_shared_object_static_init(&jcntrl_mpi_self_controller.object,
                                     jcntrl_mpi_controller_metadata_init());
#ifdef JCNTRL_USE_MPI
    jcntrl_mpi_self_controller.comm = MPI_COMM_SELF;
#endif
    jcntrl_mpi_self_controller_inited = 1;
  }
  return &jcntrl_mpi_self_controller;
}

/**
 * Returns controller for MPI_COMM_WORLD. This object cannot be deleted.
 */
const jcntrl_mpi_controller *jcntrl_mpi_controller_world(void)
{
  static jcntrl_mpi_controller jcntrl_mpi_world_controller;
  static int jcntrl_mpi_world_controller_inited = 0;
  if (!jcntrl_mpi_world_controller_inited) {
    jcntrl_shared_object_static_init(&jcntrl_mpi_world_controller.object,
                                     jcntrl_mpi_controller_metadata_init());
#ifdef JCNTRL_USE_MPI
    jcntrl_mpi_world_controller.comm = MPI_COMM_WORLD;
#endif
    jcntrl_mpi_world_controller_inited = 1;
  }
  return &jcntrl_mpi_world_controller;
}

#ifdef JCNTRL_USE_MPI
MPI_Comm jcntrl_mpi_controller_get_comm(jcntrl_mpi_controller *controller)
{
  return controller->comm;
}

int jcntrl_mpi_controller_set_comm(jcntrl_mpi_controller *controller,
                                   MPI_Comm comm)
{
  if (controller->comm != MPI_COMM_NULL)
    if (!jcntrl_safe_mpi(MPI_Comm_free(&controller->comm)))
      return 0;
  return jcntrl_safe_mpi(MPI_Comm_dup(comm, &controller->comm));
}
#endif

int jcntrl_mpi_controller_nproc(const jcntrl_mpi_controller *controller)
{
  int r;
  int nproc;
#ifdef JCNTRL_USE_MPI
  nproc = 0;
  if (!jcntrl_safe_mpi(MPI_Comm_size(controller->comm, &nproc)))
    return 0;
#else
  nproc = 1;
#endif
  return nproc;
}

int jcntrl_mpi_controller_rank(const jcntrl_mpi_controller *controller)
{
  int r;
  int rank;
#ifdef JCNTRL_USE_MPI
  rank = -1;
  if (!jcntrl_safe_mpi(MPI_Comm_rank(controller->comm, &rank)))
    return -1;
#else
  rank = 0;
#endif
  return rank;
}

jcntrl_shared_object *
jcntrl_mpi_controller_object(jcntrl_mpi_controller *controller)
{
  return &controller->object;
}

jcntrl_mpi_controller *
jcntrl_mpi_controller_downcast(jcntrl_shared_object *object)
{
  return jcntrl_shared_object_downcast(jcntrl_mpi_controller, object);
}

jcntrl_mpi_controller *
jcntrl_mpi_controller_split(const jcntrl_mpi_controller *controller, //
                            int color, int key)
{
#ifdef JCNTRL_USE_MPI
  MPI_Comm ncomm;
  jcntrl_mpi_controller *c;
  int r;

  JCNTRL_ASSERT(controller);

  ncomm = MPI_COMM_NULL;
  if (!jcntrl_safe_mpi(MPI_Comm_split(controller->comm, color, key, &ncomm)))
    return NULL;

  c = jcntrl_mpi_controller_new();
  if (jcntrl_mpi_controller_any(controller, !c)) {
    jcntrl_safe_mpi(MPI_Comm_free(&ncomm));
    if (c)
      jcntrl_mpi_controller_delete(c);
    return NULL;
  }

  c->comm = ncomm;
  return c;
#else
  return jcntrl_mpi_controller_new();
#endif
}

int jcntrl_mpi_controller_any(const jcntrl_mpi_controller *controller, int cond)
{
#ifdef JCNTRL_USE_MPI
  int r;

  JCNTRL_ASSERT(controller);

  jcntrl_safe_mpi(
    MPI_Allreduce(MPI_IN_PLACE, &cond, 1, MPI_INT, MPI_LOR, controller->comm));
  return cond;
#else
  JCNTRL_ASSERT(controller);
  return cond;
#endif
}

int jcntrl_mpi_controller_all(const jcntrl_mpi_controller *controller, int cond)
{
#ifdef JCNTRL_USE_MPI
  int r;

  JCNTRL_ASSERT(controller);

  jcntrl_safe_mpi(
    MPI_Allreduce(MPI_IN_PLACE, &cond, 1, MPI_INT, MPI_LAND, controller->comm));
  return cond;
#else
  JCNTRL_ASSERT(controller);
  return cond;
#endif
}

#ifdef JCNTRL_USE_MPI
static int jcntrl_mpi_get_elementtype(jcntrl_data_array *array,
                                      MPI_Datatype *out, int *blocklen)
{
  const jcntrl_shared_object_data *d;
  jcntrl_size_type elsize;

  elsize = jcntrl_data_array_element_size(array);
  d = jcntrl_data_array_element_type(array);

  if (elsize <= 0) {
    jcntrl_raise_argument_error(__FILE__, __LINE__, "Invalid element size");
    return 0;
  }
  if (elsize > INT_MAX) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return 0;
  }

  *blocklen = 1;
  if (d == jcntrl_char_array_metadata_init()) {
    *out = MPI_CHAR;
  } else if (d == jcntrl_bool_array_metadata_init()) {
    *out = MPI_CHAR;
  } else if (d == jcntrl_int_array_metadata_init()) {
    *out = MPI_INT;
  } else if (d == jcntrl_float_array_metadata_init()) {
    *out = MPI_FLOAT;
  } else if (d == jcntrl_double_array_metadata_init()) {
    *out = MPI_DOUBLE;
  } else if (d == jcntrl_aint_array_metadata_init()) {
    *out = MPI_AINT;
  } else {
    *out = MPI_CHAR;
    *blocklen = elsize;
  }
  return 1;
}

static int jcntrl_mpi_create_hvector_type(int count, int blocklength,
                                          MPI_Aint stride, MPI_Datatype oldtype,
                                          MPI_Datatype *newtype)
{
  int r;

  if (!jcntrl_safe_mpi(
        MPI_Type_create_hvector(count, blocklength, stride, oldtype, newtype)))
    return 0;

  if (!jcntrl_safe_mpi(MPI_Type_commit(newtype))) {
    jcntrl_safe_mpi(MPI_Type_free(newtype));
    return 0;
  }

  return 1;
}

static int jcntrl_mpi_create_elementtype(jcntrl_data_array *array,
                                         MPI_Datatype *out, int *allocd)
{
  MPI_Datatype eltype;
  int blocklen;

  if (!jcntrl_mpi_get_elementtype(array, &eltype, &blocklen))
    return 0;

  JCNTRL_ASSERT(blocklen > 0);

  if (blocklen == 1) {
    *out = eltype;
    *allocd = 0;
    return 1;
  }

  if (!jcntrl_mpi_create_hvector_type(1, blocklen, blocklen, eltype, out))
    return 0;

  *allocd = 1;
  return 1;
}

static int jcntrl_mpi_create_vectortype(jcntrl_data_array *array, int ntuple,
                                        MPI_Datatype *out)
{
  jcntrl_size_type elsize;
  MPI_Datatype eltype;
  int blocklen;

  JCNTRL_ASSERT(ntuple > 0);

  if (!jcntrl_mpi_get_elementtype(array, &eltype, &blocklen))
    return 0;

  elsize = jcntrl_data_array_element_size(array);

  JCNTRL_ASSERT(elsize > 0);
  JCNTRL_ASSERT(blocklen > 0);

  if (jcntrl_mpi_create_hvector_type(ntuple, blocklen, elsize, eltype, out))
    return 1;
  return 0;
}

static int jcntrl_mpi_create_arraytype(jcntrl_data_array *array,
                                       MPI_Datatype *out)
{
  jcntrl_size_type ntuple;

  ntuple = jcntrl_data_array_get_ntuple(array);
  if (ntuple > INT_MAX) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return 0;
  }

  return jcntrl_mpi_create_vectortype(array, ntuple, out);
}

static int jcntrl_mpi_p2p_share_error(const jcntrl_mpi_controller *controller,
                                      int peer, int cond)
{
  int r, rcond;

  JCNTRL_ASSERT(peer != jcntrl_mpi_controller_rank(controller));

  if (!jcntrl_safe_mpi(MPI_Sendrecv(&cond, 1, MPI_INT, peer, 1, &rcond, 1,
                                    MPI_INT, peer, 1, controller->comm,
                                    MPI_STATUS_IGNORE)))
    return 0;
  return cond && rcond;
}
#endif

int jcntrl_mpi_controller_send(const jcntrl_mpi_controller *controller,
                               jcntrl_data_array *array, int ntuple,
                               int dest_rank, int tag)
{
#ifdef JCNTRL_USE_MPI
  MPI_Datatype type;
  int ret;

  ret = jcntrl_mpi_create_vectortype(array, ntuple, &type);
  if (!jcntrl_mpi_p2p_share_error(controller, dest_rank, ret)) {
    if (ret)
      jcntrl_safe_mpi(MPI_Type_free(&type));
    return 0;
  }

  ret = jcntrl_safe_mpi(MPI_Send(jcntrl_data_array_get(array), 1, type,
                                 dest_rank, tag, controller->comm));

  if (!jcntrl_safe_mpi(MPI_Type_free(&type)))
    ret = 0;
  return jcntrl_mpi_p2p_share_error(controller, dest_rank, ret);
#else
  return 1;
#endif
}

int jcntrl_mpi_controller_recv(const jcntrl_mpi_controller *controller,
                               jcntrl_data_array *array, int ntuple,
                               int src_rank, int tag)
{
#ifdef JCNTRL_USE_MPI
  MPI_Datatype type;
  int ret;

  ret = jcntrl_mpi_create_vectortype(array, ntuple, &type);
  if (!jcntrl_mpi_p2p_share_error(controller, src_rank, ret)) {
    if (ret)
      jcntrl_safe_mpi(MPI_Type_free(&type));
    return 0;
  }

  ret = jcntrl_safe_mpi(MPI_Recv(jcntrl_data_array_get_writable(array), 1, type,
                                 src_rank, tag, controller->comm,
                                 MPI_STATUS_IGNORE));

  if (!jcntrl_safe_mpi(MPI_Type_free(&type)))
    ret = 0;
  return jcntrl_mpi_p2p_share_error(controller, src_rank, ret);
#else
  return 1;
#endif
}

int jcntrl_mpi_controller_senda(const jcntrl_mpi_controller *controller,
                                jcntrl_data_array *array, int dest_rank,
                                int tag)
{
#ifdef JCNTRL_USE_MPI
  MPI_Datatype type;
  jcntrl_static_size_array ntuplep;
  int ret;

  JCNTRL_ASSERT(controller);

  jcntrl_static_size_array_init_n(&ntuplep, 1,
                                  jcntrl_data_array_get_ntuple(array));

  if (!jcntrl_mpi_controller_send(controller,
                                  jcntrl_static_size_array_data(&ntuplep), 1,
                                  dest_rank, tag))
    return 0;

  ret = jcntrl_mpi_create_arraytype(array, &type);
  if (!jcntrl_mpi_p2p_share_error(controller, dest_rank, ret))
    return 0;

  ret = jcntrl_safe_mpi(MPI_Send(jcntrl_data_array_get(array), 1, type,
                                 dest_rank, tag, controller->comm));

  if (!jcntrl_safe_mpi(MPI_Type_free(&type)))
    ret = 0;
  return jcntrl_mpi_p2p_share_error(controller, dest_rank, ret);
#else
  return 1;
#endif
}

int jcntrl_mpi_controller_recva(const jcntrl_mpi_controller *controller,
                                jcntrl_data_array *array, int src_rank, int tag)
{
#ifdef JCNTRL_USE_MPI
  MPI_Datatype type;
  jcntrl_static_size_array ntuplep;
  int ret;

  JCNTRL_ASSERT(controller);

  jcntrl_static_size_array_init_n(&ntuplep, 1, 0);

  if (!jcntrl_mpi_controller_recv(controller,
                                  jcntrl_static_size_array_data(&ntuplep), 1,
                                  src_rank, tag))
    return 0;

  type = MPI_DATATYPE_NULL;
  ret =
    !!jcntrl_data_array_resize(array, *jcntrl_static_size_array_get(&ntuplep));

  if (ret)
    ret = jcntrl_mpi_create_arraytype(array, &type);

  if (!jcntrl_mpi_p2p_share_error(controller, src_rank, ret))
    return 0;

  ret = jcntrl_safe_mpi(MPI_Recv(jcntrl_data_array_get_writable(array), 1, type,
                                 src_rank, tag, controller->comm,
                                 MPI_STATUS_IGNORE));

  if (!jcntrl_safe_mpi(MPI_Type_free(&type)))
    ret = 0;

  return jcntrl_mpi_p2p_share_error(controller, src_rank, ret);
#else
  return 1;
#endif
}

int jcntrl_mpi_controller_bcast(const jcntrl_mpi_controller *controller,
                                jcntrl_data_array *ary, int ntuple, int root)
{
#ifdef JCNTRL_USE_MPI
  MPI_Datatype type;
  void *aryp;
  int ret;

  JCNTRL_ASSERT(controller);

  if (jcntrl_mpi_controller_rank(controller) == root) {
    aryp = (void *)jcntrl_data_array_get(ary);
  } else {
    aryp = jcntrl_data_array_get_writable(ary);
  }
  ret = !!aryp;

  type = MPI_DATATYPE_NULL;
  if (!jcntrl_mpi_create_vectortype(ary, ntuple, &type))
    ret = 0;

  if (!jcntrl_mpi_controller_all(controller, ret)) {
    if (type != MPI_DATATYPE_NULL)
      jcntrl_safe_mpi(MPI_Type_free(&type));
    return 0;
  }

  ret = jcntrl_safe_mpi(MPI_Bcast(aryp, 1, type, root, controller->comm));

  if (!jcntrl_safe_mpi(MPI_Type_free(&type)))
    ret = 0;

  return jcntrl_mpi_controller_all(controller, ret);
#else
  return 1; /* nop */
#endif
}

int jcntrl_mpi_controller_bcasta(const jcntrl_mpi_controller *controller,
                                 jcntrl_data_array *ary, int root)
{
#ifdef JCNTRL_USE_MPI
  int ret;
  int rank;
  jcntrl_size_type sz;
  jcntrl_static_size_array sza;
  jcntrl_data_array *d;

  JCNTRL_ASSERT(controller);

  jcntrl_static_size_array_init_base(&sza, &sz, 1);

  rank = jcntrl_mpi_controller_rank(controller);
  if (rank == root) {
    sz = jcntrl_data_array_get_ntuple(ary);
  }

  d = jcntrl_static_size_array_data(&sza);
  if (!jcntrl_mpi_controller_bcast(controller, d, 1, root))
    return 0;

  ret = 1;
  if (rank != root) {
    if (!jcntrl_data_array_resize(ary, sz))
      ret = 0;
  }
  if (!jcntrl_mpi_controller_all(controller, ret))
    ret = 0;

  if (sz == 0)
    return 1;
  return jcntrl_mpi_controller_bcast(controller, ary, sz, root);
#else
  return 1; /* nop */
#endif
}

static int jcntrl_mpi_controller_nompi_copy(jcntrl_data_array *dest,
                                            jcntrl_data_array *src, int ntuple)
{
  if (src != dest) {
    if (jcntrl_data_array_get_ntuple(dest) < ntuple)
      if (!jcntrl_data_array_resize(dest, ntuple))
        return 0;

    if (!jcntrl_data_array_copy(dest, src, ntuple, 0, 0))
      return 0;
  }
  return 1;
}

static int jcntrl_mpi_controller_gather_resize(int ntuple, int nproc,
                                               jcntrl_data_array *dest)
{
  jcntrl_size_type snproc;
  jcntrl_size_type sz;

  JCNTRL_ASSERT(ntuple > 0);
  JCNTRL_ASSERT(nproc > 0);

  snproc = nproc;
  if (snproc != nproc) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return 0;
  }

  sz = ntuple;
  if (sz != ntuple) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return 0;
  }

  if (jcntrl_s_mul_overflow(sz, snproc, &sz))
    return 0;

  if (jcntrl_data_array_get_ntuple(dest) < sz)
    return !!jcntrl_data_array_resize(dest, sz);
  return 1;
}

static int jcntrl_mpi_controller_gather_resize_c(
  int ntuple, const jcntrl_mpi_controller *controller, jcntrl_data_array *dest)
{
  int nproc = jcntrl_mpi_controller_nproc(controller);
  return jcntrl_mpi_controller_gather_resize(ntuple, nproc, dest);
}

int jcntrl_mpi_controller_gather(const jcntrl_mpi_controller *controller,
                                 jcntrl_data_array *dest,
                                 jcntrl_data_array *src, int ntuple, int root)
{
#ifdef JCNTRL_USE_MPI
  MPI_Datatype type;
  int ret;

  JCNTRL_ASSERT(controller);
  JCNTRL_ASSERT(ntuple > 0);

  ret = 1;
  if (jcntrl_mpi_controller_rank(controller) == root)
    ret = jcntrl_mpi_controller_gather_resize_c(ntuple, controller, dest);

  if (ret)
    ret = jcntrl_mpi_create_vectortype(src, ntuple, &type);

  if (!jcntrl_mpi_controller_all(controller, ret)) {
    if (ret)
      jcntrl_safe_mpi(MPI_Type_free(&type));
    return 0;
  }

  if (src == dest) {
    ret = jcntrl_safe_mpi(MPI_Gather(MPI_IN_PLACE, 1, type, //
                                     jcntrl_data_array_get_writable(dest), 1,
                                     type, root, controller->comm));
  } else {
    ret = jcntrl_safe_mpi(MPI_Gather(jcntrl_data_array_get(src), 1, type,
                                     jcntrl_data_array_get_writable(dest), 1,
                                     type, root, controller->comm));
  }

  if (!jcntrl_safe_mpi(MPI_Type_free(&type)))
    ret = 0;

  return jcntrl_mpi_controller_all(controller, ret);
#else
  return jcntrl_mpi_controller_nompi_copy(dest, src, ntuple);
#endif
}

int jcntrl_mpi_controller_allgather(const jcntrl_mpi_controller *controller,
                                    jcntrl_data_array *dest,
                                    jcntrl_data_array *src, int ntuple)
{
#ifdef JCNTRL_USE_MPI
  MPI_Datatype type;
  int r, ret, nproc;
  jcntrl_size_type sz;

  JCNTRL_ASSERT(controller);
  JCNTRL_ASSERT(ntuple > 0);

  ret = jcntrl_mpi_controller_gather_resize_c(ntuple, controller, dest);

  if (ret)
    ret = jcntrl_mpi_create_vectortype(src, ntuple, &type);

  if (!jcntrl_mpi_controller_all(controller, ret)) {
    if (!ret)
      MPI_Type_free(&type);
    return 0;
  }

  if (dest == src) {
    ret = jcntrl_safe_mpi(MPI_Allgather(MPI_IN_PLACE, 1, type, //
                                        jcntrl_data_array_get_writable(dest), 1,
                                        type, controller->comm));
  } else {
    ret = jcntrl_safe_mpi(MPI_Allgather(jcntrl_data_array_get(src), 1, type,
                                        jcntrl_data_array_get_writable(dest), 1,
                                        type, controller->comm));
  }

  if (!jcntrl_safe_mpi(MPI_Type_free(&type)))
    ret = 0;
  return jcntrl_mpi_controller_all(controller, ret);
#else
  return jcntrl_mpi_controller_nompi_copy(dest, src, ntuple);
#endif
}

static int jcntrl_mpi_controller_fill_idispls(const int *sizes, int *displs,
                                              int nproc, jcntrl_size_type *tot)
{
  if (!sizes || !displs)
    return 0;

  displs[0] = 0;
  for (int i = 1; i < nproc; ++i)
    displs[i] = displs[i - 1] + sizes[i - 1];
  *tot = displs[nproc - 1] + sizes[nproc - 1];
  return 1;
}

static int jcntrl_mpi_controller_fill_sdispls(const jcntrl_size_type *sizes,
                                              jcntrl_size_type *displs,
                                              int nproc, jcntrl_size_type *tot)
{
  if (!sizes || !displs)
    return 0;

  displs[0] = 0;
  for (int i = 1; i < nproc; ++i)
    displs[i] = displs[i - 1] + sizes[i - 1];
  *tot = displs[nproc - 1] + sizes[nproc - 1];
  return 1;
}

static int jcntrl_mpi_controller_build_displs(
  const jcntrl_mpi_controller *controller, jcntrl_data_array *sizes,
  jcntrl_data_array *displs, jcntrl_data_array *ntuple, jcntrl_size_type *totsz)
{
  const int *isizes;
  int *idispls;
  const jcntrl_size_type *ssizes;
  jcntrl_size_type *sdispls;
  int nproc;
  int ret;

  JCNTRL_ASSERT(jcntrl_mpi_controller_nproc(controller) ==
                jcntrl_data_array_get_ntuple(sizes));
  JCNTRL_ASSERT(jcntrl_data_array_get_ntuple(displs) ==
                jcntrl_data_array_get_ntuple(sizes));
  JCNTRL_ASSERT(jcntrl_data_array_get_ntuple(ntuple) == 1);

  if (!jcntrl_mpi_controller_allgather(controller, sizes, ntuple, 1))
    return 0;

  nproc = jcntrl_mpi_controller_nproc(controller);
  isizes = jcntrl_data_array_get_int(sizes);
  if (isizes) {
    idispls = jcntrl_data_array_get_writable_int(displs);
    ret = jcntrl_mpi_controller_fill_idispls(isizes, idispls, nproc, totsz);
  } else {
    ssizes = jcntrl_data_array_get_sizes(sizes);
    sdispls = jcntrl_data_array_get_writable_sizes(displs);
    ret = jcntrl_mpi_controller_fill_sdispls(ssizes, sdispls, nproc, totsz);
  }
  return jcntrl_mpi_controller_all(controller, ret);
}

/*
 * This function is collective. Pass @p dest to NULL if skip resize (for
 * non-root rank in gather to root rank).
 */
static int jcntrl_mpi_controller_gather_resizev(
  const jcntrl_mpi_controller *controller, jcntrl_data_array *dest,
  jcntrl_data_array *displs, jcntrl_data_array *sizes,
  jcntrl_data_array *ntuple)
{
  int ret;
  jcntrl_size_type totsz;

  totsz = 0;
  if (!jcntrl_mpi_controller_build_displs(controller, sizes, displs, ntuple,
                                          &totsz))
    return 0;

  ret = 1;
  if (dest && jcntrl_data_array_get_ntuple(dest) < totsz) {
    if (!jcntrl_data_array_resize(dest, totsz))
      ret = 0;
  }
  return jcntrl_mpi_controller_all(controller, ret);
}

static int jcntrl_mpi_controller_gather_resizevs(
  const jcntrl_mpi_controller *controller, jcntrl_data_array *dest,
  jcntrl_int_array *ivec, jcntrl_data_subarray **displs,
  jcntrl_data_subarray **sizes, int ntuple)
{
  int ret;
  int nproc;
  jcntrl_data_array *d;
  jcntrl_static_int_array intuple;
  JCNTRL_ASSERT(ivec);
  JCNTRL_ASSERT(displs);
  JCNTRL_ASSERT(sizes);

  jcntrl_static_int_array_init_n(&intuple, 1, ntuple);

  ret = 1;
  nproc = jcntrl_mpi_controller_nproc(controller);
  if ((jcntrl_size_type)nproc > JCNTRL_SIZE_MAX / 2) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    ret = 0;
  }

  if (ret)
    ret = !!jcntrl_int_array_resize(ivec, (jcntrl_size_type)2 * nproc);

  if (ret) {
    d = jcntrl_int_array_data(ivec);
    if (!jcntrl_data_subarray_static_init(*displs, d, 0, nproc)) {
      *displs = NULL;
      ret = 0;
    }
    if (!jcntrl_data_subarray_static_init(*sizes, d, nproc, nproc)) {
      *sizes = NULL;
      ret = 0;
    }
  }

  if (!jcntrl_mpi_controller_all(controller, ret))
    return 0;

  return jcntrl_mpi_controller_gather_resizev(
    controller, dest, jcntrl_data_subarray_data(*displs),
    jcntrl_data_subarray_data(*sizes), jcntrl_static_int_array_data(&intuple));
}

int jcntrl_mpi_controller_gatherv(const jcntrl_mpi_controller *controller,
                                  jcntrl_data_array *dest,
                                  jcntrl_data_array *src, int ntuple, int root)
{
#ifdef JCNTRL_USE_MPI
  MPI_Datatype type;
  int r, ret, nproc, type_allocd;
  jcntrl_size_type sz;
  jcntrl_int_array *ivec;
  jcntrl_static_int_array szv;
  jcntrl_data_subarray displs, sizes;
  jcntrl_data_subarray *pdispls, *psizes;
  jcntrl_data_array *rdest;

  JCNTRL_ASSERT(controller);
  JCNTRL_ASSERT(ntuple >= 0);

  pdispls = &displs;
  psizes = &sizes;

  ret = 1;
  if (!jcntrl_mpi_create_elementtype(src, &type, &type_allocd))
    ret = 0;

  ivec = jcntrl_int_array_new();
  if (!ivec)
    ret = 0;

  if (!jcntrl_mpi_controller_all(controller, ret)) {
    if (ivec)
      jcntrl_int_array_delete(ivec);
    if (type_allocd)
      jcntrl_safe_mpi(MPI_Type_free(&type));
    return 0;
  }

  rdest = (jcntrl_mpi_controller_rank(controller) == root) ? dest : NULL;
  if (!jcntrl_mpi_controller_gather_resizevs(controller, rdest, ivec, &pdispls,
                                             &psizes, ntuple)) {
    jcntrl_int_array_delete(ivec);
    if (pdispls)
      jcntrl_data_subarray_delete(pdispls);
    if (psizes)
      jcntrl_data_subarray_delete(psizes);
    if (type_allocd)
      jcntrl_safe_mpi(MPI_Type_free(&type));
    return 0;
  }

  if (src == dest) {
    ret = jcntrl_safe_mpi(MPI_Gatherv(MPI_IN_PLACE, ntuple, type,
                                      jcntrl_data_array_get_writable(dest),
                                      jcntrl_data_subarray_get_int(&sizes),
                                      jcntrl_data_subarray_get_int(&displs),
                                      type, root, controller->comm));
  } else {
    ret = jcntrl_safe_mpi(MPI_Gatherv(jcntrl_data_array_get(src), ntuple, type,
                                      jcntrl_data_array_get_writable(dest),
                                      jcntrl_data_subarray_get_int(&sizes),
                                      jcntrl_data_subarray_get_int(&displs),
                                      type, root, controller->comm));
  }

  if (type_allocd) {
    if (!jcntrl_safe_mpi(MPI_Type_free(&type)))
      ret = 0;
  }

  jcntrl_data_subarray_delete(pdispls);
  jcntrl_data_subarray_delete(psizes);
  jcntrl_int_array_delete(ivec);
  return jcntrl_mpi_controller_all(controller, ret);
#else
  return jcntrl_mpi_controller_nompi_copy(dest, src, ntuple);
#endif
}

int jcntrl_mpi_controller_allgatherv(const jcntrl_mpi_controller *controller,
                                     jcntrl_data_array *dest,
                                     jcntrl_data_array *src, int ntuple)
{
#ifdef JCNTRL_USE_MPI
  MPI_Datatype type;
  int r, ret, nproc, type_allocd;
  jcntrl_size_type sz;
  jcntrl_int_array *ivec;
  jcntrl_static_int_array szv;
  jcntrl_data_subarray displs, sizes;
  jcntrl_data_subarray *pdispls, *psizes;

  JCNTRL_ASSERT(controller);
  JCNTRL_ASSERT(ntuple >= 0);

  pdispls = &displs;
  psizes = &sizes;

  ret = 1;
  if (!jcntrl_mpi_create_elementtype(src, &type, &type_allocd))
    ret = 0;

  ivec = jcntrl_int_array_new();
  if (!ivec)
    ret = 0;

  if (!jcntrl_mpi_controller_all(controller, ret)) {
    if (ivec)
      jcntrl_int_array_delete(ivec);
    if (type_allocd)
      jcntrl_safe_mpi(MPI_Type_free(&type));
    return 0;
  }

  if (!jcntrl_mpi_controller_gather_resizevs(controller, dest, ivec, &pdispls,
                                             &psizes, ntuple)) {
    jcntrl_int_array_delete(ivec);
    if (pdispls)
      jcntrl_data_subarray_delete(pdispls);
    if (psizes)
      jcntrl_data_subarray_delete(psizes);
    if (type_allocd)
      jcntrl_safe_mpi(MPI_Type_free(&type));
    return 0;
  }

  if (src == dest) {
    ret = jcntrl_safe_mpi(MPI_Allgatherv(MPI_IN_PLACE, ntuple, type,
                                         jcntrl_data_array_get_writable(dest),
                                         jcntrl_data_subarray_get_int(&sizes),
                                         jcntrl_data_subarray_get_int(&displs),
                                         type, controller->comm));
  } else {
    ret =
      jcntrl_safe_mpi(MPI_Allgatherv(jcntrl_data_array_get(src), ntuple, type,
                                     jcntrl_data_array_get_writable(dest),
                                     jcntrl_data_subarray_get_int(&sizes),
                                     jcntrl_data_subarray_get_int(&displs),
                                     type, controller->comm));
  }

  if (type_allocd) {
    if (!jcntrl_safe_mpi(MPI_Type_free(&type)))
      ret = 0;
  }

  jcntrl_data_subarray_delete(pdispls);
  jcntrl_data_subarray_delete(psizes);
  jcntrl_int_array_delete(ivec);
  return jcntrl_mpi_controller_all(controller, ret);
#else
  return jcntrl_mpi_controller_nompi_copy(dest, src, ntuple);
#endif
}

static int jcntrl_mpi_controller_get_intuple(jcntrl_data_array *ary)
{
  jcntrl_size_type ntuple;
  ntuple = jcntrl_data_array_get_ntuple(ary);
  if (ntuple > INT_MAX) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return -1;
  }
  return ntuple;
}

int jcntrl_mpi_controller_gatherva(const jcntrl_mpi_controller *controller,
                                   jcntrl_data_array *dest,
                                   jcntrl_data_array *src, int root)
{
  int ntuple = jcntrl_mpi_controller_get_intuple(src);
  if (jcntrl_mpi_controller_any(controller, ntuple < 0))
    return 0;
  return jcntrl_mpi_controller_gatherv(controller, dest, src, ntuple, root);
}

int jcntrl_mpi_controller_allgatherva(const jcntrl_mpi_controller *controller,
                                      jcntrl_data_array *dest,
                                      jcntrl_data_array *src)
{
  int ntuple = jcntrl_mpi_controller_get_intuple(src);
  if (jcntrl_mpi_controller_any(controller, ntuple < 0))
    return 0;
  return jcntrl_mpi_controller_allgatherv(controller, dest, src, ntuple);
}

#ifdef JCNTRL_USE_MPI
static int jcntrl_mpi_create_indexed_type(jcntrl_data_array *ary,
                                          jcntrl_data_array *idx,
                                          MPI_Datatype *out)
{
  const jcntrl_aint_type *aidxd;
  const int *nelmd;
  jcntrl_aint_array *aidx;
  jcntrl_int_array *nelm;
  int r;
  jcntrl_size_type ntuple, elsize;
  MPI_Datatype eltype;
  int eltype_allocd;

  ntuple = 0;
  if (idx)
    ntuple = jcntrl_data_array_get_ntuple(idx);

  if (ntuple <= 0)
    return jcntrl_mpi_create_arraytype(ary, out);

  /*
   * We are not try to reduce the number of block. We need 1 block per
   * element.
   */
  if (ntuple > INT_MAX) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return 0;
  }

  elsize = jcntrl_data_array_element_size(ary);
  aidx = NULL;
  nelm = NULL;
  aidxd = NULL;
  nelmd = NULL;
  eltype_allocd = 0;

  r = 1;
  aidx = jcntrl_aint_array_new();
  if (!aidx)
    return 0;

  do {
    jcntrl_data_array *p;

    if (!jcntrl_aint_array_resize(aidx, ntuple)) {
      r = 0;
      break;
    }

    p = jcntrl_aint_array_data(aidx);
    for (jcntrl_size_type i = 0; i < ntuple; ++i) {
      intmax_t t;
      intmax_t melsize;
      int err;
      err = 0;
      t = jcntrl_data_array_get_ivalue(idx, i, &err);
      if (err) {
        r = 0;
        break;
      }

      melsize = elsize;
      if (melsize != elsize) {
        jcntrl_raise_overflow_error(__FILE__, __LINE__);
        r = 0;
        break;
      }

      if (jcntrl_m_mul_overflow(t, melsize, &t)) {
        r = 0;
        break;
      }

      if (!jcntrl_data_array_set_ivalue(p, i, t)) {
        r = 0;
        break;
      }
    }
    if (!r)
      break;

    aidxd = jcntrl_aint_array_get(aidx);
  } while (0);

  do {
    int *p;

    if (!r)
      break;

    nelm = jcntrl_int_array_new();
    if (!nelm) {
      r = 0;
      break;
    }

    if (!jcntrl_int_array_resize(nelm, ntuple)) {
      r = 0;
      break;
    }

    p = jcntrl_int_array_get_writable(nelm);
    if (!p) {
      r = 0;
      break;
    }

    for (jcntrl_size_type i = 0; i < ntuple; ++i)
      p[i] = 1;

    nelmd = p;
  } while (0);

  do {
    if (!r)
      break;

    if (!jcntrl_mpi_create_elementtype(ary, &eltype, &eltype_allocd)) {
      r = 0;
      break;
    }

    if (!jcntrl_safe_mpi(
          MPI_Type_create_hindexed(ntuple, nelmd, aidxd, eltype, out))) {
      r = 0;
      break;
    }

    if (!jcntrl_safe_mpi(MPI_Type_commit(out))) {
      jcntrl_safe_mpi(MPI_Type_free(out));
      r = 0;
    }
  } while (0);

  if (aidx)
    jcntrl_aint_array_delete(aidx);
  if (nelm)
    jcntrl_int_array_delete(nelm);
  if (eltype_allocd) {
    if (!jcntrl_safe_mpi(MPI_Type_free(&eltype)))
      r = 0;
  }
  return r;
}

static int jcntrl_mpi_create_indexed_types(int nproc,
                                           jcntrl_int_array *nelements,
                                           jcntrl_data_array *ary,
                                           jcntrl_data_array **idxs,
                                           MPI_Datatype *types)
{
  int r;
  int *p;

  if (!jcntrl_int_array_resize(nelements, nproc))
    return 0;

  p = jcntrl_int_array_get_writable(nelements);
  for (int i = 0; i < nproc; ++i)
    p[i] = idxs[i] && jcntrl_data_array_get_ntuple(idxs[i]) > 0;

  for (int i = 0; i < nproc; ++i)
    types[i] = MPI_DATATYPE_NULL;

  r = 1;
  for (int i = 0; i < nproc; ++i) {
    if (p[i]) {
      if (!jcntrl_mpi_create_indexed_type(ary, idxs[i], &types[i]))
        r = 0;
    }
  }
  return r;
}
#endif

int jcntrl_mpi_controller_gathervi(const jcntrl_mpi_controller *controller,
                                   jcntrl_data_array *dest,
                                   jcntrl_data_array **dstidx,
                                   jcntrl_data_array *src,
                                   jcntrl_data_array *srcidx, int root)
{
#ifdef JCNTRL_USE_MPI
  MPI_Datatype *desttypes;
  MPI_Datatype srctype;
  void *raw_dst;
  const void *raw_src;
  jcntrl_int_array *nelements;
  const int *elements;
  int nproc;
  int r;

  JCNTRL_ASSERT(src);

  if (jcntrl_mpi_controller_rank(controller) == root) {
    JCNTRL_ASSERT(dest);
    JCNTRL_ASSERT(dstidx);
  } else {
    dest = NULL;
    dstidx = NULL;
  }

  nproc = jcntrl_mpi_controller_nproc(controller);
  desttypes = NULL;
  srctype = MPI_DATATYPE_NULL;
  nelements = NULL;
  elements = NULL;
  r = 1;

  do {
    if (nproc <= 0) {
      r = 0;
      break;
    }

    if (dest) {
      desttypes = malloc(sizeof(MPI_Datatype) * nproc);
      if (!desttypes) {
        jcntrl_raise_allocation_failed(__FILE__, __LINE__);
        r = 0;
        break;
      }

      nelements = jcntrl_int_array_new();
      if (!nelements) {
        r = 0;
        break;
      }

      if (!jcntrl_mpi_create_indexed_types(nproc, nelements, dest, dstidx,
                                           desttypes))
        r = 0;
    }

    if (!jcntrl_mpi_create_indexed_type(src, srcidx, &srctype))
      r = 0;

    raw_dst = NULL;
    if (dest) {
      raw_dst = jcntrl_data_array_get_writable(dest);
      if (!raw_dst)
        r = 0;
    }

    raw_src = NULL;
    if (jcntrl_data_array_get_ntuple(srcidx) > 0) {
      raw_src = jcntrl_data_array_get(src);
      if (!raw_src)
        r = 0;
    }

    if (nelements) {
      elements = jcntrl_int_array_get(nelements);
      if (!elements)
        r = 0;
    }
  } while (0);

  if (!jcntrl_mpi_controller_all(controller, r))
    r = 0;

  do {
    int rank;

    if (!r)
      break;

    rank = jcntrl_mpi_controller_rank(controller);
    if (rank == root) {
      for (int i = 0; i < nproc; ++i) {
        if (!elements[i])
          continue;

        if (rank == i) {
          if (!jcntrl_data_array_copyidx(dest, src, dstidx[i], srcidx))
            r = 0;
        } else {
          if (!jcntrl_safe_mpi(MPI_Recv(raw_dst, 1, desttypes[i], i, 0,
                                        controller->comm, MPI_STATUS_IGNORE)))
            r = 0;
        }
      }
    } else {
      if (jcntrl_data_array_get_ntuple(srcidx) > 0) {
        r = jcntrl_safe_mpi(
          MPI_Send(raw_src, 1, srctype, root, 0, controller->comm));
      }
    }
  } while (0);
  if (desttypes) {
    for (int i = 0; i < nproc; ++i) {
      if (desttypes[i] != MPI_DATATYPE_NULL) {
        if (!jcntrl_safe_mpi(MPI_Type_free(&desttypes[i])))
          r = 0;
      }
    }
    free(desttypes);
  }
  if (srctype != MPI_DATATYPE_NULL)
    if (!jcntrl_safe_mpi(MPI_Type_free(&srctype)))
      r = 0;
  if (nelements)
    jcntrl_int_array_delete(nelements);
  return jcntrl_mpi_controller_all(controller, r);
#else
  if (jcntrl_data_array_get_ntuple(srcidx) <= 0)
    return 1;
  return jcntrl_data_array_copyidx(dest, src, dstidx[0], srcidx);
#endif
}

int jcntrl_mpi_controller_allgathervi(const jcntrl_mpi_controller *controller,
                                      jcntrl_data_array *dest,
                                      jcntrl_data_array **dstidx,
                                      jcntrl_data_array *src,
                                      jcntrl_data_array *srcidx)
{
#ifdef JCNTRL_USE_MPI
  MPI_Datatype *desttypes;
  MPI_Datatype srctype;
  void *raw_dst;
  const void *raw_src;
  jcntrl_int_array *nelements;
  const int *elements;
  int nproc;
  int rank;
  int r;

  JCNTRL_ASSERT(src);
  JCNTRL_ASSERT(dest);
  JCNTRL_ASSERT(dstidx);

  nproc = jcntrl_mpi_controller_nproc(controller);
  rank = jcntrl_mpi_controller_rank(controller);
  desttypes = NULL;
  srctype = MPI_DATATYPE_NULL;
  nelements = NULL;
  elements = NULL;
  r = 1;

  do {
    if (nproc <= 0) {
      r = 0;
      break;
    }

    desttypes = malloc(sizeof(MPI_Datatype) * nproc);
    if (!desttypes) {
      jcntrl_raise_allocation_failed(__FILE__, __LINE__);
      r = 0;
      break;
    }

    nelements = jcntrl_int_array_new();
    if (!nelements) {
      r = 0;
      break;
    }

    if (!jcntrl_mpi_create_indexed_types(nproc, nelements, dest, dstidx,
                                         desttypes))
      r = 0;

    if (!jcntrl_mpi_create_indexed_type(src, srcidx, &srctype))
      r = 0;

    if (nelements) {
      elements = jcntrl_int_array_get(nelements);
      if (!elements)
        r = 0;
    }

    raw_dst = jcntrl_data_array_get_writable(dest);
    if (!raw_dst)
      r = 0;

    raw_src = jcntrl_data_array_get(src);
    if (!raw_src) {
      if (elements[rank])
        r = 0;
    }
  } while (0);

  if (!jcntrl_mpi_controller_all(controller, r))
    r = 0;

  do {
    if (!r)
      break;

    if (!jcntrl_data_array_copyidx(dest, src, dstidx[rank], srcidx))
      r = 0;

    for (int k = 1; k < nproc; ++k) {
      /*
       * Pairwise exchange algorithm. We may want to use Bruck algorithm
       *
       *  0 -> 0, 1 -> 1, 2 -> 2, 3 -> 3, 4 -> 4 +- 0 (above)
       *  0 -> 1, 1 -> 2, 2 -> 3, 3 -> 4, 4 -> 0 +- 1
       *  0 -> 2, 1 -> 3, 2 -> 4, 3 -> 0, 4 -> 1 +- 2
       *  0 -> 3, 1 -> 4, 2 -> 0, 3 -> 1, 4 -> 2 +- 3
       *  0 -> 4, 1 -> 0, 2 -> 1, 3 -> 2, 4 -> 3 +- 4
       */
      int isend, irecv;
      isend = rank + k;
      irecv = rank - k;
      if (isend >= nproc)
        isend -= nproc;
      if (irecv < 0)
        irecv += nproc;

      if (!elements[rank])
        isend = -1;
      if (!elements[irecv])
        irecv = -1;

      if (isend == rank)
        isend = -1;
      if (irecv == rank)
        irecv = -1;

      if (isend >= 0 && irecv >= 0) {
        if (!jcntrl_safe_mpi(MPI_Sendrecv(raw_src, 1, srctype, isend, 0, //
                                          raw_dst, 1, desttypes[irecv], irecv,
                                          0, controller->comm,
                                          MPI_STATUS_IGNORE)))
          r = 0;
      } else if (isend >= 0) {
        if (!jcntrl_safe_mpi(
              MPI_Send(raw_src, 1, srctype, isend, 0, controller->comm)))
          r = 0;
      } else if (irecv >= 0) {
        if (!jcntrl_safe_mpi(MPI_Recv(raw_dst, 1, desttypes[irecv], irecv, 0,
                                      controller->comm, MPI_STATUS_IGNORE)))
          r = 0;
      }
    }
  } while (0);
  if (desttypes) {
    for (int i = 0; i < nproc; ++i) {
      if (desttypes[i] != MPI_DATATYPE_NULL) {
        if (!jcntrl_safe_mpi(MPI_Type_free(&desttypes[i])))
          r = 0;
      }
    }
    free(desttypes);
  }
  if (srctype != MPI_DATATYPE_NULL)
    if (!jcntrl_safe_mpi(MPI_Type_free(&srctype)))
      r = 0;
  if (nelements)
    jcntrl_int_array_delete(nelements);
  return jcntrl_mpi_controller_all(controller, r);
#else
  if (jcntrl_data_array_get_ntuple(srcidx) <= 0)
    return 1;

  return jcntrl_data_array_copyidx(dest, src, dstidx[0], srcidx);
#endif
}

#ifdef JCNTRL_USE_MPI
static int jcntrl_reduce_op_mpi_op_create(const jcntrl_reduce_op *op,
                                          MPI_Op *mpi_op);
static int jcntrl_reduce_op_mpi_op_free(const jcntrl_reduce_op *op,
                                        MPI_Op *mpi_op);
static void
jcntrl_reduce_op_set_datatype(const jcntrl_shared_object_data *type);
static void jcntrl_reduce_op_set_op(const jcntrl_reduce_op *op);
#endif

int jcntrl_mpi_controller_reduce(const jcntrl_mpi_controller *controller,
                                 jcntrl_data_array *dest,
                                 jcntrl_data_array *src,
                                 const jcntrl_reduce_op *op, int root)
{
#ifdef JCNTRL_USE_MPI
  MPI_Op mpi_op;
  MPI_Datatype type;
  int type_allocd;
  jcntrl_size_type ntuple;
  int intuple;
  int r;

  JCNTRL_ASSERT(controller);
  JCNTRL_ASSERT(dest);
  JCNTRL_ASSERT(src);
  JCNTRL_ASSERT(
    jcntrl_shared_object_data_is_a(jcntrl_data_array_element_type(src),
                                   jcntrl_data_array_element_type(dest)));

  jcntrl_reduce_op_set_op(op);
  jcntrl_reduce_op_set_datatype(jcntrl_data_array_element_type(src));

  r = 1;
  mpi_op = MPI_OP_NULL;
  type_allocd = 0;

  do {
    if (!jcntrl_reduce_op_mpi_op_create(op, &mpi_op))
      r = 0;

    ntuple = jcntrl_data_array_get_ntuple(src);
    intuple = ntuple;

    if (intuple != ntuple) {
      jcntrl_raise_overflow_error(__FILE__, __LINE__);
      r = 0;
    }

    if (!jcntrl_mpi_create_elementtype(src, &type, &type_allocd))
      r = 0;
  } while (0);

  r = jcntrl_mpi_controller_all(controller, r);
  do {
    int is_root;
    const void *raw_src;
    void *raw_dst;

    if (!r)
      break;

    is_root = jcntrl_mpi_controller_rank(controller) == root;
    raw_dst = NULL;
    if (is_root)
      raw_dst = jcntrl_data_array_get_writable(dest);
    raw_src = jcntrl_data_array_get(src);
    if (dest == src && is_root) {
      r = jcntrl_safe_mpi(MPI_Reduce(MPI_IN_PLACE, raw_dst, intuple, type,
                                     mpi_op, root, controller->comm));
    } else {
      r = jcntrl_safe_mpi(MPI_Reduce(raw_src, raw_dst, intuple, type, mpi_op,
                                     root, controller->comm));
    }
  } while (0);

  if (type_allocd)
    r = jcntrl_safe_mpi(MPI_Type_free(&type));

  if (!jcntrl_reduce_op_mpi_op_free(op, &mpi_op))
    r = 0;

  return jcntrl_mpi_controller_all(controller, r);
#else
  if (dest != src)
    return jcntrl_data_array_copy(dest, src, jcntrl_data_array_get_ntuple(src),
                                  0, 0);
  return 1;
#endif
}

int jcntrl_mpi_controller_allreduce(const jcntrl_mpi_controller *controller,
                                    jcntrl_data_array *dest,
                                    jcntrl_data_array *src,
                                    const jcntrl_reduce_op *op)
{
#ifdef JCNTRL_USE_MPI
  MPI_Op mpi_op;
  MPI_Datatype type;
  int type_allocd;
  jcntrl_size_type ntuple;
  int intuple;
  int r;

  JCNTRL_ASSERT(controller);
  JCNTRL_ASSERT(dest);
  JCNTRL_ASSERT(src);
  JCNTRL_ASSERT(
    jcntrl_shared_object_data_is_a(jcntrl_data_array_element_type(src),
                                   jcntrl_data_array_element_type(dest)));

  jcntrl_reduce_op_set_op(op);
  jcntrl_reduce_op_set_datatype(jcntrl_data_array_element_type(src));

  r = 1;
  mpi_op = MPI_OP_NULL;
  type_allocd = 0;

  do {
    if (!jcntrl_reduce_op_mpi_op_create(op, &mpi_op))
      r = 0;

    ntuple = jcntrl_data_array_get_ntuple(src);
    intuple = ntuple;

    if (intuple != ntuple) {
      jcntrl_raise_overflow_error(__FILE__, __LINE__);
      r = 0;
    }

    if (!jcntrl_mpi_create_elementtype(src, &type, &type_allocd))
      r = 0;
  } while (0);

  r = jcntrl_mpi_controller_all(controller, r);
  do {
    int is_root;
    const void *raw_src;
    void *raw_dst;

    if (!r)
      break;

    raw_dst = jcntrl_data_array_get_writable(dest);
    raw_src = jcntrl_data_array_get(src);
    if (dest == src) {
      r = jcntrl_safe_mpi(MPI_Allreduce(MPI_IN_PLACE, raw_dst, intuple, type,
                                        mpi_op, controller->comm));
    } else {
      r = jcntrl_safe_mpi(MPI_Allreduce(raw_src, raw_dst, intuple, type, mpi_op,
                                        controller->comm));
    }
  } while (0);

  if (type_allocd)
    r = jcntrl_safe_mpi(MPI_Type_free(&type));

  if (!jcntrl_reduce_op_mpi_op_free(op, &mpi_op))
    r = 0;

  return jcntrl_mpi_controller_all(controller, r);
#else
  if (dest != src)
    return jcntrl_data_array_copy(dest, src, jcntrl_data_array_get_ntuple(src),
                                  0, 0);
  return 1;
#endif
}

//---- reduce_op

static jcntrl_reduce_op *
jcntrl_reduce_op_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_reduce_op, obj);
}

static void *jcntrl_reduce_op_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_reduce_op_downcast_impl(obj);
}

static int jcntrl_reduce_op_initializer(jcntrl_shared_object *obj)
{
  jcntrl_reduce_op *op = jcntrl_reduce_op_downcast_impl(obj);
  op->commute = 0;
  return 1;
}

#ifdef JCNTRL_USE_MPI
static const jcntrl_reduce_op *jcntrl_reduce_op_data = NULL;
static const jcntrl_shared_object_data *jcntrl_reduce_array_type = NULL;
#ifdef _OPENMP
#pragma omp threadprivate(jcntrl_reduce_op_data)
#pragma omp threadprivate(jcntrl_reduce_array_type)
#endif

union jcntrl_reduce_array_u
{
  jcntrl_static_char_array ch;
  jcntrl_static_bool_array b;
  jcntrl_static_int_array i;
  jcntrl_static_size_array s;
  jcntrl_static_aint_array a;
  jcntrl_static_float_array f;
  jcntrl_static_double_array d;
};

typedef jcntrl_data_array *
jcntrl_reduce_array_binder(union jcntrl_reduce_array_u *u, void *p, int *len,
                           MPI_Datatype *type);

jcntrl_reduce_array_binder *jcntrl_reduce_array_binder_func = NULL;
#ifdef _OPENMP
#pragma omp threadprivate(jcntrl_reduce_array_binder_func)
#endif

static jcntrl_size_type jcntrl_reduce_array_size(int *len, MPI_Datatype *type,
                                                 jcntrl_size_type elsize)
{
  return *len;
}

static jcntrl_data_array *
jcntrl_reduce_char_array_binder(union jcntrl_reduce_array_u *u, void *p,
                                int *len, MPI_Datatype *type)
{
  jcntrl_size_type sz;
  sz = jcntrl_reduce_array_size(len, type,
                                jcntrl_static_char_array_element_size());
  jcntrl_static_char_array_init_base(&u->ch, p, sz);
  return jcntrl_static_char_array_data(&u->ch);
}

static jcntrl_data_array *
jcntrl_reduce_bool_array_binder(union jcntrl_reduce_array_u *u, void *p,
                                int *len, MPI_Datatype *type)
{
  jcntrl_size_type sz;
  sz = jcntrl_reduce_array_size(len, type,
                                jcntrl_static_bool_array_element_size());
  jcntrl_static_bool_array_init_base(&u->b, p, sz);
  return jcntrl_static_bool_array_data(&u->b);
}

static jcntrl_data_array *
jcntrl_reduce_int_array_binder(union jcntrl_reduce_array_u *u, void *p,
                               int *len, MPI_Datatype *type)
{
  jcntrl_size_type sz;
  sz =
    jcntrl_reduce_array_size(len, type, jcntrl_static_int_array_element_size());
  jcntrl_static_int_array_init_base(&u->i, p, sz);
  return jcntrl_static_int_array_data(&u->i);
}

static jcntrl_data_array *
jcntrl_reduce_size_array_binder(union jcntrl_reduce_array_u *u, void *p,
                                int *len, MPI_Datatype *type)
{
  jcntrl_size_type sz;
  sz = jcntrl_reduce_array_size(len, type,
                                jcntrl_static_size_array_element_size());
  jcntrl_static_size_array_init_base(&u->s, p, sz);
  return jcntrl_static_size_array_data(&u->s);
}

static jcntrl_data_array *
jcntrl_reduce_float_array_binder(union jcntrl_reduce_array_u *u, void *p,
                                 int *len, MPI_Datatype *type)
{
  jcntrl_size_type sz;
  sz = jcntrl_reduce_array_size(len, type,
                                jcntrl_static_float_array_element_size());
  jcntrl_static_float_array_init_base(&u->f, p, sz);
  return jcntrl_static_float_array_data(&u->f);
}

static jcntrl_data_array *
jcntrl_reduce_double_array_binder(union jcntrl_reduce_array_u *u, void *p,
                                  int *len, MPI_Datatype *type)
{
  jcntrl_size_type sz;
  sz = jcntrl_reduce_array_size(len, type,
                                jcntrl_static_double_array_element_size());
  jcntrl_static_double_array_init_base(&u->d, p, sz);
  return jcntrl_static_double_array_data(&u->d);
}

static jcntrl_data_array *
jcntrl_reduce_aint_array_binder(union jcntrl_reduce_array_u *u, void *p,
                                int *len, MPI_Datatype *type)
{
  jcntrl_size_type sz;
  sz = jcntrl_reduce_array_size(len, type,
                                jcntrl_static_aint_array_element_size());
  jcntrl_static_aint_array_init_base(&u->a, p, sz);
  return jcntrl_static_aint_array_data(&u->a);
}

static void jcntrl_reduce_op_set_datatype(const jcntrl_shared_object_data *type)
{
  jcntrl_reduce_array_type = type;
  jcntrl_reduce_array_binder_func = NULL;
  if (jcntrl_shared_object_data_is_a(jcntrl_char_array_metadata_init(), type)) {
    jcntrl_reduce_array_binder_func = jcntrl_reduce_char_array_binder;
  } else if (jcntrl_shared_object_data_is_a(jcntrl_bool_array_metadata_init(),
                                            type)) {
    jcntrl_reduce_array_binder_func = jcntrl_reduce_bool_array_binder;
  } else if (jcntrl_shared_object_data_is_a(jcntrl_int_array_metadata_init(),
                                            type)) {
    jcntrl_reduce_array_binder_func = jcntrl_reduce_int_array_binder;
  } else if (jcntrl_shared_object_data_is_a(jcntrl_size_array_metadata_init(),
                                            type)) {
    jcntrl_reduce_array_binder_func = jcntrl_reduce_size_array_binder;
  } else if (jcntrl_shared_object_data_is_a(jcntrl_float_array_metadata_init(),
                                            type)) {
    jcntrl_reduce_array_binder_func = jcntrl_reduce_float_array_binder;
  } else if (jcntrl_shared_object_data_is_a(jcntrl_double_array_metadata_init(),
                                            type)) {
    jcntrl_reduce_array_binder_func = jcntrl_reduce_double_array_binder;
  } else if (jcntrl_shared_object_data_is_a(jcntrl_aint_array_metadata_init(),
                                            type)) {
    jcntrl_reduce_array_binder_func = jcntrl_reduce_aint_array_binder;
  }
  JCNTRL_ASSERT(jcntrl_reduce_array_binder_func);
}

static void jcntrl_reduce_op_set_op(const jcntrl_reduce_op *op)
{
  jcntrl_reduce_op_data = op;
}

static void jcntrl_reduce_op_func(void *invec, void *outvec, int *len,
                                  MPI_Datatype *type)
{
  jcntrl_data_array *id, *od;
  union jcntrl_reduce_array_u invecu, outvecu;
  id = jcntrl_reduce_array_binder_func(&invecu, invec, len, type);
  od = jcntrl_reduce_array_binder_func(&outvecu, outvec, len, type);
  jcntrl_reduce_op_calc((jcntrl_reduce_op *)jcntrl_reduce_op_data, id, od);
}

typedef int jcntrl_reduce_op_mpi_op_create_func(jcntrl_shared_object *object,
                                                MPI_Op *mpi_op);

struct jcntrl_reduce_op_mpi_op_args
{
  MPI_Op *mpi_op;
  int ret;
};

static void jcntrl_reduce_op_mpi_op_create__wrapper(
  jcntrl_shared_object *obj, void *args,
  jcntrl_reduce_op_mpi_op_create_func *func)
{
  struct jcntrl_reduce_op_mpi_op_args *a = args;
  a->ret = func(obj, a->mpi_op);
}

static int jcntrl_reduce_op_mpi_op_create(const jcntrl_reduce_op *op,
                                          MPI_Op *mpi_op)
{
  jcntrl_reduce_op *xop = (jcntrl_reduce_op *)op;
  struct jcntrl_reduce_op_mpi_op_args d = {
    .mpi_op = mpi_op,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_reduce_op_object(xop),
                                    jcntrl_reduce_op, mpi_op_create, &d);
  return d.ret;
}

static int jcntrl_reduce_op_mpi_op_create_impl(jcntrl_shared_object *obj,
                                               MPI_Op *mpi_op)
{
  jcntrl_reduce_op *op = jcntrl_reduce_op_downcast_impl(obj);
  return jcntrl_safe_mpi(
    MPI_Op_create(jcntrl_reduce_op_func, op->commute, mpi_op));
}

JCNTRL_VIRTUAL_WRAP(jcntrl_reduce_op, jcntrl_reduce_op, mpi_op_create)

typedef int jcntrl_reduce_op_mpi_op_free_func(jcntrl_shared_object *object,
                                              MPI_Op *mpi_op);

static void
jcntrl_reduce_op_mpi_op_free__wrapper(jcntrl_shared_object *obj, void *args,
                                      jcntrl_reduce_op_mpi_op_free_func *func)
{
  struct jcntrl_reduce_op_mpi_op_args *a = args;
  a->ret = func(obj, a->mpi_op);
}

static int jcntrl_reduce_op_mpi_op_free(const jcntrl_reduce_op *op,
                                        MPI_Op *mpi_op)
{
  jcntrl_reduce_op *xop = (jcntrl_reduce_op *)op;
  struct jcntrl_reduce_op_mpi_op_args d = {
    .mpi_op = mpi_op,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_reduce_op_object(xop),
                                    jcntrl_reduce_op, mpi_op_free, &d);
  return d.ret;
}

static int jcntrl_reduce_op_mpi_op_free_impl(jcntrl_shared_object *obj,
                                             MPI_Op *mpi_op)
{
  jcntrl_reduce_op *op = jcntrl_reduce_op_downcast_impl(obj);
  return jcntrl_safe_mpi(MPI_Op_free(mpi_op));
}

JCNTRL_VIRTUAL_WRAP(jcntrl_reduce_op, jcntrl_reduce_op, mpi_op_free)
#endif

static void jcntrl_reduce_op_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_reduce_op_downcast_v;
  p->initializer = jcntrl_reduce_op_initializer;
  p->destructor = NULL;
  p->allocator = NULL;
  p->deleter = NULL;
#ifdef JCNTRL_USE_MPI
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_reduce_op, jcntrl_reduce_op, mpi_op_create);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_reduce_op, jcntrl_reduce_op, mpi_op_free);
#endif
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_reduce_op, jcntrl_reduce_op_init_func)

struct jcntrl_reduce_op_predefined
{
  jcntrl_reduce_op op;
#ifdef JCNTRL_USE_MPI
  MPI_Op mpi_op;
#endif
};
typedef struct jcntrl_reduce_op_predefined jcntrl_reduce_op_predefined;
#define jcntrl_reduce_op_predefined__ancestor jcntrl_reduce_op
#define jcntrl_reduce_op_predefined__dnmem op.jcntrl_reduce_op__dnmem
JCNTRL_VTABLE_NONE(jcntrl_reduce_op_predefined);

static jcntrl_reduce_op_predefined *
jcntrl_reduce_op_predefined_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_reduce_op_predefined, obj);
}

static void *jcntrl_reduce_op_predefined_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_reduce_op_predefined_downcast_impl(obj);
}

#ifdef JCNTRL_USE_MPI
static int
jcntrl_reduce_op_predefined_mpi_op_create_impl(jcntrl_shared_object *obj,
                                               MPI_Op *mpi_op)
{
  jcntrl_reduce_op_predefined *op;
  op = jcntrl_reduce_op_predefined_downcast_impl(obj);
  *mpi_op = op->mpi_op;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_reduce_op_predefined, jcntrl_reduce_op,
                    mpi_op_create)

static int
jcntrl_reduce_op_predefined_mpi_op_free_impl(jcntrl_shared_object *obj,
                                             MPI_Op *mpi_op)
{
  /* nop */
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_reduce_op_predefined, jcntrl_reduce_op, mpi_op_free)
#endif

static void jcntrl_reduce_op_predefined_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_reduce_op_predefined_downcast_v;
#ifdef JCNTRL_USE_MPI
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_reduce_op_predefined, jcntrl_reduce_op,
                          mpi_op_create);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_reduce_op_predefined, jcntrl_reduce_op,
                          mpi_op_free);
#endif
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_reduce_op_predefined,
                                   jcntrl_reduce_op_predefined_init_func)

#ifdef JCNTRL_USE_MPI
static inline const jcntrl_reduce_op *
jcntrl_reduce_op_predefined_init(jcntrl_reduce_op_predefined *p, int *init,
                                 MPI_Op op)
{
  if (*init)
    return &p->op;

  *init = 1;
  jcntrl_shared_object_static_init(jcntrl_reduce_op_object(&p->op),
                                   jcntrl_reduce_op_predefined_metadata_init());
  p->mpi_op = op;
  return &p->op;
}

#define jcntrl_reduce_op_predefined_init(p, init, op) \
  jcntrl_reduce_op_predefined_init(p, init, op)

#else
static inline const jcntrl_reduce_op *
jcntrl_reduce_op_predefined_init(jcntrl_reduce_op_predefined *p, int *init)
{
  if (*init)
    return &p->op;

  *init = 1;
  jcntrl_shared_object_static_init(jcntrl_reduce_op_object(&p->op),
                                   jcntrl_reduce_op_predefined_metadata_init());
  return &p->op;
}

#define jcntrl_reduce_op_predefined_init(p, init, op) \
  jcntrl_reduce_op_predefined_init(p, init)
#endif

const jcntrl_reduce_op *jcntrl_reduce_op_sum(void)
{
  static jcntrl_reduce_op_predefined p;
  static int init = 0;
  return jcntrl_reduce_op_predefined_init(&p, &init, MPI_SUM);
}

const jcntrl_reduce_op *jcntrl_reduce_op_prod(void)
{
  static jcntrl_reduce_op_predefined p;
  static int init = 0;
  return jcntrl_reduce_op_predefined_init(&p, &init, MPI_PROD);
}

const jcntrl_reduce_op *jcntrl_reduce_op_max(void)
{
  static jcntrl_reduce_op_predefined p;
  static int init = 0;
  return jcntrl_reduce_op_predefined_init(&p, &init, MPI_MAX);
}

const jcntrl_reduce_op *jcntrl_reduce_op_min(void)
{
  static jcntrl_reduce_op_predefined p;
  static int init = 0;
  return jcntrl_reduce_op_predefined_init(&p, &init, MPI_MIN);
}

const jcntrl_reduce_op *jcntrl_reduce_op_land(void)
{
  static jcntrl_reduce_op_predefined p;
  static int init = 0;
  return jcntrl_reduce_op_predefined_init(&p, &init, MPI_LAND);
}

const jcntrl_reduce_op *jcntrl_reduce_op_lor(void)
{
  static jcntrl_reduce_op_predefined p;
  static int init = 0;
  return jcntrl_reduce_op_predefined_init(&p, &init, MPI_LOR);
}

const jcntrl_reduce_op *jcntrl_reduce_op_lxor(void)
{
  static jcntrl_reduce_op_predefined p;
  static int init = 0;
  return jcntrl_reduce_op_predefined_init(&p, &init, MPI_LXOR);
}

const jcntrl_reduce_op *jcntrl_reduce_op_band(void)
{
  static jcntrl_reduce_op_predefined p;
  static int init = 0;
  return jcntrl_reduce_op_predefined_init(&p, &init, MPI_BAND);
}

const jcntrl_reduce_op *jcntrl_reduce_op_bor(void)
{
  static jcntrl_reduce_op_predefined p;
  static int init = 0;
  return jcntrl_reduce_op_predefined_init(&p, &init, MPI_BOR);
}

const jcntrl_reduce_op *jcntrl_reduce_op_bxor(void)
{
  static jcntrl_reduce_op_predefined p;
  static int init = 0;
  return jcntrl_reduce_op_predefined_init(&p, &init, MPI_BXOR);
}
