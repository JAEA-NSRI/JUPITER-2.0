#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>

#include <jupiter/csv.h>
#include <jupiter/struct.h>
#include <jupiter/func.h>
#include <jupiter/optparse.h>
#include <jupiter/csvutil.h>
#include <jupiter/geometry/defs.h>
#include <jupiter/geometry/list.h>
#include <jupiter/geometry/data.h>
#include <jupiter/geometry/file.h>
#include <jupiter/geometry/udata.h>
#include <jupiter/geometry/rbtree.h>
#include <jupiter/geometry/vector.h>

int extfile_extract_main(int *argc, char ***argv);
int extfile_extract(parameter *prm, int argc, char **argv);

int main(int argc, char **argv)
{
  int status;

#ifdef JUPITER_MPI
  MPI_Init(&argc, &argv);
#endif

  status = extfile_extract_main(&argc, &argv);

#ifdef JUPITER_MPI
  MPI_Finalize();
#endif

  return (status) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int extfile_extract_main(int *argc, char ***argv)
{
  int status;
  parameter *prm;
  enum set_parameters_options opts;

  opts = SET_PARAMETERS_DOMAIN | SET_PARAMETERS_READ_FLAGS |
    SET_PARAMETERS_READ_PARAM | SET_PARAMETERS_READ_GEOMETRY |
    SET_PARAMETERS_READ_CONTROL | SET_PARAMETERS_READ_PLIST |
    SET_PARAMETERS_MPI;

  prm = set_parameters(argc, argv, opts, 0);
  if (!prm) return 1;

#ifdef JUPITER_MPI
  CSVASSERT(prm->mpi);
  if (prm->mpi->npe_glob > 1 || prm->mpi->npe > 1) {
    if (prm->mpi->rank == 0) {
      csvperrorf("<command line>", 0, 0, CSV_EL_FATAL, NULL, "Run with only 1 MPI process. We have %d MPI processes.", prm->mpi->npe);
    }
    free_parameter(prm);
    return 1;
  }
#endif

  status = extfile_extract(prm, *argc, *argv);

  free_parameter(prm);
  return status;
}

void help(const char *pname) {
  fprintf(
    stderr,
    "Usage: %s [JUPITER options] -- [geometry-files|tm-tables] [-o output]\n"
    "\n"
    "JUPITER Options:\n"
    "  -input [file]   Parameter input file\n"
    "  -flags [file]   Flags input file\n"
    "  -geom  [file]   Geometry input file\n"
    "  -control [file] Control input file\n"
    "  -plist [file]   Property list input file\n"
    "\n"
    "  NOTE: Some part of these input files won't be checked, but must be "
    "given\n"
    "\n"
    "Extractor options:\n"
    "  geometry-files  Collects information of binary files may be converted\n"
    "                  from STL (STereo Lithography) files\n"
    "  tm-tables       Collects information of binary files may be converted\n"
    "                  from Liquidus/Solidus temperature CSV files\n"
    "  -o [file]       Output filename\n",
    pname);
}

int extract_geometry_files(parameter *prm, const char *output);
int extract_tm_tables(parameter *prm, const char *output);

int extfile_extract(parameter *prm, int argc, char **argv)
{
  const char *output;
  const char *mode;

  if (argc < 2) {
    help(argv[0]);
    return 1;
  }
  mode = argv[1];

  if (argc >= 3) {
    output = argv[2];
    if (strncmp(output, "-o", 2) == 0) {
      if (output[2] != '\0') {
        output = &output[2];
      } else {
        if (argc >= 4) {
          output = argv[3];
        } else {
          csvperrorf("<command line>", 0, 0, CSV_EL_FATAL, NULL,
                     "Missing parameters for option -o");
          return 1;
        }
      }
    } else {
      csvperrorf("<command line>", 0, 0, CSV_EL_FATAL, NULL,
                 "Invalid parameter: %s", output);
      return 1;
    }
  } else {
    output = NULL;
  }

  if (strcmp("geometry-files", mode) == 0) {
    return extract_geometry_files(prm, output);
  } else if (strcmp("tm-tables", mode) == 0) {
    return extract_tm_tables(prm, output);
  }

  csvperrorf("<command line>", 0, 0, CSV_EL_FATAL, NULL,
             "Invalid mode: %s", mode);
  return 1;
}

/* STL files */

struct extract_geometry_file_data
{
  struct geom_rbtree tree;
  geom_file_data *file;
  const char *path;
};
#define extract_geometry_file_data_entry(ptr) \
  geom_rbtree_entry(ptr, struct extract_geometry_file_data, tree);

static int
extract_geometry_file_data_comp(struct geom_rbtree *a, struct geom_rbtree *b)
{
  struct extract_geometry_file_data *fa, *fb;
  int r;

  fa = extract_geometry_file_data_entry(a);
  fb = extract_geometry_file_data_entry(b);

  r = strcmp(fa->path, fb->path);
  if (r == 0) return 0;
  if (r < 0) return -1;
  return 1;
}

static void
extract_geometry_file_tree_delete_all(struct extract_geometry_file_data *rootp)
{
  struct geom_rbtree *root;
  struct geom_rbtree *nroot;
  struct extract_geometry_file_data *fp;

  CSVASSERT(rootp);

  root = &rootp->tree;
  while (root) {
    nroot = geom_rbtree_delete(root, root, NULL);
    fp = extract_geometry_file_data_entry(root);
    free(fp);
    root = nroot;
  }
}

geom_data *
extract_geometry_files_core(csv_data *geom_csv, const char *geom_file,
                            int geom_num, parameter *prm, geom_vec3 stl_origin,
                            double unit_factor,
                            struct extract_geometry_file_data **rootp)
{
  domain *cdo;
  char *filen;
  FILE *ofp, *fp;
  geom_data *geom;
  geom_data_element *data_el;
  struct extract_geometry_file_data *item;
  struct geom_rbtree *root;

  CSVASSERT(prm);
  CSVASSERT(geom_csv);
  CSVASSERT(geom_file);

  cdo = prm->cdo;
  CSVASSERT(cdo);
  CSVASSERT(rootp);
  CSVASSERT(!*rootp || geom_rbtree_is_root(&(*rootp)->tree));
  CSVASSERT(geom_num > 0);

  root = NULL;
  geom = set_geom(geom_csv, geom_file, &prm->comps_data_head, //
                  cdo->gnx, cdo->gny, cdo->gnz, geom_num, &prm->status,
                  prm->controls, &prm->control_head);
  if (!geom) {
    return NULL;
  }

  if (prm->status == ON) {
    geom_data_delete(geom);
    return NULL;
  }

  for (data_el = geom_data_get_element(geom); data_el;
       data_el = geom_data_element_next(data_el)) {
    const char *path;
    geom_file_data *file;
    jupiter_geom_ext_file_data *ext_fldata;
    const geom_user_defined_data *ud;
    int pathlen;

    file = geom_data_element_get_file(data_el);
    if (!file) continue;

    path = geom_file_data_get_file_path(file);
    if (!path) continue;

    pathlen = strlen(path);
    if (pathlen < 4 || strcmp(&path[pathlen - 4], ".bin") != 0) {
#ifndef NDEBUG
      csvperrorf(path, 0, 0, CSV_EL_DEBUG, NULL,
                 "Skipped for non-\".bin\" files");
#endif
      continue;
    }

    ud = geom_file_data_get_extra_data(file);
    ext_fldata = geom_user_defined_data_get(ud);
    if (!ext_fldata) {
      csvperrorf(path, 0, 0, CSV_EL_ERROR, NULL, "Could not obtain extra data");
      continue;
    }

    switch(ext_fldata->read_mode) {
    case BINARY_OUTPUT_BYPROCESS:
      csvperrorf(path, 0, 0, CSV_EL_WARN, NULL, "Splitted files are not supported");
      continue;
    case BINARY_OUTPUT_UNIFY_GATHER:
    case BINARY_OUTPUT_UNIFY_MPI:
      {
        geom_svec3 size, origin;
        geom_svec3 size_ref, origin_ref;
        origin = geom_file_data_get_origin(file);
        origin_ref = geom_svec3_c(0, 0, 0);
        size = geom_file_data_get_size(file);
        size_ref = geom_svec3_c(cdo->gnx, cdo->gny, cdo->gnz);

        if (geom_svec3_eql(origin_ref, origin) &&
            geom_svec3_eql(size_ref, size)) {
          item = (struct extract_geometry_file_data *)
            calloc(1, sizeof(struct extract_geometry_file_data));
          if (!item) {
            csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                      CSV_ERR_NOMEM, 0, 0, NULL);
            if (root) {
              struct extract_geometry_file_data *fp;
              fp = extract_geometry_file_data_entry(root);
              extract_geometry_file_tree_delete_all(fp);
            }
            geom_data_delete(geom);
            return NULL;
          }
          item->file = file;
          item->path = path;
          if (root) {
            struct geom_rbtree *nroot;
            nroot = geom_rbtree_insert(root, &item->tree,
                                       extract_geometry_file_data_comp, NULL);
            if (nroot) {
              root = nroot;
            } else {
              /* duplicated (local) */
              free(item);
            }
          } else {
            root = &item->tree;
            geom_rbtree_init(root);
          }
        } else {
          csvperrorf(path, 0, 0, CSV_EL_WARN, NULL,
                     "This program will skip customized size/origin data"
                     " (origin: (%"PRIdMAX", %"PRIdMAX", %"PRIdMAX"), "
                     "size: (%"PRIdMAX", %"PRIdMAX", %"PRIdMAX"))",
                     (intmax_t)geom_svec3_x(origin),
                     (intmax_t)geom_svec3_y(origin),
                     (intmax_t)geom_svec3_z(origin),
                     (intmax_t)geom_svec3_x(size),
                     (intmax_t)geom_svec3_y(size),
                     (intmax_t)geom_svec3_z(size));
          continue;
        }
      }
      break;
    case BINARY_OUTPUT_INVALID:
    default:
      csvperrorf(path, 0, 0, CSV_EL_WARN, NULL, "File read mode unknown");
      continue;
    }
  }

  if (*rootp) {
    struct geom_rbtree *merge_root;
    struct geom_rbtree *merge_nroot;
    struct geom_rbtree *nroot;

    merge_root = &(*rootp)->tree;
    while (root) {
      nroot = geom_rbtree_delete(root, root, NULL);
      merge_nroot = geom_rbtree_insert(merge_root, root,
                                       extract_geometry_file_data_comp, NULL);
      if (merge_nroot) {
        merge_root = merge_nroot;
      } else {
        struct extract_geometry_file_data *fp;

        /* duplicated (global) */
        fp = extract_geometry_file_data_entry(root);
        free(fp);
      }
      root = nroot;
    }
    *rootp = extract_geometry_file_data_entry(merge_root);
  } else {
    *rootp = extract_geometry_file_data_entry(root);
  }
  return geom;
}

