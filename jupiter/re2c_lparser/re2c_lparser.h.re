/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */
/**
 * @file re2c_large.h
 * @brief Arbitrary large input processing support functions for re2c.
 */

#ifndef JUPITER_RE2C_LPARSER_H
#define JUPITER_RE2C_LPARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parser struct
 *
 * `line` and `col` will not updated until re2c_lparser_loc_upd_utf8()
 * is called.
 */
struct re2c_lparser
{
  char *buf;          ///< Buffer origin
  const char *cur;    ///< Cursor, `YYCURSOR`
  const char *mrk;    ///< Marker, `YYMARKER`
  const char *tok;    ///< Token pointer
  const char *ctxmrk; ///< Context marker, `YYCTXMARKER`
  const char *lim;    ///< Limit, `YYLIMIT`
  const char *lexeme; ///< Further point which should not be freed
  const char *eof;    ///< Location of EOF.
  long line;          ///< Line number at Cursor
  long col;           ///< Column number at Cursor
  long tokline;       ///< Line number at Token
  long tokcol;        ///< Column number at Token
  ptrdiff_t bufsz;    ///< Current buffer size.
  int readerr;        ///< Flag for read error occured. (errno value or -1 if it's 0)
};

/**
 * @brief Initialize parser struct
 * @param p parser struct to be inited.
 *
 * To reset parser, do re2c_lparser_clean() first.
 */
static inline void
re2c_lparser_init(struct re2c_lparser *p)
{
  p->buf = NULL;
  p->cur = NULL;
  p->mrk = NULL;
  p->tok = NULL;
  p->ctxmrk = NULL;
  p->lim = NULL;
  p->lexeme = NULL;
  p->eof = NULL;
  p->line = 0;
  p->col = 0;
  p->tokline = 0;
  p->tokcol = 0;
  p->bufsz = 0;
  p->readerr = 0;
}

/**
 * @brief Fill data from stream
 * @param p perser struct
 * @param input Input stream to read from
 * @param ifn Filename of @p input (currently unused)
 * @param n Number of charactors required at least.
 * @param offs Sets the address offset to be used for moving
 *             extra pointers that not handled by struct ::re2c_lparser.
 *
 * This function do these operations:
 *
 *   1. Discards the data before @p p->lexeme
 *
 *     - We do not guarantee that @p p->lexeme becomes beginning of
 *       the buffer, because @p p->mrk or @p p->ctxmrk may point
 *       before the @p p->lexeme, which is used by re2c internal.
 *
 *     - @p p->buf is always beginning of the buffer, if you really
 *       need such information, but it is usually useless, and it may
 *       move to another location. (see 2.)
 *
 *   2. Allocate and/or move buffer if necessary
 *
 *     - If sum of requested size and remaining data size is less than
 *       the size of current available buffer, resize buffer and move
 *       if necessary by using realloc() standard library function.
 *
 *     - If negative value or 0 given for @p n, 1024 bytes will be
 *       reserved and read. You should give explicitly smaller value
 *       (such as 1 or 2) to debugging. Allocating 1024 bytes will
 *       avoid most reallocations.
 *
 *   3. Read the data from @p input stream
 *
 *     - If @p input reaches the EOF, @p p->eof will set to that
 *       location. If @p p->eof is set already, this function does not
 *       read any more.
 *
 *     - If an error occured while reading, @p p->readerr will be set.
 *       The value will be the `errno` value set by fread() function, or
 *       -1 if `errno` is not set.
 *
 * The reallcation will performed only if the requeted size is larger
 * than current size.
 */
