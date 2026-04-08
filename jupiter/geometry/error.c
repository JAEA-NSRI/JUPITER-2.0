#include "defs.h"
#include "error.h"

const char *geom_strerror(geom_error eval)
{
  switch(eval) {
  case GEOM_SUCCESS:
    return "No error";
  case GEOM_ERR_NOMEM:
    return "Allocation failed";
  case GEOM_ERR_ALREADY_REGISTERED_SURFACE_SHAPE:
    return "Surface shape is already installed";
  case GEOM_ERR_ALREADY_REGISTERED_SHAPE:
    return "Shape is already installed";
  case GEOM_ERR_ALREADY_REGISTERED_INIT_FUNC:
    return "Initialzation function is already installed";
  case GEOM_ERR_DEPENDENCY:
    return "Dependent parameter not found";
  case GEOM_ERR_HAS_POINTER:
    return "This pointer already registered";
  case GEOM_ERR_INVALID_INIT_FUNC:
    return "Invalid initialization function used";
  case GEOM_ERR_LIST_HEAD:
    return "List head specified";
  case GEOM_ERR_NOT_LIST_HEAD:
    return "List head required";
  case GEOM_ERR_OVERFLOW:
    return "Overflow detected";
  case GEOM_ERR_POINTER_NOT_FOUND:
    return "Specified pointer not registered";
  case GEOM_ERR_RANGE:
    return "Value out-of-range";
  case GEOM_ERR_SHORT_LIST:
    return "Too short list";
  case GEOM_ERR_VARIANT_TYPE:
    return "Invalid variant type";
  case GEOM_ERR_INVALID_SHAPE:
    return "Invalid geometry shape";
  case GEOM_ERR_INVALID_SHAPE_OP:
    return "Invalid or unusable operator for this shape";
  case GEOM_ERR_SHAPE_NOT_SET:
    return "Shape is not set";
  case GEOM_ERR_NO_BODY_SHAPES:
    return "No 'body' shapes defined";
  case GEOM_ERR_SHAPE_OP_SHOULD_SET:
    return "Shape operation should be `SET'";
  case GEOM_ERR_SHAPE_STACK_OVERFLOW:
    return "Shape stack is overflowed";
  case GEOM_ERR_SHAPE_STACK_UNDERFLOW:
    return "Shape stack is underflowed";
  case GEOM_ERR_SHAPE_STACK_UNCLOSED:
    return "Uncombined shapes found";
  case GEOM_ERR_GROUP_STACK_OVERFLOW:
    return "Group stack is overflowed";
  case GEOM_ERR_GROUP_STACK_UNDERFLOW:
    return "Group stack is underflowed";
  case GEOM_ERR_GROUP_STACK_UNCLOSED:
    return "Unclosed group found";
  case GEOM_ERR_NO_SHAPES_TO_TRANSFORM:
    return "There is no shapes to transform";
  case GEOM_ERR_SINGULAR_TRANSFORM:
    return "Transformation matrix has high singularity";
  case GEOM_ERR_COMB_WITHOUT_PUSH:
    return "COMB found without any PUSH/SET shapes";
  case GEOM_ERR_INVALID_STRUCTURE:
    return "Invalid structure found";
  case GEOM_ERR_INVALID_SURFACE_SHAPE:
    return "Invalid surface shape used";
  case GEOM_ERR_NO_SURFACE_IN_SHAPE:
    return "No surfaces are available in the shape";
  case GEOM_ERR_NO_ENABLED_SURFACE:
    return "No surfaces are enabled in the shape";
  case GEOM_ERR_INACCURATE_SURFACE:
    return "Inaccurate surface point has been returned";
  }
  return "Unknown geometry error";
}
