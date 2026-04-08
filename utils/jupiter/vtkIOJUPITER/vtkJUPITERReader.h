/* This is -*- c++ -*- source. */
/* This is vim: set ft=cpp: source. */

#ifndef vtkJUPITERReader_h
#define vtkJUPITERReader_h

#ifndef __cplusplus
#error "This header is C++ header"
#endif

#include <vtkABI.h>
#include <vtkAlgorithm.h>
#include <vtkDataObject.h>
#include <vtkDataSetAlgorithm.h>
#include <vtkMultiProcessController.h>
#include <vtkSetGet.h>
#include <vtkType.h>

#include <jupiter/vtkIOJUPITER/vtkIOJUPITER.h>

class VTKIOJUPITER_DECL vtkJUPITERReader : public vtkDataSetAlgorithm
{
  class Internal;

  Internal *internal;

  vtkMultiProcessController *Controller;
  bool FilesModified;
  bool InputDirectoryModified;
  vtkMTimeType LastUpdate;
  char *FlagsFile;
  char *ParamFile;
  char *GeomFile;
  char *ControlFile;
  char *InputDirectory;
  int TimeKey;
  vtkTypeBool FindTimeFiles;
  vtkTypeBool ReadGeomDump;
  vtkTypeBool ReadMeshOnly;
  vtkTypeBool ProcessFluid;
  vtkTypeBool ProcessParticle;
  vtkTypeBool GenerateMultiBlock;
  vtkTypeBool GeneratePolyDataParticle;
  vtkTypeBool UseRawDataName;
  vtkTypeBool IncludeExtendedInformation;

protected:
  vtkJUPITERReader();
  ~vtkJUPITERReader();

  virtual int FillOutputPortInformation(int port,
                                        vtkInformation *info) override;

  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector) override;

  virtual int RequestUpdateExtent(vtkInformation *request,
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector) override;

  virtual int RequestDataObject(vtkInformation *request,
                                vtkInformationVector **inputVector,
                                vtkInformationVector *outputVector) override;

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) override;

public:
  static vtkJUPITERReader *New();
  vtkTypeMacro(vtkJUPITERReader, vtkDataSetAlgorithm)

  // vtkSetStringMacro(FlagsFile);
  void SetFlagsFile(const char *_arg)
  {
    vtkSetStringBodyMacro(FlagsFile, _arg);
    FilesModified = true;
  }

  vtkGetStringMacro(FlagsFile);

  // vtkSetStringMacro(ParamFile);
  void SetParamFile(const char *_arg)
  {
    vtkSetStringBodyMacro(ParamFile, _arg);
    FilesModified = true;
  }

  vtkGetStringMacro(ParamFile);

  // vtkSetStringMacro(GeomFile);
  void SetGeomFile(const char *_arg)
  {
    vtkSetStringBodyMacro(GeomFile, _arg);
    FilesModified = true;
  }

  vtkGetStringMacro(GeomFile);

  // vtkSetStringMacro(ControlFile);
  void SetControlFile(const char *_arg)
  {
    vtkSetStringBodyMacro(ControlFile, _arg);
    FilesModified = true;
  }
  vtkGetStringMacro(ControlFile);

  // vtkSetStringMacro(InputDirectory);
  void SetInputDirectory(const char *_arg)
  {
    vtkSetStringBodyMacro(InputDirectory, _arg);
    InputDirectoryModified = true;
  }
  vtkGetStringMacro(InputDirectory);

  vtkSetMacro(TimeKey, int);
  vtkGetMacro(TimeKey, int);

  vtkSetObjectMacro(Controller, vtkMultiProcessController);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  /**
   * Whether look for time.dat files
   */
  vtkSetMacro(FindTimeFiles, vtkTypeBool);
  vtkGetMacro(FindTimeFiles, vtkTypeBool);
  vtkBooleanMacro(FindTimeFiles, vtkTypeBool);

  /**
   * Whether reads fluid mesh only
   */
  vtkSetMacro(ReadMeshOnly, vtkTypeBool);
  vtkGetMacro(ReadMeshOnly, vtkTypeBool);
  vtkBooleanMacro(ReadMeshOnly, vtkTypeBool);

  /**
   * Whether reads fluid data
   */
  vtkSetMacro(ProcessFluid, vtkTypeBool);
  vtkGetMacro(ProcessFluid, vtkTypeBool);
  vtkBooleanMacro(ProcessFluid, vtkTypeBool);

  /**
   * Whether reads particle data
   */
  vtkSetMacro(ProcessParticle, vtkTypeBool);
  vtkGetMacro(ProcessParticle, vtkTypeBool);
  vtkBooleanMacro(ProcessParticle, vtkTypeBool);

  /**
   * Whether reads files specified in Geom_dump.
   */
  vtkSetMacro(ReadGeomDump, vtkTypeBool);
  vtkGetMacro(ReadGeomDump, vtkTypeBool);
  vtkBooleanMacro(ReadGeomDump, vtkTypeBool);

  /**
   * Whether generate vtkMultiBlockDataSet even if not both of fluid or particle
   * data are converted.
   */
  vtkSetMacro(GenerateMultiBlock, vtkTypeBool);
  vtkGetMacro(GenerateMultiBlock, vtkTypeBool);
  vtkBooleanMacro(GenerateMultiBlock, vtkTypeBool);

  /**
   * Whether generate vtkPolyData for particles, instead of vtkUnstructuredGrid
   */
  vtkSetMacro(GeneratePolyDataParticle, vtkTypeBool);
  vtkGetMacro(GeneratePolyDataParticle, vtkTypeBool);
  vtkBooleanMacro(GeneratePolyDataParticle, vtkTypeBool);

  /**
   * Whether generate with raw data name, or descriptive name
   */
  vtkSetMacro(UseRawDataName, vtkTypeBool);
  vtkGetMacro(UseRawDataName, vtkTypeBool);
  vtkBooleanMacro(UseRawDataName, vtkTypeBool);

  /**
   * Whether include extended information (which ParaView never use)
   */
  vtkSetMacro(IncludeExtendedInformation, vtkTypeBool);
  vtkGetMacro(IncludeExtendedInformation, vtkTypeBool);
  vtkBooleanMacro(IncludeExtendedInformation, vtkTypeBool);

  vtkIdType GetLocalSize() const;

  static constexpr const char FluidBlockName[] = "Fluid";
  static constexpr const char ParticleBlockName[] = "Particles";

  /**
   * Raw (filename-safe) variable name
   */
  static vtkInformationStringKey *RAW_NAME();

  /**
   * Component index of raw variable name
   */
  static vtkInformationIntegerKey *COMPONENT_INDEX();
};

#endif
