#ifndef JUPITER_CONTROL_CSVPARSER_H
#define JUPITER_CONTROL_CSVPARSER_H

#include "defs.h"
#include "shared_object.h"

#include <stdio.h>

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_csvparser;
typedef struct jcntrl_csvparser jcntrl_csvparser;

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_csvparser);

JUPITER_CONTROL_DECL
jcntrl_csvparser *jcntrl_csvparser_new(void);
JUPITER_CONTROL_DECL
void jcntrl_csvparser_delete(jcntrl_csvparser *p);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_csvparser_object(jcntrl_csvparser *p);
JUPITER_CONTROL_DECL
jcntrl_csvparser *jcntrl_csvparser_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
int jcntrl_csvparser_set_input_file(jcntrl_csvparser *p, const char *filename);
JUPITER_CONTROL_DECL
int jcntrl_csvparser_set_input_file_d(jcntrl_csvparser *p,
                                      jcntrl_data_array *filename);
JUPITER_CONTROL_DECL
void jcntrl_csvparser_set_stream(jcntrl_csvparser *p, FILE *fp);
JUPITER_CONTROL_DECL
int jcntrl_csvparser_open_file(jcntrl_csvparser *p);
JUPITER_CONTROL_DECL
int jcntrl_csvparser_is_open(jcntrl_csvparser *p);
JUPITER_CONTROL_DECL
int jcntrl_csvparser_rewind(jcntrl_csvparser *p);
JUPITER_CONTROL_DECL
void jcntrl_csvparser_close_stream(jcntrl_csvparser *p);

JUPITER_CONTROL_DECL
int jcntrl_csvparser_error(jcntrl_csvparser *p);
JUPITER_CONTROL_DECL
int jcntrl_csvparser_eof(jcntrl_csvparser *p);

/**
 * The filename is NUL-terminated
 */
JUPITER_CONTROL_DECL
const char *jcntrl_csvparser_get_input_file(jcntrl_csvparser *p);

JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_csvparser_read_cell(jcntrl_csvparser *p);
JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_csvparser_read_cell_copy(jcntrl_csvparser *p);
JUPITER_CONTROL_DECL
jcntrl_string_array *jcntrl_csvparser_read_row(jcntrl_csvparser *p);
JUPITER_CONTROL_DECL
jcntrl_string_array *jcntrl_csvparser_read_row_copy(jcntrl_csvparser *p);

JUPITER_CONTROL_DECL_END

#endif
