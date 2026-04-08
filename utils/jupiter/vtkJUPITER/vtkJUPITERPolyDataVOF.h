/* This is -*- c++ -*- source. */
/* This is vim: set ft=cpp: source. */

#ifndef vtkJUPITERPolyDataVOF_h
#define vtkJUPITERPolyDataVOF_h

#ifndef __cplusplus
#error "This header is C++ header"
#endif

#include <jupiter/vtkJUPITER/vtkJUPITER.h>
#include <vtkPassInputTypeAlgorithm.h>
#include <vtkSetGet.h>

class VTKJUPITER_DECL vtkJUPITERPolyDataVOF : public vtkPassInputTypeAlgorithm
{
  vtkTypeMacro(vtkJUPITERPolyDataVOF, vtkPassInputTypeAlgorithm)

protected:
  vtkJUPITERPolyDataVOF();
  virtual ~vtkJUPITERPolyDataVOF();

  bool ClipFraction; /*!< Clip fraction to [0, 1] */
  bool InsideOut; /*!< Reverse result */
  double Tolerance; /*!< Implicit function tolerance */
  double MergeTolerance; /*!< Clip merge tolerance */
  char *CellVolumeArrayName;
  char *CutVolumeArrayName;
  char *FractionArrayName;

  int FillInputPortInformation(int port, vtkInformation *info) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

public:
  static vtkJUPITERPolyDataVOF *New();

  vtkSetStringMacro(CellVolumeArrayName);
  vtkGetStringMacro(CellVolumeArrayName);

  vtkSetStringMacro(CutVolumeArrayName);
  vtkGetStringMacro(CutVolumeArrayName);

  vtkSetStringMacro(FractionArrayName);
  vtkGetStringMacro(FractionArrayName);

  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);

  vtkSetMacro(MergeTolerance, double);
  vtkGetMacro(MergeTolerance, double);

  vtkSetMacro(InsideOut, bool);
  vtkGetMacro(InsideOut, bool);
  vtkBooleanMacro(InsideOut, bool);

  vtkSetMacro(ClipFraction, bool);
  vtkGetMacro(ClipFraction, bool);
  vtkBooleanMacro(ClipFraction, bool);
};

#endif
