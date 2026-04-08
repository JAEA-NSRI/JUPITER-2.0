/* This is -*- c -*- file */
/* This is vim: set ft=c: source. */

#include <jupiter/component_data.h>
#include <jupiter/component_data_defs.h>
#include <jupiter/control/defs.h>
#include <jupiter/control/manager.h>
#include <jupiter/csv.h>
#include <jupiter/csvutil.h>
#include <jupiter/field_control.h>
#include <jupiter/func.h>
#include <jupiter/geometry/bitarray.h>
#include <jupiter/optparse.h>
#include <jupiter/struct.h>

#include <float.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

/*
 * Prints easy parsable mesh info from JUPITER parameter and flag input file
 */

void print_usage(const char *progname)
{
  fprintf(stderr,
          "Prints easy parsable mesh info from JUPITER input files.\n"
          "\n"
          "Usage: %s -input param.txt -flags flags.txt [--] [output-file] "
          "[X[DMF|ML]]\n"
          "\n"
          "JUPITER options:\n"
          "    -input FILE     Parameter input file\n"
          "    -flags FILE     Flags input file\n"
          "\n"
          "  If [output-file] not given, writes to stdout. But since this\n"
          "  progrem uses JUPITER to process input, stdout may contain excess\n"
          "  output.\n"
          "\n"
          "  If `X`, `XDMF` or `XML` specified for the last argument or\n"
          "  the extentsion of [output-file] is `.xmf`, `.xdmf` or `.xml`,\n"
          "  writes in XDMF (based on XML), to use with XML editors,\n"
          "  translators or libraries\n"
          "\n",
          progname);
}

csv_error open_read_csv(csv_data **csv, const char *fname)
{
  long el, ec;
  csv_error ret;
  FILE *fp;

  el = -1;
  ec = -1;
  fp = fopen(fname, "rb");
  if (fp) {
    ret = readCSV(fp, csv, &el, &ec);
    fclose(fp);
  } else {
    ret = CSV_ERR_SYS;
  }

  if (ret != CSV_ERR_SUCC) {
    csvperror(fname, el, ec, CSV_EL_ERROR, NULL, ret, errno, 0, NULL);
  }
  return ret;
}

void dump_mesh_main(const char *flags_file, const char *param_file,
                    csv_data *flags_csv, csv_data *param_csv, flags **pflg,
                    domain **pcdo, jcntrl_executive_manager *manager,
                    controllable_type *chead, component_data *comp_data_head,
                    jupiter_options *opts, int *status)
{
  flags *flg;
  domain *cdo;
  mpi_param mpi;
  int lst;

#ifdef JUPITER_MPI
  mpi.CommJUPITER = MPI_COMM_SELF;
#endif

  flg = init_flag(flags_file, flags_csv, param_file, param_csv, opts, status);
  if (!flg)
    return;

  *pflg = flg;

  lst = OFF;
  set_mpi_for_rank(&mpi, flg->pex, flg->pey, flg->pez, flg->pea, 0, 0, &lst);
  if (lst == ON) {
    if (status)
      *status = ON;
    return;
  }

  cdo = init_cdomain(flg, &mpi, param_file, param_csv, manager, chead,
                     comp_data_head, status);
  if (!cdo)
    return;

  *pcdo = cdo;
}

