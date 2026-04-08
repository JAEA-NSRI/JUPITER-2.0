
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <math.h>

#include <jupiter/serializer/buffer.h>
#include <jupiter/serializer/msgpackx.h>
#include <jupiter/serializer/error.h>

static const char *progname = NULL;
static void print_error(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "# ");
  if (progname) {
    fprintf(stderr, "%s: ", progname);
  }
  fprintf(stderr, "error: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

static void print_indent(int n)
{
  while (n > 0) {
    printf("  ");
    n--;
  }
}

static void print_bin(const char *bin, ptrdiff_t l, int indent)
{
  ptrdiff_t m, s, i;
  int ls;

  m = l;
  ls = 0;
  while (m > 0) {
    ++ls;
    m >>= 4;
  }
  if (ls < 8) ls = 8;
  for (s = 0; s < l; s += 16) {
    m = l - s;
    if (m > 16) m = 16;
    print_indent(indent);
    printf("%0*" PRIxMAX ":", ls, (intmax_t)s);
    for (i = 0; i < m; i += 2) {
      ptrdiff_t x, y;
      unsigned char cx, cy;
      x = s + i;
      y = s + i + 1;
      cx = bin[x];
      if (y < l) {
        cy = bin[y];
        printf(" %02x%02x", cx & 0xff, cy & 0xff);
      } else {
        printf(" %02x  ", cx & 0xff);
      }
    }
    for (; i < 16; i += 2) {
      printf("     ");
    }
    printf(" ");
    for (i = 0; i < m; i++) {
      unsigned char uc;
      uc = bin[s + i];
      if (isprint(uc)) {
        printf("%c", uc);
      } else {
        printf(".");
      }
    }
    printf("\n");
  }

}

static msgpackx_error
node_to_text(msgpackx_node *node, int indent, int mcont, int no_nl, int blked)
{
  msgpackx_error err;
  msgpackx_array_node *ah, *an;
  msgpackx_map_node *mh, *mn;
  int i;
  int f;

  err = MSGPACKX_SUCCESS;
  ah = msgpackx_node_get_array(node);
  if (ah) {
    f = 0;
    an = msgpackx_array_node_next(ah);
    if (blked) {
      printf("[");
    }
    if (an == ah) {
      printf("[]");
      if (!no_nl)
        printf("\n");
    } else {
      for (; an != ah; an = msgpackx_array_node_next(an)) {
        node = msgpackx_array_node_get_child_node(an);
        if (f) {
          if (!blked) {
            print_indent(indent);
          }
        } else {
          if (mcont) {
            printf("\n");
            print_indent(indent);
          }
        }
        if (!blked) {
          printf("- ");
          err = node_to_text(node, indent + 1, 0, 0, 0);
        } else {
          if (f) {
            printf(", ");
          }
          err = node_to_text(node, indent + 1, 0, 1, 1);
        }
        f = 1;
      }
      if (blked) {
        printf("]");
        if (!no_nl) {
          printf("\n");
        }
      }
    }
    return err;
  }

  mh = msgpackx_node_get_map(node);
  if (mh) {
    f = 0;
    mn = msgpackx_map_node_next(mh);
    if (mn == mh) {
      printf("{}");
      if (!no_nl)
        printf("\n");
    } else {
      for (; mn != mh; mn = msgpackx_map_node_next(mn)) {
        node = msgpackx_map_node_get_key(mn);
        if (f) {
          print_indent(indent);
        } else {
          if (mcont) {
            printf("\n");
            print_indent(indent);
          }
        }
        err = node_to_text(node, indent + 1, 0, 1, 1);
        printf(": ");
        node = msgpackx_map_node_get_value(mn);
        err = node_to_text(node, indent + 1, 1, 0, 0);
        f = 1;
      }
    }
    return err;
  }

  err = MSGPACKX_SUCCESS;
  {
    intmax_t im;

    im = msgpackx_node_get_int(node, &err);
    if (err == MSGPACKX_SUCCESS) {
      printf("%" PRIdMAX, im);
      if (!no_nl) {
        printf("\n");
      }
      return err;
    }
  }

  err = MSGPACKX_SUCCESS;
  {
    uintmax_t um;

    um = msgpackx_node_get_uint(node, &err);
    if (err == MSGPACKX_SUCCESS) {
      printf("%" PRIuMAX, um);
      if (!no_nl) {
        printf("\n");
      }
      return err;
    }
  }

  err = MSGPACKX_SUCCESS;
  {
    float f;
    float fi;

    f = msgpackx_node_get_float(node, &err);
    if (err == MSGPACKX_SUCCESS) {
      if (modff(f, &fi) == 0.0) {
        if (fi < 1000000.0) {
          printf("%.1f", f);
        } else {
          printf("%.17e", f);
        }
      } else {
        printf("%.17g", f);
      }
      if (!no_nl) {
        printf("\n");
      }
      return err;
    }
  }

  err = MSGPACKX_SUCCESS;
  {
    double d;
    double di;

    d = msgpackx_node_get_double(node, &err);
    if (err == MSGPACKX_SUCCESS) {
      if (modf(d, &di) == 0.0) {
        if (di < 1000000.0) {
          printf("%.1f", d);
        } else {
          printf("%.17e", d);
        }
      } else {
        printf("%.17g", d);
      }
      if (!no_nl) {
        printf("\n");
      }
      return err;
    }
  }

  err = MSGPACKX_SUCCESS;
  {
    const char *str;
    ptrdiff_t l;
    int il;

    str = msgpackx_node_get_str(node, &l, &err);
    if (err == MSGPACKX_SUCCESS) {
      il = l;
      if (il != l) {
        err = MSGPACKX_ERR_RANGE;
        printf("\"\"");
      } else {
        if (il == 0) {
          printf("\"\"");
        } else {
          printf("%*.*s", il, il, str);
        }
      }
      if (!no_nl) {
        printf("\n");
      }
      return err;
    }
  }

  err = MSGPACKX_SUCCESS;
  {
    const char *bin;
    ptrdiff_t l;

    bin = msgpackx_node_get_bin(node, &l, &err);
    if (err == MSGPACKX_SUCCESS) {
      if (l > 0 && !blked) {
        printf("!bin |\n");
        print_bin(bin, l, indent);
      } else {
        printf("!bin \"");
        for (; l > 0; --l, ++bin) {
          unsigned char uc;
          uc = *bin;
          if (isprint(uc)) {
            printf("%c", uc);
          } else {
            printf("\\x%02x", uc);
          }
        }
        printf("\"");
        if (!no_nl) {
          printf("\"\n");
        }
      }
      return err;
    }
  }

  err = MSGPACKX_SUCCESS;
  {
    const char *bin;
    int typ;
    ptrdiff_t l;

    bin = msgpackx_node_get_ext(node, &l, &typ, &err);
    if (err == MSGPACKX_SUCCESS) {
      if (l > 0 && !blked) {
        printf("!ext\n");
        print_indent(indent);
        printf("type: %d\n", typ);
        print_indent(indent);
        printf("data: |\n");
        print_bin(bin, l, indent + 1);
      } else {
        printf("!ext { type: %d, data: \"\" }", typ);
      }
      return err;
    }
  }

  err = MSGPACKX_SUCCESS;
  {
    int b;
    b = msgpackx_node_get_bool(node, &err);
    if (err == MSGPACKX_SUCCESS) {
      if (b) {
        printf("true");
      } else {
        printf("false");
      }
      if (!no_nl) {
        printf("\n");
      }
      return err;
    }
  }

  err = MSGPACKX_SUCCESS;
  if (msgpackx_node_is_nil(node)) {
    printf("!!nil ");
    if (!no_nl) {
      printf("\n");
    }
    return err;
  }

  err = MSGPACKX_ERR_MSG_TYPE;
  return err;
}

int main(int argc, char **argv)
{
  FILE *fp;
  msgpackx_buffer *buf;
  msgpackx_data *data;
  msgpackx_error err;
  msgpackx_node *cur;
  size_t rsz;
  size_t bsz;
  int r;
  int i;

  fp = NULL;
  bsz = 1024;
  r = EXIT_SUCCESS;
  progname = argv[0];

  data = NULL;
  buf = msgpackx_buffer_new();
  if (!buf) {
    print_error("Cannot allocate memory");
    return EXIT_FAILURE;
  }
  if (!msgpackx_buffer_resize(buf, bsz)) {
    msgpackx_buffer_delete(buf);
    print_error("Cannot allocate memory");
    return EXIT_FAILURE;
  }

  for (i = 1; i < argc; ++i) {
    fp = fopen(argv[i], "rb");
    if (!fp) {
      r = EXIT_FAILURE;
      print_error("Cannot open file %s: %s", argv[i], strerror(errno));
      continue;
    }

    msgpackx_buffer_goto(buf, 0, -1, MSGPACKX_SEEK_SET);
    rsz = 0;
    while (!feof(fp) && !ferror(fp)) {
      bsz = fread(msgpackx_buffer_pointer(buf), sizeof(char), bsz, fp);
      rsz += bsz;
      msgpackx_buffer_goto(buf, bsz, -1, MSGPACKX_SEEK_CUR);
      bsz = 1024;
      if (!msgpackx_buffer_resize_substr(buf, bsz)) {
        print_error("Cannot allocate memory");
        r = EXIT_FAILURE;
        goto clean;
      }
    }
    fclose(fp);
    fp = NULL;

    msgpackx_buffer_goto(buf, 0, rsz, MSGPACKX_SEEK_SET);

    err = MSGPACKX_SUCCESS;
    data = msgpackx_data_parse(buf, &err, NULL);
    if (err != MSGPACKX_SUCCESS) {
      print_error("Cannot parse data %s: %s", argv[i], msgpackx_strerror(err));
      data = NULL;
      continue;
    }

    cur = msgpackx_data_root_node(data);

    if (cur) {
      printf("--- # %s\n", argv[i]);
      err = node_to_text(cur, 0, 0, 0, 0);
      printf("...\n");
    } else {
      print_error("Data file is empty");
    }

    msgpackx_data_delete(data);
    data = NULL;
  }

clean:
  if (fp) fclose(fp);
  msgpackx_buffer_delete(buf);
  if (data) msgpackx_data_delete(data);
  return r;
}