static inline int
re2c_lparser_fill(struct re2c_lparser *p, FILE *input, const char *ifn,
                  int n, ptrdiff_t *offs)
{
  char *nbuf;
  ptrdiff_t sz;
  ptrdiff_t szret;
  ptrdiff_t base;
  ptrdiff_t szread;
  ptrdiff_t szmv;
  const char *lexeme;
  char *lastp;
  char *buflim;

  if (n <= 0) {
    n = 1024;
  }
  if (p->buf) {
    lexeme = p->lexeme;
    if ((!lexeme || p->tok    < lexeme) && p->tok)    lexeme = p->tok;
    if ((!lexeme || p->ctxmrk < lexeme) && p->ctxmrk) lexeme = p->ctxmrk;
    if ((!lexeme || p->mrk    < lexeme) && p->mrk)    lexeme = p->mrk;
    if ((!lexeme || p->cur    < lexeme) && p->cur)    lexeme = p->cur;
    if (!lexeme) lexeme = p->buf;
    szmv = 0;
    if (p->lim) szmv = p->lim - lexeme;
    sz = szmv + n + 1; /* 1 for NUL. */
    if (sz > p->bufsz) {
      if (lexeme > p->buf) {
        nbuf = (char *)malloc(sizeof(char) * sz);
      } else {
        nbuf = (char *)realloc(p->buf, sizeof(char) * sz);
      }
      if (!nbuf) {
#ifdef ENOMEM
        p->readerr = ENOMEM;
#else
        p->readerr = -1;
#endif
        return -1;
      }
    } else {
      /* we have enough storage to keep data */
      nbuf = p->buf;
      sz = p->bufsz;
    }
    if (lexeme > p->buf) {
      if (szmv > 0) {
        memmove(nbuf, lexeme, szmv);
      }
      if (nbuf != p->buf) {
        /* allocated by malloc */
        free(p->buf);
      }
    }
    lastp = nbuf + (p->lim - lexeme);
    base = szmv;
  } else {
    lexeme = NULL;
    sz = n;
    nbuf = (char *)malloc(sizeof(char) * sz);
    if (!nbuf) {
#ifdef ENOMEM
      p->readerr = ENOMEM;
#else
      p->readerr = -1;
#endif
      return -1;
    }
    lastp = nbuf;
    base = 0;
  }
  buflim = nbuf + sz;
  szret = 0;
  if (!p->eof) {
    szread = sz - base - 1;
    errno = 0;
    szret = fread(nbuf + base, sizeof(char), szread, input);
    lastp = nbuf + base + szret;
    if (feof(input)) {
      /* re2c may read beyond the EOF */
      p->eof = lastp;
      p->lim = buflim;
    } else if (ferror(input)) {
      p->readerr = (errno != 0) ? errno : -1;
      p->eof = lastp;
      p->lim = buflim;
    } else {
      p->lim = lastp;
    }
  } else {
    p->lim = buflim;
  }
  if (offs) *offs = nbuf - lexeme;
  if (lexeme) {
    p->cur = nbuf + (p->cur - lexeme);
    if (p->tok)    p->tok = nbuf + (p->tok - lexeme);
    if (p->mrk)    p->mrk = nbuf + (p->mrk - lexeme);
    if (p->ctxmrk) p->ctxmrk = nbuf + (p->ctxmrk - lexeme);
    if (p->lexeme) p->lexeme = nbuf + (p->lexeme - lexeme);
    if (p->eof)    p->eof = lastp;
  } else {
    p->cur = nbuf;
    p->tok = nbuf;
    p->mrk = NULL;
    p->ctxmrk = NULL;
    p->lexeme = NULL;
    p->line = 1;
    p->col = 1;
    p->tokline = 1;
    p->tokcol = 1;
  }
  p->buf = nbuf;
  p->bufsz = sz;

  sz = buflim - lastp;
  memset(lastp, 0, sz);

  return 0;
}

/**
 * @brief Test the stream is reached to the EOF
 * @param p Parser struct to test
 * @return non-0 if stream and cursor reached the EOF, 0 otherwise.
 */
static inline int
re2c_lparser_eof(struct re2c_lparser *p)
{
  return (p->eof && p->cur >= p->eof);
}

/**
 * @brief Save the token position
 * @param p Parser struct to set
 *
 * Save current position is start position of the token.
 *
 * If you want to count up the location of the text by
 * re2c_lparser_loc_upd_utf8(), you need to call this function. It
 * counts the charators from @p p->tok.
 *
 * Otherwise this function is not required because current version (1.0
 * or later) of re2c does not require this feature (because they
 * implement regex 'capture').
 */
static inline
void re2c_lparser_start_token(struct re2c_lparser *p)
{
  p->tok = p->cur;
  p->mrk = NULL;
  p->ctxmrk = NULL;
  p->tokline = p->line;
  p->tokcol = p->col;
}

/**
 * @brief Update location position (by UTF-8 enconding)
 * @param px Parser struct inclement
 *
 * Counts the number of charactors in UTF-8 encoding, between @p
 * px->tok and @p px->cur and adds it to @p p->col. If token contains
 * a newline (all of "\r\n", "\r" and "\n" are counted as 1 new line),
 * incrments @p p->line and reset @p p->col to 1.
 *
 * The code that parses UTF-8 is generated by re2c. This function does
 * not implement actual UTF-8 parsing code.
 */
static inline
void re2c_lparser_loc_upd_utf8(struct re2c_lparser *px)
{
  const char *cur;
  const char *mrk;

  cur = px->tok;
  while (cur < px->cur) {
    /*!re2c
      re2c:define:YYCTYPE = "unsigned char";
      re2c:define:YYCURSOR = "cur";
      re2c:define:YYMARKER = "mrk";
      re2c:yyfill:enable = 0;
      re2c:indent:string = "  ";
      re2c:indent:top = 2;
      re2c:flags:8 = 1;

      nl     = ('\r\n'|'\r'|'\n');
      ctrl   = [\u0000-\u0008\u000b\u000c\u000e-\u001f];
      nonspc = [\u0021-\uffff\U00010000-\U0010ffff];
      spc    = [\t ]; // tab and space
      print  = (spc|nonspc);

      *  { break; }  // Invalid UTF-8 sequence.
      nl { px->line++; px->col = 1; continue; }
      (ctrl|print) { px->col++; continue; }
     */
  }
}

/**
 * @brief Copies the token to another stream
 * @param p Parser to copy from
 * @param output Stream to output
 *
 * The data between @p p->tok and @p p->cur will be copied to @p
 * output.
 */
static inline
void re2c_lparser_copy_token(struct re2c_lparser *p, FILE *output)
{
  fwrite(p->tok, sizeof(char), p->cur - p->tok, output);
}

/**
 * @brief Clean up the allocted buffer by ::re2c_lparser struct
 * @param p Parser to clean up
 */
static inline
void re2c_lparser_clean(struct re2c_lparser *p)
{
  free(p->buf);
}

#ifdef __cplusplus
}
#endif

#endif
