/* This is -*- c -*- source */
/* This is vim: set ft=c: source */

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>

#include "csv.h"
#include "geometry/bitarray.h"
#include "optparse.h"
#include "csvutil.h"
#include "os/asprintf.h"

/*!re2c
  re2c:define:YYCTYPE = "unsigned char";
  re2c:define:YYCURSOR = "cursor";
  re2c:define:YYMARKER = "marker";
  re2c:define:YYCTXMARKER = "ctx_marker";
  re2c:indent:string = "  ";
  re2c:yyfill:enable = 0;
*/

static int accept_decimal(int v, int digit, int errval)
{
  if (v < 0 || v == errval)
    return errval;
  if (v < INT_MAX / 10)
    return v * 10 + digit;
  if (v <= INT_MAX / 10) {
    v *= 10;
    if (v <= INT_MAX - digit)
      return v + digit;
  }
  return errval;
}

static int accept_decimalc(int v, char ch, int errval)
{
  char buf[2] = { ch, '\0' };
  const char *cursor = buf;
  /*!re2c
    re2c:indent:top = 1;

    *   { return errval; }
    '0' { return accept_decimal(v, 0, errval); }
    '1' { return accept_decimal(v, 1, errval); }
    '2' { return accept_decimal(v, 2, errval); }
    '3' { return accept_decimal(v, 3, errval); }
    '4' { return accept_decimal(v, 4, errval); }
    '5' { return accept_decimal(v, 5, errval); }
    '6' { return accept_decimal(v, 6, errval); }
    '7' { return accept_decimal(v, 7, errval); }
    '8' { return accept_decimal(v, 8, errval); }
    '9' { return accept_decimal(v, 9, errval); }
   */
  CSVUNREACHABLE();
  return errval;
}

/**
 * @brief Makes plural form of `noun` (in English)
 * @param buf pointer to store result
 * @param noun Noun to make singular (a/an -) or plural (-s)
 * @param num Number for `noun`
 * @param cap 0 to leave the first charactor in lower case. Otherwise
 *        capiterlize.
 * @return number of bytes written, or -1 if error (allocation failed).
 *
 * Genereted string must be freed by `free()`.
 *
 * Nouns which transform illegally (such as 'datum', 'phenomenon',
 * 'mouse', etc.) are not supported.
 *
 * If the leading charactor of noun is ASCII symbols or numerals (such
 * as '!', '$'), the usage of 'a' or 'an' is determined its reading.
 * (for example, '!' (exclamation mark) will be 'an !', ':' (colon)
 * will be 'a :', '0' (zero) will be 'a 0')
 *
 * Non-ASCII charactors are considered non-vowel.
 *
 * Examples:
 * - (&buf, "cake", 1, 1) // -> "A cake"
 * - (&buf, "cake", 1, 0) // -> "a cake"
 * - (&buf, "cake", 2, 1) // -> "2 cakes"
 * - (&buf, "cake", 2, 0) // -> "2 cakes"
 * - (&buf, "cake", 100, 0) // -> "100 cakes"
 * - (&buf, "apple", 1, 1) // -> "An apple"
 * - (&buf, "apple", 0, 1) // -> "No apples"
 * - (&buf, "  apple", 0, 0) // -> "no apples" (leading spaces will be removed)
 * - (&buf, "city", 10, 0)  // -> "10 cities"
 */
