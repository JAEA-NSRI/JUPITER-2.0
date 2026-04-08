#include "vtkJUPITERPolyDataVOF.h"

#include <iomanip>

#include <limits>
#include <utility>
#include <vtkAlgorithm.h>
#include <vtkBuffer.h>
#include <vtkCellData.h>
#include <vtkCellSizeFilter.h>
#include <vtkClipDataSet.h>
#include <vtkClipVolume.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIdTypeArray.h>
#include <vtkImplicitPolyDataDistance.h>
#include <vtkInformation.h>
#include <vtkLongLongArray.h>
#include <vtkNew.h>
#include <vtkOStreamWrapper.h>
#include <vtkPolyData.h>
#include <vtkSMPTools.h>
#include <vtkSetGet.h>
#include <vtkSmartPointer.h>
#include <vtkType.h>

static vtkOStreamWrapper &operator<<(vtkOStreamWrapper &os,
                                     const decltype(std::setprecision(0)) &p)
{
  os.GetOStream() << p;
  return os;
}

vtkJUPITERPolyDataVOF::vtkJUPITERPolyDataVOF()
  : CellVolumeArrayName(nullptr), CutVolumeArrayName(nullptr),
    FractionArrayName(nullptr), ClipFraction(true), InsideOut(false)
{
  SetNumberOfInputPorts(2);
  SetNumberOfOutputPorts(1);
  SetTolerance([]() -> double {
    vtkNew<vtkImplicitPolyDataDistance> f;
    return f->GetTolerance();
  }());
  SetMergeTolerance([]() -> double {
    vtkNew<vtkClipDataSet> c;
    return c->GetMergeTolerance();
  }());
  SetCellVolumeArrayName("CellVolume");
  SetCutVolumeArrayName("CutVolume");
  SetFractionArrayName("Fraction");
}

vtkJUPITERPolyDataVOF::~vtkJUPITERPolyDataVOF()
{
  SetCellVolumeArrayName(nullptr);
  SetCutVolumeArrayName(nullptr);
  SetFractionArrayName(nullptr);
}

int vtkJUPITERPolyDataVOF::FillInputPortInformation(int port,
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

int vtkJUPITERPolyDataVOF::RequestData(vtkInformation *request,
                                       vtkInformationVector **inputVector,
                                       vtkInformationVector *outputVector)
{
  vtkDataSet *input = vtkDataSet::GetData(inputVector[0]);
  vtkPolyData *poly = vtkPolyData::GetData(inputVector[1]);
  vtkDataSet *output = vtkDataSet::GetData(outputVector);

  output->ShallowCopy(input);

  vtkSmartPointer<vtkDataSet> work;
  work.TakeReference(input->NewInstance());
  work->ShallowCopy(input);

  vtkNew<vtkCellSizeFilter> sizeFilter;
  sizeFilter->ComputeAreaOff();
  sizeFilter->ComputeLengthOff();
  sizeFilter->ComputeSumOff();
  sizeFilter->ComputeVertexCountOff();
  sizeFilter->ComputeVolumeOn();
  sizeFilter->SetVolumeArrayName(CellVolumeArrayName);
  sizeFilter->SetInputDataObject(work);
  sizeFilter->Update();

  vtkNew<vtkIdTypeArray> ids;
  vtkIdType numCells = input->GetNumberOfCells();

  ids->SetNumberOfComponents(1);
  ids->SetNumberOfTuples(numCells);
  ids->SetName("OriginalId");

  vtkSMPTools::For(0, numCells, [&](vtkIdType first, vtkIdType last) {
    for (vtkIdType i = first; i < last; ++i)
      ids->SetValue(i, i);
  });

  vtkDataSet *tmp =
    vtkDataSet::SafeDownCast(sizeFilter->GetOutputDataObject(0));
  work.TakeReference(tmp->NewInstance());
  work->ShallowCopy(tmp);

  vtkCellData *workCD = work->GetCellData();
  workCD->AddArray(ids);
  vtkDoubleArray *cellVolume =
    vtkDoubleArray::SafeDownCast(workCD->GetArray(CellVolumeArrayName));

  vtkNew<vtkClipDataSet> clip;
  vtkNew<vtkImplicitPolyDataDistance> func;

  func->SetInput(poly);
  func->SetTolerance(GetTolerance());
  clip->SetClipFunction(func);
  clip->GenerateClipScalarsOff();
  clip->GenerateClippedOutputOff();
  /* func() < 0 will be removed (clipped) by default */
  clip->SetInsideOut(!GetInsideOut());
  clip->SetMergeTolerance(GetMergeTolerance());
  clip->SetInputDataObject(work);
  sizeFilter->SetVolumeArrayName(CutVolumeArrayName);
  sizeFilter->SetInputConnection(clip->GetOutputPort(0));
  sizeFilter->Update();

  this->UpdateProgress(0.66);

  vtkNew<vtkFloatArray> fraction;
  vtkNew<vtkDoubleArray> cutVolume;

  cutVolume->SetNumberOfComponents(1);
  cutVolume->SetNumberOfTuples(numCells);
  cutVolume->SetName(CutVolumeArrayName);

  fraction->SetNumberOfComponents(1);
  fraction->SetNumberOfTuples(numCells);
  fraction->SetName(FractionArrayName);

  vtkDataSet *cut =
    vtkDataSet::SafeDownCast(sizeFilter->GetOutputDataObject(0));
  vtkIdType numCutCells = cut->GetNumberOfCells();
  vtkCellData *cutCD = cut->GetCellData();
  vtkDoubleArray *cutSetVolume =
    vtkDoubleArray::SafeDownCast(cutCD->GetArray(CutVolumeArrayName));
  vtkIdTypeArray *oid =
    vtkIdTypeArray::SafeDownCast(cutCD->GetArray("OriginalId"));

  vtkSMPTools::For(0, numCells, [&](vtkIdType first, vtkIdType last) {
    for (vtkIdType i = first; i < last; ++i)
      cutVolume->SetValue(i, 0.0);
  });

  /*
   * Requires C++20 (for std::atomic<double>) to thread parallelize.
   */
  for (vtkIdType i = 0; i < numCutCells; ++i) {
    vtkIdType j = oid->GetValue(i);
    double v = cutVolume->GetValue(j);
    cutVolume->SetValue(j, v + cutSetVolume->GetValue(i));
  }

  vtkSMPTools::For(0, numCells, [&](vtkIdType first, vtkIdType last) {
    constexpr int precision{std::numeric_limits<double>::digits10 + 1};
    for (vtkIdType i = first; i < last; ++i) {
      double cut = cutVolume->GetValue(i);
      double cell = cellVolume->GetValue(i);
      double f = cut / cell;
      if (ClipFraction) {
        f = (f < 0.0) ? 0.0 : f;
        f = (f > 1.0) ? 1.0 : f;
      }
      fraction->SetValue(i, f);
    }
  });

  vtkCellData *outCD = output->GetCellData();
  outCD->AddArray(cellVolume);
  outCD->AddArray(cutVolume);
  outCD->AddArray(fraction);

  this->UpdateProgress(1.0);
  this->CheckAbort();

  return 1;
}

vtkStandardNewMacro(vtkJUPITERPolyDataVOF)
