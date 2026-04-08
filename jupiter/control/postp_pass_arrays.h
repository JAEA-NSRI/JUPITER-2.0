#ifndef JUPITER_CONTROL_POSTP_PASS_ARRAY_H
#define JUPITER_CONTROL_POSTP_PASS_ARRAY_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_postp_pass_arrays;
typedef struct jcntrl_postp_pass_arrays jcntrl_postp_pass_arrays;

struct jcntrl_postp_del_variable;
typedef struct jcntrl_postp_del_variable jcntrl_postp_del_variable;

struct jcntrl_postp_del_variable_except;
typedef struct jcntrl_postp_del_variable_except
  jcntrl_postp_del_variable_except;

enum jcntrl_postp_pass_array_mode
{
  JCNTRL_POSTP_PASS_KEEP,   ///< Keep given arrays, and delete others
  JCNTRL_POSTP_PASS_DELETE, ///< Delete given arrays
};
typedef enum jcntrl_postp_pass_array_mode jcntrl_postp_pass_array_mode;

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_postp_pass_arrays);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_postp_del_variable);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_postp_del_variable_except);

JUPITER_CONTROL_DECL
jcntrl_postp_pass_arrays *jcntrl_postp_pass_arrays_new(void);
JUPITER_CONTROL_DECL
void jcntrl_postp_pass_arrays_delete(jcntrl_postp_pass_arrays *p);

JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_postp_pass_arrays_object(jcntrl_postp_pass_arrays *p);

JUPITER_CONTROL_DECL
jcntrl_executive *
jcntrl_postp_pass_arrays_executive(jcntrl_postp_pass_arrays *p);

JUPITER_CONTROL_DECL
int jcntrl_postp_pass_arrays_set_number_of_variables(
  jcntrl_postp_pass_arrays *p, jcntrl_size_type nvars);

JUPITER_CONTROL_DECL
jcntrl_size_type
jcntrl_postp_pass_arrays_get_number_of_variables(jcntrl_postp_pass_arrays *p);

JUPITER_CONTROL_DECL
int jcntrl_postp_pass_arrays_set_variable(jcntrl_postp_pass_arrays *p,
                                          jcntrl_size_type index,
                                          jcntrl_data_array *name);

JUPITER_CONTROL_DECL
int jcntrl_postp_pass_arrays_set_variable_c(jcntrl_postp_pass_arrays *p,
                                            jcntrl_size_type index,
                                            const char *name,
                                            jcntrl_size_type len);

JUPITER_CONTROL_DECL
jcntrl_char_array *
jcntrl_postp_pass_arrays_get_variable(jcntrl_postp_pass_arrays *p,
                                      jcntrl_size_type index);

JUPITER_CONTROL_DECL
jcntrl_postp_pass_array_mode
jcntrl_postp_pass_arrays_get_mode(jcntrl_postp_pass_arrays *p);

/**
 * @note This function will reject setting mode if @p p is an instance of
 * jcntrl_variable or jcntrl_variable_except.
 */
JUPITER_CONTROL_DECL
int jcntrl_postp_pass_arrays_set_mode(jcntrl_postp_pass_arrays *p,
                                      jcntrl_postp_pass_array_mode mode);

//---

JUPITER_CONTROL_DECL
jcntrl_postp_del_variable *jcntrl_postp_del_variable_new(void);
void jcntrl_postp_del_variable_delete(jcntrl_postp_del_variable *p);

JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_postp_del_variable_object(jcntrl_postp_del_variable *p);

JUPITER_CONTROL_DECL
jcntrl_executive *
jcntrl_postp_del_variable_executive(jcntrl_postp_del_variable *p);

JUPITER_CONTROL_DECL
jcntrl_postp_pass_arrays *
jcntrl_postp_del_variable_pass_arrays(jcntrl_postp_del_variable *p);

JUPITER_CONTROL_DECL
jcntrl_postp_del_variable *
jcntrl_postp_del_variable_downcast(jcntrl_shared_object *obj);

//---

JUPITER_CONTROL_DECL
jcntrl_postp_del_variable_except *jcntrl_postp_del_variable_except_new(void);
JUPITER_CONTROL_DECL
void jcntrl_postp_del_variable_except_delete(
  jcntrl_postp_del_variable_except *p);

JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_postp_del_variable_except_object(jcntrl_postp_del_variable_except *p);

JUPITER_CONTROL_DECL
jcntrl_executive *
jcntrl_postp_del_variable_except_executive(jcntrl_postp_del_variable_except *p);

JUPITER_CONTROL_DECL
jcntrl_postp_pass_arrays *jcntrl_postp_del_variable_except_pass_arrays(
  jcntrl_postp_del_variable_except *p);

JUPITER_CONTROL_DECL
jcntrl_postp_del_variable_except *
jcntrl_postp_del_variable_except_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL_END

#endif