int extract_geometry_files(parameter *prm, const char *output)
{
  domain *cdo;
  char *filen;
  FILE *ofp, *fp;
  geom_data *geom, *ctrl;
  geom_vec3 stl_origin;
  double unit_factor;
  struct extract_geometry_file_data *root;
  int geom_num;

  CSVASSERT(prm);

  cdo = prm->cdo;
  CSVASSERT(cdo);

  {
    csv_data *csv;
    const char *fname;
    double x, y, z;
    SET_P_INIT_NOLOC(prm->geom_data, prm->geom_file);

    SET_P(&x, exact_double, "STL_origin", 1, 0.0);
    if (!SET_P_PERROR_FINITE(x, ERROR, "STL origin must be finite")) {
      prm->status = ON;
    }

    SET_P_NEXT(&y, exact_double, 0.0);
    if (!SET_P_PERROR_FINITE(y, ERROR, "STL origin must be finite")) {
      prm->status = ON;
    }

    SET_P_NEXT(&z, exact_double, 0.0);
    if (!SET_P_PERROR_FINITE(z, ERROR, "STL origin must be finite")) {
      prm->status = ON;
    }

    stl_origin = geom_vec3_c(x, y, z);

    SET_P(&unit_factor, exact_double, "STL_unit_factor", 1, 1.0);
    if (!SET_P_PERROR_GREATER(unit_factor, 0.0, OFF, OFF, ERROR,
                              "Unit factor should be positive")) {
      prm->status = ON;
    } else if (!SET_P_PERROR_FINITE(unit_factor, ERROR,
                                    "Unit factor must be positive")) {
      prm->status = ON;
    }
  }

  root = NULL;
  geom = NULL;
  if (prm->geom_data && prm->geom_file) {
    geom_num = get_geom_num(prm->geom_data, prm->geom_file, NULL);
    if (geom_num > 0) {
      geom = extract_geometry_files_core(prm->geom_data, prm->geom_file,
                                         geom_num, prm, stl_origin, unit_factor,
                                         &root);
      if (!geom) {
        prm->status = ON;
      }
    } else if (geom_num < 0) {
      prm->status = ON;
    }
  }

  ctrl = NULL;
  if (prm->control_data && prm->control_file) {
    geom_num = get_geom_num(prm->control_data, prm->control_file, NULL);
    if (geom_num > 0) {
      ctrl = extract_geometry_files_core(prm->control_data, prm->control_file,
                                         geom_num, prm, stl_origin, unit_factor,
                                         &root);
      if (!ctrl) {
        prm->status = ON;
      }
    } else if (geom_num < 0) {
      prm->status = ON;
    }
  }

  if (prm->status == ON) {
    if (geom)
      geom_data_delete(geom);
    if (ctrl)
      geom_data_delete(ctrl);
    if (root)
      extract_geometry_file_tree_delete_all(root);
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
               "Aborting because of previous error(s)");
    return 1;
  }

  if (output) {
    ofp = fopen(output, "w");
    if (!ofp) {
      if (errno == 0) {
        csvperror(output, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_FOPEN, 0, 0, NULL);
      } else {
        csvperror(output, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
      }
      fp = stdout;
      fprintf(fp, "\n");
    } else {
      fp = ofp;
    }
  } else {
    ofp = NULL;
    fp = stdout;
    fprintf(fp, "\n");
  }

  if (root) {
    struct geom_rbtree *lp;
    struct extract_geometry_file_data *item;
    int pathlen;

    if (ofp) {
      fprintf(stdout, "\nFollowing files will be processed:\n");
    }

    geom_rbtree_foreach_succ (lp, &root->tree) {
      item = extract_geometry_file_data_entry(lp);
      pathlen = strlen(item->path);

      if (ofp) {
        fprintf(stdout, " - %.*s.stl\n", pathlen - 4, item->path);
        fprintf(stdout, "   -> %s\n", item->path);
      }

      /* (f)printf in msvcrt does not print proper values for %g */
      fprintf(fp,
              "stl_si %.*s.stl %.17e %.17e %.17e %.17e %.17e %.17e"
              " %d %d %d %s\n",
              pathlen - 4, item->path, geom_vec3_x(stl_origin),
              geom_vec3_y(stl_origin), geom_vec3_z(stl_origin),
              cdo->dx * unit_factor, cdo->dy * unit_factor,
              cdo->dz * unit_factor, cdo->gnx, cdo->gny, cdo->gnz,
              item->path);
    }
  } else {
    if (ofp) {
      fprintf(stdout, "\n* Nothing requires conversion.\n");
    }
  }

  if (geom)
    geom_data_delete(geom);
  if (ctrl)
    geom_data_delete(ctrl);
  if (root)
    extract_geometry_file_tree_delete_all(root);

  if (ofp) {
    fprintf(stdout, "\nDone (written in %s).\n", output);
  }

  if (ofp) {
    fclose(ofp);
  }

  return 0;
}

