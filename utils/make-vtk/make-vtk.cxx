
#include "make-vtk-app.h"

#include <iostream>
#include <jupiter/vtkIOJUPITER/vtkJUPITERReader.h>

#include <vtkDataObject.h>
#include <vtkGarbageCollector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>
#include <vtkSetGet.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVersionMacros.h>
#include <vtkWriter.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkXMLMultiBlockDataWriter.h>
#include <vtkXMLPDataObjectWriter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLRectilinearGridWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkXMLWriter.h>

#ifdef MAKE_VTK_MPI
#include <mpi.h>

#include <vtkXMLPDataSetWriter.h>
#include <vtkXMLPMultiBlockDataWriter.h>
#include <vtkXMLPPolyDataWriter.h>
#include <vtkXMLPRectilinearGridWriter.h>
#include <vtkXMLPUnstructuredGridWriter.h>
#endif

class MakePVTRApp : public JUPITERMakeVTKApp
{
  template <typename BaseWriter, typename... Args> class WriterGenerator;

  template <typename BaseWriter, typename Writer, typename DataType,
            typename... Args>
  class WriterGenerator<BaseWriter, DataType, Writer, Args...>
  {
  public:
    BaseWriter *operator()(vtkDataObject *object) const
    {
      if (DataType::SafeDownCast(object))
        return Writer::New();
      return WriterGenerator<BaseWriter, Args...>{}(object);
    }
  };

  template <typename BaseWriter>
  class WriterGenerator<BaseWriter>
  {
  public:
    BaseWriter *operator()(vtkDataObject *object) const
    {
      return nullptr;
    }
  };

public:
  MakePVTRApp(int argc, char **argv) : JUPITERMakeVTKApp(argc, argv) {}

  virtual int Process(vtkMultiProcessController *controller) override {
    vtkSmartPointer<vtkJUPITERReader> reader =
      vtkSmartPointer<vtkJUPITERReader>::New();

    reader->SetController(controller);
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
    reader->FindTimeFilesOff();

    std::string &inpdir = mkvtk_options.input_directory;
    std::string &outdir = mkvtk_options.output_directory;

    if (!inpdir.empty()) {
      reader->SetInputDirectory(inpdir.c_str());
    }

    reader->UpdateDataObject();
    vtkDataObject *object = reader->GetOutputDataObject(0);

    vtkSmartPointer<vtkXMLWriter> writer;

#ifdef MAKE_VTK_MPI
    int rank = controller->GetLocalProcessId();
    int nprc = controller->GetNumberOfProcesses();
#endif

    using SingleWriterGenerator = WriterGenerator<
      vtkXMLWriter, vtkMultiBlockDataSet, vtkXMLMultiBlockDataWriter,
      vtkRectilinearGrid, vtkXMLRectilinearGridWriter, vtkPolyData,
      vtkXMLPolyDataWriter, vtkUnstructuredGrid, vtkXMLUnstructuredGridWriter>;

#ifdef MAKE_VTK_MPI
    if (nprc > 1) {
      /* vtkXMLPMultiBlockDataWriter does not share base */
      if (vtkMultiBlockDataSet::SafeDownCast(object)) {
        vtkXMLPMultiBlockDataWriter *mwriter;
        mwriter = vtkXMLPMultiBlockDataWriter::New();
        mwriter->SetNumberOfPieces(nprc);
        mwriter->SetStartPiece(rank);
        mwriter->SetController(controller);
        mwriter->SetWriteMetaFile(rank == 0);
        writer = vtkSmartPointer<vtkXMLWriter>::Take(mwriter);
      } else {
        vtkXMLPDataObjectWriter *mwriter;
        mwriter =
          WriterGenerator<vtkXMLPDataObjectWriter, vtkRectilinearGrid,
                          vtkXMLPRectilinearGridWriter, vtkPolyData,
                          vtkXMLPPolyDataWriter, vtkUnstructuredGrid,
                          vtkXMLPUnstructuredGridWriter>{}(object);

        mwriter->SetController(controller);
        mwriter->SetNumberOfPieces(nprc);
        mwriter->SetStartPiece(rank);
        mwriter->SetEndPiece(rank);
#if defined(VTK_MAJOR_VERSION) && VTK_MAJOR_VERSION >= 8
        mwriter->SetUseSubdirectory(true);
#endif
        writer = vtkSmartPointer<vtkXMLWriter>::Take(mwriter);
      }
    } else {
      writer =
        vtkSmartPointer<vtkXMLWriter>::Take(SingleWriterGenerator{}(object));
    }
#else
    writer =
      vtkSmartPointer<vtkXMLWriter>::Take(SingleWriterGenerator{}(object));
#endif
    writer->SetInputConnection(reader->GetOutputPort());
    writer->SetDataModeToAppended();
    writer->SetCompressorTypeToZLib();
    writer->SetEncodeAppendedData(false);
    //writer->SetDataModeToAscii();

    if (mkvtk_options.numbers.empty()) {
      std::cerr << "No time indices specified\n";
      return 1;
    }

    int r = 0;
    for (const int &iout : mkvtk_options.numbers) {
      reader->SetTimeKey(iout);

      if (writer) {
        std::ostringstream os;
        const char *ext = writer->GetDefaultFileExtension();

        if (iout >= 0) {
          std::cout << "Processing " << iout << std::endl;
          os << outdir << "/" << "time_" << iout << "." << ext;
        } else {
          std::cout << "Processing restart data" << std::endl;
          os << outdir << "/" << "restart_data" << "." << ext;
        }

        writer->SetFileName(os.str().c_str());

        std::cout << "Writing " << writer->GetFileName() << std::endl;
        writer->Write();
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

  MakePVTRApp app(argc, argv);
  if (app.ParseOptions()) {
    app.Exec();
  }

  vtkGarbageCollector::Collect();

#ifdef MAKE_VTK_MPI
  MPI_Finalize();
#endif
  return app.ResultValue();
}
