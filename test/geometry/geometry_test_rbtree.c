
#include "test-util.h"

#include "geometry_test.h"

#include <jupiter/geometry/defs.h>
#include <jupiter/geometry/rbtree.h>

struct rbtree_test_data
{
  struct geom_rbtree node;
  int key;
};

static struct rbtree_test_data *
rbtree_test_entry(struct geom_rbtree *n)
{
  return geom_rbtree_entry(n, struct rbtree_test_data, node);
}

static
int rbtree_test_compare(struct geom_rbtree *a, struct geom_rbtree *b)
{
  struct rbtree_test_data *ad;
  struct rbtree_test_data *bd;
  ad = rbtree_test_entry(a);
  bd = rbtree_test_entry(b);

  /*
   * If a  < b, returns negative.
   * If a == b, returns zero.
   * If a >  b, returns positive.
   */
  return ad->key - bd->key;
}

static
void rbtree_print_node(struct geom_rbtree *node, struct geom_rbtree *parent,
                       int depth, int is_left)
{
  char color;
  struct rbtree_test_data *td;
  int i, j;
  /*
   * If the tree's max_depth is 64, the tree's minimum leaf depth is
   * greater than 32. So there is 2^32 nodes at least. This is enough.
   */
  enum { max_depth = 64 };
  int bar[max_depth];

  if (depth > 0) {
    if (node == parent) {
      for (i = 0; i < depth; ++i) {
        fprintf(stderr, "---");
      }
    } else {
      struct geom_rbtree *p;
      p = parent;
      bar[0] = is_left ? 1 : 0;
      for (i = 1; i <= depth && i < max_depth; ++i) {
        if (geom_rbtree_is_root(p)) {
          bar[i] = -1;
          break;
        } else {
          parent = geom_rbtree_parent(p);
          /* Test the parent node is drawn above of the node. */
          bar[i] = (p == geom_rbtree_left(parent)) ? 1 : 0;
          p = parent;
        }
      }
      for (j = i; j < depth; ++j) {
        fprintf(stderr, "...");
      }
      for (j = 1; j < depth && j < max_depth; ++j) {
        int jj, l, r;
        jj = i - j;
        l = bar[jj];
        r = bar[jj - 1];
        if (r < 0) {
          fprintf(stderr, "---");
        } else {
          if ((l && r) || (!l && !r)) {
            fprintf(stderr, "   ");
          } else {
            fprintf(stderr, " | ");
          }
        }
      }
      if (is_left) {
        fprintf(stderr, " .-");
      } else {
        fprintf(stderr, " '-");
      }
    }
  }
  switch(geom_rbtree_color(node)) {
  case GEOM_RBTREE_BLACK:
    color = 'B';
    break;
  case GEOM_RBTREE_RED:
    color = 'R';
    break;
  default:
    color = '?';
    break;
  }
  if (node) {
    td = rbtree_test_entry(node);
    fprintf(stderr, "[%c][%2d] %d (%#x)\n", color, depth, td->key, td->key);
  } else {
    fprintf(stderr, "[%c][%2d] (nil)\n", color, depth);
  }
}

static
void rbtree_print(struct geom_rbtree *root, int draw_nils)
{
  int depth = -1;
  struct geom_rbtree *lp;

  geom_rbtree_foreach_succ_depth(lp, root, depth) {
    struct geom_rbtree *p;

    if (draw_nils && !geom_rbtree_left(lp)) {
      rbtree_print_node(NULL, lp, depth + 1, 1);
    }
    p = geom_rbtree_parent(lp);
    rbtree_print_node(lp, p, depth, geom_rbtree_left(p) == lp);
    if (draw_nils && !geom_rbtree_right(lp)) {
      rbtree_print_node(NULL, lp, depth + 1, 0);
    }
  }
}