static int jupiter_make_nplural(char **buf, const char *noun, int num, int cap)
{
  int is_vowel = 0;
  const char *cursor;
  const char *token;
  const char *tail;
  const char *lim;
  const char *marker;
  const char *ctx_marker;
  ptrdiff_t ns;

  CSVASSERT(num >= 0);

  cursor = noun;
  for (;;) {
    token = cursor;
    /*!re2c
      re2c:indent:top = 2;
      *            { break; }
      [ \t\r\n]    { continue; }
    */
    CSVUNREACHABLE();
  }

  if (num != 1) {
    noun = token;
    cursor = noun;
    for (;;) {
      token = cursor;
      lim = cursor;
      /*!re2c
        re2c:indent:top = 3;
        *  { continue; }
        [ \t\r\n]*'\x00' { tail = "s"; break; }
        [BCDFGHJ-NP-TV-Zbcdfghj-np-tv-z]'y'/[ \t\r\n]*'\x00'
            { tail = "ies"; lim = cursor - 1; break; }
        [BCDFGHJ-NP-TV-Zbcdfghj-np-tv-z]'o'/[ \t\r\n]*'\x00'
            { tail = "es"; lim = cursor; break; }
        [sSxX]/[ \t\r\n]*'\x00' { tail = "es"; lim = cursor; break; }
        [sScC][hH]/[ \t\r\n]*'\x00' { tail = "es"; lim = cursor; break; }
        'f'/'e'?[ \t\r\n]*'\x00' { tail = "ves"; lim = cursor - 1; break; }
      */
    }
    ns = lim - noun;

    if (num == 0) {
      return jupiter_asprintf(buf, "%s %.*s%s",
                              cap ? "No" : "no", ns, noun, tail);
    } else {
      return jupiter_asprintf(buf, "%d %.*s%s",
                              num, ns, noun, tail);
    }
  } else {
    cursor = noun;
    for (;;) {
      /*!re2c
        re2c:indent:top = 3;

        *              { is_vowel = 0; break; }
        '\x00'         { is_vowel = 0; break; }
        ("["|"]"|"\\") { is_vowel = 0; break; }
        ["#$%(),./:;<=>?`^~{}|-] { is_vowel = 0; break; }
        [!@&_]       { is_vowel = 1; break; }
        [aiueoAIUEO] { is_vowel = 1; break; }
        [0-79]       { is_vowel = 0; break; }
        '8'          { is_vowel = 1; break; }
        [BCDFGHJ-NP-TV-Zbcdfghj-np-tv-z] { is_vowel = 0; break; }
      */
      CSVUNREACHABLE();
    }
    return jupiter_asprintf(buf, "%c%s %s",
                            cap ? 'A' : 'a',
                            is_vowel ? "n" : "", noun);
  }
  CSVUNREACHABLE();
}

/**
 * @brief Parses integer value
 * @param ret Pointer to the memory where should be stored the result
 * @param string Parsing string
 * @param argflg Command line flag name which is being parsed
 */
static int jupiter_parse_int(int *ret, const char *string, const char *argflg)
{
  const char *token;
  const char *cursor;

  unsigned int intv;
  unsigned int intm;
  int sint;
  int neg;

  CSVASSERT(ret);
  CSVASSERT(string);

  cursor = string;

  neg = 0;
  for (;;) {
    token = cursor;
    /*!re2c
      re2c:indent:top = 3;

      *         { cursor = token; break; }
      '\x00'    { goto empty; }
      [ \t\r\n] { continue; }
      '-'       { neg = 1; break; }
      '+'       { neg = 0; break; }
    */
    CSVUNREACHABLE();
  }

  for (;;) {
    token = cursor;
    /*!re2c
      re2c:indent:top = 3;

      *         { goto invalid_int; }
      '\x00'    { goto empty; }
      [0-9]     { cursor = token; break; }
      [ \t]     { continue; }
    */
  }

  string = token;
  intv = 0;
  for (;;) {
    token = cursor;
    /*!re2c
      re2c:indent:top = 3;

      *      { goto invalid_int; }
      '\x00' { break; }
      [0-9]  { goto decimal_token; }
    */
    CSVUNREACHABLE();

  decimal_token:
    intv = accept_decimalc(intv, *token, -1);
    if (intv < 0)
      goto int_overflow;
    sint = intv;
    if (sint < 0 || (unsigned int)sint != intv) goto int_overflow;
    continue;

  invalid_int:
    if (isprint(*token)) {
      csvperrorf("<command line>", 0, 0, CSV_EL_ERROR,
                 argflg, "Invalid integer charactor: '%c' of `%s`",
                 *token, string);
    } else {
      csvperrorf("<command line>", 0, 0, CSV_EL_ERROR,
                 argflg, "Invalid integer charactor: '\\x%02x'", *token);
    }
    return -1;

  int_overflow:
    csvperrorf("<command line>", 0, 0, CSV_EL_ERROR,
               argflg, "Too large integer value: %s", string);
    return -1;

  empty:
    csvperrorf("<command line>", 0, 0, CSV_EL_ERROR,
               argflg, "No decimal value found");
    return -1;
  }

  if (neg) {
    sint = -sint;
  }

  *ret = sint;
  return 0;
}

