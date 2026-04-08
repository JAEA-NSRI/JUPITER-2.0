/* This is -*- c++ -*- source. */
/* This is vim: set ft=cpp: source. */

#ifndef vtkJUPITERWriter_h
#define vtkJUPITERWriter_h

#ifndef __cplusplus
#error "This header is C++ header"
#endif

#include <jupiter/vtkIOJUPITER/vtkIOJUPITER.h>

#include <vtkDataArray.h>
#include <vtkDataObject.h>
#include <vtkIntArray.h>
#include <vtkMultiProcessController.h>
#include <vtkSetGet.h>
#include <vtkWriter.h>

/**
 * Base class of JUPITER data writer
 */
class VTKIOJUPITER_DECL vtkJUPITERWriter : public vtkWriter
{
  vtkTypeMacro(vtkJUPITERWriter, vtkWriter)

public:
  enum OutputMode
  {
    BYPROCESS,    /*!< BINARY_OUTPUT_BYPROCESS */
    UNIFY_MPI,    /*!< BINARY_OUTPUT_UNIFY_MPI */
    UNIFY_GATHER, /*!< BINARY_OUTPUT_UNIFY_GATHER */
  };

protected:
  vtkMultiProcessController *Controller;
  OutputMode Mode;
  int WholeExtent[6];

  vtkJUPITERWriter();
  virtual ~vtkJUPITERWriter() override;

  virtual void WriteArray(const char *filename, const int data_extent[6],
                          const int write_extent[6], vtkDataArray *array);

  virtual void WriteArray(const char *filename, const int data_extent[6],
                          int number_of_ghost_levels, vtkDataArray *array);

  virtual void WriteArray(const char *filename, vtkDataObject *object,
                          vtkDataArray *array);

  int FillInputPortInformation(int port, vtkInformation *info) override;

public:
  vtkSetObjectMacro(Controller, vtkMultiProcessController);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  vtkSetEnumMacro(Mode, OutputMode);
  vtkGetEnumMacro(Mode, OutputMode);

  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);

  /**
   * Reduce structured number of processes from extents of each ranks
   *
   * Assumes number of tuples in each_rank_extents to be number of processes.
   * Number of compoents in each_rank_extents must be 6.
   */
  static int ComputeStructuredNumberOfPrcoess(vtkIntArray *each_rank_extents,
                                              int *npex, int *npey, int *npez);

  /**
   * Compute structured rank number of @p rank
   */
  static int ComputeStructuredRankNumber(int npex, int npey, int npez, int rank,
                                         int *rank_x, int *rank_y, int *rank_z);

  /**
   * Reduce strutured number of process and rank number from local extents.
   *
   * This function is collective.
   */
  static int ComputeStructuredNumberOfPrcoess(
    const int local_extent[6], vtkMultiProcessController *controller, int *npex,
    int *npey, int *npez, int *rank_x, int *rank_y, int *rank_z);

  /**
   * Get unit size (number of bytes per AOS tuple)
   */
  static int GetUnitSizeOfArray(vtkDataArray *array);

  /**
   * Get element size of array (size of value type)
   */
  static size_t GetElementSizeOfArray(vtkDataArray *array);
};

#endif
