/* This is -*- c++ -*- source. */
/* This is vim: set ft=cpp: source. */

#ifndef vtkJUPITERDataSetWriter_h
#define vtkJUPITERDataSetWriter_h

#ifndef __cplusplus
#error "This header is C++ header"
#endif

#include "jupiter/vtkIOJUPITER/vtkIOJUPITER.h"
#include "jupiter/vtkIOJUPITER/vtkJUPITERWriter.h"

#include <vtkDataArray.h>
#include <vtkDataObject.h>
#include <vtkSetGet.h>

/**
 * Writes all attributes in restart data structure
 *
 * Note: In current version, metedata (e.g., time and comp_data) and particles
 * are not written (and vtkRectilinearGrid or vtkImageData is required).
 *
 * If an (cell) attribute array has JUPITER extended info
 * `vtkJUPITERReader::RAW_NAME()`, use it for writing filename. Otherwise, if
 * the name of array is in format of
 * `[valid-C-identifier][:[component-number]]`, uses it for writing filename.
 */
class VTKIOJUPITER_DECL vtkJUPITERDataSetWriter : public vtkJUPITERWriter
{
  vtkTypeMacro(vtkJUPITERDataSetWriter, vtkJUPITERWriter)

protected:
  vtkJUPITERDataSetWriter();
  virtual ~vtkJUPITERDataSetWriter() override;

  int TimeKey;
  char *OutputDirectory;
  char *SingleFileNamePattern;
  char *MultiFileNamePattern;

  virtual void WriteArray(vtkDataObject *source, vtkDataArray *array);
  void WriteData() override;

public:
  static vtkJUPITERDataSetWriter *New();

  /**
   * Embedding value for `%n` for filename pattern
   *
   * @note `-1` will be embedded as-is (because no multiple patterns are not
   * assignable).
   */
  vtkSetMacro(TimeKey, int);
  vtkGetMacro(TimeKey, int);

  vtkSetStringMacro(OutputDirectory);
  vtkGetStringMacro(OutputDirectory);

  /**
   * File name pattern if array does not have a component index
   */
  vtkSetStringMacro(SingleFileNamePattern);
  vtkGetStringMacro(SingleFileNamePattern);

  /**
   * File name pattern if array has a component index
   */
  vtkSetStringMacro(MultiFileNamePattern);
  vtkGetStringMacro(MultiFileNamePattern);
};

#endif