/**
 * @brief Parses integers on command line
 * @param argflg Command line flag name which is being parsed
 * @param argc size of argv
 * @param argv array of command line argument
 * @param num number of integers (must be constant)
 * @param ... pointer to int where store the result
 * @return number of arguments consumed on argv, 0 when error.
 *
 * argv must point the first element which will be parsed,
 * and argc must be number of available number of arguments.
 *
 * For example, if original argv is
 * ```
 *   argv[0]  =  "../jupiter/run";
 *   argv[1]  =  "-input";
 *   argv[2]  =  "params.txt";
 *   argv[3]  =  "-post";
 *   argv[4]  =  "2";
 *   argv[5]  =  "5";
 *   argv[6]  =  "-flags";
 *   argv[7]  =  "flags.txt";
 * ```
 * and argc == 8, argument of this function will be for "-post" argument
 * will be 4 for argc and &argv[4] for argv.
 * ```
 *   jupiter_parse_int_array("-post", 4, &argv[4], 2,
 *                           &opts->post_s, &opts->post_e);
 * ```
 */
static int jupiter_parse_int_array(const char *argflg, int argc, char **argv,
                                   int num, ...)
{
  int r;
  int *retp;
  int i;
  va_list ap;
  char *buf;
  const char *msg;

  r = 0;

  va_start(ap, num);
  for (i = 0; i < num && i < argc; ++i) {
    retp = va_arg(ap, int*);
    CSVASSERT(retp);
    r = jupiter_parse_int(retp, argv[i], argflg);
    if (r != 0) break;
  }
  va_end(ap);

  if (r == 0 && i != num) {
    r = jupiter_make_nplural(&buf, "integer value", num, 1);
    if (r >= 0) {
      msg = buf;
    } else {
      buf = NULL;
      msg = "Integer value(s)";
    }
    csvperrorf("<command line>", 0, 0, CSV_EL_ERROR,
               NULL, "%s required after `%s`", msg, argflg);
    free(buf);
    r = 1;
  }
  if (r != 0) {
    return 0;
  }
  return i;
}

/**
 * Examples:
 *
 * - `0`: print on rank 0, no print on others, by default levels
 *
 * - `all`: print on all ranks by default levels
 *
 * - `*`  : print on all ranks by defualt levels
 *
 * - `0:+all`:
 *   - print on rank 0 with all levels, not print on others
 *
 * - `0:-warn`
 *   - print on rank 0 by default levels but not warnings, no print on others
 *
 * - `0:-all+fatal,1:all`
 *   - print on rank 0 with fatals only, print on rank 1 with all levels,
 *     no print on others
 *
 * - `all:-all,0:+all-debug`
 *   - print on rank 0 without debugs, no print on others
 *     (explicitly specified, equivalent to default)
 *
 * - `2..5`
 *   - print on rank 2 to 5 by default levels.
 */
