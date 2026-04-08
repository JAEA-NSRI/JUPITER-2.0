#include "jupiter/cxxoptparse/exceptions.h"
#include "jupiter/cxxoptparse/value_printer.h"
#include "jupiter/vtkIOJUPITER/vtkJUPITERAttributeWriter.h"
#include <iomanip>
#include <jupiter/cxxoptparse/enum.h>
#include <jupiter/cxxoptparse/optparse.h>
#include <jupiter/cxxoptparse/types.h>

#include <array>
#include <iostream>
#include <ostream>

#include <jupiter/vtkIOJUPITER/vtkJUPITERReader.h>
#include <jupiter/vtkJUPITER/vtkJUPITERPolyDataLevelSet.h>
#include <jupiter/vtkJUPITER/vtkJUPITERPolyDataVOF.h>

#include <vtkAlgorithm.h>
#include <vtkBuffer.h>
#include <vtkCommunicator.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkObject.h>
#include <vtkSMPTools.h>
#include <vtkSTLReader.h>
#include <vtkCommand.h>
#include <vtkSetGet.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVersion.h>
#include <vtkVersionQuick.h>
#include <vtkXMLRectilinearGridWriter.h>
#include <vtkXMLWriter.h>

#ifdef JUPITER_MPI
#include <vtkMPIController.h>
#include <vtkXMLPRectilinearGridWriter.h>
#else
#include <vtkDummyController.h>
#endif

enum process_mode
{
  STL2BIN_LevelSet,
  STL2BIN_VOF,
};

template <process_mode value> class process_mode_trait;

JUPITER_CXXOPTPARSE_ENUM2STR(process_mode_trait, process_mode, STL2BIN_LevelSet,
                             "LevelSet",
                             "Use distance to surface\n(outputs only 0 or 1)")
JUPITER_CXXOPTPARSE_ENUM2STR(
  process_mode_trait, process_mode, STL2BIN_VOF, "VOF",
  "Compute volume fraction of cut cell\n(outputs fractions)")

using process_mode_tr = std::tuple<process_mode_trait<STL2BIN_LevelSet>,
                                   process_mode_trait<STL2BIN_VOF>>;

template <> class JUPITER::option_parser::value_print<process_mode>
{
public:
  static std::ostream &print(std::ostream &os, const process_mode &value)
  {
    using enum2str = JUPITER::option_parser::core::enum2str<process_mode_tr>;
    return os << enum2str{}(value);
  }
};

namespace options
{
using namespace JUPITER::option_parser;

class input_file : public string_option<>
{
public:
  static constexpr char short_option_name = 'i';
  static constexpr auto &long_option_name = "input";
  static std::string value_name() { return "FILE"; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "JUPITER parameter input filename";
  }
};

class flags_file : public string_option<>
{
public:
  static constexpr char short_option_name = 'f';
  static constexpr auto &long_option_name = "flags";
  static std::string value_name() { return "FILE"; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "JUPITER flags input filename";
  }
};

class stl_input : public string_option<>
{
public:
  static constexpr char short_option_name = 's';
  static constexpr auto &long_option_name = "stl-input";
  static std::string value_name() { return "FILE"; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "STL input filename";
  }
};

class bin_output : public string_option<>
{
public:
  static constexpr char short_option_name = 'o';
  static constexpr auto &long_option_name = "bin-output";
  static std::string value_name() { return "FILE"; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Converted binary output filename";
  }
};

class vtk_output : public string_option<>
{
public:
  static constexpr auto &long_option_name = "vtk-output";
  static std::string value_name() { return "FILE"; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "VTK XML dataset output filename (.vtr/.pvtr)";
  }
};

class process_mode_option : public enum_option<process_mode_tr>
{
public:
  static constexpr char short_option_name = 'M';
  static constexpr auto &long_option_name = "process-mode";
  static std::string value_name() { return "MODE"; }
  static process_mode default_value() { return STL2BIN_LevelSet; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Compute mode";
  }
};

using finite_double = finite_double_option_property<double>;
class transform : public array_option<double_option<double, finite_double>, 16>
{
public:
  static constexpr char short_option_name = 'T';
  static constexpr auto &long_option_name = "transform";
  static std::string value_name() { return "a11,a12,...,a44"; }

  static std::array<double, 16> default_value()
  {
    return std::array<double, 16>{
      1.0, 0.0, 0.0, 0.0, //
      0.0, 1.0, 0.0, 0.0, //
      0.0, 0.0, 1.0, 0.0, //
      0.0, 0.0, 0.0, 1.0, //
    };
  }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Transformation matrix for STL model";
  }
};

