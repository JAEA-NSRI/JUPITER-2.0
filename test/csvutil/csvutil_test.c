#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <jupiter/csvutil.h>
#include <jupiter/csvtmpl_format.h>

int format_tester(void);
int make_glob_test(void);
int match_tester(void);

int main(int argc, char **argv)
{
  int r;
  r  = format_tester();
  r += make_glob_test();
  r += match_tester();
  if (r > 0) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}


/*
 * fmt: format to test, val: value to embed, exp: expected result
 * (NULL for expected to fail)
 */
static int format_test(const char *fmt, const char *f, int val,
                       const char *exp)
{
  char *t; const char *e = exp;
  int x;

  t = NULL;
  x = 1;
  format_integers(&t, fmt, f, val);
  if (t) {
    if (e) x = strcmp(t, e);
    fprintf(stderr, "\"%s\" -> \"%s\" --- %s (key: \"%s\")\n", fmt, t,
            (x == 0) ? "ok" : "ng", f);
    free(t);
  } else {
    if (e) {
      fprintf(stderr, "\"%s\" --- failed (key: \"%s\")\n", fmt, f);
    } else {
      fprintf(stderr, "\"%s\" --- ok (exfail) (key: \"%s\")\n", fmt, f);
      x = 0;
    }
  }
  return (x != 0);
}

static int format_test2(const char *fmt, const char *f,
                        const char *exp, ...) {
  va_list ap;
  char *t; const char *e = exp;
  int x;

  x = 1;
  t = NULL;
  va_start(ap, exp);
  format_integersv(&t, fmt, f, ap);
  va_end(ap);
  if (t) {
    if (e) x = strcmp(t, e);
    fprintf(stderr, "\"%s\" -> \"%s\" --- %s (key: \"%s\")\n", fmt, t,
            (x == 0) ? "ok" : "ng", f);
    free(t);
  } else {
    if (e) {
      fprintf(stderr, "\"%s\" --- failed (key: \"%s\")\n", fmt, f);
    } else {
      fprintf(stderr, "\"%s\" --- ok (exfail) (key: \"%s\")\n", fmt, f);
      x = 0;
    }
  }
  return (x != 0);
}

