
#include "jupiter/vtkIOJUPITER/vtkJUPITERDataSetWriter.h"
#include "jupiter/vtkIOJUPITER/vtkJUPITERReader.h"
#include "jupiter/vtkIOJUPITER/vtkJUPITERWriter.h"

#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkDummyController.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkRectilinearGrid.h>

#ifdef JUPITER_MPI
#include <vtkMPIController.h>
#endif

int main(int argc, char **argv)
{
#ifdef JUPITER_MPI
  using controller_type = vtkMPIController;
#else
  using controller_type = vtkDummyController;
#endif
  vtkNew<controller_type> controller;
  controller->Initialize(&argc, &argv);

  vtkNew<vtkRectilinearGrid> grid;
  grid->SetDimensions(3, 3, 3);
  grid->SetExtent(0, 2, 0, 2, 0, 2);

  vtkNew<vtkFloatArray> x, y, z;
  for (auto ary : {x.Get(), y.Get(), z.Get()}) {
    ary->SetNumberOfComponents(1);
    ary->SetNumberOfTuples(3);
    for (int i = 0; i < 3; ++i)
      ary->SetValue(i, static_cast<float>(i));
  }

  grid->SetXCoordinates(x);
  grid->SetYCoordinates(y);
  grid->SetZCoordinates(z);

  vtkCellData *cdata = grid->GetCellData();
  vtkNew<vtkFloatArray> t, uvw;
  t->SetNumberOfComponents(1);
  t->SetNumberOfTuples(8);
  for (int i = 0; i < 8; ++i)
    t->SetValue(i, static_cast<float>(i) * 100.0);
  t->SetName("fs:1");

  uvw->SetNumberOfComponents(3);
  uvw->SetNumberOfTuples(8);
  for (int i = 0; i < 3 * 8; ++i)
    uvw->SetValue(i, static_cast<float>(i) * 11.0);
  uvw->SetName("Velocity [m/s]");

  vtkInformation *info = uvw->GetInformation();
  info->Set(vtkJUPITERReader::RAW_NAME(), "uuu");

  cdata->AddArray(t);
  cdata->AddArray(uvw);

  vtkNew<vtkJUPITERDataSetWriter> writer;
  writer->SetController(controller);
  writer->SetOutputDirectory("tmp/test");
  writer->SetMode(vtkJUPITERWriter::UNIFY_MPI);
  writer->SetInputDataObject(grid);
  writer->Write();

  controller->Finalize();
  return 0;
}