class stl_origin : public array_option<double_option<double, finite_double>, 3>
{
public:
  static constexpr char short_option_name = 'O';
  static constexpr auto &long_option_name = "stl-origin";
  static std::string value_name() { return "X,Y,Z"; }

  static std::array<double, 3> default_value()
  {
    return std::array<double, 3>{0.0, 0.0, 0.0};
  }

  static std::ostream &description(std::ostream &os)
  {
    return os << "The origin position of the STL model in the JUPITER "
                 "coordinate system";
  }
};

class stl_scale : public array_option<double_option<double, finite_double>, 3>
{
public:
  static constexpr char short_option_name = 'S';
  static constexpr auto &long_option_name = "stl-scale";
  static std::string value_name() { return "X,Y,Z"; }

  static std::array<double, 3> default_value()
  {
    return std::array<double, 3>{1.0, 1.0, 1.0};
  }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Scale of STL model\n(The required value is 1 meter in the "
                 "STL model's length units. For example, if STL was created in "
                 "millimeters, the value will be 1000.)";
  }
};

class inside_out : public bool_option
{
public:
  static constexpr auto &long_option_name = "inside-out";
  static bool default_value() { return false; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Generate value inside-out";
  }
};

class stl_relaxed_conformance : public bool_option
{
public:
  static constexpr auto &long_option_name = "stl-relaxed-conformance";

  /*
   * They only provide doxygen manual for nightly snapshot (We need to build
   * them to get for specific versions), and we does not test for released
   * versions. So this #if condition is inaccurate.
   */
#if VTK_VERSION_NUMBER_QUICK >= VTK_VERSION_CHECK(9, 6, 0)
  static bool default_value()
  {
    vtkNew<vtkSTLReader> r;
    return r->GetRelaxedConformance();
  }
#endif

  static std::ostream &description(std::ostream &os)
  {
#if VTK_VERSION_NUMBER_QUICK >= VTK_VERSION_CHECK(9, 6, 0)
    return os << "Turn on to support malformed files";
#else
    return os << "(this option is not supported in vtk "
              << vtkVersion::GetVTKVersion() << ")";
#endif
  }
};

class stl_merging : public bool_option
{
public:
  static constexpr auto &long_option_name = "stl-merging";

  static bool default_value()
  {
    vtkNew<vtkSTLReader> r;
    return r->GetMerging();
  }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Turn on/off the merging of coincident points to restore "
                 "neighborhood information";
  }
};

class tolerance : public double_option<double, finite_double>
{
public:
  static constexpr auto &long_option_name = "tolerance";
  static std::string value_name() { return "VALUE"; }

  static double default_value()
  {
    vtkNew<vtkJUPITERPolyDataLevelSet> s;
    return s->GetTolerance();
  }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Point locator tolerance for STL model";
  }
};

class merge_tolerance : public double_option<double, finite_double>
{
public:
  static constexpr auto &long_option_name = "merge-tolerance";
  static std::string value_name() { return "VALUE"; }

  static double default_value()
  {
    vtkNew<vtkJUPITERPolyDataVOF> s;
    return s->GetMergeTolerance();
  }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Cell merge tolerance for clipping mesh\n"
                 "This option is applicable only for `VOF` mode";
  }
};

class clip_fraction : public bool_option
{
public:
  static constexpr auto &long_option_name = "clip-fraction";

  static bool default_value()
  {
    vtkNew<vtkJUPITERPolyDataVOF> s;
    return s->GetClipFraction();
  }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Whether clip resulting fraction to [0, 1]\n"
                 "This option is applicable only for `VOF` mode";
  }
};

class print_matrix : public bool_option
{
public:
  static constexpr auto &long_option_name = "print-matrix";

  static std::ostream &description(std::ostream &os)
  {
    return os << "Print transformation matrix for STL model";
  }
};

class num_threads_int : public default_int_option_property<int>
{
public:
  static constexpr int min = 0;
};

class num_threads : public int_option<int, num_threads_int>
{
public:
  static constexpr auto &long_option_name = "num-threads";
  static constexpr char short_option_name = 't';
  static std::string value_name() { return "N"; }
  static int default_value() { return 0; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Number of threads to use for thread-parallel processing\n"
                 "(Backend: "
              << vtkSMPTools::GetBackend() << ")";
  }

  static std::ostream &option_value_help(std::ostream &os)
  {
    os << " * 0: Auto\n";
    return int_option<int, num_threads_int>::option_value_help(os);
  }
};

class verbose : public bool_option
{
public:
  static constexpr auto &long_option_name = "verbose";
  static constexpr char short_option_name = 'v';