int format_tester(void)
{
  int ret;
  ret = 0;
  ret += format_test("abc", "", -1, "abc");
  ret += format_test("abc%s", "", -1, NULL);
  ret += format_test("abc%04dabc", "d", -1, "abc-001abc");
  ret += format_test("abc%dabc", "d", 1, "abc1abc");
  ret += format_test("abc%04dabc", "d", 10, "abc0010abc");
  ret += format_test("abc%dabc", "d", 123, "abc123abc");
  ret += format_test("abc%2dabc", "d", 123, "abc123abc");
  ret += format_test("abc%2.1dabc", "d", 123, "abc123abc");
  ret += format_test("abc%dabc", "d", -4567, "abc-4567abc");
  ret += format_test("abc%+dabc", "d", 890, "abc+890abc");
  ret += format_test("abc%+5dabc", "d", 890, "abc +890abc");
  ret += format_test("abc%+05dabc", "d", 890, "abc +890abc");
  ret += format_test("abc% dabc", "d", -4567, "abc-4567abc");
  ret += format_test("abc% dabc", "d",  4567, "abc 4567abc");
  ret += format_test("abc%6dabc", "d", 123, "abc   123abc");
  ret += format_test("abc%06dabc", "d", 123, "abc000123abc");
  ret += format_test("abc%6.4dabc", "d", 123, "abc  0123abc");
  ret += format_test("def%6.4ddef", "d", -456, "def -0456def");
  ret += format_test("abc%6.1dabc", "d", 123, "abc   123abc");
  ret += format_test("abc%06.4dabc", "d", 123, "abc  0123abc");
  ret += format_test("abc%-6.1dabc", "d", 123, "abc123   abc");
  ret += format_test("abc%-6.4dabc", "d", 123, "abc0123  abc");
  ret += format_test("abc%- 8.4dabc", "d", 123, "abc 0123   abc");
  ret += format_test("def%-6.4ddef", "d", -456, "def-0456 def");
  ret += format_test("abc%-6dabc", "d", 123, "abc123   abc");
  ret += format_test("abc%-06dabc", "d", 123, "abc123   abc");
  ret += format_test("a%d%4d!%dd", "d", 123, "a123 123!123d");
  ret += format_test("[%04d]", "d", 123, "[0123]");
  ret += format_test("[%#04d]", "d", 1234567890, "[1234567890]");
  ret += format_test("lld%lld", "d", 123, NULL);
  ret += format_test("lldp%lld%", "d", 123, NULL);
  ret += format_test("lldp%000", "d", 123, NULL);
  ret += format_test("aa%%aa%d", "d", 123, "aa%aa123");
  ret += format_test("abc%xxxxd", "d", 123, NULL);
  ret += format_test("abc%fffff", "d", 123, NULL);
  ret += format_test("abc%1$d", "d", 123, NULL);
  ret += format_test("abc%fffff", "f", 123, "abc123ffff");

  ret += format_test2("abc%c_%i_%r", "ir[s]c", "abcdef_2_3", 2, 3, "def");
  ret += format_test2("abc%5c_%i_%r", "ir[s]c", NULL, 2, 3, "def");
  ret += format_test2("abc%c_%04i_%r", "r[s]c[d]i", "abcdef_0002_3", 3, "def", 2);

  ret += format_test2("abc%c_%04i_%r", "r [s]c [d] i", "abcdef_0002_3", 3, "def", 2);

  ret += format_test2("abc%d_%e_%z", "dez", "abc1_2_3", 1, 2, 3);
  ret += format_test2("abc%c_%03b_%q", "abq", NULL, 1, 2, 3);
  ret += format_test2("abc%a_%03b_%q", "abq", "abc1_002_3", 1, 2, 3);

#define T_Ss(x) #x
#define T_Es(x) T_Ss(x)
#define T_Ex(x) T_Es(VAL_##x)
#define T_Pp(x) "%" #x
#define T_Vv(x) VAL_##x

#define CC_A(_X,SP) _X(A)
#define CC_B(_X,SP) _X(B)
#define CC_C(_X,SP) _X(C)
#define CC_D(_X,SP) _X(D)
#define CC_E(_X,SP) _X(E)
#define CC_F(_X,SP) _X(F)
#define CC_G(_X,SP) _X(G)
#define CC_H(_X,SP) _X(H)
#define CC_I(_X,SP) _X(I)
#define CC_J(_X,SP) _X(J)
#define CC_K(_X,SP) _X(K)
#define CC_L(_X,SP) _X(L)
#define CC_M(_X,SP) _X(M)
#define CC_N(_X,SP) _X(N)
#define CC_O(_X,SP) _X(O)
#define CC_P(_X,SP) _X(P)
#define CC_Q(_X,SP) _X(Q)
#define CC_R(_X,SP) _X(R)
#define CC_S(_X,SP) _X(S)
#define CC_T(_X,SP) _X(T)
#define CC_U(_X,SP) _X(U)
#define CC_V(_X,SP) _X(V)
#define CC_W(_X,SP) _X(W)
#define CC_X(_X,SP) _X(X)
#define CC_Y(_X,SP) _X(Y)
#define CC_Z(_X,SP) _X(Z)

#define CC_a(_X,SP) _X(a)
#define CC_b(_X,SP) _X(b)
#define CC_c(_X,SP) _X(c)
#define CC_d(_X,SP) _X(d)
#define CC_e(_X,SP) _X(e)
#define CC_f(_X,SP) _X(f)
#define CC_g(_X,SP) _X(g)
#define CC_h(_X,SP) _X(h)
#define CC_i(_X,SP) _X(i)
#define CC_j(_X,SP) _X(j)
#define CC_k(_X,SP) _X(k)
#define CC_l(_X,SP) _X(l)
#define CC_m(_X,SP) _X(m)
#define CC_n(_X,SP) _X(n)
#define CC_o(_X,SP) _X(o)
#define CC_p(_X,SP) _X(p)
#define CC_q(_X,SP) _X(q)
#define CC_r(_X,SP) _X(r)
#define CC_s(_X,SP) _X(s)
#define CC_t(_X,SP) _X(t)
#define CC_u(_X,SP) _X(u)
#define CC_v(_X,SP) _X(v)
#define CC_w(_X,SP) _X(w)
#define CC_x(_X,SP) _X(x)
#define CC_y(_X,SP) _X(y)
#define CC_z(_X,SP) _X(z)

#define CC01(X,SP,A)   CC##A(X,SP)
#define CC02(X,SP,A,B) CC##A(X,SP) S_##SP CC##B(X,SP)
#define CC03(X,SP,A,B,C) CC01(X,SP,A) S_##SP CC02(X,SP,B,C)
#define CC04(X,SP,A,B,C,D) CC02(X,SP,A,B) S_##SP CC02(X,SP,C,D)
#define CC05(X,SP,A,B,C,D,E) CC03(X,SP,A,B,C) S_##SP CC02(X,SP,D,E)
#define CC06(X,SP,A,B,C,D,E,F) CC04(X,SP,A,B,C,D) S_##SP CC02(X,SP,E,F)
#define CC08(X,SP,A,B,C,D,E,F,G,H) CC04(X,SP,A,B,C,D) S_##SP CC04(X,SP,E,F,G,H)
#define CC10(X,SP,A,B,C,D,E,F,G,H,I,J) CC05(X,SP,A,B,C,D,E) S_##SP CC05(X,SP,F,G,H,I,J)
#define CC12(X,SP,A,B,C,D,E,F,G,H,I,J,K,L) CC06(X,SP,A,B,C,D,E,F) S_##SP CC04(X,SP,G,H,I,J,K,L)
#define CC16(X,SP,A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P) CC08(X,SP,A,B,C,D,E,F,G,H) S_##SP CC08(X,SP,I,J,K,L,M,N,O,P)
#define CC32(X,SP,A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) CC16(X,SP,A,B,C,D,E,F,G,H,J,K,L,M,N,O,P) S_##SP CC16(X,SP,a,b,c,d,e,f,g,h,j,k,l,m,n,o,p)
#define CC26(X,SP) CC16(X,SP,_A,_B,_C,_D,_E,_F,_G,_H,_I,_J,_K,_L,_M,_N,_O,_P) S_##SP CC10(X,SP,_Q,_R,_S,_T,_U,_V,_W,_X,_Y,_Z)
#define CC26s(X,SP) CC16(X,SP,_a,_b,_c,_d,_e,_f,_g,_h,_i,_j,_k,_l,_m,_n,_o,_p) S_##SP CC10(X,SP,_q,_r,_s,_t,_u,_v,_w,_x,_y,_z)

#define CC52(X,SP) CC26(X,SP) S_##SP CC26s(X,SP)

  /* do not surround with (). The compilation will fail. */
#define VAL_A 1
#define VAL_B 112
#define VAL_C 3
#define VAL_D 54
#define VAL_E 5
#define VAL_F 26
#define VAL_G 7
#define VAL_H 8
#define VAL_I 11
#define VAL_J 112
#define VAL_K 13
#define VAL_L 14
#define VAL_M 145
#define VAL_N 16
#define VAL_O 217
#define VAL_P 18
#define VAL_Q 111
#define VAL_R 155
#define VAL_S 513
#define VAL_T 114
#define VAL_U 75
#define VAL_V 116
#define VAL_W 97
#define VAL_X 118
#define VAL_Y 9631
#define VAL_Z 110

#define VAL_a 13
#define VAL_b 12
#define VAL_c 35
#define VAL_d 5
#define VAL_e 66
#define VAL_f 222
#define VAL_g 22
#define VAL_h 8888
#define VAL_i 99
#define VAL_j 15
#define VAL_k 111
#define VAL_l 0
#define VAL_m 133
#define VAL_n 1333
#define VAL_o 22
#define VAL_p 18
#define VAL_q 11
#define VAL_r 199
#define VAL_s 511
#define VAL_t 144
#define VAL_u 733
#define VAL_v 3333
#define VAL_w 45454
#define VAL_x 193
#define VAL_y 9633
#define VAL_z 1111

#define S_NUL
#define S_CMA ,
#define S_SCMA ","

#define TEST(CASE_N) format_test2(CASE_N(T_Pp,NUL), CASE_N(T_Ss,NUL), CASE_N(T_Ex,NUL), CASE_N(T_Vv,CMA))

#define CASE1(X,SP) CC04(X,SP,_A,_B,_C,_Z)
  ret += TEST(CASE1);

#define CASE2(X,SP) CC26(X,SP)
  ret += TEST(CASE2);

#define CASE3(X,SP) CC26s(X,SP)
  ret += TEST(CASE3);

#define CASE4(X,SP) CC52(X,SP)
  ret += TEST(CASE4);

#define TEST2(CASE_N) format_test2(CASE_N(T_Pp,SCMA), CASE_N(T_Ss,NUL), CASE_N(T_Ex,SCMA), CASE_N(T_Vv,CMA))
  ret += TEST2(CASE1);
  ret += TEST2(CASE2);
  ret += TEST2(CASE3);
  ret += TEST2(CASE4);

  return ret;
}

