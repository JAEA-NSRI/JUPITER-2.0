#ifndef JUPITER_CONTROL_MASK_LOP_H
#define JUPITER_CONTROL_MASK_LOP_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_mask_lop_function;
typedef struct jcntrl_mask_lop_function jcntrl_mask_lop_function;

/**
 * Base class for mask logical operations
 */
struct jcntrl_mask_lop;
typedef struct jcntrl_mask_lop jcntrl_mask_lop;

struct jcntrl_mask_or;
typedef struct jcntrl_mask_or jcntrl_mask_or;
// jcntrl_mask_add does not exist as class name.

struct jcntrl_mask_sub;
typedef struct jcntrl_mask_sub jcntrl_mask_sub;

struct jcntrl_mask_and;
typedef struct jcntrl_mask_and jcntrl_mask_and;
// jcntrl_mask_mul does not exist as class name.

struct jcntrl_mask_xor;
typedef struct jcntrl_mask_xor jcntrl_mask_xor;

struct jcntrl_mask_eqv;
typedef struct jcntrl_mask_eqv jcntrl_mask_eqv;

struct jcntrl_mask_nor;
typedef struct jcntrl_mask_xor jcntrl_mask_nor;

struct jcntrl_mask_nand;
typedef struct jcntrl_mask_xor jcntrl_mask_nand;

struct jcntrl_mask_xnor;
typedef struct jcntrl_mask_xor jcntrl_mask_xnor;

struct jcntrl_mask_neqv;
typedef struct jcntrl_mask_neqv jcntrl_mask_neqv;

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_lop_function);

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_lop);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_or);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_sub);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_and);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_xor);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_eqv);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_nand);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_nor);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_xnor);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_neqv);

JUPITER_CONTROL_DECL
jcntrl_mask_lop *jcntrl_mask_lop_new(void);
JUPITER_CONTROL_DECL
jcntrl_mask_or *jcntrl_mask_or_new(void);
JUPITER_CONTROL_DECL
jcntrl_mask_sub *jcntrl_mask_sub_new(void);
JUPITER_CONTROL_DECL
jcntrl_mask_and *jcntrl_mask_and_new(void);
JUPITER_CONTROL_DECL
jcntrl_mask_xor *jcntrl_mask_xor_new(void);
JUPITER_CONTROL_DECL
jcntrl_mask_eqv *jcntrl_mask_eqv_new(void);
JUPITER_CONTROL_DECL
jcntrl_mask_nand *jcntrl_mask_nand_new(void);
JUPITER_CONTROL_DECL
jcntrl_mask_nor *jcntrl_mask_nor_new(void);
JUPITER_CONTROL_DECL
jcntrl_mask_xnor *jcntrl_mask_xnor_new(void);
JUPITER_CONTROL_DECL
jcntrl_mask_neqv *jcntrl_mask_neqv_new(void);

JUPITER_CONTROL_DECL
void jcntrl_mask_lop_delete(jcntrl_mask_lop *m);
JUPITER_CONTROL_DECL
void jcntrl_mask_or_delete(jcntrl_mask_or *m);
JUPITER_CONTROL_DECL
void jcntrl_mask_sub_delete(jcntrl_mask_sub *m);
JUPITER_CONTROL_DECL
void jcntrl_mask_and_delete(jcntrl_mask_and *m);
JUPITER_CONTROL_DECL
void jcntrl_mask_xor_delete(jcntrl_mask_xor *m);
JUPITER_CONTROL_DECL
void jcntrl_mask_eqv_delete(jcntrl_mask_eqv *m);
JUPITER_CONTROL_DECL
void jcntrl_mask_nand_delete(jcntrl_mask_nand *m);
JUPITER_CONTROL_DECL
void jcntrl_mask_nor_delete(jcntrl_mask_nand *m);
JUPITER_CONTROL_DECL
void jcntrl_mask_xnor_delete(jcntrl_mask_xnor *m);
JUPITER_CONTROL_DECL
void jcntrl_mask_neqv_delete(jcntrl_mask_neqv *m);

JUPITER_CONTROL_DECL jcntrl_shared_object *
jcntrl_mask_lop_object(jcntrl_mask_lop *m);
JUPITER_CONTROL_DECL jcntrl_shared_object *
jcntrl_mask_or_object(jcntrl_mask_or *m);
JUPITER_CONTROL_DECL jcntrl_shared_object *
jcntrl_mask_sub_object(jcntrl_mask_sub *m);
JUPITER_CONTROL_DECL jcntrl_shared_object *
jcntrl_mask_and_object(jcntrl_mask_and *m);
JUPITER_CONTROL_DECL jcntrl_shared_object *
jcntrl_mask_xor_object(jcntrl_mask_xor *m);
JUPITER_CONTROL_DECL jcntrl_shared_object *
jcntrl_mask_eqv_object(jcntrl_mask_eqv *m);
JUPITER_CONTROL_DECL jcntrl_shared_object *
jcntrl_mask_nand_object(jcntrl_mask_nand *m);
JUPITER_CONTROL_DECL jcntrl_shared_object *
jcntrl_mask_nor_object(jcntrl_mask_nor *m);
JUPITER_CONTROL_DECL jcntrl_shared_object *
jcntrl_mask_xnor_object(jcntrl_mask_xnor *m);
JUPITER_CONTROL_DECL jcntrl_shared_object *
jcntrl_mask_neqv_object(jcntrl_mask_neqv *m);