void write_mesh_text_fp(flags *flg, domain *cdo, const char *outfile, FILE *fp,
                        int *status)
{
  int mant_dig = 17;
#ifdef JUPITER_DOUBLE
#ifdef DBL_DECIMAL_DIG
  mant_dig = DBL_DECIMAL_DIG;
#else
  mant_dig = DECIMAL_DIG;
#endif
#else
#ifdef FLT_DECIMAL_DIG
  mant_dig = FLT_DECIMAL_DIG;
#else
  mant_dig = DECIMAL_DIG;
#endif
#endif

  fprintf(fp, "# %suniform grid\n",
          (flg->has_non_uniform_grid == ON) ? "non-" : "");
  fprintf(fp, "# gnx gny gnz\n");
  fprintf(fp, "%d %d %d\n\n", cdo->gnx, cdo->gny, cdo->gnz);
  fprintf(fp, "# pex pey pez\n");
  fprintf(fp, "%d %d %d\n\n", flg->pex, flg->pey, flg->pez);
  if (cdo->gx && cdo->gy && cdo->gz) {
    fprintf(fp, "# Origin (x y z)\n");
    fprintf(fp, "%.*g %.*g %.*g\n\n", mant_dig, cdo->gx[cdo->stm], mant_dig,
            cdo->gy[cdo->stm], mant_dig, cdo->gz[cdo->stm]);
    fprintf(fp, "# Delta (x y z)%s\n",
            (flg->has_non_uniform_grid == ON) ? " (minimum)" : "");
    fprintf(fp, "%.*g %.*g %.*g\n\n", mant_dig, cdo->dx, mant_dig, cdo->dy,
            mant_dig, cdo->dz);
    fprintf(fp, "# X coordinates (i x)\n");
    for (int i = 0; i < cdo->gnx + 1; ++i)
      fprintf(fp, "%d %.*g\n", i, mant_dig, cdo->gx[i + cdo->stm]);
    fprintf(fp, "\n# Y coordinates (i y)\n");
    for (int i = 0; i < cdo->gny + 1; ++i)
      fprintf(fp, "%d %.*g\n", i, mant_dig, cdo->gy[i + cdo->stm]);
    fprintf(fp, "\n# Z coordinates (i z)\n");
    for (int i = 0; i < cdo->gnz + 1; ++i)
      fprintf(fp, "%d %.*g\n", i, mant_dig, cdo->gz[i + cdo->stm]);
    fprintf(fp, "\n# end\n");
  }
}

void write_mesh_xdmf_fp(flags *flg, domain *cdo, const char *outfile, FILE *fp,
                        int *status)
{
  int mant_dig = 17;
  int wrap = 3;
#ifdef JUPITER_DOUBLE
#ifdef DBL_DECIMAL_DIG
  mant_dig = DBL_DECIMAL_DIG;
#else
  mant_dig = DECIMAL_DIG;
#endif
#else
#ifdef FLT_DECIMAL_DIG
  mant_dig = FLT_DECIMAL_DIG;
#else
  mant_dig = DECIMAL_DIG;
#endif
#endif
  wrap = 80 / mant_dig - 1;
  if (wrap <= 0)
    wrap = 1;

  fprintf(fp, "<?xml version=\"1.0\"?>\n");
  fprintf(fp, "<Xdmf Version=\"2.0\">\n");
  fprintf(fp, "<Domain>\n");
  fprintf(fp, "<Grid GridType=\"Uniform\">\n");
  fprintf(fp, "<Topology Type=\"3DRectMesh\" Dimensions=\"%d %d %d\"/>\n",
          cdo->gnx, cdo->gny, cdo->gnz);
  fprintf(fp, "<Geometry GeometryType=\"VxVyVz\">\n");
  if (cdo->gx && cdo->gy && cdo->gz) {
    fprintf(fp,
            "<DataItem Dimensions=\"%d\" NumberType=\"Float\""
            " Format=\"XML\">",
            cdo->gnx + 1);
    for (int i = 0; i < cdo->gnx + 1; ++i)
      fprintf(fp, "%s%.*g", (i % wrap == 0) ? "\n  " : " ", mant_dig,
              cdo->gx[i + cdo->stm]);
    fprintf(fp, "\n</DataItem>\n");
    fprintf(fp,
            "<DataItem Dimensions=\"%d\" NumberType=\"Float\""
            " Format=\"XML\">",
            cdo->gny + 1);
    for (int i = 0; i < cdo->gny + 1; ++i)
      fprintf(fp, "%s%.*g", (i % wrap == 0) ? "\n  " : " ", mant_dig,
              cdo->gy[i + cdo->stm]);
    fprintf(fp, "\n</DataItem>\n");
    fprintf(fp,
            "<DataItem Dimensions=\"%d\" NumberType=\"Float\""
            " Format=\"XML\">",
            cdo->gnz + 1);
    for (int i = 0; i < cdo->gnz + 1; ++i)
      fprintf(fp, "%s%.*g", (i % wrap == 0) ? "\n  " : " ", mant_dig,
            cdo->gz[i + cdo->stm]);
    fprintf(fp, "\n</DataItem>\n");
  }
  fprintf(fp, "</Geometry>\n");
  fprintf(fp, "</Grid>\n");
  fprintf(fp, "</Domain>\n");
  fprintf(fp, "</Xdmf>\n");
}

