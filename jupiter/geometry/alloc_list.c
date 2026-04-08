
#ifdef GEOM_ALLOC_DEBUG
#include <stdio.h>
#endif

#include <stdlib.h>

#include "geom_assert.h"
#include "rbtree.h"
#include "alloc_list.h"

struct geom_alloc_node
{
  struct geom_rbtree node;
  void *p;
  geom_deallocator *dealloc;
};

struct geom_alloc_list
{
  struct geom_alloc_node *root;
};

#define geom_alloc_node_entry(head) \
  geom_rbtree_entry(head, struct geom_alloc_node, node)

static int geom_alloc_compare(struct geom_rbtree *a, struct geom_rbtree *b)
{
  struct geom_alloc_node *aa;
  struct geom_alloc_node *ab;

  GEOM_ASSERT(a);
  GEOM_ASSERT(b);

  aa = geom_alloc_node_entry(a);
  ab = geom_alloc_node_entry(b);

  if (aa->p <  ab->p) return -1;
  if (aa->p == ab->p) return 0;
  return 1;
}

static struct geom_alloc_node *
geom_alloc_find_entry(geom_alloc_list *allocs, void *n)
{
  struct geom_rbtree *root;
  struct geom_rbtree *fnd;
  struct geom_alloc_node search;

  GEOM_ASSERT(allocs);

  if (!allocs->root) return NULL;

  search.p = n;
  search.dealloc = NULL;

  root = &allocs->root->node;
  fnd = geom_rbtree_find(root, &search.node, geom_alloc_compare);
  return geom_alloc_node_entry(fnd);
}

geom_alloc_list *geom_alloc_list_new(void)
{
  geom_alloc_list *n;

  n = (geom_alloc_list *)malloc(sizeof(geom_alloc_list));
  if (!n) return NULL;

  n->root = NULL;
  return n;
}

geom_error geom_alloc_add(geom_alloc_list *p, void *n, geom_deallocator *d)
{
  struct geom_alloc_node *new;
  struct geom_rbtree *root;

  GEOM_ASSERT(p);

  new = (struct geom_alloc_node *)malloc(sizeof(struct geom_alloc_node));
  if (!new) return GEOM_ERR_NOMEM;

  new->p = n;
  new->dealloc = d;

  if (p->root) {
    root = geom_rbtree_insert(&p->root->node, &new->node, geom_alloc_compare,
                              NULL);
  } else {
    root = &new->node;
    geom_rbtree_init(root);
  }
  if (root) {
    p->root = geom_alloc_node_entry(root);
  } else {
    free(new);
    return GEOM_ERR_HAS_POINTER;
  }
#ifdef GEOM_ALLOC_DEBUG
  fprintf(stderr, "geom_alloc: list %p: add %p, root %p, ptr %p\n",
	  p, new, p->root, new->p);
#endif
  return GEOM_SUCCESS;
}

geom_error geom_alloc_free(geom_alloc_list *p, void *n)
{
  struct geom_rbtree *root;
  struct geom_alloc_node *afind;
  struct geom_alloc_node *aroot;

  GEOM_ASSERT(p);

  if (!p->root) return GEOM_ERR_POINTER_NOT_FOUND;

  afind = geom_alloc_find_entry(p, n);
  if (!afind) return GEOM_ERR_POINTER_NOT_FOUND;

  root = &p->root->node;
  root = geom_rbtree_delete(root, &afind->node, NULL);

  if (afind->dealloc) {
    afind->dealloc(afind->p);
  }

  if (root) {
    aroot = geom_alloc_node_entry(root);
    p->root = aroot;
  } else {
    /* Removing the last node. */
    p->root = NULL;
  }

#ifdef GEOM_ALLOC_DEBUG
  fprintf(stderr, "geom_rbtree_alloc: list %p: free %p, root %p, ptr %p\n",
	  p, afind, p->root, afind->p);
#endif
  free(afind);
  return GEOM_SUCCESS;
}


void geom_alloc_free_all(geom_alloc_list *allocs)
{
  struct geom_alloc_node *aroot;
  struct geom_rbtree *root;

  if (!allocs) return;
  if (allocs->root) {
    root = &allocs->root->node;
    while (root) {
      aroot = geom_alloc_node_entry(root);
      root = geom_rbtree_delete(root, root, NULL);
      if (root) {
        allocs->root = geom_alloc_node_entry(root);
      } else {
        allocs->root = NULL;
      }
      if (aroot->dealloc) {
        aroot->dealloc(aroot->p);
        /* allocs->root may change in aroot->dealloc. */
        if (allocs->root) {
          root = &allocs->root->node;
        } else {
          root = NULL;
        }
      }
#ifdef GEOM_ALLOC_DEBUG
      fprintf(stderr, "geom_alloc: list %p: free %p, root %p, ptr %p\n",
	      allocs, aroot, allocs->root, aroot->p);
#endif
      free(aroot);
    }
  }
  free(allocs);
}

void *geom_alloc_find(geom_alloc_list *allocs, void *n)
{
  struct geom_alloc_node *fnd;
  fnd = geom_alloc_find_entry(allocs, n);
  if (fnd) {
    return n;
  } else {
    return NULL;
  }
}
