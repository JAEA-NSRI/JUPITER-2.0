/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */
/**
 * @addtogroup cmake_doxygen_filter
 * @{
 * @file cmake-doxygen-filter.c
 * @brief Filters CMake source to Doxygen input.
 *
 * Filters CMake source to Doxygen input by formatting like Javascript.
 * (i.e., use Javascript parser for CMake files in your Doxyfile)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>

#define DEFAULT_PROG_NAME "cmake-doxygen-filter"

#include "doxygen-filter.h"

#include <jupiter/re2c_lparser/re2c_lparser.h>

static int cmake_doxygen_filter(FILE *input, const char *ifname,
                                FILE *output, const char *ofname);

/**
 * @brief Main function of CMake to Doxygen filter
 */
int main(int argc, char **argv)
{
  int iarg;

  if (argc > 0) {
    set_prog_name(argv[0]);
  } else {
    set_prog_name("");
  }

  if (argc <= 1) {
    print_error("No file name given");
    return 1;
  }

  for(iarg = 1; iarg < argc; ++iarg) {
    FILE *fp;
    int ret;
    errno = 0;
    fp = fopen(argv[iarg], "rb");
    if (!fp) {
      if (errno != 0) {
        print_error("cannot open %s: %s", argv[iarg], strerror(errno));
      } else {
        print_error("cannot open %s", argv[iarg]);
      }
      return 1;
    }
    ret = cmake_doxygen_filter(fp, argv[iarg], stdout, "<stdout>");
    fclose(fp);
    if (ret) return ret;
  }
  return 0;
}

/*!re2c
  re2c:define:YYCTYPE = "unsigned char";
  re2c:define:YYCURSOR = "p->cur";
  re2c:define:YYMARKER = "p->mrk";
  re2c:define:YYCTXMARKER = "p->ctxmrk";
  re2c:define:YYLIMIT = "p->lim";
  re2c:indent:string = "  ";
  re2c:flags:8 = 1; // parse input as UTF-8.

  nl     = ('\r\n'|'\r'|'\n');
  ctrl   = [\u0000-\u0008\u000b\u000c\u000e-\u001f];
  nonspc = [\u0021-\uffff\U00010000-\U0010ffff];
  spc    = [\t ]; // tab and space
  print  = (spc|nonspc);
*/