static csv_error jupiter_parse_print_rank(const char *arg,
                                          const char *flg, int *print_my_rank,
                                          jupiter_print_levels *levels)
{
  const char *t1, *t2;
  const char *cursor;
  const char *marker;
  csv_error_level lvl = CSV_EL_MAX;
  int ret;
  int srank;
  int erank;
  int print_rank = 0;
  int apply_rank = 0;
  int myrank = 0;
#ifdef JUPITER_MPI
  int inited = 0;
#endif
  const int rankerrval = -2;

#ifdef JUPITER_MPI
  MPI_Initialized(&inited);
  if (inited)
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
#endif

  cursor = arg;
  for (;;) {
    lvl = CSV_EL_MAX;
    srank = rankerrval;
    erank = rankerrval;
    ret = 1;
    /* parse start rank */
    for (;;) {
      t1 = cursor;
      /*!re2c
        re2c:indent:top = 3;

        pr_space = [ \t\r\n]*;
        pr_comma = ',';
        pr_colon = ':';
        pr_plus  = '+';
        pr_minus = '-';
        pr_range = '..';
        pr_all   = 'all' | '*';
        pr_end   = '\x00';

        *                { goto invalid_rank; }
        pr_end           { goto end; }
        pr_all pr_colon  { srank = -1; erank = -1; goto parse_levels; }
        pr_range         { srank = -1; break; }
        '0' pr_range     { srank = 0; break; }
        '0' pr_colon     { srank = 0; erank = 0; goto parse_levels; }
        '0' /pr_end      { srank = 0; erank = 0; goto parse_levels; }
        [1-9]            { cursor = t1; srank = 0; goto nums; }
      */
      CSVUNREACHABLE();

    nums:
      for (;;) {
        t2 = cursor;
        /*!re2c
          re2c:indent:top = 4;

          *        { goto invalid_rank; }
          pr_colon { erank = srank; goto parse_levels; }
          pr_end   { cursor = t2; break; }
          pr_range { break; }

          [0-9]    { goto digit_s; }
        */
        CSVUNREACHABLE();

      digit_s:
        srank = accept_decimalc(srank, *t2, -2);
        if (srank < 0)
          goto int_overflow;
        continue;
      }
      break;
    }
    /* parse end rank */
    for (;;) {
      t1 = cursor;
      /*!re2c
        re2c:indent:top = 3;

        *            { goto invalid_rank; }
        pr_end       { if (erank < -1) erank = srank; cursor = t1; break; }
        pr_colon     { if (erank < -1) erank = srank; break; }
        '0' pr_colon { erank = 0; break; }
        '0' /pr_end  { erank = 0; break; }
        [1-9]        { cursor = t1; erank = 0; goto nume; }
      */
      CSVUNREACHABLE();

    nume:
      for (;;) {
        t2 = cursor;
        /*!re2c
          re2c:indent:top = 4;

          *        { goto invalid_rank; }
          pr_colon { goto parse_levels; }
          pr_end   { cursor = t2; break; }

          [0-9]    { goto digit_e; }
        */
        CSVUNREACHABLE();

      digit_e:
        erank = accept_decimalc(erank, *t2, -2);
        if (erank < 0)
          goto int_overflow;
        continue;
      }
    }
  parse_levels:
    if (srank < -1 || erank < -1)
      goto invalid_rank;
    apply_rank = 0;
    if (srank <= -1 && myrank <= erank) {
      apply_rank = 1;
    } else if (erank <= -1 && myrank >= srank) {
      apply_rank = 1;
    } else if (srank <= myrank && myrank <= erank) {
      apply_rank = 1;
    }
    if (apply_rank)
      print_rank = 1;
    for (;;) {
      t1 = cursor;
      /*!re2c
        re2c:indent:top = 3;

        *        { goto invalid_level; }
        pr_end   { cursor = t1; break; }
        pr_comma { break; }
        pr_plus  /pr_end   { ret = 1; goto lvall; }
        pr_plus  /pr_comma { ret = 1; goto lvall; }
        pr_minus /pr_end   { ret = 0; goto lvall; }
        pr_minus /pr_comma { ret = 0; goto lvall; }
        pr_plus  pr_all    { ret = 1; goto lvall; }
        pr_minus pr_all    { ret = 0; goto lvall; }
        pr_plus  'debug'   { ret = 1; lvl = CSV_EL_DEBUG; goto lv1; }
        pr_minus 'debug'   { ret = 0; lvl = CSV_EL_DEBUG; goto lv1; }
        pr_plus  'info'    { ret = 1; lvl = CSV_EL_INFO; goto lv1; }
        pr_minus 'info'    { ret = 0; lvl = CSV_EL_INFO; goto lv1; }
        pr_plus  'warn'    { ret = 1; lvl = CSV_EL_WARN; goto lv1; }
        pr_minus 'warn'    { ret = 0; lvl = CSV_EL_WARN; goto lv1; }
        pr_plus  'error'   { ret = 1; lvl = CSV_EL_ERROR; goto lv1; }
        pr_minus 'error'   { ret = 0; lvl = CSV_EL_ERROR; goto lv1; }
        pr_plus  'fatal'   { ret = 1; lvl = CSV_EL_FATAL; goto lv1; }
        pr_minus 'fatal'   { ret = 0; lvl = CSV_EL_FATAL; goto lv1; }
      */
      CSVUNREACHABLE();

    lvall:
      if (!apply_rank)
        continue;

      geom_bitarray_element_setall(levels->levels, CSV_EL_MAX, ret);
      continue;

    lv1:
      if (!apply_rank)
        continue;

      geom_bitarray_element_set(levels->levels, lvl, ret);
      continue;
    }
  }
  CSVUNREACHABLE();

end:
  *print_my_rank = print_rank;
  return CSV_ERR_SUCC;

int_overflow:
  csvperrorf("<command line>", 0, 0, CSV_EL_ERROR, flg,
             "Rank number too large.");
  return CSV_ERR_CMDLINE_INVAL;

invalid_rank:
  csvperrorf("<command line>", 0, 0, CSV_EL_ERROR, flg,
             "Failed to parse print rank value or range, format: "
             "`<s>[..<e>]:<levels...>[,...]`");
  return CSV_ERR_CMDLINE_INVAL;

invalid_level:
  csvperrorf("<command line>", 0, 0, CSV_EL_ERROR, flg,
             "Failed to parse level names. It must be sequence of "
             "'[-+]all', '[-+]debug', '[-+]info', '[-+]warn', "
             "'[-+]error' or '[-+]fatal', or `[-+]` for all");
  return CSV_ERR_CMDLINE_INVAL;
}