void write_mesh_text(flags *flg, domain *cdo, const char *outfile, int *status,
                     void (*writer)(flags *flg, domain *cdo,
                                    const char *outfile, FILE *fp, int *status))
{
  FILE *fp = stdout;
  if (outfile) {
    fp = fopen(outfile, "w");
    if (!fp) {
      if (status)
        *status = ON;
      csvperror(outfile, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_FOPEN, errno, 0,
                NULL);
      return;
    }
  } else {
    outfile = "<stdout>";
  }

  writer(flg, cdo, outfile, fp, status);

  if (fp != stdout)
    fclose(fp);
}

int main(int argc, char **argv)
{
  flags *flg;
  domain *cdo;
  jupiter_options opts;
  component_data comp_data_head;
  csv_error csverr;
  csv_data *flag_csv, *param_csv;
  controllable_type chead;
  jcntrl_executive_manager *manager;
  const char *outfile;
  int status;
  jupiter_print_levels plvls = jupiter_print_levels_all();
  jupiter_file_inputs_flags f = jupiter_file_inputs_flags_zero();
  geom_bitarray_element_set(f.f, JUPITER_INPUT_FILE_FLAGS, 1);
  geom_bitarray_element_set(f.f, JUPITER_INPUT_FILE_PARAMS, 1);

  if (argc <= 1) {
    if (argc == 1)
      print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  jupiter_options_init(&opts, "all:-all+error+fatal", plvls);
  csverr = jupiter_optparse_files(&opts, &argc, &argv, &f);
  if (csverr != CSV_ERR_SUCC) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  outfile = NULL;
  if (argc > 1)
    outfile = argv[1];

  status = OFF;
  flag_csv = NULL;
  param_csv = NULL;
  flg = NULL;
  cdo = NULL;
  component_data_init(&comp_data_head);

  if (opts.flags_file) {
    csverr = open_read_csv(&flag_csv, opts.flags_file);
    if (csverr != CSV_ERR_SUCC)
      status = ON;
  } else {
    csvperrorf(argv[0], 0, 0, CSV_EL_ERROR, NULL, "No flags file given");
    status = ON;
  }

  if (opts.param_file) {
    csverr = open_read_csv(&param_csv, opts.param_file);
    if (csverr != CSV_ERR_SUCC)
      status = ON;
  } else {
    csvperrorf(argv[0], 0, 0, CSV_EL_ERROR, NULL, "No parameter file given");
    status = ON;
  }

  controllable_type_init(&chead);
  manager = jcntrl_executive_manager_new();
  if (!manager) {
    csvperror(argv[0], 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
    status = ON;
  }

  if (manager && flag_csv && param_csv)
    dump_mesh_main(opts.flags_file, opts.param_file, flag_csv, param_csv, &flg,
                   &cdo, manager, &chead, &comp_data_head, &opts, &status);

  if (flg && cdo) {
    void (*writer)(flags *flg, domain *cdo, const char *outfile, FILE *fp,
                   int *status);
    writer = write_mesh_text_fp;
    if (argc > 2) {
      const char *p = argv[2];
      const char *m;

      for (;;) {
        /*!re2c
          re2c:define:YYCTYPE = "unsigned char";
          re2c:define:YYCURSOR = "p";
          re2c:define:YYMARKER = "m";
          re2c:yyfill:enable = 0;
          re2c:indent:top = 4;
          re2c:indent:string = "  ";

          end = "\000";

          * { break; }
          'X'('ML'|'DMF')?/end { writer = write_mesh_xdmf_fp; break; }
        */
        CSVUNREACHABLE();
      }
    } else if (outfile) {
      const char *p = outfile;
      const char *m;

      for (;;) {
        /*!re2c
          re2c:indent:top = 4;

          * { break; }
          [\x01-\xff]*'.x'('d'?'mf'|'ml')/end
          { writer = write_mesh_xdmf_fp; break; }
        */
        CSVUNREACHABLE();
      }
    }

    write_mesh_text(flg, cdo, outfile, &status, writer);
  }

  component_data_delete_all(&comp_data_head);

  if (manager)
    jcntrl_executive_manager_delete(manager);
  if (flag_csv)
    freeCSV(flag_csv);
  if (param_csv)
    freeCSV(param_csv);
  if (cdo)
    free_domain(cdo);
  if (flg)
    free_flags(flg);

  if (status == ON)
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