static int cmake_doxygen_filter(FILE *input, const char *ifname,
                                FILE *output, const char *ofname)
{
  int ret;
  int cont_of_doxcomment;
  int indent;
  int i;
  struct re2c_lparser pp;
  struct re2c_lparser *p = &pp;

  re2c_lparser_init(p);
  if (re2c_lparser_fill(p, input, ifname, 0, NULL) < 0) {
    return 1;
  }

  ret = 0;
  indent = 0;
  cont_of_doxcomment = 0;

  while (!re2c_lparser_eof(p)) {
    re2c_lparser_start_token(p);
    /* debug_parser(p); */

    /*!re2c
      re2c:yyfill:enable = 1;
      re2c:define:YYFILL:naked = 1;
      re2c:define:YYFILL = "{ if (re2c_lparser_fill(p, input, ifname, @@, NULL) < 0) goto error; }";
      re2c:indent:top = 2;

      identifier = [a-zA-Z_][a-zA-Z0-9_]*;

      * { goto invalid_utf8; }
      '#'            { re2c_lparser_loc_upd_utf8(p); goto comment; }
      '##'/[^#]      { re2c_lparser_loc_upd_utf8(p); goto doxycomment; }
      nl             { re2c_lparser_loc_upd_utf8(p); goto newline; }
      spc            { re2c_lparser_loc_upd_utf8(p); continue; }
      (ctrl|nonspc)  { p->cur = p->tok; cont_of_doxcomment = 0; goto command; }
    */

  newline:
    re2c_lparser_copy_token(p, output);
    cont_of_doxcomment = 0;
    continue;

  comment:
    if (cont_of_doxcomment) {
      for(i = indent; i > 0; i--) fprintf(output, "  ");
#ifdef PYTHON_OUTPUT
      fprintf(output, "#");
#else
      fprintf(output, "///");
#endif
      goto doxcomment_cont;
    }
    while(!re2c_lparser_eof(p)) {
      re2c_lparser_start_token(p);
      /*!re2c
        re2c:indent:top = 3;

        *  { goto invalid_utf8; }
        nl {
          re2c_lparser_loc_upd_utf8(p);
          re2c_lparser_copy_token(p, output);
          break;
        }
        (ctrl|print) {
          re2c_lparser_loc_upd_utf8(p);
          continue;
        }
      */
    }
    continue;

  doxycomment:
    cont_of_doxcomment = 1;
    for(i = indent; i > 0; i--) fprintf(output, "  ");
#ifdef PYTHON_OUTPUT
    fprintf(output, "##");
#else
    fprintf(output, "///");
#endif
  doxcomment_cont:
    while (!re2c_lparser_eof(p)) {
      re2c_lparser_start_token(p);
      /*!re2c
        re2c:indent:top = 3;

        *  { goto invalid_utf8; }
        nl {
          re2c_lparser_loc_upd_utf8(p);
          re2c_lparser_copy_token(p, output);
          break;
        }
        (ctrl|print) {
          re2c_lparser_loc_upd_utf8(p);
          re2c_lparser_copy_token(p, output);
          continue;
        }
      */
    }
    continue;

  command:
    {
      int cnt_paren;
      int narg;
      const char *cur_save;
      enum command_name {
        others,
        option, dep_option, function, endfunction, cmake_foreach, endforeach,
        cmake_if, endif, cmake_while, endwhile,
        set, macro, endmacro,
      } command;

      p->lexeme = p->tok;
      do {
        re2c_lparser_start_token(p);
        /*!re2c
          re2c:indent:top = 4;

          *        { goto invalid_utf8; }
          identifier/(spc|nl|"("|ctrl)
                   { re2c_lparser_loc_upd_utf8(p); p->lexeme = p->tok; break; }
          nonspc   { p->cur = p->tok; goto need_ident; }
        */
      } while(0);
      if (re2c_lparser_eof(p)) goto req_paren;
      cur_save = p->cur;
      p->cur = p->lexeme;
      for(;;) {
        /*!re2c
          re2c:indent:top = 4;
          re2c:yyfill:enable = 0;

          *                        { command = others; break; }
          'option'                 { command = option; break; }
          'cmake_dependent_option' { command = dep_option; break; }
          'if'                     { command = cmake_if; break; }
          'endif'                  { command = endif; break; }
          'while'                  { command = cmake_while; break; }
          'endwhile'               { command = endwhile; break; }
          'foreach'                { command = cmake_foreach; break; }
          'endforeach'             { command = endforeach; break; }
          'set'                    { command = set; break; }
          'function'               { command = function; break; }
          'endfunction'            { command = endfunction; break; }
          'macro'                  { command = macro; break; }
          'endmacro'               { command = endmacro; break; }
        */
        /*!re2c
          re2c:yyfill:enable = 1;
        */
      }
      switch(command) {
      case cmake_if:
      case cmake_while:
      case cmake_foreach:
      case function:
      case macro:
        indent += 1;
        break;
      case endif:
      case endwhile:
      case endforeach:
      case endfunction:
      case endmacro:
        indent -= 1;
        if (indent < 0) indent = 0;
        break;
      default:
        /* nop */
        break;
      }

      p->cur = cur_save;
      p->lexeme = NULL;
      while(!re2c_lparser_eof(p)) {
        re2c_lparser_start_token(p);
        /*!re2c
          re2c:indent:top = 4;

          *    { goto invalid_utf8; }
          spc  { re2c_lparser_loc_upd_utf8(p); continue; }
          "("  { re2c_lparser_loc_upd_utf8(p); break; }
          (nl|ctrl|print) { goto req_paren; }
        */
      }
      if (re2c_lparser_eof(p)) goto req_paren;
      narg = 1;
      for(cnt_paren = 1; cnt_paren > 0; ) {
        const char *lastp;
        int quote_used;
        quote_used = 0;

        for(;;) {
          re2c_lparser_start_token(p);
          /*!re2c
            re2c:indent:top = 5;

            *   { break; }
            (nl|spc) { re2c_lparser_loc_upd_utf8(p); continue; }
          */
        }
        p->cur = p->tok;
        p->line = p->tokline;
        p->col = p->tokcol;
        p->lexeme = p->cur;

        lastp = NULL;
        while(!re2c_lparser_eof(p)) {
          re2c_lparser_start_token(p);
          /*!re2c
            re2c:indent:top = 5;

            *     { goto invalid_utf8; }
            '"'   { re2c_lparser_loc_upd_utf8(p); goto arg_quote; }
            '('   { re2c_lparser_loc_upd_utf8(p); goto open_paren; }
            ')'   { re2c_lparser_loc_upd_utf8(p); goto close_paren; }
            (ctrl|nl|spc)  { p->cur = p->tok; lastp = p->cur; break; }
            nonspc { re2c_lparser_loc_upd_utf8(p); continue; }
          */

        arg_quote:
          quote_used = 1;
          if (p->tok > p->lexeme) {
          in_middle_quote:
            print_error("%s(%ld,%ld): This program cannot handle in-middle quotes correctly",
                        ifname, p->tokline, p->tokcol);
            continue;
          } else {
            p->lexeme = p->cur;
          }
          while (!re2c_lparser_eof(p)) {
            re2c_lparser_start_token(p);
            /*!re2c
              re2c:indent:top = 6;

              *   { goto invalid_utf8; }
              '\\"' { re2c_lparser_loc_upd_utf8(p); continue; }
              '"'   { re2c_lparser_loc_upd_utf8(p); break; }
              (ctrl|nl|print) { re2c_lparser_loc_upd_utf8(p); continue; }
            */
          }
          if (re2c_lparser_eof(p)) goto unexp_eof;
          lastp = p->tok;
          re2c_lparser_start_token(p);
          /*!re2c
            re2c:indent:top = 5;

            *    { goto invalid_utf8; }
            (ctrl|nl|spc)   { p->cur = p->tok; break; }
            [()]            { p->cur = p->tok; continue; }
            nonspc          { p->cur = p->tok; goto in_middle_quote; }
          */
          assert(0 && "Unreachable reached");

          /* parentheses creates one arguement independently */
        open_paren:
          cnt_paren += 1;
          if (!lastp) lastp = p->tok;
          break;

        close_paren:
          cnt_paren -= 1;
          if (!lastp) lastp = p->tok;
          break;
        }
        if (re2c_lparser_eof(p)) {
          if (cnt_paren > 0) goto unexp_eof;
        }

        /* no argument */
        if (!quote_used) {
          if (!lastp || lastp <= p->lexeme) continue;
        }

        switch(command) {
        case option:
        case dep_option:
          switch(narg) {
          case 1:
            for (i = indent; i > 0; --i) {
              fprintf(output, "  ");
            }
            fprintf(output, "%.*s", (int)(lastp - p->lexeme), p->lexeme);
            break;
          case 3:
            fprintf(output, " = \"%.*s\"", (int)(lastp - p->lexeme), p->lexeme);
            break;
          }
          break;
        case function:
        case macro:
          if (narg == 1) {
            for (i = indent; i > 0; --i) {
              fprintf(output, "  ");
            }
#ifdef PYTHON_OUTPUT
            fprintf(output, "def ");
#else
#if 0
            if (command == macro) {
              fprintf(output, "cmake_macro ");
            } else {
              fprintf(output, "cmake_function ");
            }
#else
            if (command == macro) {
              fprintf(output, "macro ");
            } else {
              fprintf(output, "function ");
            }
#endif
#endif
            fprintf(output, "%.*s(", (int)(lastp - p->lexeme), p->lexeme);
          } else {
            if (narg > 2) {
              fprintf(output, ", ");
            }
#ifndef PYTHON_OUTPUT
            //fprintf(output, "cmake_value ");
#endif
            fprintf(output, "%.*s", (int)(lastp - p->lexeme), p->lexeme);
          }
          break;
        default:
          /* nop */
          break;
        }

        narg++;
      }
      if (command == function || command == macro) {
        fprintf(output, ")");
#ifdef PYTHON_OUTPUT
        fprintf(output, ":");
#else
#if 1
        fprintf(output, " {}");
#endif
#endif
      }
      if (command == option || command == dep_option) {
#ifndef PYTHON_OUTPUT
        fprintf(output, ";");
#endif
      }
    }
    continue;
  }

 clean:
  free(p->buf);
  return ret;

 req_paren:
  if (!re2c_lparser_eof(p)) {
    p->cur = p->tok;
    p->line = p->tokline;
    p->col  = p->tokcol;
    for (;;) {
      re2c_lparser_start_token(p);
      /*!re2c
        re2c:indent:top = 2;

        *  {
          print_error("%s(%ld,%ld): Requires '(', but found '%.*s'",
          ifname, p->tokline, p->tokcol, (int)(p->cur - p->tok), p->tok);
          break;
        }
        nl {
          print_error("%s(%ld,%ld): Requires '(', but found newline",
          ifname, p->tokline, p->tokcol);
          break;
        }
      */
    }
  } else {
    print_error("%s(%ld,%ld): Requires '(', but reached EOF",
                ifname, p->tokline, p->tokcol);
  }
  goto error;

 unexp_eof:
  print_error("%s(%ld,%ld): Unexpected EOF reached",
              ifname, p->tokline, p->tokcol);
  goto error;

 need_ident:
  do {
    /*!re2c
      re2c:indent:top = 2;

      *       { break; }
      nonspc+ { break; } // increment cur to nonspace chars.
    */
  } while(0);
  print_error("%s(%ld,%ld): Requires identifier, but found '%.*s'",
              ifname, p->tokline, p->tokcol, (int)(p->cur - p->tok), p->tok);
  goto error;

 invalid_utf8:
  print_error("%s(%ld,%ld): Invalid utf-8 sequence",
              ifname, p->tokline, p->tokcol);
  goto error;

 error:
  if (p->readerr) {
    print_error("%s(%ld,%ld): Read error: %s",
                ifname, p->tokline, p->tokcol, strerror(p->readerr));
  }
  ret = 1;
  goto clean;
}

/** @} */
