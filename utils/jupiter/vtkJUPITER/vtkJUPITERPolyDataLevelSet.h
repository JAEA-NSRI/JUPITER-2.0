/* This is -*- c++ -*- source. */
/* This is vim: set ft=cpp: source. */

#ifndef vtkJUPITERPolyDataLevelSet_h
#define vtkJUPITERPolyDataLevelSet_h

#ifndef __cplusplus
#error "This header is C++ header"
#endif

#include <jupiter/vtkJUPITER/vtkJUPITER.h>

#include <vtkPassInputTypeAlgorithm.h>
#include <vtkSetGet.h>

class VTKJUPITER_DECL vtkJUPITERPolyDataLevelSet
  : public vtkPassInputTypeAlgorithm
{
  vtkTypeMacro(vtkJUPITERPolyDataLevelSet, vtkPassInputTypeAlgorithm)

protected:
  vtkJUPITERPolyDataLevelSet();
  virtual ~vtkJUPITERPolyDataLevelSet();

  bool InsideOut;
  double Tolerance;
  char *LevelSetArrayName;
  char *FractionArrayName;

  int FillInputPortInformation(int port, vtkInformation *info) override;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

public:
  static vtkJUPITERPolyDataLevelSet *New();

  vtkSetStringMacro(LevelSetArrayName);
  vtkGetStringMacro(LevelSetArrayName);

  vtkSetStringMacro(FractionArrayName);
  vtkGetStringMacro(FractionArrayName);

  vtkGetMacro(Tolerance, double);
  vtkSetMacro(Tolerance, double);

  vtkGetMacro(InsideOut, bool);
  vtkSetMacro(InsideOut, bool);
  vtkBooleanMacro(InsideOut, bool);
};

#endif