JUPITER_CONTROL_DECL jcntrl_executive *
jcntrl_mask_lop_executive(jcntrl_mask_lop *m);
JUPITER_CONTROL_DECL jcntrl_executive *
jcntrl_mask_or_executive(jcntrl_mask_or *m);
JUPITER_CONTROL_DECL jcntrl_executive *
jcntrl_mask_sub_executive(jcntrl_mask_sub *m);
JUPITER_CONTROL_DECL jcntrl_executive *
jcntrl_mask_and_executive(jcntrl_mask_and *m);
JUPITER_CONTROL_DECL jcntrl_executive *
jcntrl_mask_xor_executive(jcntrl_mask_xor *m);
JUPITER_CONTROL_DECL jcntrl_executive *
jcntrl_mask_eqv_executive(jcntrl_mask_eqv *m);
JUPITER_CONTROL_DECL jcntrl_executive *
jcntrl_mask_nand_executive(jcntrl_mask_nand *m);
JUPITER_CONTROL_DECL jcntrl_executive *
jcntrl_mask_nor_executive(jcntrl_mask_nor *m);
JUPITER_CONTROL_DECL jcntrl_executive *
jcntrl_mask_xnor_executive(jcntrl_mask_xnor *m);
JUPITER_CONTROL_DECL jcntrl_executive *
jcntrl_mask_neqv_executive(jcntrl_mask_neqv *m);

JUPITER_CONTROL_DECL jcntrl_mask_lop *
jcntrl_mask_lop_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL jcntrl_mask_or *
jcntrl_mask_or_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL jcntrl_mask_sub *
jcntrl_mask_sub_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL jcntrl_mask_and *
jcntrl_mask_and_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL jcntrl_mask_xor *
jcntrl_mask_xor_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL jcntrl_mask_eqv *
jcntrl_mask_eqv_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL jcntrl_mask_nand *
jcntrl_mask_nand_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL jcntrl_mask_nor *
jcntrl_mask_nor_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL jcntrl_mask_xnor *
jcntrl_mask_xnor_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL jcntrl_mask_neqv *
jcntrl_mask_neqv_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL jcntrl_mask_lop *jcntrl_mask_or_lop(jcntrl_mask_or *m);
JUPITER_CONTROL_DECL jcntrl_mask_lop *jcntrl_mask_sub_lop(jcntrl_mask_sub *m);
JUPITER_CONTROL_DECL jcntrl_mask_lop *jcntrl_mask_and_lop(jcntrl_mask_and *m);
JUPITER_CONTROL_DECL jcntrl_mask_lop *jcntrl_mask_xor_lop(jcntrl_mask_xor *m);
JUPITER_CONTROL_DECL jcntrl_mask_lop *jcntrl_mask_eqv_lop(jcntrl_mask_eqv *m);
JUPITER_CONTROL_DECL jcntrl_mask_lop *jcntrl_mask_nand_lop(jcntrl_mask_nand *m);
JUPITER_CONTROL_DECL jcntrl_mask_lop *jcntrl_mask_nor_lop(jcntrl_mask_nor *m);
JUPITER_CONTROL_DECL jcntrl_mask_lop *jcntrl_mask_xnor_lop(jcntrl_mask_xnor *m);
JUPITER_CONTROL_DECL jcntrl_mask_lop *jcntrl_mask_neqv_lop(jcntrl_mask_neqv *m);

/**
 * @note This function will reject setting operator for @p m which is an
 * instance of subclass of jcntrl_mask_lop.
 */
JUPITER_CONTROL_DECL
int jcntrl_mask_lop_set_op(jcntrl_mask_lop *m, jcntrl_logical_operator op);
JUPITER_CONTROL_DECL jcntrl_logical_operator
jcntrl_mask_lop_op(jcntrl_mask_lop *m);

//----

JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_mask_lop_function_object(jcntrl_mask_lop_function *f);

JUPITER_CONTROL_DECL
jcntrl_mask_function *
jcntrl_mask_lop_function_function(jcntrl_mask_lop_function *f);

JUPITER_CONTROL_DECL
jcntrl_mask_lop_function *
jcntrl_mask_lop_function_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
int jcntrl_mask_lop_function_set_op(jcntrl_mask_lop_function *m,
                                    jcntrl_logical_operator op);

JUPITER_CONTROL_DECL
jcntrl_logical_operator
jcntrl_mask_lop_function_op(jcntrl_mask_lop_function *m);

JUPITER_CONTROL_DECL
int jcntrl_mask_lop_function_set_number_of_inputs(jcntrl_mask_lop_function *m,
                                                  jcntrl_size_type n);

JUPITER_CONTROL_DECL
jcntrl_size_type
jcntrl_mask_lop_function_get_number_of_inputs(jcntrl_mask_lop_function *m);

JUPITER_CONTROL_DECL
int jcntrl_mask_lop_function_set_input(jcntrl_mask_lop_function *m,
                                       jcntrl_size_type index,
                                       jcntrl_mask_function *f);

JUPITER_CONTROL_DECL
jcntrl_mask_function *
jcntrl_mask_lop_function_get_input(jcntrl_mask_lop_function *m,
                                   jcntrl_size_type index);

JUPITER_CONTROL_DECL_END

#endif