  static std::ostream &description(std::ostream &os)
  {
    return os << "Be verbose (show progress)";
  }
};

using stl2bin_parser = JUPITER::option_parser::parser<
  input_file, flags_file, stl_input, bin_output, vtk_output,
  process_mode_option, inside_out, stl_origin, stl_scale, transform,
  stl_relaxed_conformance, stl_merging, tolerance, merge_tolerance,
  clip_fraction, print_matrix, num_threads, verbose>;
} // namespace options

struct func_data
{
  options::stl2bin_parser p;
  int retval;
};

class stl2bin_progress : public vtkCommand
{
  vtkTypeMacro(stl2bin_progress, vtkCommand)

public:
  vtkMultiProcessController *Controller;

  static stl2bin_progress *New();
  stl2bin_progress() : Controller(nullptr) {}
  virtual ~stl2bin_progress() override {}

  void Execute(vtkObject *caller, unsigned long eventId,
               void *callData) override
  {
    if (Controller) {
      std::clog << "[" << Controller->GetLocalProcessId() << "] ";
    }
    std::clog << "Progressing " << caller->GetClassName() << " ("
              << std::setw(5) << std::setprecision(1) << std::fixed
              << *(double *)callData * 100.0 << "%)...\n";
  }
};

vtkStandardNewMacro(stl2bin_progress)

class stl2bin_process : public vtkObject
{
  vtkTypeMacro(stl2bin_process, vtkObject)

public:
  static stl2bin_process *New();

