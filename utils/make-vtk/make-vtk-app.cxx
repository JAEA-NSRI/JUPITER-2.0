
#include <vtkMultiProcessController.h>
#ifdef MAKE_VTK_MPI
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#else
#include "vtkDummyController.h"
#endif

#include <jupiter/vtkiobind/bind.h>
#include "make-vtk-app.h"

JUPITERMakeVTKApp::JUPITERMakeVTKApp(int argc, char **argv)
  : result_value(1)
{
  margc = argc;
  margv = new char*[margc];
  for (int i = 0; i < margc; ++i) {
    margv[i] = argv[i];
  }
  options = 0;
}

JUPITERMakeVTKApp::~JUPITERMakeVTKApp()
{
  delete[] margv;
  jupiter_opt_delete(options);
}

bool JUPITERMakeVTKApp::ParseOptions()
{
  if (!ParseJUPITEROptions()) return false;
  if (!ParseMakeVTKOptions()) return false;
  return true;
}

bool JUPITERMakeVTKApp::ParseJUPITEROptions()
{
  options = jupiter_optparse_alloc(&margc, &margv);
  return (!!options);
}

bool JUPITERMakeVTKApp::ParseMakeVTKOptions()
{
  return mkvtk_options.parse(margc, margv);
}

void JUPITERMakeVTKApp::CallProcess(vtkMultiProcessController *controller)
{
  this->result_value = Process(controller);
}

static void start_parallel(vtkMultiProcessController *controller, void *arg)
{
  JUPITERMakeVTKApp *app;

  app = static_cast<JUPITERMakeVTKApp *>(arg);
  app->CallProcess(controller);
}

int JUPITERMakeVTKApp::Exec()
{
#ifdef MAKE_VTK_MPI
  vtkMPIController *master = vtkMPIController::New();
#else
  vtkDummyController *master = vtkDummyController::New();
#endif

  master->Initialize(&margc, &margv, 1);

  master->SetSingleMethod(start_parallel, this);
  master->SingleMethodExecute();

  master->Finalize(1);
  master->Delete();

  return result_value;
}

const char *JUPITERMakeVTKApp::JUPITERParamFile() const
{
  if (!options) return 0;

  return jupiter_opt_param(options);
}

const char *JUPITERMakeVTKApp::JUPITERFlagsFile() const
{
  if (!options) return 0;

  return jupiter_opt_flags(options);
}

const char *JUPITERMakeVTKApp::JUPITERGeomFile() const
{
  if (!options)
    return 0;

  return jupiter_opt_geom(options);
}

const char *JUPITERMakeVTKApp::JUPITERControlFile() const
{
  if (!options)
    return 0;

  return jupiter_opt_control(options);
}