int rbtree_test(void)
{
  enum num { N = 100 };
  struct rbtree_test_data arr[N];
  struct geom_rbtree *root;
  struct geom_rbtree *pnt;
  int i;

  for (i = 0; i < N; ++i) {
    arr[i].key = i;
  }

  root = &arr[7].node;
  root = geom_rbtree_insert(NULL, root, rbtree_test_compare, NULL);

  root = geom_rbtree_insert(root, &arr[ 3].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 1);
  test_compare(rbtree_test_entry(geom_rbtree_minimum(root))->key, 3);

  root = geom_rbtree_insert(root, &arr[11].node, rbtree_test_compare, NULL);
  root = geom_rbtree_insert(root, &arr[ 8].node, rbtree_test_compare, NULL);
  root = geom_rbtree_insert(root, &arr[ 2].node, rbtree_test_compare, NULL);
  root = geom_rbtree_insert(root, &arr[18].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 1);
  test_compare(rbtree_test_entry(geom_rbtree_minimum(root))->key, 2);

  root = geom_rbtree_insert(root, &arr[ 6].node, rbtree_test_compare, NULL);
  root = geom_rbtree_insert(root, &arr[ 5].node, rbtree_test_compare, NULL);
  root = geom_rbtree_insert(root, &arr[10].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);
  test_compare(rbtree_test_entry(geom_rbtree_minimum(root))->key, 2);

  root = geom_rbtree_insert(root, &arr[ 4].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);
  test_compare(rbtree_test_entry(geom_rbtree_minimum(root))->key, 2);

  root = geom_rbtree_insert(root, &arr[19].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);
  test_compare(rbtree_test_entry(geom_rbtree_minimum(root))->key, 2);

  root = geom_rbtree_insert(root, &arr[ 0].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);
  test_compare(rbtree_test_entry(geom_rbtree_minimum(root))->key, 0);

  root = geom_rbtree_insert(root, &arr[13].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_insert(root, &arr[14].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_insert(root, &arr[15].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_insert(root, &arr[17].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_insert(root, &arr[16].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_delete(root, root, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  pnt = geom_rbtree_find(root, &arr[ 6].node, rbtree_test_compare);
  fprintf(stderr, "------------------\n");
  if (pnt) {
    fprintf(stderr, "..... %d\n", rbtree_test_entry(pnt)->key);
  } else {
    fprintf(stderr, "..... (not found)\n");
  }

  root = geom_rbtree_delete(root, pnt, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_delete(root, &arr[11].node, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_delete(root, &arr[ 5].node, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_delete(root, &arr[15].node, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_insert(root, &arr[ 5].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_insert(root, &arr[15].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  arr[0].key = 41;
  arr[1].key = 38;
  arr[2].key = 31;
  arr[3].key = 12;
  arr[4].key = 19;
  arr[5].key = 8;

  root = geom_rbtree_insert(NULL, &arr[0].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);
  test_compare(rbtree_test_entry(geom_rbtree_minimum(root))->key, 41);

  root = geom_rbtree_insert(root, &arr[1].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");

  rbtree_print(root, 0);
  root = geom_rbtree_insert(root, &arr[2].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");

  rbtree_print(root, 0);
  root = geom_rbtree_insert(root, &arr[3].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");

  rbtree_print(root, 0);
  root = geom_rbtree_insert(root, &arr[4].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_insert(root, &arr[5].node, rbtree_test_compare, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);
  test_compare(rbtree_test_entry(geom_rbtree_minimum(root))->key, 8);

  root = geom_rbtree_delete(root, &arr[5].node, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_delete(root, &arr[3].node, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_delete(root, &arr[4].node, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_delete(root, &arr[2].node, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  root = geom_rbtree_delete(root, &arr[1].node, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);
  test_compare(rbtree_test_entry(geom_rbtree_minimum(root))->key, 41);

  root = geom_rbtree_delete(root, &arr[0].node, NULL);
  fprintf(stderr, "------------------\n");
  rbtree_print(root, 0);

  arr[ 0].key = 0x7ce980; /* add 0x7d3180 */
  arr[ 1].key = 0x7d3040; /* add 0x7d31c0 */
  arr[ 2].key = 0x7cb3d0; /* add 0x7d3200 */
  arr[ 3].key = 0x7d3240; /* add 0x7d32c0 */
  arr[ 4].key = 0x7d3300; /* add 0x7d3320 */
  arr[ 5].key = 0x7d3360; /* add 0x7d33a0 */
  arr[ 6].key = 0x7d33e0; /* add 0x7d3440 */
  arr[ 7].key = 0x7d3680; /* add 0x7d36a0 */
  arr[ 8].key = 0x7d3720; /* add 0x7d36e0 */
  arr[ 9].key = 0x7d3780; /* add 0x7d37a0 */
  arr[10].key = 0x000000; /* add 0x7d37e0 */
  arr[11].key = 0x7d3820; /* add 0x7d3880 */
  arr[12].key = 0x7d38c0; /* add 0x7d38e0 */
  arr[13].key = 0x7d3960; /* add 0x7d3920 */
  arr[14].key = 0x7d39c0; /* add 0x7d39e0 */
  arr[15].key = 0x7d3a60; /* add 0x7d3a20 */
  arr[16].key = 0x7d3ac0; /* add 0x7d3ae0 */
  arr[17].key = 0x7d3b60; /* add 0x7d3b20 */
  arr[18].key = 0x7d3bc0; /* add 0x7d3be0 */
  arr[19].key = 0x7cad90; /* add 0x7d3c20 */
  arr[20].key = 0x7d3c60; /* add 0x7d3cb0 */
  arr[21].key = 0x7d3480; /* add 0x7d3cf0 */
  arr[22].key = 0x7d34a0; /* add 0x7d3d30 */
  arr[23].key = 0x7d3d70; /* add 0x7d3e90 */
  arr[24].key = 0x7d3ed0; /* add 0x7d3f80 */
  arr[25].key = 0x7d3fc0; /* add 0x7d3fe0 */
  arr[26].key = 0x7d3150; /* add 0x7db640 */
  arr[27].key = 0x7d3520; /* add 0x7dee00 */
  arr[28].key = 0x7d4020; /* add 0x7dee40 */
  arr[29].key = 0x7def60; /* add 0x7defa0 */
  arr[30].key = 0x7db610; /* add 0x7defe0 */
  arr[31].key = 0x7df020; /* add 0x7df0d0 */
  arr[32].key = 0x7df110; /* add 0x7df130 */
  arr[33].key = 0x7df190; /* add 0x7df1b0 */
  arr[34].key = 0x7d35d0; /* add 0x7df1f0 */
  arr[35].key = 0x7df230; /* add 0x7df2e0 */
  arr[36].key = 0x7df170; /* add 0x7df320 */
  arr[37].key = 0x7df380; /* add 0x7df3d0 */
  arr[38].key = 0x7dee80; /* add 0x7e2b90 */
  arr[39].key = 0x7df360; /* add 0x7e2bd0 */
  arr[40].key = 0x7e2cf0; /* add 0x7e2d30 */
  arr[41].key = 0x7df3a0; /* add 0x7e2d70 */
  arr[42].key = 0x7e2db0; /* add 0x7e2e60 */
  arr[43].key = 0x7e2ea0; /* add 0x7e2ec0 */
  arr[44].key = 0x7e2f20; /* add 0x7e2f40 */
  arr[45].key = 0x7def30; /* add 0x7e2f80 */
  arr[46].key = 0x7e2f00; /* add 0x7e2fc0 */
  arr[47].key = 0x7e3020; /* add 0x7e3050 */
  arr[48].key = 0x7e3090; /* add 0x7e30f0 */
  arr[49].key = 0x7e3130; /* add 0x7e3150 */
  arr[50].key = 0x7e3000; /* add 0x7e3190 */

  root = NULL;
  for (i = 0; i < 51; ++i) {
    geom_rbtree_init(&arr[i].node);
    root = geom_rbtree_insert(root, &arr[i].node, rbtree_test_compare, NULL);
  }
  test_compare(rbtree_test_entry(root)->key, 0x7d3ac0);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7d3180, root 0x7d3ae0, ptr 0x7ce980
  root = geom_rbtree_delete(root, &arr[0].node, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7d3ac0);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7d3ae0, root 0x7d3b20, ptr 0x7d3ac0
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7d3b60);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7d3b20, root 0x7d3be0, ptr 0x7d3b60
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7d3bc0);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7d3be0, root 0x7d3cb0, ptr 0x7d3bc0
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7d3c60);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7d3cb0, root 0x7d3e90, ptr 0x7d3c60
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7d3d70);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7d3e90, root 0x7d3f80, ptr 0x7d3d70
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7d3ed0);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7d3f80, root 0x7d3fe0, ptr 0x7d3ed0
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7d3fc0);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7d3fe0, root 0x7dee40, ptr 0x7d3fc0
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7d4020);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7dee40, root 0x7defe0, ptr 0x7d4020
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7db610);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7defa0, root 0x7e2b90, ptr 0x7def60
  root = geom_rbtree_delete(root, &arr[29].node, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7db610);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7defe0, root 0x7e2b90, ptr 0x7db610
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7dee80);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7e2b90, root 0x7e2f80, ptr 0x7dee80
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7def30);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7e2f40, root 0x7df0d0, ptr 0x7e2f20
  root = geom_rbtree_delete(root, &arr[44].node, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7def30);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7e2f80, root 0x7df0d0, ptr 0x7def30
  root = geom_rbtree_delete(root, &arr[45].node, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7df020);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7df0d0, root 0x7df130, ptr 0x7df020
  root = geom_rbtree_delete(root, &arr[31].node, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7df110);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7df130, root 0x7df320, ptr 0x7df110
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7df170);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7df320, root 0x7df1b0, ptr 0x7df170
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7df190);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7df1b0, root 0x7df2e0, ptr 0x7df190
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7df230);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7df2e0, root 0x7e2bd0, ptr 0x7df230
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7df360);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7e2bd0, root 0x7df3d0, ptr 0x7df360
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7df380);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7df3d0, root 0x7e2d70, ptr 0x7df380
  root = geom_rbtree_delete(root, root, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7df3a0);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7e2d30, root 0x7d36a0, ptr 0x7e2cf0
  root = geom_rbtree_delete(root, &arr[40].node, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7d3680);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7e2d70, root 0x7d36a0, ptr 0x7df3a0
  root = geom_rbtree_delete(root, &arr[41].node, NULL);
  test_compare(rbtree_test_entry(root)->key, 0x7d3680);
  rbtree_print(root, 0);

  // list 0x7d3090: free 0x7e2d30, root 0x7e2d30, ptr 0x7e2cf0
  // list 0x7d3090: free 0x7e2d30, root 0x7e2d30, ptr 0x7e2cf0

  return 0;
}