  void Exec(vtkMultiProcessController *controller, func_data *data)
  {
    vtkSMPTools::Initialize(data->p.get<options::num_threads>());

    std::string &input_file = data->p.get<options::input_file>();
    std::string &flags_file = data->p.get<options::flags_file>();
    std::string &stl_input_file = data->p.get<options::stl_input>();
    std::string &bin_output_file = data->p.get<options::bin_output>();
    std::string &vtk_output_file = data->p.get<options::vtk_output>();

    if (input_file.empty()) {
      vtkErrorMacro(<< "No JUPITER parameter input file given");
      data->retval = 1;
    }
    if (flags_file.empty()) {
      vtkErrorMacro(<< "No JUPITER flags input file given");
      data->retval = 1;
    }
    if (stl_input_file.empty()) {
      vtkErrorMacro(<< "No STL input file given");
      data->retval = 1;
    }
    if (bin_output_file.empty() && vtk_output_file.empty()) {
      vtkErrorMacro(<< "No VTK nor binary output file given");
      data->retval = 1;
    }
    if (controller) {
      int t;
      controller->AllReduce(&data->retval, &t, 1, vtkCommunicator::MAX_OP);
      data->retval = t;
    }
    if (data->retval)
      return;

    vtkSmartPointer<stl2bin_progress> progress;
    if (data->p.get<options::verbose>())
      progress = vtkSmartPointer<stl2bin_progress>::New();

    vtkNew<vtkJUPITERReader> jupiter_reader;
    vtkNew<vtkSTLReader> stl_reader;
    vtkNew<vtkTransformPolyDataFilter> transform_filter;
    vtkNew<vtkTransform> transform;
    vtkNew<vtkTransform> origin_transform;
    vtkNew<vtkTransform> scale_transform;
    vtkSmartPointer<vtkJUPITERPolyDataLevelSet> lset;
    vtkSmartPointer<vtkJUPITERPolyDataVOF> vof;

    if (progress) {
      progress->Controller = controller;
      jupiter_reader->AddObserver(vtkCommand::ProgressEvent, progress);
      stl_reader->AddObserver(vtkCommand::ProgressEvent, progress);
      transform_filter->AddObserver(vtkCommand::ProgressEvent, progress);
    }

    jupiter_reader->SetController(controller);
    jupiter_reader->SetFlagsFile(flags_file.c_str());
    jupiter_reader->SetParamFile(input_file.c_str());
    jupiter_reader->ReadMeshOnlyOn();

    stl_reader->SetFileName(stl_input_file.c_str());
#if VTK_VERSION_NUMBER_QUICK >= VTK_VERSION_CHECK(9, 6, 0)
    stl_reader->SetRelaxedConformance(
      data->p.get<options::stl_relaxed_conformance>());
#endif
    stl_reader->SetMerging(data->p.get<options::stl_merging>());

    scale_transform->Scale(data->p.get<options::stl_scale>().data());
    scale_transform->Inverse();

    origin_transform->PostMultiply();
    origin_transform->SetInput(scale_transform);
    origin_transform->Translate(data->p.get<options::stl_origin>().data());

    transform->SetMatrix(data->p.get<options::transform>().data());
    transform->SetInput(origin_transform);

    if (data->p.get<options::print_matrix>()) {
      bool b = this->GetDebug();
      this->DebugOn();
      vtkDebugMacro(<< "Transformation matrix: \n" << *transform->GetMatrix());
      this->SetDebug(b);
    }

    transform_filter->SetInputConnection(stl_reader->GetOutputPort(0));
    transform_filter->SetTransform(transform);

    vtkAlgorithm *output = nullptr;
    switch (data->p.get<options::process_mode_option>()) {
    case STL2BIN_LevelSet:
      lset = vtkSmartPointer<vtkJUPITERPolyDataLevelSet>::New();
      lset->SetInsideOut(data->p.get<options::inside_out>());
      lset->SetTolerance(data->p.get<options::tolerance>());
      output = lset;
      break;
    case STL2BIN_VOF:
      vof = vtkSmartPointer<vtkJUPITERPolyDataVOF>::New();
      vof->SetInsideOut(data->p.get<options::inside_out>());
      vof->SetTolerance(data->p.get<options::tolerance>());
      vof->SetMergeTolerance(data->p.get<options::merge_tolerance>());
      vof->SetClipFraction(data->p.get<options::clip_fraction>());
      output = vof;
      break;
    }
    if (!output) {
      vtkErrorMacro(<< "Invalid process mode used");
      data->retval = 1;
      return;
    }

    if (progress) {
      output->AddObserver(vtkCommand::ProgressEvent, progress);
    }
    output->SetInputConnection(0, jupiter_reader->GetOutputPort(0));
    output->SetInputConnection(1, transform_filter->GetOutputPort(0));
    output->Update();
    if (output->GetErrorCode() != 0) {
      data->retval = 1;
      return;
    }

    if (!vtk_output_file.empty()) {
#ifdef JUPITER_MPI
      vtkSmartPointer<vtkXMLWriter> vtkwriter;
      if (controller->GetNumberOfProcesses() > 1) {
        int ir = controller->GetLocalProcessId();
        vtkXMLPRectilinearGridWriter *writer;
        writer = vtkXMLPRectilinearGridWriter::New();
        writer->SetController(controller);
        writer->SetNumberOfPieces(controller->GetNumberOfProcesses());
        writer->SetStartPiece(ir);
        writer->SetEndPiece(ir);
        vtkwriter.TakeReference(writer);
      } else {
        vtkXMLRectilinearGridWriter *writer;
        writer = vtkXMLRectilinearGridWriter::New();
        vtkwriter.TakeReference(writer);
      }
#else
      vtkNew<vtkXMLRectilinearGridWriter> vtkwriter;
#endif
      if (progress)
        vtkwriter->AddObserver(vtkCommand::ProgressEvent, progress);
      vtkwriter->SetFileName(vtk_output_file.c_str());
      vtkwriter->SetInputConnection(output->GetOutputPort(0));
      vtkwriter->Write();
    }

    if (!bin_output_file.empty()) {
      vtkNew<vtkJUPITERAttributeWriter> writer;
      if (progress)
        writer->AddObserver(vtkCommand::ProgressEvent, progress);
      writer->SetInputConnection(output->GetOutputPort(0));
      writer->SetInputArrayToProcess("Fraction", vtkDataObject::CELL);
      writer->SetFileName(bin_output_file.c_str());
      writer->SetController(controller);
      writer->SetMode(vtkJUPITERWriter::UNIFY_MPI);
      writer->Write();
    }
  }
};

vtkStandardNewMacro(stl2bin_process);

static void parallel_func(vtkMultiProcessController *controller, void *arg)
{
  func_data *data = static_cast<func_data *>(arg);
  vtkNew<stl2bin_process> proc;
  proc->Exec(controller, data);
}

int main(int argc, char **argv)
{
  func_data data;
  data.retval = 0;

#ifdef JUPITER_MPI
  vtkNew<vtkMPIController> master;
#else
  vtkNew<vtkDummyController> master;
#endif

  master->Initialize(&argc, &argv);

  try {
    data.p.parse(argc, argv);
  } catch (const JUPITER::option_parser::invalid_option_error &e) {
    std::cerr << argv[0] << ": " << e.what() << "\n";
    std::exit(1);
  }
  if (data.p.get_help_value().get_help_printed())
    return 1;

  master->SetSingleMethod(parallel_func, &data);
  master->SingleMethodExecute();
  master->Finalize();
  return data.retval;
}
