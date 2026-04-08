
#include <stdlib.h>
#include <string.h>

#include "executive.h"
#include "manager.h"
#include "defs.h"
#include "error.h"
#include "overflow.h"

#include <jupiter/geometry/rbtree.h>

/**
 * @brief Provides management feature for using control data
 */
struct jcntrl_executive_manager
{
  struct geom_rbtree *root;
};

struct jcntrl_executive_manager_entry
{
  struct geom_rbtree tree;
  const char *name;
  char *aname;
  jcntrl_executive *exec;
  int mark;
};
#define jcntrl_executive_manager_tree_entry(p)                                 \
  geom_rbtree_entry(p, struct jcntrl_executive_manager_entry, tree)

static int jcntrl_executive_manager_comp(struct geom_rbtree *ta,
                                         struct geom_rbtree *tb)
{
  int r;
  jcntrl_executive_manager_entry *a, *b;
  a = jcntrl_executive_manager_tree_entry(ta);
  b = jcntrl_executive_manager_tree_entry(tb);
  return strcmp(a->name, b->name);
}

jcntrl_executive_manager *jcntrl_executive_manager_new(void)
{
  jcntrl_executive_manager *man;
  man = (jcntrl_executive_manager *)malloc(sizeof(jcntrl_executive_manager));
  if (!man) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return NULL;
  }
  man->root = NULL;
  return man;
}

/**
 * @brief Delete (Deallocate) manager entry
 * @param entry Entry to remove
 *
 * @p entry must be removed from the tree before calling this function.
 */
static void
jcntrl_executive_manager_delete_entry(jcntrl_executive_manager_entry *entry)
{
  JCNTRL_ASSERT(entry);
  if (entry->aname) {
    free(entry->aname);
  }
  if (entry->exec) {
    jcntrl_executive_delete(entry->exec);
  }
  free(entry);
}

static void
jcntrl_executive_manager_remove_all(jcntrl_executive_manager *manager)
{
  struct geom_rbtree *root;
  JCNTRL_ASSERT(manager);

  root = manager->root;
  while (root) {
    struct geom_rbtree *nroot;
    jcntrl_executive_manager_entry *entry;

    nroot = geom_rbtree_delete(root, root, NULL);
    entry = jcntrl_executive_manager_tree_entry(root);
    jcntrl_executive_manager_delete_entry(entry);

    root = nroot;
  }
}

void jcntrl_executive_manager_delete(jcntrl_executive_manager *manager)
{
  JCNTRL_ASSERT(manager);
  jcntrl_executive_manager_remove_all(manager);
  free(manager);
}

static jcntrl_executive_manager_entry *
jcntrl_executive_manager_find(jcntrl_executive_manager *manager,
                              const char *name)
{
  struct geom_rbtree *tree;
  jcntrl_executive_manager_entry find;

  JCNTRL_ASSERT(manager);

  tree = NULL;
  find.name = name;
  if (manager->root) {
    tree = geom_rbtree_find(manager->root, &find.tree,
                            jcntrl_executive_manager_comp);
  }
  if (tree) {
    return jcntrl_executive_manager_tree_entry(tree);
  }
  return NULL;
}

jcntrl_executive *
jcntrl_executive_manager_get(jcntrl_executive_manager *manager,
                             const char *name)
{
  jcntrl_executive_manager_entry *e;

  e = jcntrl_executive_manager_find(manager, name);
  if (e) {
    return e->exec;
  }
  return NULL;
}

/**
 * @brief Removes from tree
 */
static void
jcntrl_executive_manager_remove_entry(jcntrl_executive_manager *manager,
                                      jcntrl_executive_manager_entry *e)
{
  struct geom_rbtree *nroot;

  JCNTRL_ASSERT(manager);
  JCNTRL_ASSERT(manager->root);
  JCNTRL_ASSERT(e);

  nroot = geom_rbtree_delete(manager->root, &e->tree, NULL);
  manager->root = nroot;
}

jcntrl_executive *
jcntrl_executive_manager_disown(jcntrl_executive_manager *manager,
                                const char *name)
{
  jcntrl_executive_manager_entry *e;

  e = jcntrl_executive_manager_find(manager, name);
  if (e) {
    jcntrl_executive *exec;
    exec = e->exec;
    e->exec = NULL;

    jcntrl_executive_manager_remove_entry(manager, e);
    jcntrl_executive_manager_delete_entry(e);
    return exec;
  }
  return NULL;
}

int jcntrl_executive_manager_add(jcntrl_executive_manager *manager,
                                 jcntrl_executive *executive)
{
  const char *name;
  jcntrl_executive_manager_entry *e;

  name = jcntrl_executive_get_name(executive);
  if (!name || strcmp(name, "") == 0) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Name is not set for an executive");
    return 0;
  }

  e = jcntrl_executive_manager_find(manager, name);
  if (!e) {
    e = jcntrl_executive_manager_reserve(manager, name);
    if (!e) {
      return 0;
    }
  }
  return jcntrl_executive_manager_bind(e, executive);
}