static csv_error jupiter_parse_print_levels(const char *arg, const char *flg,
                                            jupiter_print_levels *levels)
{
  const char *t1, *t2;
  const char *cursor;
  const char *marker;
  csv_error_level lvl = CSV_EL_MAX;
  int ret;

  cursor = arg;
  for (;;) {
    lvl = CSV_EL_MAX;
    ret = 1;
    t1 = cursor;
    for (;;) {
      /*!re2c
        re2c:indent:top = 3;

        pl_space = [ \t\r\n]*;
        pl_comma = ',';
        pl_end = '\x00';

        *        { cursor = t1; break; }
        pl_space { t1 = cursor; continue; }
        pl_end   { return CSV_ERR_SUCC; }
        '-'      { ret = 0; break; }
        '+'      { ret = 1; break; }
      */
    }
    t2 = cursor;
    for (;;) {
      /*!re2c
        re2c:indent:top = 3;

        *        { goto invalid; }
        pl_end   { cursor = t2; break; }
        pl_comma { if (t1 == t2) goto invalid; break; }
        'debug' pl_space / pl_end { lvl = CSV_EL_DEBUG; break; }
        'debug' pl_space pl_comma { lvl = CSV_EL_DEBUG; break; }
        'info'  pl_space / pl_end { lvl = CSV_EL_INFO; break; }
        'info'  pl_space pl_comma { lvl = CSV_EL_INFO; break; }
        'warn'  pl_space / pl_end { lvl = CSV_EL_WARN; break; }
        'warn'  pl_space pl_comma { lvl = CSV_EL_WARN; break; }
        'error' pl_space / pl_end { lvl = CSV_EL_ERROR; break; }
        'error' pl_space pl_comma { lvl = CSV_EL_ERROR; break; }
        'fatal' pl_space / pl_end { lvl = CSV_EL_FATAL; break; }
        'fatal' pl_space pl_comma { lvl = CSV_EL_FATAL; break; }
      */
      CSVUNREACHABLE();
    }

    if (lvl == CSV_EL_MAX) {
      geom_bitarray_element_setall(levels->levels, CSV_EL_MAX, ret);
    } else {
      geom_bitarray_element_set(levels->levels, lvl, ret);
    }
  }

  return CSV_ERR_SUCC;

invalid:
  csvperrorf("<command line>", 0, 0, CSV_EL_ERROR, flg,
             "Failed to parse level names. It must be comma separated "
             "'debug', 'info', 'warn', 'error' or 'fatal'");
  return CSV_ERR_CMDLINE_INVAL;
}

