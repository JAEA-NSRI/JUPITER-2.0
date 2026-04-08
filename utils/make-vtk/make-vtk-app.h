
#ifndef makevtkapp_h
#define makevtkapp_h

#include "jupiter/optparse.h"
#include "vtkMultiProcessController.h"

#include "make-vtk-optparse.h"

class JUPITERMakeVTKApp
{
protected:
  int margc;
  char **margv;
  jupiter_options *options;
  MakeVTKOptions mkvtk_options;
  int result_value;

public:
  JUPITERMakeVTKApp(int argc, char **argv);
  ~JUPITERMakeVTKApp();

  bool ParseJUPITEROptions();
  bool ParseMakeVTKOptions();
  virtual bool ParseOptions();

  const char *JUPITERParamFile() const;
  const char *JUPITERFlagsFile() const;
  const char *JUPITERControlFile() const;
  const char *JUPITERGeomFile() const;

  int ResultValue() const { return result_value; }

  /**
   * Directly calls Process() and store result.
   */
  void CallProcess(vtkMultiProcessController *controller);

  /**
   * Start application.
   */
  int Exec();

  /**
   * Override this function.
   */
  virtual int Process(vtkMultiProcessController *controller) {
    return 0;
  };
};

#endif
