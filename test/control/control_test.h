
#ifndef CONTROL_TEST_H
#define CONTROL_TEST_H

#include <jupiter/control/defs.h>

int test_control_util(void);
int test_control_grid_data_feeder(void);

int test_control_object(void);
int test_control_error(void);
int test_control_overflow(void);
int test_control_extent(void);
int test_control_logical_operator(void);
int test_control_comparator(void);
int test_control_cell(void);
int test_control_information(void);
int test_control_input(void);
int test_control_output(void);
int test_control_grid_data(void);
int test_control_cell_data(void);
int test_control_connection(void);
int test_control_data_array(void);
int test_control_executive(void);
int test_control_manager(void);
int test_control_subarray(void);
int test_control_static_array(void);
int test_control_string_array(void);
int test_control_csvparser(void);
int test_control_mpi_controller(void);

int test_control_postp_mask(void);
int test_control_postp_volume_integral(void);
int test_control_postp_sum(void);
int test_control_postp_pass_arrays(void);

int test_control_mask_extent(void);
int test_control_mask_point(void);
int test_control_mask_lop(void);

int test_control_fv_table(void);
int test_control_fv_get(void);

int test_control_write_fv_csv(void);

/**
 * error handler of control tester
 *
 * @p data is not used currently.
 */
int error_handler(void *data, jcntrl_information *errinfo);

#endif