int jcntrl_executive_manager_remove(jcntrl_executive_manager *manager,
                                    const char *name)
{
  jcntrl_executive_manager_entry *e;

  e = jcntrl_executive_manager_find(manager, name);
  if (e) {
    jcntrl_executive_manager_remove_entry(manager, e);
    jcntrl_executive_manager_delete_entry(e);
    return 1;
  }
  return 0;
}

static jcntrl_executive_manager_entry *
jcntrl_executive_manager_entry_new(void)
{
  jcntrl_executive_manager_entry *e;
  e = (jcntrl_executive_manager_entry *)
    malloc(sizeof(jcntrl_executive_manager_entry));
  if (!e) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return NULL;
  }
  geom_rbtree_init(&e->tree);
  e->aname = NULL;
  e->name = NULL;
  e->exec = NULL;
  e->mark = 0;
  return e;
}

static int
jcntrl_executive_manager_add_entry(jcntrl_executive_manager *manager,
                                   jcntrl_executive_manager_entry *e)
{
  JCNTRL_ASSERT(manager);

  if (manager->root) {
    struct geom_rbtree *nroot;
    nroot = geom_rbtree_insert(manager->root, &e->tree,
                               jcntrl_executive_manager_comp, NULL);
    if (!nroot) {
      jcntrl_raise_argument_error(__FILE__, __LINE__,
                                  "Specified name already exists");
      return 0;
    }
    manager->root = nroot;
  } else {
    manager->root = &e->tree;
  }
  return 1;
}

jcntrl_executive_manager_entry *
jcntrl_executive_manager_reserve(jcntrl_executive_manager *manager,
                                 const char *name)
{
  int l;
  char *aname;
  jcntrl_executive_manager_entry *e;

  JCNTRL_ASSERT(name);

  e = jcntrl_executive_manager_find(manager, name);
  if (e)
    return e;

  l = strlen(name);
  if (l <= 0) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Requested name of executive is empty");
    return NULL;
  }
  if (jcntrl_i_add_overflow(l, 1, &l))
    return NULL;

  aname = (char *)malloc(sizeof(char) * l);
  if (!aname) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return NULL;
  }
  strcpy(aname, name);

  e = jcntrl_executive_manager_entry_new();
  if (!e) {
    free(aname);
    return NULL;
  }

  e->name = aname;
  e->aname = aname;

  if (!jcntrl_executive_manager_add_entry(manager, e)) {
    jcntrl_executive_manager_delete_entry(e);
    return NULL;
  }

  return e;
}

jcntrl_executive_manager_entry *
jcntrl_executive_manager_has(jcntrl_executive_manager *manager,
                             const char *name)
{
  return jcntrl_executive_manager_find(manager, name);
}

int jcntrl_executive_manager_bind(jcntrl_executive_manager_entry *entry,
                                  jcntrl_executive *executive)
{
  JCNTRL_ASSERT(entry);

  if (entry->exec) {
    if (entry->exec == executive) {
      return 1;
    }
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Given manager entry has already bound to "
                                "another executive");
    return 0;
  }

  jcntrl_executive_set_name(executive, entry->name);

  /* jcntrl_executive_set_name() copies the name */
  if (entry->aname) {
    free(entry->aname);
    entry->aname = NULL;
  }
  entry->name = jcntrl_executive_get_name(executive);
  entry->exec = executive;
  return 1;
}

jcntrl_executive *
jcntrl_executive_manager_entry_get(jcntrl_executive_manager_entry *entry)
{
  JCNTRL_ASSERT(entry);

  return entry->exec;
}

const char *
jcntrl_executive_manager_entry_name(jcntrl_executive_manager_entry *entry)
{
  JCNTRL_ASSERT(entry);

  return entry->name;
}

void jcntrl_executive_manager_set_mark(jcntrl_executive_manager_entry *entry,
                                       int mark)
{
  JCNTRL_ASSERT(entry);

  entry->mark = mark;
}

int jcntrl_executive_manager_mark(jcntrl_executive_manager_entry *entry)
{
  JCNTRL_ASSERT(entry);

  return entry->mark;
}

void jcntrl_executive_manager_set_all_marks(jcntrl_executive_manager *manager,
                                            int mark)
{
  JCNTRL_ASSERT(manager);

  if (manager->root) {
    struct geom_rbtree *lp;
    geom_rbtree_foreach_succ(lp, manager->root) {
      jcntrl_executive_manager_entry *ep;
      ep = jcntrl_executive_manager_tree_entry(lp);
      ep->mark = mark;
    }
  }
}
