#include "vtkJUPITERAttributeWriter.h"

#include <vtkObjectFactory.h>
#include <vtkDataArray.h>
#include <vtkDataObject.h>

vtkJUPITERAttributeWriter::vtkJUPITERAttributeWriter()
  : FileName(nullptr)
{
  SetNumberOfInputPorts(1);
}

vtkJUPITERAttributeWriter::~vtkJUPITERAttributeWriter()
{
  SetFileName(nullptr);
}

void vtkJUPITERAttributeWriter::WriteData()
{
  vtkDataObject *object = this->GetInput();
  vtkDataArray *array = this->GetInputArrayToProcess(0, object);
  if (array) {
    if (array->GetNumberOfTuples() ==
        object->GetNumberOfElements(vtkDataObject::CELL)) {
      WriteArray(FileName, object, array);
    } else {
      vtkErrorMacro(<< "The specified input array size does not match to "
                       "number of cells (not from cell data?)");
    }
  } else {
    vtkErrorMacro(<< "Specified input array is not found");
  }
}

vtkStandardNewMacro(vtkJUPITERAttributeWriter)
