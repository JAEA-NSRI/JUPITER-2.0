
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>

#include <jupiter/vtkiobind/bind.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#include "make-vtk-app.h"
#include "jupiter/vtkIOJUPITER/vtkJUPITERReader.h"

#include "vtkSmartPointer.h"

#ifdef MAKE_VTK_MPI
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkPXdmf3Writer.h"
#endif
#include "vtkXdmf3Writer.h"

#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkGarbageCollector.h"

class MakeXdmfApp : public JUPITERMakeVTKApp
{
public:
  MakeXdmfApp(int argc, char **argv) : JUPITERMakeVTKApp(argc, argv) {}

  virtual int Process(vtkMultiProcessController *controller) override {
    vtkSmartPointer<vtkJUPITERReader> reader =
      vtkSmartPointer<vtkJUPITERReader>::New();

    reader->SetFlagsFile(JUPITERFlagsFile());
    reader->SetParamFile(JUPITERParamFile());
    reader->SetGeomFile(JUPITERGeomFile());
    reader->SetControlFile(JUPITERControlFile());
    if (mkvtk_options.fluid) {
      reader->ProcessFluidOn();
    } else {
      reader->ProcessFluidOff();
    }
    if (mkvtk_options.particle) {
      reader->ProcessParticleOn();
    } else {
      reader->ProcessParticleOff();
    }
    if (mkvtk_options.read_geom_dump) {
      reader->ReadGeomDumpOn();
    } else {
      reader->ReadGeomDumpOff();
    }
    if (mkvtk_options.use_raw_name) {
      reader->UseRawDataNameOn();
    } else {
      reader->UseRawDataNameOff();
    }
    if (mkvtk_options.include_ext_info) {
      reader->IncludeExtendedInformationOn();
    } else {
      reader->IncludeExtendedInformationOff();
    }
    reader->GenerateMultiBlockOff();

    std::string &inpdir = mkvtk_options.input_directory;
    std::string &outdir = mkvtk_options.output_directory;

    if (!inpdir.empty()) {
      reader->SetInputDirectory(inpdir.c_str());
    }

    vtkSmartPointer<vtkXdmf3Writer> writer;

#ifdef MAKE_VTK_MPI
    int rank = controller->GetLocalProcessId();
    int nprc = controller->GetNumberOfProcesses();

    if (nprc > 1) {
      writer = vtkSmartPointer<vtkPXdmf3Writer>::New();
    } else {
      writer = vtkSmartPointer<vtkXdmf3Writer>::New();
    }
#else
    writer = vtkSmartPointer<vtkXdmf3Writer>::New();
#endif
    writer->SetInputConnection(reader->GetOutputPort(0));

    if (mkvtk_options.numbers.empty()) {
      std::ostringstream os;
      os << outdir << "/" << "all.xmf";

      reader->SetFindTimeFiles(true);
      writer->SetWriteAllTimeSteps(true);
      writer->SetFileName(os.str().c_str());
      writer->Write();

    } else {
      reader->FindTimeFilesOff();
      writer->SetWriteAllTimeSteps(false);

      for (int &iout : mkvtk_options.numbers) {
        reader->SetTimeKey(iout);

        if (writer) {
          std::ostringstream os;

          if (iout >= 0) {
            std::cout << "Processing " << iout << std::endl;
            os << outdir << "/" << "time_" << iout << ".xmf";
          } else {
            std::cout << "Processing restart data" << std::endl;
            os << outdir << "/" << "restart_data.xmf";
          }

          writer->SetFileName(os.str().c_str());
          writer->Write();
        }
      }
    }

    return 0;
  }
};

int main(int argc, char **argv)
{
#ifdef MAKE_VTK_MPI
  MPI_Init(&argc, &argv);
#endif

  MakeXdmfApp app(argc, argv);
  if (app.ParseOptions()) {
    app.Exec();
  }

  vtkGarbageCollector::Collect();

#ifdef MAKE_VTK_MPI
  MPI_Finalize();
#endif
  return app.ResultValue();
}