static int make_glob_test_comp(const char *pattern, const char *exp)
{
  char *x;
  int r;
  r = make_glob_pattern(&x, pattern);
  if (r < 0) {
    if (exp) {
      fprintf(stderr, "fail: make_glob_pattern(%s) -> fail, (expect: %s)\n",
              pattern, exp);
      return 1;
    } else {
      fprintf(stderr, "exfail: make_glob_pattern(%s) -> fail\n", pattern);
      return 0;
    }
  }

  if (exp) {
    if (strcmp(x, exp) != 0) {
      fprintf(stderr, "fail: make_glob_pattern(%s) -> %s, (expect: %s)\n",
              pattern, x, exp);
      r = 1;
    } else {
      fprintf(stderr, "pass: make_glob_pattern(%s) -> %s\n", pattern, x);
      r = 0;
    }
  } else {
    fprintf(stderr, "expass: make_glob_pattern(%s) -> %s, (expect fail)\n",
            pattern, x);
    r = 1;
  }
  free(x);
  return r;
}

int make_glob_test(void)
{
  int r;
  r  = make_glob_test_comp("ab%c%scd", "ab*cd");
  r += make_glob_test_comp("ab%%scd", "ab%scd");
  r += make_glob_test_comp("ab%xscd", "ab*scd");
  r += make_glob_test_comp("ab%0scd", "ab*cd");
  r += make_glob_test_comp("ab% -#.0scd", "ab*cd");
  r += make_glob_test_comp("ab% -#.0s%5scd", "ab*cd");
  r += make_glob_test_comp("data/binary_data/%c/%04n.dat",
                           "data/binary_data/*/*.dat");
  return r;
}

