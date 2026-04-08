#include "defs.h"
#include "global.h"
#include "error.h"
#include "executive.h"
#include "mask_extent.h"
#include "struct_data.h"

#include "fv_table.h"

#include <jupiter/geometry/rbtree.h>
#include <stdlib.h>

struct jcntrl_executive_class_entry
{
  struct geom_rbtree cls_tree;
  int id;
  const jcntrl_shared_object_data *cls;
};
#define jcntrl_executive_class_entry_of_cls(ptr) \
  geom_rbtree_entry(ptr, struct jcntrl_executive_class_entry, cls_tree)

static struct geom_rbtree *cls_root = NULL;

static struct jcntrl_executive_class_entry jcntrl_executive_db[JCNTRL_EXE_MAX] =
    {0};

static int jcntrl_executive_user_last = JCNTRL_EXE_USER_FIRST;

int jcntrl_initialize(void)
{
  int ret;
  ret = 0;
  if (!jcntrl_install_fv_table())
    ret = 1;
  if (!jcntrl_install_mask_extent())
    ret = 1;
  return ret;
}

static int
jcntrl_executive_cls_comp(struct geom_rbtree *aa, struct geom_rbtree *bb)
{
  struct jcntrl_executive_class_entry *ea, *eb;
  ea = jcntrl_executive_class_entry_of_cls(aa);
  eb = jcntrl_executive_class_entry_of_cls(bb);

  return (char *)eb->cls - (char *)ea->cls;
}

static int
jcntrl_executive_find_id(const jcntrl_shared_object_data *cls)
{
  struct geom_rbtree *result;
  struct jcntrl_executive_class_entry entry;

  JCNTRL_ASSERT(cls);

  if (!cls_root) return JCNTRL_EXE_INVALID;

  entry.cls = cls;
  result = geom_rbtree_find(cls_root, &entry.cls_tree, jcntrl_executive_cls_comp);
  if (result) {
    struct jcntrl_executive_class_entry *e;
    e = jcntrl_executive_class_entry_of_cls(result);
    return e->id;
  }

  return JCNTRL_EXE_INVALID;
}

#undef jcntrl_executive_install
int jcntrl_executive_install(int want_id, const jcntrl_shared_object_data *cls)
{
  const jcntrl_shared_object_data *execls, *p;
  int id;
  struct jcntrl_executive_class_entry *dest;

  JCNTRL_ASSERT(cls);
  JCNTRL_ASSERT(want_id > JCNTRL_EXE_INVALID && want_id < JCNTRL_EXE_MAX);

  p = cls;
  execls = jcntrl_executive_metadata_init();
  while (p && p != execls)
    p = p->ancestor;

  JCNTRL_ASSERT(p == execls);

  id = jcntrl_executive_find_id(cls);
  if (id != JCNTRL_EXE_INVALID) {
    return id;
  }

  dest = NULL;
  if (want_id >= JCNTRL_EXE_USER_FIRST && want_id <= JCNTRL_EXE_USER_LAST) {
    want_id = JCNTRL_EXE_USER;
  }
  if (want_id == JCNTRL_EXE_USER) {
    if (jcntrl_executive_user_last == JCNTRL_EXE_USER_LAST) {
      jcntrl_raise_argument_error(__FILE__, __LINE__,
                                  "[dev] Too many user-defined executives are "
                                  "installed. Note that creating an executive "
                                  "class dynamically is not recommended.");
      return JCNTRL_EXE_INVALID;
    }

    id = jcntrl_executive_user_last;
    jcntrl_executive_user_last += 1;
  } else {
    id = want_id;
  }

  dest = &jcntrl_executive_db[id];
  JCNTRL_ASSERT_X(!dest->cls, "Executive ID %d is already registered", id);
  if (dest->cls) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "[dev] Given ID is already registered. "
                                "Plaese use JCNTRL_EXE_USER for "
                                "user-defined executives");
    return JCNTRL_EXE_INVALID;
  }

  dest->cls = cls;
  dest->id = id;
  geom_rbtree_init(&dest->cls_tree);
  if (cls_root) {
    struct geom_rbtree *root;

    root = geom_rbtree_insert(cls_root, &dest->cls_tree, jcntrl_executive_cls_comp, NULL);
    JCNTRL_ASSERT(root);

    cls_root = root;
  } else {
    cls_root = &dest->cls_tree;
  }

  return want_id;
}

const jcntrl_shared_object_data *jcntrl_executive_get_from_id(int id)
{
  if (id < 0 || id >= JCNTRL_EXE_MAX) {
    jcntrl_raise_index_error(__FILE__, __LINE__, id);
    return NULL;
  }

  return jcntrl_executive_db[id].cls;
}
