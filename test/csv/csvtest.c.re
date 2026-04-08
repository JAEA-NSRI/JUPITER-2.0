/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

#include <jupiter/csv.h>
#include <jupiter/csvutil.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#include <jupiter/re2c_lparser/re2c_lparser.h>

int run_csv_test(int argc, char **argv);

int main(int argc, char **argv)
{
  int ret;

#ifdef JUPITER_MPI
  MPI_Init(&argc, &argv);
#endif

  ret = run_csv_test(argc, argv);

#ifdef JUPITER_MPI
  MPI_Finalize();
#endif
  return ret;
}

void write_escape_cmake(FILE *output, const char *str)
{
  const char *lt;

  for (;;) {
    lt = str;
    /*!re2c
      re2c:define:YYCTYPE = "unsigned char";
      re2c:define:YYCURSOR = "str";
      re2c:define:YYMARKER = "mrk";
      re2c:yyfill:enable = 0;
      re2c:indent:string = "  ";
      re2c:indent:top = 2;

      *    { fprintf(output, "\\\\x%02x", *lt); continue; }
      '\x00' { break; }
      '\n' { fprintf(output, "\\\\n"); continue; }
      '\r' { fprintf(output, "\\\\r"); continue; }
      '\t' { fprintf(output, "\\\\t"); continue; }
      '"'  { fprintf(output, "\\\""); continue; }
      ';'  { fprintf(output, "\\;"); continue; }
      '$'  { fprintf(output, "\\$"); continue; }
      '\\' { fprintf(output, "\\\\\\\\"); continue; }
      ([-A-Za-z0-9!@#%^&*()_=+' ?,.<>/|`~{}]|"["|"]")+ {
        fprintf(stdout, "%.*s", (int)(str - lt), lt);
        continue;
      }
    */
    CSVUNREACHABLE();
  }
}

int run_csv_test(int argc, char **argv)
{
  int iarg;
  int ret;
  int run;
  const char *path;

  run = 0;
  ret = 0;

  for (iarg = 1; iarg < argc; iarg++) {
    long ecol;
    long eline;
    csv_error rest;
    const char *reststr;
    FILE *fp;
    csv_data *csv;

    run++;
    path = argv[iarg];
    fp = fopen(path, "r");
    if (!fp) {
      csvperror(path, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0, NULL);
      continue;
    }

    rest = readCSV(fp, &csv, &eline, &ecol);
    if (ferror(fp)) {
      csvperror(path, 0, 0, CSV_EL_WARN, "read error", CSV_ERR_SYS,
                errno, 0, NULL);
    } else if (!feof(fp)) {
      if (rest == CSV_ERR_SUCC) {
        csvperrorf(path, 0, 0, CSV_EL_WARN, NULL,
                   "Did not reach to the end-of-file.");
      }
    }
    fclose(fp);

    fprintf(stdout,
            "\n\ninclude(\"");
    write_escape_cmake(stdout, path);
    fprintf(stdout,
            ".cmake\")\n"
            "set(RET 0)\n");
    reststr = "";
#define CSV_ERR(x) \
    CSV_ERR_##x: reststr = #x; goto case_CSV_ERR_##x; case_CSV_ERR_##x
    switch(rest) {
    case CSV_ERR_SUCC:
      reststr = "TRUE";
      break;
    case CSV_ERR(2BIG): break;
    case CSV_ERR(DATA_AFTER_CONT): break;
    case CSV_ERR(BLOCK_COMMENT_AFTER_CONT): break;
    case CSV_ERR(EOF): break;
    case CSV_ERR(MIX_QUOTE): break;

    case CSV_ERR(FOPEN): break;
    case CSV_ERR(NOMEM): break;
    case CSV_ERR(CMDLINE_INVAL): break;
    case CSV_ERR(CMDLINE_RANGE): break;
    case CSV_ERR(SYS): break;
    case CSV_ERR(MPI): break;
    case CSV_ERR(GEOMETRY): break;
    case CSV_ERR(SERIALIZE): break;
    }
    fprintf(stdout,
            "set(_FPN \"");
    write_escape_cmake(stdout, path);
    fprintf(stdout, "\")\n");
    fprintf(stdout,
            "get_filename_component(_BNN \"${_FPN}\" NAME)\n");

    fprintf(stdout,
            "if(NOT \"${RESULT}\" STREQUAL \"%s\")\n"
            "  set(RET 1)\n"
            "  message(SEND_ERROR \"[${_BNN}] Got %s but expected ${RESULT}\")\n"
            "else()\n"
            "  message(STATUS \"[${_BNN}] %s == ${RESULT}\")\n",
            reststr, reststr, reststr);
    if (rest == CSV_ERR_SUCC) {
      const char *lc;
      csv_row *row;
      csv_column *col;
      int rowi, coli;

      fprintf(stdout,
              "  if(\"%s\" STREQUAL \"TRUE\")\n",
              reststr);

      fprintf(stdout,
              "    set(DATA_RESULT \"");

      rowi = 0;
      row = getRowOfCSV(csv, 0);
      for (; row; row = getNextRow(row)) {
        if (rowi)  fprintf(stdout, "\n");
        rowi = 1;
        coli = 0;
        col = getColumnOfCSV(row, 0);
        for (; col; col = getNextColumn(col)) {
          if (coli) fprintf(stdout, ";");
          coli = 1;
          lc = getCSVValue(col);
          if (lc) {
            write_escape_cmake(stdout, lc);
          } else {
            fprintf(stdout, "\\x00");
          }
        }
      }
      fprintf(stdout, "\")\n");
      fprintf(stdout,
              "    csv_compare(\"${_BNN}\")\n"
              "  endif()\n");
    }
    fprintf(stdout,
            "endif()\n");
    if (csv) {
      freeCSV(csv);
    }
  }
  if (run == 0) {
    csvperrorf(argv[0], 0, 0, CSV_EL_ERROR, NULL, "No files given");
    ret = 1;
  }

  return ret;
}

