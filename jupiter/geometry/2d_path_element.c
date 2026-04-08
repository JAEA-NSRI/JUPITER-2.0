#include "2d_path_element.h"
#include "struct_data.h"
#include "defs.h"
#include "geom_assert.h"
#include "vector.h"
#include "2d_path.h"

static const char geom_2d_path_move_element_entry[] = "dd";
#define geom_2d_path_move_element_entry_size \
  (sizeof(geom_2d_path_move_element_entry) / sizeof(char))

static const char geom_2d_path_line_element_entry[] = "dd";
#define geom_2d_path_line_element_entry_size \
  (sizeof(geom_2d_path_line_element_entry) / sizeof(char))

static const char geom_2d_path_circ_element_entry[] = "diidd";
#define geom_2d_path_circ_element_entry_size \
  (sizeof(geom_2d_path_circ_element_entry) / sizeof(char))

static const char geom_2d_path_end_element_entry[] = "";
#define geom_2d_path_end_element_entry_size \
  (sizeof(geom_2d_path_end_element_entry) / sizeof(char))

geom_size_type geom_2d_path_get_max_entry(void)
{
  static const geom_size_type sz[] = {
    geom_2d_path_move_element_entry_size,
    geom_2d_path_line_element_entry_size,
    geom_2d_path_circ_element_entry_size,
    geom_2d_path_end_element_entry_size,
  };

  geom_size_type max = 0;
  for (geom_size_type i = 0; i < sizeof(sz) / sizeof(geom_size_type); ++i) {
    if (sz[i] > max)
      max = sz[i];
  }
  return max;
}

geom_size_type geom_2d_path_get_nentry1(geom_2d_path_element_type element_type)
{
  switch (element_type) {
  case GEOM_2D_PATH_MOVE:
  case GEOM_2D_PATH_RELMOVE:
    return geom_2d_path_move_element_entry_size;

  case GEOM_2D_PATH_LINE:
  case GEOM_2D_PATH_RELLINE:
    return geom_2d_path_line_element_entry_size;

  case GEOM_2D_PATH_CIRC:
  case GEOM_2D_PATH_RELCIRC:
    return geom_2d_path_circ_element_entry_size;

  case GEOM_2D_PATH_END:
    return geom_2d_path_end_element_entry_size;

  case GEOM_2D_PATH_INVALID:
    return 0;
  }
  GEOM_UNREACHABLE();
}

geom_size_type geom_2d_path_get_nentry(const geom_2d_path_element *elements,
                                       geom_size_type number_of_elements)
{
  geom_size_type sz = 0;
  for (geom_size_type i = 0; i < number_of_elements; ++i) {
    geom_size_type szi = geom_2d_path_get_nentry1(elements[i].type);
    if (szi <= 0)
      return 0;
    sz += szi;
  }
  return sz + 1;
}

static void
geom_2d_path_element_get_vec2(const char **seq_code,
                              const union geom_2d_path_element_entry **entry,
                              geom_vec2 *dest)
{
  double x, y;

  GEOM_ASSERT(**seq_code == 'd');
  (*seq_code)++;
  x = (++(*entry))->d;

  GEOM_ASSERT(**seq_code == 'd');
  (*seq_code)++;
  y = (++(*entry))->d;

  *dest = geom_vec2_c(x, y);
}

static void
geom_2d_path_element_get_double(const char **seq_code,
                                const union geom_2d_path_element_entry **entry,
                                double *dest)
{
  GEOM_ASSERT(**seq_code == 'd');
  (*seq_code)++;
  *dest = (++(*entry))->d;
}

static void
geom_2d_path_element_get_int(const char **seq_code,
                             const union geom_2d_path_element_entry **entry,
                             int *dest)
{
  GEOM_ASSERT(**seq_code == 'i');
  (*seq_code)++;
  *dest = (++(*entry))->i;
}

geom_2d_path_element_iterator
geom_2d_path_element_next(geom_2d_path_element_iterator iter,
                          geom_2d_path_element *element)
{
  GEOM_ASSERT(iter.entry);
  GEOM_ASSERT(element);
  const char *seq = NULL;
  const union geom_2d_path_element_entry *entry;
  geom_vec2 last_move_p = iter.last_move_point;
  geom_vec2 origin_p = iter.origin_point;
  int relative = 0;

  entry = iter.entry;
  element->origin = origin_p;
  element->type = entry->t;

  switch (entry->t) {
  case GEOM_2D_PATH_RELMOVE:
  case GEOM_2D_PATH_RELLINE:
  case GEOM_2D_PATH_RELCIRC:
    relative = 1;
    break;
  default:
    break;
  }

  switch (entry->t) {
  case GEOM_2D_PATH_END:
    seq = geom_2d_path_end_element_entry;
    element->data.end.start_p = last_move_p;
    break;

  case GEOM_2D_PATH_MOVE:
  case GEOM_2D_PATH_RELMOVE:
  {
    geom_2d_path_move_element *m = &element->data.move;
    element->type = GEOM_2D_PATH_MOVE;
    seq = geom_2d_path_move_element_entry;

    geom_2d_path_element_get_vec2(&seq, &entry, &m->to);
    if (relative)
      m->to = geom_vec2_add(origin_p, m->to);

    last_move_p = m->to;
    origin_p = m->to;
  } break;

  case GEOM_2D_PATH_LINE:
  case GEOM_2D_PATH_RELLINE:
  {
    geom_2d_path_line_element *l = &element->data.line;
    element->type = GEOM_2D_PATH_LINE;
    seq = geom_2d_path_line_element_entry;

    geom_2d_path_element_get_vec2(&seq, &entry, &l->to);
    if (relative)
      l->to = geom_vec2_add(origin_p, l->to);

    origin_p = l->to;
  } break;

  case GEOM_2D_PATH_CIRC:
  case GEOM_2D_PATH_RELCIRC:
  {
    geom_2d_path_circ_element *c = &element->data.circ;
    seq = geom_2d_path_circ_element_entry;
    geom_2d_path_element_get_double(&seq, &entry, &c->radius);
    geom_2d_path_element_get_int(&seq, &entry, &c->large_arc_flag);
    geom_2d_path_element_get_int(&seq, &entry, &c->sweep_flag);
    geom_2d_path_element_get_vec2(&seq, &entry, &c->to);
    if (relative)
      c->to = geom_vec2_add(origin_p, c->to);

    origin_p = c->to;
  } break;

  case GEOM_2D_PATH_INVALID:
  {
    geom_2d_path_element_iterator it = {
      .entry = NULL,
      .last_move_point = last_move_p,
      .origin_point = origin_p,
    };
    return it;
  }
  }

  GEOM_ASSERT(seq);
  GEOM_ASSERT(*seq == '\0');

  geom_2d_path_element_iterator it = {
    .entry = entry,
    .last_move_point = last_move_p,
    .origin_point = origin_p,
  };
  return it;
}

geom_size_type geom_2d_path_from_elements(geom_2d_path_data *data,
                                          const geom_2d_path_element *elements,
                                          geom_size_type number_of_elements)
{
  geom_size_type nentry;
  geom_size_type i, j;

  GEOM_ASSERT(data);
  GEOM_ASSERT(number_of_elements >= 0);

  j = 0;
  for (i = 0; i < number_of_elements; ++i, ++elements) {
    geom_2d_path_element_type type = elements->type;
    const char *seq = NULL;
  }
  return i;
}
