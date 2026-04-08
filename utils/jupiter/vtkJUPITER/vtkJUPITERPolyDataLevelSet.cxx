#include "vtkJUPITERPolyDataLevelSet.h"

#include <vtkAlgorithm.h>
#include <vtkBuffer.h>
#include <vtkCellData.h>
#include <vtkDataObject.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkCellCenters.h>
#include <vtkImplicitPolyDataDistance.h>
#include <vtkType.h>
#include <vtkSMPTools.h>

vtkJUPITERPolyDataLevelSet::vtkJUPITERPolyDataLevelSet()
  : LevelSetArrayName(nullptr), FractionArrayName(nullptr), InsideOut(false)
{
  SetNumberOfInputPorts(2);
  SetNumberOfOutputPorts(1);
  SetTolerance([]() -> double {
    vtkNew<vtkImplicitPolyDataDistance> f;
    return f->GetTolerance();
  }());
  SetLevelSetArrayName("LevelSet");
  SetFractionArrayName("Fraction");
}

vtkJUPITERPolyDataLevelSet::~vtkJUPITERPolyDataLevelSet()
{
  SetLevelSetArrayName(nullptr);
  SetFractionArrayName(nullptr);
}

int vtkJUPITERPolyDataLevelSet::FillInputPortInformation(int port,
                                                         vtkInformation *info)
{
  switch (port) {
  case 0:
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    break;
  case 1:
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    break;
  }
  return 1;
}

int vtkJUPITERPolyDataLevelSet::RequestData(vtkInformation *request,
                                            vtkInformationVector **inputVector,
                                            vtkInformationVector *outputVector)
{
  vtkDataSet *input = vtkDataSet::GetData(inputVector[0]);
  vtkPolyData *poly = vtkPolyData::GetData(inputVector[1]);
  vtkDataSet *output = vtkDataSet::GetData(outputVector);

  output->ShallowCopy(input);

  vtkNew<vtkDoubleArray> cellCenterArray;
  vtkIdType numCells = input->GetNumberOfCells();

  cellCenterArray->SetNumberOfComponents(3);
  cellCenterArray->SetNumberOfTuples(numCells);
  vtkCellCenters::ComputeCellCenters(input, cellCenterArray);

  this->UpdateProgress(0.33);

  vtkNew<vtkImplicitPolyDataDistance> func;
  func->SetInput(poly);
  func->SetTolerance(GetTolerance());

  vtkNew<vtkDoubleArray> levelSet;
  vtkNew<vtkFloatArray> fraction;

  func->FunctionValue(cellCenterArray, levelSet);

  this->UpdateProgress(0.66);

  fraction->SetNumberOfComponents(1);
  fraction->SetNumberOfTuples(numCells);

  vtkSMPTools::For(0, numCells, [&](vtkIdType first, vtkIdType last) {
    for (vtkIdType i = first; i < last; ++i) {
      double t = levelSet->GetValue(i);
      if (InsideOut) {
        fraction->SetValue(i, (t > 0.0) ? 1.0 : 0.0);
      } else {
        fraction->SetValue(i, (t < 0.0) ? 1.0 : 0.0);
      }
    }
  });

  levelSet->SetName(LevelSetArrayName);
  fraction->SetName(FractionArrayName);

  vtkCellData *outCD = output->GetCellData();
  outCD->AddArray(levelSet);
  outCD->AddArray(fraction);

  this->UpdateProgress(1.0);
  this->CheckAbort();

  return 1;
}

vtkStandardNewMacro(vtkJUPITERPolyDataLevelSet);