static int match_test(const char *scan, const char *fmt, int ndata,
                      format_integers_match_data *data, int expect_match, ...)
{
  int r;
  int i;
  va_list ap;
  r = format_integers_match(scan, fmt, ndata, data);
  if (r == expect_match) {
    switch (r) {
    case 1:
      fprintf(stderr, "pass: match(Match: %s, Format: %s) matched.\n",
              scan, fmt);
      va_start(ap, expect_match);
      for (i = 0; i < ndata; ++i) {
        int exm = va_arg(ap, int);
        if (data[i].matched == exm) {
          if (!exm) {
            fprintf(stderr, "....: %c: no match (ok)\n", data[i].key);
            continue;
          }
        } else {
          fprintf(stderr, "....: %c: no match (expected match)\n",
                  data[i].key);
          r = 1;
          continue;
        }

        if (data[i].type == FORMAT_MATCH_INT) {
          int exv = va_arg(ap, int);
          if (data[i].value == exv) {
            fprintf(stderr, "....: %c: %d (ok)\n", data[i].key, data[i].value);
          } else {
            fprintf(stderr, "....: %c: %d (expectecd: %d)\n",
                    data[i].key, data[i].value, exv);
            r = 1;
          }
        } else if (data[i].type == FORMAT_MATCH_STR) {
          const char *exv = va_arg(ap, char *);
          if (exv && data[i].string) {
            const char *cp;
            cp = *data[i].string;
            if (cp && strcmp(exv, cp) == 0) {
              fprintf(stderr, "....: %c: %s (ok)\n", data[i].key, exv);
            } else {
              fprintf(stderr, "....: %c: %s (expected: %s)\n",
                      data[i].key, cp, exv);
              r = 1;
            }
          } else {
            fprintf(stderr, "....: %c: (value not tested)\n", data[i].key);
            r = 1;
          }
        }
      }
      va_end(ap);
      break;
    case 0:
      fprintf(stderr, "pass: match(Match: %s, Format: %s) does not match.\n",
              scan, fmt);
      break;
    case -1:
      fprintf(stderr, "pass: match(Match: %s, Format: %s) expected error.\n",
              scan, fmt);
      break;
    }
    r = 0;
  } else {
    if (r) {
      fprintf(stderr, "fail: match(Match: %s, Format: %s) does match (expected does not match)\n",
              scan, fmt);
    } else {
      fprintf(stderr, "fail: match(Match: %s, Format: %s) does not match (expected does match)\n",
              scan, fmt);
    }
    r = 1;
  }
  return r;
}

