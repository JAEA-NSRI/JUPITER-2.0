#include "vtkJUPITERDataSetWriter.h"

#include "jupiter/func.h"
#include "jupiter/struct.h"
#include "jupiter/vtkIOJUPITER/vtkJUPITERReader.h"
#include "jupiter/vtkIOJUPITER/vtkJUPITERWriter.h"

#include <regex>
#include <string>
#include <vtkObjectFactory.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>

vtkJUPITERDataSetWriter::vtkJUPITERDataSetWriter() : TimeKey(-1),
  SingleFileNamePattern(nullptr), MultiFileNamePattern(nullptr),
  OutputDirectory(nullptr)
{
  SetSingleFileNamePattern("%c/%c.dat");
  SetMultiFileNamePattern("%c_%i/%c_%i.dat");
}

vtkJUPITERDataSetWriter::~vtkJUPITERDataSetWriter()
{
  SetSingleFileNamePattern(nullptr);
  SetMultiFileNamePattern(nullptr);
  SetOutputDirectory(nullptr);
}

void vtkJUPITERDataSetWriter::WriteArray(vtkDataObject *source,
                                         vtkDataArray *array)
{
  vtkInformation *info = array->GetInformation();
  std::string comp_name_buff;
  const char *comp_name = nullptr;
  int comp_id = -1;

  if (info->Has(vtkJUPITERReader::RAW_NAME())) {
    comp_name = info->Get(vtkJUPITERReader::RAW_NAME());
    if (info->Has(vtkJUPITERReader::COMPONENT_INDEX()))
      comp_id = info->Get(vtkJUPITERReader::COMPONENT_INDEX());
  } else {
    std::regex fname_pat("([a-zA-Z_][a-zA-Z0-9_]*)(:([1-9][0-9]*))?");
    std::cmatch m;
    if (std::regex_match(array->GetName(), m, fname_pat)) {
      if (m[1].matched) {
        comp_name_buff = m[1].str();
        comp_name = comp_name_buff.c_str();
        if (m[3].matched)
          comp_id = std::stoi(m[3].str());
      }
    }
  }

  if (!comp_name) {
    vtkWarningMacro(<< "Array '" << array->GetName()
                    << "' did not meet the array name contract. Skip.");
    return;
  }

  char *fname;
  int rank = Controller ? Controller->GetLocalProcessId() : 0;
  binary_output_mode out_mode;
  switch (Mode) {
  case BYPROCESS:
    out_mode = BINARY_OUTPUT_BYPROCESS;
    break;
  case UNIFY_MPI:
    out_mode = BINARY_OUTPUT_UNIFY_MPI;
    break;
  case UNIFY_GATHER:
    out_mode = BINARY_OUTPUT_UNIFY_GATHER;
    break;
  }

  int r = make_file_name(&fname, OutputDirectory,
                         (comp_id >= 0) ? MultiFileNamePattern
                                        : SingleFileNamePattern,
                         comp_name, comp_id, -1, rank, out_mode);
  if (r < 0) {
    vtkErrorMacro(<< "Failed to make output file name for array: " << *array);
    return;
  }

  try {
    std::string dir = vtksys::SystemTools::GetParentDirectory(fname);
    vtksys::SystemTools::MakeDirectory(dir);

    vtkJUPITERWriter::WriteArray(fname, source, array);
  } catch (...) {
    free(fname);
    throw;
  }

  free(fname);
}

void vtkJUPITERDataSetWriter::WriteData()
{
  vtkDataSet *object = vtkDataSet::SafeDownCast(GetInput());
  vtkCellData *cdata = object->GetCellData();

  int narray = cdata->GetNumberOfArrays();
  for (int iarray = 0; iarray < narray; ++iarray) {
    vtkAbstractArray *aary = cdata->GetAbstractArray(iarray);
    vtkDataArray *dary = vtkDataArray::SafeDownCast(aary);
    if (!dary)
      continue;

    WriteArray(object, dary);
  }
}

vtkStandardNewMacro(vtkJUPITERDataSetWriter)
