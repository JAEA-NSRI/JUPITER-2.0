
#include "control_test.h"
#include "jupiter/control/logical_operator.h"
#include "test-util.h"

int test_control_logical_operator(void)
{
  int ret;
  ret = 0;

  if (!test_compare_ii(jcntrl_logical_set_c(1, (char[]){1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(1, (char[]){0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_or_c(1, (char[]){1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(1, (char[]){0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_sub_c(1, (char[]){1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(1, (char[]){0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_and_c(1, (char[]){1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(1, (char[]){0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_xor_c(1, (char[]){1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(1, (char[]){0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_eqv_c(1, (char[]){1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(1, (char[]){0}), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_nor_c(1, (char[]){1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(1, (char[]){0}), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_nand_c(1, (char[]){1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(1, (char[]){0}), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_xnor_c(1, (char[]){1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(1, (char[]){0}), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_neqv_c(1, (char[]){1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(1, (char[]){0}), ==, 0))
    ret = 1;

  // 2 inputs
  if (!test_compare_ii(jcntrl_logical_set_c(2, (char[]){1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(2, (char[]){1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(2, (char[]){0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(2, (char[]){0, 0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_or_c(2, (char[]){1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(2, (char[]){1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(2, (char[]){0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(2, (char[]){0, 0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_sub_c(2, (char[]){1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(2, (char[]){1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(2, (char[]){0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(2, (char[]){0, 0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_and_c(2, (char[]){1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(2, (char[]){1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(2, (char[]){0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(2, (char[]){0, 0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_xor_c(2, (char[]){1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(2, (char[]){1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(2, (char[]){0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(2, (char[]){0, 0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_eqv_c(2, (char[]){1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(2, (char[]){1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(2, (char[]){0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(2, (char[]){0, 0}), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_nor_c(2, (char[]){1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(2, (char[]){1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(2, (char[]){0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(2, (char[]){0, 0}), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_nand_c(2, (char[]){1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(2, (char[]){1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(2, (char[]){0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(2, (char[]){0, 0}), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_xnor_c(2, (char[]){1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(2, (char[]){1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(2, (char[]){0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(2, (char[]){0, 0}), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_neqv_c(2, (char[]){1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(2, (char[]){1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(2, (char[]){0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(2, (char[]){0, 0}), ==, 0))
    ret = 1;

  // 4 inputs
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){1, 1, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){1, 1, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){1, 1, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){1, 1, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){1, 0, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){1, 0, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){1, 0, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){1, 0, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){0, 1, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){0, 1, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){0, 1, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){0, 1, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){0, 0, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){0, 0, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){0, 0, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_set_c(4, (char[]){0, 0, 0, 0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){1, 1, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){1, 1, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){1, 1, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){1, 1, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){1, 0, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){1, 0, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){1, 0, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){1, 0, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){0, 1, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){0, 1, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){0, 1, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){0, 1, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){0, 0, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){0, 0, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){0, 0, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_or_c(4, (char[]){0, 0, 0, 0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){1, 1, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){1, 1, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){1, 1, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){1, 1, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){1, 0, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){1, 0, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){1, 0, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){1, 0, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){0, 1, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){0, 1, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){0, 1, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){0, 1, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){0, 0, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){0, 0, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){0, 0, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_sub_c(4, (char[]){0, 0, 0, 0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){1, 1, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){1, 1, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){1, 1, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){1, 1, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){1, 0, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){1, 0, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){1, 0, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){1, 0, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){0, 1, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){0, 1, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){0, 1, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){0, 1, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){0, 0, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){0, 0, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){0, 0, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_and_c(4, (char[]){0, 0, 0, 0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){1, 1, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){1, 1, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){1, 1, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){1, 1, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){1, 0, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){1, 0, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){1, 0, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){1, 0, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){0, 1, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){0, 1, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){0, 1, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){0, 1, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){0, 0, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){0, 0, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){0, 0, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xor_c(4, (char[]){0, 0, 0, 0}), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){1, 1, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){1, 1, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){1, 1, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){1, 1, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){1, 0, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){1, 0, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){1, 0, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){1, 0, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){0, 1, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){0, 1, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){0, 1, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){0, 1, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){0, 0, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){0, 0, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){0, 0, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_eqv_c(4, (char[]){0, 0, 0, 0}), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){1, 1, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){1, 1, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){1, 1, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){1, 1, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){1, 0, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){1, 0, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){1, 0, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){1, 0, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){0, 1, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){0, 1, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){0, 1, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){0, 1, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){0, 0, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){0, 0, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){0, 0, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nor_c(4, (char[]){0, 0, 0, 0}), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){1, 1, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){1, 1, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){1, 1, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){1, 1, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){1, 0, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){1, 0, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){1, 0, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){1, 0, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){0, 1, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){0, 1, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){0, 1, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){0, 1, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){0, 0, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){0, 0, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){0, 0, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_nand_c(4, (char[]){0, 0, 0, 0}), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){1, 1, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){1, 1, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){1, 1, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){1, 1, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){1, 0, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){1, 0, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){1, 0, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){1, 0, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){0, 1, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){0, 1, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){0, 1, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){0, 1, 0, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){0, 0, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){0, 0, 1, 0}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){0, 0, 0, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_xnor_c(4, (char[]){0, 0, 0, 0}), ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){1, 1, 1, 1}), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){1, 1, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){1, 1, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){1, 1, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){1, 0, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){1, 0, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){1, 0, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){1, 0, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){0, 1, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){0, 1, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){0, 1, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){0, 1, 0, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){0, 0, 1, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){0, 0, 1, 0}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){0, 0, 0, 1}), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_logical_neqv_c(4, (char[]){0, 0, 0, 0}), ==, 0))
    ret = 1;

  return ret;
}
