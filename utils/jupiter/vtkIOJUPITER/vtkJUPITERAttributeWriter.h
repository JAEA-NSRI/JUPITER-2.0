/* This is -*- c++ -*- source. */
/* This is vim: set ft=cpp: source. */

#ifndef vtkJUPITERAttributeWriter_h
#define vtkJUPITERAttributeWriter_h

#ifndef __cplusplus
#error "This header is C++ header"
#endif

#include <jupiter/vtkIOJUPITER/vtkJUPITERWriter.h>
#include <jupiter/vtkIOJUPITER/vtkIOJUPITER.h>

#include <vtkSetGet.h>

/**
 * Writes single attribute of the input dataset as raw binary file,
 * using JUPITER routine.
 *
 * No explicit parameter for which attribute to write. Use
 * SetInputArrayToProcess() to specify (but only supports CELL data)
 */
class VTKIOJUPITER_DECL vtkJUPITERAttributeWriter : public vtkJUPITERWriter
{
  vtkTypeMacro(vtkJUPITERAttributeWriter, vtkJUPITERWriter)

protected:
  vtkJUPITERAttributeWriter();
  virtual ~vtkJUPITERAttributeWriter();

  /**
   * Output Filename (can have `%r` notation for BYPROCESS output)
   */
  char *FileName;

  void WriteData() override;

public:
  static vtkJUPITERAttributeWriter *New();

  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
};

#endif