/* tm_table */

struct extract_tm_table_data
{
  struct geom_rbtree tree;
  struct geom_list link;
  char *path;
  csv_row *row;
};
#define extract_tm_table_data_entry(ptr)                       \
  geom_rbtree_entry(ptr, struct extract_tm_table_data, tree)

#define extract_tm_table_list_entry(ptr) \
  geom_list_entry(ptr, struct extract_tm_table_data, link)

static int
extract_tm_table_data_comp(struct geom_rbtree *a, struct geom_rbtree *b)
{
  struct extract_tm_table_data *fa, *fb;
  int r;

  fa = extract_tm_table_data_entry(a);
  fb = extract_tm_table_data_entry(b);

  r = strcmp(fa->path, fb->path);
  if (r == 0) return 0;
  if (r < 0) return -1;
  return 1;
}

static struct geom_rbtree *
add_tm_table_file(const char *fname, csv_row *found_row, csv_column *found_col,
                  struct extract_tm_table_data *head,
                  struct extract_tm_table_data *data, char *file,
                  struct geom_rbtree *root, int *status)
{
  struct geom_rbtree *nroot;

  if (file) {
    int pathlen;
    SET_P_INIT(NULL, fname, &found_row, &found_col);
    pathlen = strlen(file);

    if (strcmp(file, "-") == 0) {
      SET_P_PERROR(ERROR, "Cannot use standard input as a table");
      free(file);
      if (status) *status = ON;
    } else if (strcmp(file, "") == 0) {
      SET_P_PERROR(ERROR, "File name is empty");
      free(file);
      if (status) *status = ON;
    } else {
      if (strcmp(&file[pathlen - 4], ".bin") != 0) {
        SET_P_PERROR(WARN, "Extension is not \".bin\", skipped");
        free(file);
        data->path = NULL;
      } else {
        data->path = file;
        if (root) {
          nroot = geom_rbtree_insert(root, &data->tree,
                                     extract_tm_table_data_comp, NULL);
        } else {
          nroot = &data->tree;
          geom_rbtree_init(nroot);
        }
        if (nroot) {
          geom_list_insert_prev(&head->link, &data->link);
          root = nroot;
        } else {
          free(file);
          data->path = NULL;
          csvperrorf_col(fname, found_col, CSV_EL_WARN,
                         "Multiple times specified. Command will be written only once.");
        }
      }
    }
  }
  return root;
}