int match_tester(void)
{
  format_integers_match_data data[] = {
    { .key = 'A', .type = FORMAT_MATCH_INT, .string = NULL },
    { .key = 'B', .type = FORMAT_MATCH_STR, .string = NULL },
    { .key = 'm', .type = FORMAT_MATCH_INT, .string = NULL },
    { .key = 'k', .type = FORMAT_MATCH_STR, .string = NULL },
    { .key = 'a', .type = FORMAT_MATCH_INT, .string = NULL },
    { .key = 'z', .type = FORMAT_MATCH_INT, .string = NULL },
    { .key = 'T', .type = FORMAT_MATCH_INT, .string = NULL },
    { .key = 'W', .type = FORMAT_MATCH_STR, .string = NULL },
    { .key = 'G', .type = FORMAT_MATCH_INT, .string = NULL },
    { .key = 'p', .type = FORMAT_MATCH_INT, .string = NULL },
    { .key = '\0' },

    { .key = 'n', .type = FORMAT_MATCH_INT, .string = NULL },
    { .key = 'c', .type = FORMAT_MATCH_STR, .string = NULL },
    { .key = 'r', .type = FORMAT_MATCH_INT, .string = NULL },
    { .key = 'i', .type = FORMAT_MATCH_INT, .string = NULL },
  };
  int r;
  char *m[10];

  r = 0;

  data[ 1].string = &m[0];
  /* data[ 3].string = &m[1]; */
  data[12].string = &m[2];
  r += match_test("100_x50_1_t", "%A_%B_%A_%B", 2, data,
                  1, 1, 1, 1, "t");
  free(m[0]);


  r += match_test("100_x50_1_t", "%A_%B_%a_%k", 5, data,
                  1,
                  /* A */ 1, 100,
                  /* B */ 1, "x50",
                  /* m */ 0,
                  /* k */ 1, NULL,
                  /* a */ 1, 1);
  free(m[0]);

  r += match_test("100_x50_1_t", "%.0A_%B_%-# 0a_%10k", 5, data,
                  1,
                  /* A */ 1, 100,
                  /* B */ 1, "x50",
                  /* m */ 0,
                  /* k */ 1, NULL,
                  /* a */ 1, 1);
  free(m[0]);

  r += match_test("100_50", "%A_%B", 2, data, 0);
  r += match_test("aza", "aaa", 2, data, 0);
  r += match_test("aaa", "aaa", 2, data, 1, /* A */ 0, /* B */ 0);
  r += match_test("a%a", "a%%a", 2, data, 1, /* A */ 0, /* B */ 0);

  r += match_test("data/binary_data/time/0410.dat",
                  "data/binary_data/%c/%04n.dat", 4, data + 11,
                  1,
                  /* n */ 1, 410,
                  /* c */ 1, "time",
                  /* r */ 0,
                  /* i */ 0);
  free(m[2]);

  return r;
}
