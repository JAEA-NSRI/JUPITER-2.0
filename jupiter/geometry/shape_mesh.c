#include <stdio.h>

#include "geom_assert.h"
#include "shape_mesh.h"

int geom_shape_mesh_export_STL(struct geom_shape_mesh_data *p,
                               const char *path, const char *name)
{
  FILE *fp;
  geom_size_type i;
  struct geom_shape_face_data *facep;
  geom_vec3 vi, vj, vk, a, b, n, s;
  int opn;

  GEOM_ASSERT(p);

  if (!name) name = "unamed-mesh";

  opn = 0;
  if (path) {
    fp = fopen(path, "wb");
    if (!fp) return 1;
    opn = 1;
  } else {
    fp = stdout;
  }

  fprintf(fp, "solid %s\n", name);

  s = geom_vec3_c(0.0, 0.0, 0.0);

  for (i = 0; i < p->nfaces; ++i) {
    facep = &p->faces[i].face;
    vi = p->points[facep->i - 1].point;
    vj = p->points[facep->j - 1].point;
    vk = p->points[facep->k - 1].point;

    a = geom_vec3_sub(vj, vi);
    b = geom_vec3_sub(vk, vi);
    n = geom_vec3_cross_prod(a, b);
    s = geom_vec3_add(s, n);

    fprintf(fp, "facet normal %g %g %g\n",
            geom_vec3_x(n), geom_vec3_y(n), geom_vec3_z(n));
    fprintf(fp, "  outer loop\n");
    fprintf(fp, "    vertex %g %g %g\n",
            geom_vec3_x(vi), geom_vec3_y(vi), geom_vec3_z(vi));
    fprintf(fp, "    vertex %g %g %g\n",
            geom_vec3_x(vj), geom_vec3_y(vj), geom_vec3_z(vj));
    fprintf(fp, "    vertex %g %g %g\n",
            geom_vec3_x(vk), geom_vec3_y(vk), geom_vec3_z(vk));
    fprintf(fp, "  endloop\n");
    fprintf(fp, "endfacet\n");
  }

  fprintf(fp, "endsolid %s\n", name);

  if (opn) {
    fclose(fp);
  }

  s = geom_vec3_factor(s, 0.5);
  geom_warn("Sum of Area: (%g, %g, %g)",
            geom_vec3_x(s), geom_vec3_y(s), geom_vec3_z(s));

  return 0;
}