int extract_tm_tables(parameter *prm, const char *output)
{
  const char *fname;
  csv_column *col;
  csv_row *row;
  ptrdiff_t np, ip;
  char *file;
  struct extract_tm_table_data first_data[3];
  struct extract_tm_table_data *list;
  const char *keynames[2] = {"tm_liq_table", "tm_soli_table"};
  const int keyindices[2] = {1, 2};
  int i;
  int retval;
  struct geom_rbtree *root, *nroot;
  FILE *ofp, *fp;
  SET_P_INIT(prm->plist_data, prm->plist_file, &row, &col);

  if (!prm->plist_data) {
    return 1;
  }

  root = NULL;

  retval = 0;
  np = 0;

  /* 0 is head */
  geom_list_init(&first_data[0].link);

  for (i = 0; i < 2; ++i) {
    struct extract_tm_table_data *ptr;
    ptr = &first_data[keyindices[i]];

    SET_P(&file, charp, keynames[i], 1, NULL);
    root = add_tm_table_file(fname, row, col,
                             &first_data[0], ptr, file, root, &prm->status);

    ptr->row = row;
    if (row) {
      for (row = findCSVRowNext(row); row; row = findCSVRowNext(row)) {
        if (np == PTRDIFF_MAX) {
          csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                     "Too many files is given (overflowed)");
          prm->status = ON;
          break;
        }
        np += 1;
      }
    }
  }

  if (prm->status != ON) {
    list = (struct extract_tm_table_data *)
      calloc(sizeof(struct extract_tm_table_data), np);

    if (!list) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM,
                0, 0, NULL);
      prm->status = ON;
    }
  }

  if (prm->status == ON) {
    retval = 1;
    goto clean;
  }

  ip = 0;
  for (i = 0; i < 2; ++i) {
    row = first_data[keyindices[i]].row;
    if (!row) continue;

    for (row = findCSVRowNext(row); row; row = findCSVRowNext(row)) {
      struct extract_tm_table_data *ptr;

      col = getColumnOfCSV(row, 1);
      if (!col) {
        csvperrorf_row(fname, row, 0, CSV_EL_WARN,
                       "Database filename not found");
        continue;
      }
      ptr = &list[ip++];

      SET_P_CURRENT(&file, charp, NULL);
      root = add_tm_table_file(fname, row, col,
                               &first_data[0], ptr, file, root, &prm->status);
    }
  }

  if (prm->status == ON) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
               "Stopped by previous error. Please check");
    retval = 1;
    goto clean;
  }

  if (output) {
    ofp = fopen(output, "w");
    if (!ofp) {
      if (errno == 0) {
        csvperror(output, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_FOPEN, 0, 0, NULL);
      } else {
        csvperror(output, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_SYS, errno, 0, NULL);
      }
      fp = stdout;
      fprintf(fp, "\n");
    } else {
      fp = ofp;
    }
  } else {
    ofp = NULL;
    fp = stdout;
    fprintf(fp, "\n");
  }

  {
    int i;
    struct geom_list *lp, *lh;

    i = 0;
    lh = &first_data[0].link;

    geom_list_foreach(lp, lh) {
      struct extract_tm_table_data *ptr;
      int pathlen;

      if (i == 0) {
        if (ofp) {
          fprintf(stdout, "\nFollowing files will be processed:\n");
        }
      }
      ++i;

      ptr = extract_tm_table_list_entry(lp);
      pathlen = strlen(ptr->path);

      if (ofp) {
        fprintf(stdout, " - %.*s.csv\n", pathlen - 4, ptr->path);
        fprintf(stdout, "   -> %s\n", ptr->path);
      }

      /* (f)printf in msvcrt does not print proper values for %g */
      fprintf(fp,
              "csv2table -s --adjust-coordinate %.*s.csv %s\n",
              pathlen - 4, ptr->path, ptr->path);
    }

    if (i == 0) {
      if (ofp) {
        fprintf(stdout, "\n* Nothing requires conversion.\n");
      }
    }
  }

  if (ofp) {
    fprintf(stdout, "\nDone (written in %s).\n", output);
  }

  if (ofp) {
    fclose(ofp);
  }

clean:
  {
    struct geom_list *lp, *lh;
    lh = &first_data[0].link;

    geom_list_foreach(lp, lh) {
      struct extract_tm_table_data *tp;
      tp = extract_tm_table_list_entry(lp);
      free(tp->path);
    }
  }

  free(list);

  return retval;
}
