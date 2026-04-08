#ifndef JUPITER_TRIP_CONTROL_H
#define JUPITER_TRIP_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* YSE: Add trip control type enumetrator */
/**
 * @brief trip control type
 */
enum trip_control
{
  TRIP_CONTROL_INVALID,
  TRIP_CONTROL_CONST,
  TRIP_CONTROL_CONTROL,
  TRIP_CONTROL_PULSE,
};
typedef enum trip_control trip_control;

#ifdef __cplusplus
}
#endif

#endif