static csv_error jupiter_optparse_base(jupiter_options *opts, int *argc,
                                       char ***argv,
                                       jupiter_file_inputs_flags *f,
                                       int accept_others)
{
  const char *token;
  const char *cursor;
  const char *marker;
  int i;
  int r;
  int oargc;
  int nargc;
  char **oargv;
  char **nargv;
  csv_error ret;
  const char *flg;
  const char **floc;
  enum jupiter_file_inputs fi;
  int print_rank_level = 0;

  CSVASSERT(opts);
  CSVASSERT(argc);
  CSVASSERT(argv);

  oargc = *argc;
  oargv = *argv;

  nargc = 0;
  nargv = (char **)calloc(sizeof(char *), oargc);
  if (nargv) {
    nargv[0] = oargv[0];
    nargc++;
  }

  for (i = 1; i < oargc; ++i) {
    cursor = oargv[i];

    for (;;) {
      token = cursor;

      /*!re2c
        re2c:indent:top = 3;

        end = "\x00";

        *    { break; }
        "--" { goto double_hyphen; }
        "-"  { goto hyphen; }
      */
      CSVUNREACHABLE();
    }

    /* Collect non option parameters */
    if (nargv) {
      nargv[nargc] = oargv[i];
      nargc++;
    }
    continue; /* goto next argument */

  double_hyphen:
    for (;;) {
      token = cursor;
      /*!re2c
        re2c:indent:top = 2;

        *   { cursor = token; break; }
        end { goto stop_parse; }
      */
      CSVUNREACHABLE();
    }

  hyphen:
    for (;;) {
      token = cursor;
      /*!re2c
        re2c:indent:top = 2;

        *                  { goto invalid; }
        "flags"/end        { goto flags_file; }
        "input"/end        { goto input_file; }
        "plist"/end        { goto plist_file; }
        "geom"/end         { goto geom_file; }
        "control"/end      { goto control_file; }

        "restart_job"/end  { goto restart_job; }

        "restart"/end      { goto restart; }
        "post"/end         { goto post; }

        "print_rank"/end   { goto print_rank; }
        "print_level"/end  { goto print_level; }
      */
      CSVUNREACHABLE();
    }
    continue; /* goto next argument */

  flags_file:
    fi = JUPITER_INPUT_FILE_FLAGS;
    floc = &opts->flags_file;
    goto set_file;

  input_file:
    fi = JUPITER_INPUT_FILE_PARAMS;
    floc = &opts->param_file;
    goto set_file;

  plist_file:
    fi = JUPITER_INPUT_FILE_PLIST;
    floc = &opts->plist_file;
    goto set_file;

  geom_file:
    fi = JUPITER_INPUT_FILE_GEOM;
    floc = &opts->geom_file;
    goto set_file;

  control_file:
    fi = JUPITER_INPUT_FILE_CONTROL;
    floc = &opts->control_file;
    goto set_file;

  set_file:
    if (!geom_bitarray_element_get(f->f, fi))
      goto invalid;

    if (i + 1 >= oargc) {
      csvperrorf("<command line>", 0, 0, CSV_EL_ERROR,
                 NULL, "A file name required after `%s`", oargv[i]);
      ret = CSV_ERR_CMDLINE_INVAL;
      goto clean;
    }
    ++i;
    *floc = oargv[i];
    continue;

  restart_job:
    if (!accept_others)
      goto invalid;

    opts->restart_job = 1;
    continue;

  restart:
    if (!accept_others)
      goto invalid;

    {
      flg = oargv[i];

      ++i;
      r = jupiter_parse_int_array(flg, oargc - i, &oargv[i],
                                  1, &opts->restart);
      if (r == 0) return -1;
      if (opts->restart <= 0) {
        csvperrorf("<command line>", 0, 0, CSV_EL_ERROR, flg,
                   "Must be positive value: %d", opts->restart);
        opts->restart = -1;
        ret = CSV_ERR_CMDLINE_RANGE;
        goto clean;
      }
      i += r - 1;
    }
    continue;

  post:
    if (!accept_others)
      goto invalid;

    {
      flg = oargv[i];

      ++i;
      r = jupiter_parse_int_array(flg, oargc - i, &oargv[i],
                                  2, &opts->post_s, &opts->post_e);
      if (r == 0) return -1;
      if (opts->post_s < 0) {
        csvperrorf("<command line>", 0, 0, CSV_EL_ERROR, flg,
                   "Must be positive value or zero: %d", opts->post_s);
        opts->post_s = -1;
        opts->post_e = -1;
        ret = CSV_ERR_CMDLINE_RANGE;
        goto clean;
      }
      if (opts->post_e < opts->post_s) {
        csvperrorf("<command line>", 0, 0, CSV_EL_ERROR, flg,
                   "Last point must be greater than start: [%d, %d]",
                   opts->post_s, opts->post_e);
        opts->post_s = -1;
        opts->post_e = -1;
        ret = CSV_ERR_CMDLINE_RANGE;
        goto clean;
      }
      i += r - 1;
    }
    continue;

  print_rank:
    /*
     * Accepts only if MPI enabled.
     */
#ifdef JUPITER_MPI
    flg = oargv[i];
    ++i;
    if (i >= oargc) {
      csvperrorf("<command line>", 0, 0, CSV_EL_ERROR, flg,
                 "Missing print_rank specifier");
      ret = -1;
      goto clean;
    }
    ret = jupiter_parse_print_rank(oargv[i], flg, &opts->print_my_rank,
                                   &opts->print_levels);
    if (ret != CSV_ERR_SUCC)
      goto clean;

    print_rank_level = 1;
    continue;
#endif
    goto invalid;

  print_level:
    flg = oargv[i];
    ++i;
    if (i >= oargc) {
      csvperrorf("<command line>", 0, 0, CSV_EL_ERROR, flg,
                 "Missing print_level specifier");
      ret = -1;
      goto clean;
    }

    ret = jupiter_parse_print_levels(oargv[i], flg, &opts->print_levels);
    if (ret != CSV_ERR_SUCC)
      goto clean;

    print_rank_level = 1;
    continue;
  }

 stop_parse:
  if (!print_rank_level) {
    jupiter_parse_print_rank(opts->print_rank, "<defaults>",
                             &opts->print_my_rank, &opts->print_levels);
  }
  set_jupiter_print_levels(opts->print_levels);
  set_jupiter_print_rank(opts->print_my_rank);

  if (nargv) {
    for (++i; i < oargc; ++i) {
      nargv[nargc] = oargv[i];
      ++nargc;
    }
    for (i = 0; i < nargc; ++i) {
      oargv[i] = nargv[i];
    }
    free(nargv);
    *argc = nargc;
  } else {
    return CSV_ERR_NOMEM;
  }
  return CSV_ERR_SUCC;

 invalid:
  csvperrorf("<command line>", 0, 0, CSV_EL_ERROR,
             NULL, "Invalid argument: %s", oargv[i]);
  ret = CSV_ERR_CMDLINE_INVAL;
  goto clean;

 clean:
  free(nargv);
  return ret;
 }

csv_error jupiter_optparse(jupiter_options *opts, int *argc, char ***argv)
{
  jupiter_file_inputs_flags f = jupiter_file_inputs_flags_all();
  return jupiter_optparse_base(opts, argc, argv, &f, 1);
}

csv_error jupiter_optparse_files(jupiter_options *opts, int *argc, char ***argv,
                                 jupiter_file_inputs_flags *accepts)
{
  return jupiter_optparse_base(opts, argc, argv, accepts, 0);
}
