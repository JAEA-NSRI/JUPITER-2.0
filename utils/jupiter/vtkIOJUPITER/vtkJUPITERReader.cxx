#include "vtkJUPITERReader.h"

#include <cstddef>
#include <iostream>
#include <jupiter/func.h>
#include <jupiter/struct.h>
#include <jupiter/vtkiobind/bind.h>

#include <cstdlib>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include <vtkAbstractArray.h>
#include <vtkBuffer.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkCharArray.h>
#include <vtkCommunicator.h>
#include <vtkDataArray.h>
#include <vtkDataObject.h>
#include <vtkDataSet.h>
#include <vtkDataSetAlgorithm.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkGlobFileNames.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkInformationKey.h>
#include <vtkInformationStringKey.h>
#include <vtkInformationIntegerKey.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiPieceDataSet.h>
#include <vtkObject.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>
#include <vtkSetGet.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkType.h>
#include <vtkUnstructuredGrid.h>

vtkStandardNewMacro(vtkJUPITERReader);

namespace
{
template <jupiter_bind_data_type type> struct jupiter_bind_data_type_trait;

template <> struct jupiter_bind_data_type_trait<JUPITER_BIND_TYPE_DOUBLE>
{
  using type = vtkDoubleArray;
};

template <> struct jupiter_bind_data_type_trait<JUPITER_BIND_TYPE_FLOAT>
{
  using type = vtkFloatArray;
};

template <> struct jupiter_bind_data_type_trait<JUPITER_BIND_TYPE_INT>
{
  using type = vtkIntArray;
};

template <> struct jupiter_bind_data_type_trait<JUPITER_BIND_TYPE_CHAR>
{
  using type = vtkCharArray;
};

template <template <jupiter_bind_data_type v> class F, typename... Args>
decltype(F<JUPITER_BIND_TYPE_CHAR>{}(std::declval<Args>()...))
  bind_data_apply(jupiter_bind_data_type type, Args &&...args)
{
  switch (type) {
  case JUPITER_BIND_TYPE_DOUBLE:
    return F<JUPITER_BIND_TYPE_DOUBLE>{}(std::forward<Args>(args)...);
  case JUPITER_BIND_TYPE_FLOAT:
    return F<JUPITER_BIND_TYPE_FLOAT>{}(std::forward<Args>(args)...);
  case JUPITER_BIND_TYPE_INT:
    return F<JUPITER_BIND_TYPE_INT>{}(std::forward<Args>(args)...);
  case JUPITER_BIND_TYPE_CHAR:
    return F<JUPITER_BIND_TYPE_CHAR>{}(std::forward<Args>(args)...);
  }
  throw std::logic_error("invalid type");
}

template <jupiter_bind_data_type type> class allocate_raw_array_f
{
public:
  void *operator()(size_t n) const
  {
    using value_type =
      typename jupiter_bind_data_type_trait<type>::type::ValueType;
    return malloc(sizeof(value_type) * n);
  }
};

static void *allocate_raw_array(jupiter_bind_data_type type, size_t n)
{
  return bind_data_apply<allocate_raw_array_f>(type, n);
}

template <jupiter_bind_data_type type> class allocate_array_f
{
public:
  vtkAbstractArray *operator()() const
  {
    return jupiter_bind_data_type_trait<type>::type::New();
  }
};

static vtkAbstractArray *allocate_array(jupiter_bind_data_type type)
{
  return bind_data_apply<allocate_array_f>(type);
}
} // namespace

class vtkJUPITERReader::Internal
{
public:
  std::vector<int> time_steps;
  std::vector<double> time_step_values;

  parameter *prm;
  bool input_ready;

  int stm[3];
  int stp[3];
  int global_start[3];
  int global_size[3];
  int local_size[3];

private:
  vtkGlobFileNames *glob;

public:
  Internal()
    : stm{0, 0, 0}, stp{0, 0, 0}, global_start{0, 0, 0}, global_size{0, 0, 0},
      local_size{0, 0, 0}
  {
    this->prm = 0;
    this->input_ready = false;
    this->glob = vtkGlobFileNames::New();
    this->glob->RecurseOff();
  }

  ~Internal()
  {
    if (this->prm) {
      free_parameter(this->prm);
    }
    this->glob->Delete();
  }

public:
  int ReadFile(const char *input, const char *flags,
               const char *geom_file = nullptr,
               const char *control_file = nullptr);

  bool FindTimeFiles(const char *input_directory);

  void GetGlobalExtent(int extent[6]) const
  {
    if (input_ready) {
      extent[0] = 0;
      extent[1] = this->global_size[0];
      extent[2] = 0;
      extent[3] = this->global_size[1];
      extent[4] = 0;
      extent[5] = this->global_size[2];
    } else {
      extent[0] = 0;
      extent[1] = 0;
      extent[2] = 0;
      extent[3] = 0;
      extent[4] = 0;
      extent[5] = 0;
    }
  }

  void GetLocalExtent(int extent[6]) const
  {
    if (input_ready) {
      extent[0] = this->global_start[0];
      extent[1] = extent[0] + this->local_size[0];
      extent[2] = this->global_start[1];
      extent[3] = extent[2] + this->local_size[1];
      extent[4] = this->global_start[2];
      extent[5] = extent[4] + this->local_size[2];
    } else {
      extent[0] = 0;
      extent[1] = 0;
      extent[2] = 0;
      extent[3] = 0;
      extent[4] = 0;
      extent[5] = 0;
    }
  }

  vtkIdType GetLocalSize() const
  {
    vtkIdType id;

    if (!input_ready)
      return 0;

    id = local_size[0];
    id *= local_size[1];
    id *= local_size[2];
    return id;
  }

  int PieceNumber() const
  {
    if (!this->prm || !input_ready)
      return 0;
    return get_mpi_rank(this->prm);
  }

  int NumberOfPieces() const
  {
    if (!this->prm || !input_ready)
      return 0;
    return get_mpi_nproc(this->prm);
  }

  /**
   * @brief Number of LPT particles
   * @return Number of particle at current time key, -1 if failed.
   *
   * If LPT is not enabled, this method always returns 0.
   */
  int NumberOfParticle(int iout, const char *input_directory) const
  {
    return get_number_of_particles(this->prm, iout, input_directory);
  }

  struct funcs_data
  {
    vtkJUPITERReader *reader;
    vtkDataObject *data;
    vtkDataObject::AttributeTypes attribute_type;
    bool use_raw_name;
    bool set_extended_info;
  };

  static void *AllocateArray(size_t n, jupiter_bind_data_type type, void *arg)
  {
    return allocate_raw_array(type, n);
  }

  static vtkAbstractArray *MakeArray(const char *descriptive_name,
                                     const char *unit_name,
                                     const char *raw_name, int soa_icompo,
                                     void *val, size_t ntuple, int unit_size,
                                     jupiter_bind_data_type type,
                                     const char **component_names, int alloc,
                                     bool use_raw_name, bool set_extented_info)
  {
    vtkAbstractArray *array = allocate_array(type);
    if (!array)
      return 0;

    // array->SetNumberOfTuples(ntuple);
    array->SetNumberOfComponents(unit_size);
    if (alloc) {
      array->SetVoidArray(val, ntuple * unit_size,
                          vtkAbstractArray::VTK_DATA_ARRAY_FREE);
    } else {
      array->SetArrayFreeFunction(nullptr);
      array->SetVoidArray(val, ntuple * unit_size,
                          vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED);
    }

    if (raw_name && use_raw_name) {
      std::string name = raw_name;
      if (soa_icompo >= 0)
        name += ":" + std::to_string(soa_icompo);
      array->SetName(name.c_str());
    } else {
      array->SetName(descriptive_name);
    }

    vtkInformation *info = array->GetInformation();
    if (unit_name)
      info->Set(vtkDataArray::UNITS_LABEL(), unit_name);

    if (raw_name && set_extented_info) {
      info->Set(vtkJUPITERReader::RAW_NAME(), raw_name);
      if (soa_icompo >= 0)
        info->Set(vtkJUPITERReader::COMPONENT_INDEX(), soa_icompo);
    }

    if (component_names) {
      for (int i = 0; i < unit_size; ++i)
        array->SetComponentName(i, component_names[i]);
    }
    return array;
  }

  static int AddAttribute(const char *descriptive_name, const char *unit_name,
                          const char *raw_name, int soa_icompo, void *val,
                          size_t ntuple, int unit_size,
                          jupiter_bind_data_type type,
                          const char **component_names, void *arg)
  {
    funcs_data *data = static_cast<funcs_data *>(arg);
    vtkAbstractArray *array;
    array = MakeArray(descriptive_name, unit_name, raw_name, soa_icompo, val,
                      ntuple, unit_size, type, component_names, 1,
                      data->use_raw_name, data->set_extended_info);
    if (!array)
      return 1;

    vtkDataSetAttributes *attr;
    attr = data->data->GetAttributes(data->attribute_type);
    attr->AddArray(array);

    /* disown the array */
    array->Delete();
    return 0;
  }

  static int SetTime(double time, void *arg)
  {
    funcs_data *data = static_cast<funcs_data *>(arg);
    data->data->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), time);
    return 0;
  }

  static int SetCoordinates(void (vtkRectilinearGrid::*setter)(vtkDataArray *),
                            const char *name, void *val, size_t ntuple,
                            int unit_size, jupiter_bind_data_type type,
                            int alloc, void *arg)
  {
    funcs_data *data;
    vtkRectilinearGrid *grid;
    vtkDataArray *darray;
    vtkAbstractArray *array;

    data = static_cast<funcs_data *>(arg);
    array = MakeArray(name, nullptr, nullptr, -1, val, ntuple, unit_size, type,
                      NULL, alloc, false, false);
    if (!array)
      return 1;

    grid = vtkRectilinearGrid::SafeDownCast(data->data);
    if (!grid)
      throw std::logic_error("data is not vtkRectilinearGrid");

    darray = vtkDataArray::SafeDownCast(array);
    (grid->*setter)(darray);

    /* disown the array */
    array->Delete();
    return 0;
  }

  static int SetXCoordinates(void *val, int length, jupiter_bind_data_type type,
                             int alloc, void *arg)
  {
    return SetCoordinates(&vtkRectilinearGrid::SetXCoordinates, "X", val,
                          length, 1, type, alloc, arg);
  }

  static int SetYCoordinates(void *val, int length, jupiter_bind_data_type type,
                             int alloc, void *arg)
  {
    return SetCoordinates(&vtkRectilinearGrid::SetYCoordinates, "Y", val,
                          length, 1, type, alloc, arg);
  }

  static int SetZCoordinates(void *val, int length, jupiter_bind_data_type type,
                             int alloc, void *arg)
  {
    return SetCoordinates(&vtkRectilinearGrid::SetZCoordinates, "Z", val,
                          length, 1, type, alloc, arg);
  }

  template <typename G, typename S, typename C>
  static int SetPoints(const G &getter, const S &setter, const C &cell_setter,
                       vtkDataArray *array)
  {
    vtkIdType npt;
    vtkCellArray *cells;
    vtkIdTypeArray *cdata;

    vtkPoints *points = getter();
    if (!points) {
      points = vtkPoints::New();
      setter(points);
    }
    points->SetData(array);

    npt = array->GetNumberOfTuples();
    cdata = vtkIdTypeArray::New();
    cdata->SetNumberOfComponents(1);
    cdata->SetNumberOfTuples(npt);
    for (vtkIdType i = 0; i < npt; ++i)
      cdata->SetValue(i, i);

    cells = vtkCellArray::New();
    cells->Initialize();
    cells->SetData(1, cdata);
    cell_setter(cells);
    return 1;
  }

  static int SetPoints(vtkUnstructuredGrid *ugrid, vtkDataArray *array)
  {
    return SetPoints([ugrid]() { return ugrid->GetPoints(); },
                     [ugrid](vtkPoints *p) { ugrid->SetPoints(p); },
                     [ugrid](vtkCellArray *array) {
                       ugrid->SetCells(VTK_POLY_VERTEX, array);
                     },
                     array);
  }

  static int SetPoints(vtkPolyData *poly, vtkDataArray *array)
  {
    return SetPoints([poly]() { return poly->GetPoints(); },
                     [poly](vtkPoints *p) { poly->SetPoints(p); },
                     [poly](vtkCellArray *array) { poly->SetVerts(array); },
                     array);
  }

  static int SetLPTPositions(void *val, int npt, jupiter_bind_data_type type,
                             void *arg)
  {
    funcs_data *data;
    vtkPoints *points;
    vtkUnstructuredGrid *udata;
    vtkPolyData *pdata;
    vtkAbstractArray *array;
    vtkDataArray *darray;

    data = static_cast<funcs_data *>(arg);
    array = MakeArray("Points", nullptr, nullptr, -1, val, npt, 3, type, NULL,
                      1, false, false);
    darray = vtkDataArray::SafeDownCast(array);
    if (!darray)
      throw std::logic_error("Failed to cast to vtkDataArray");

    pdata = vtkPolyData::SafeDownCast(data->data);
    if (pdata)
      return SetPoints(pdata, darray);

    udata = vtkUnstructuredGrid::SafeDownCast(data->data);
    if (udata)
      return SetPoints(udata, darray);

    return 0;
  }

  int ConvertFluidMesh(vtkJUPITERReader *reader,
                       vtkRectilinearGrid *output) const
  {
    convert_fluid_mesh_funcs funcs;
    funcs_data d;
    funcs.allocate_array = AllocateArray;
    funcs.set_x_coordinate = SetXCoordinates;
    funcs.set_y_coordinate = SetYCoordinates;
    funcs.set_z_coordinate = SetZCoordinates;
    d.reader = reader;
    d.data = output;
    d.attribute_type = vtkDataObject::CELL;
    d.use_raw_name = reader->GetUseRawDataName();
    d.set_extended_info = reader->GetIncludeExtendedInformation();
    funcs.arg = &d;
    return convert_fluid_mesh(this->prm, &funcs);
  }

  int ConvertFluid(vtkJUPITERReader *reader, vtkRectilinearGrid *output,
                   int iout, const char *input_directory) const
  {
    convert_fluid_funcs funcs;
    funcs_data d;
    funcs.allocate_array = AllocateArray;
    funcs.add_attribute = AddAttribute;
    funcs.set_time = SetTime;
    funcs.set_x_coordinate = SetXCoordinates;
    funcs.set_y_coordinate = SetYCoordinates;
    funcs.set_z_coordinate = SetZCoordinates;
    d.reader = reader;
    d.data = output;
    d.attribute_type = vtkDataObject::CELL;
    d.use_raw_name = reader->GetUseRawDataName();
    d.set_extended_info = reader->GetIncludeExtendedInformation();
    funcs.arg = &d;
    return convert_fluid(this->prm, iout, input_directory, &funcs);
  }

  int ConvertParticles(
    vtkJUPITERReader *reader, vtkDataObject *output, int iout,
    const char *input_directory, int npt = -1,
    vtkDataObject::AttributeTypes types = vtkDataObject::POINT) const
  {
    convert_particles_funcs funcs;
    funcs_data d;

    if (npt < 0) {
      npt = this->NumberOfParticle(iout, input_directory);
      if (npt < 0) {
        return 1;
      }
    }

    funcs.allocate_array = AllocateArray;
    funcs.add_attribute = AddAttribute;
    funcs.set_time = SetTime;
    funcs.set_lpt_positions = SetLPTPositions;
    d.reader = reader;
    d.data = output;
    d.attribute_type = types;
    d.use_raw_name = reader->GetUseRawDataName();
    d.set_extended_info = reader->GetIncludeExtendedInformation();
    funcs.arg = &d;
    return convert_particles(this->prm, iout, npt, input_directory, &funcs);
  }
};

vtkJUPITERReader::vtkJUPITERReader()
{
  this->Controller = nullptr;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->LastUpdate = this->GetMTime();
  this->FilesModified = true;
  this->InputDirectoryModified = true;
  this->FlagsFile = nullptr;
  this->ParamFile = nullptr;
  this->GeomFile = nullptr;
  this->ControlFile = nullptr;
  this->InputDirectory = nullptr;
  this->TimeKey = 0;
  this->ReadMeshOnly = false;
  this->ProcessFluid = true;
  this->ProcessParticle = true;
  this->ReadGeomDump = false;
  this->GenerateMultiBlock = false;
  this->GeneratePolyDataParticle = false;
  this->UseRawDataName = false;
  this->IncludeExtendedInformation = true;
  this->internal = new Internal;
}

vtkJUPITERReader::~vtkJUPITERReader()
{
  delete[] this->ParamFile;
  delete[] this->FlagsFile;
  delete[] this->GeomFile;
  delete[] this->ControlFile;
  delete[] this->InputDirectory;
  if (Controller)
    Controller->Delete();
  delete this->internal;
}

int vtkJUPITERReader::FillOutputPortInformation(int vtkNotUsed(port),
                                                vtkInformation *info)
{
  if (ReadMeshOnly) {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkRectilinearGrid");
  } else if (GenerateMultiBlock) {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  } else {
    if (ProcessParticle) {
      if (ProcessFluid) {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
      } else {
        if (GeneratePolyDataParticle) {
          info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
        } else {
          info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
        }
      }
    } else {
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkRectilinearGrid");
    }
  }
  return 1;
}

int vtkJUPITERReader::RequestInformation(
  vtkInformation *request, vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  bool filesModified = this->FilesModified;
  bool idirModified = this->InputDirectoryModified;

  if (filesModified) {
    int r;
    if (ReadGeomDump) {
      r = internal->ReadFile(this->ParamFile, this->FlagsFile, this->GeomFile,
                             this->ControlFile);
    } else {
      r = internal->ReadFile(this->ParamFile, this->FlagsFile);
    }
    if (!r) {
      vtkErrorMacro("Failed to read input files.");
      return 0;
    }
    this->FilesModified = false;
  }

  const char *idir;
  if (this->InputDirectory) {
    idir = this->InputDirectory;
    this->InputDirectoryModified = false;
  } else {
    idir = get_input_directory(internal->prm, (this->TimeKey < 0) ? 1 : 0);
  }

  vtkInformation *info = outputVector->GetInformationObject(0);
  if (filesModified || idirModified) {
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    if (!ReadMeshOnly && FindTimeFiles &&
        internal->FindTimeFiles(InputDirectory)) {
      double range[2];
      const double *arr;
      size_t sz;

      arr = internal->time_step_values.data();
      sz = internal->time_step_values.size();

      range[0] = arr[0];
      range[1] = arr[sz - 1];

      info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
      info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), arr, sz);
    } else {
      internal->time_step_values.clear();
      internal->time_step_values.clear();
    }
  }

  int gextent[6];
  int lextent[6];
  internal->GetGlobalExtent(gextent);
  internal->GetLocalExtent(lextent);

  info->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);
  if (ProcessFluid || ReadMeshOnly) {
    info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), gextent, 6);
  } else {
    info->Remove(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  }

  return 1;
}

int vtkJUPITERReader::RequestDataObject(vtkInformation *request,
                                        vtkInformationVector **inputVector,
                                        vtkInformationVector *outputVector)
{
  /* let VTK to create new data objects */
  return 1;
}

int vtkJUPITERReader::RequestUpdateExtent(vtkInformation *request,
                                          vtkInformationVector **inputVector,
                                          vtkInformationVector *outputVector)
{
  vtkInformation *info;
  int r;
  r = Superclass::RequestUpdateExtent(request, inputVector, outputVector);

  info = outputVector->GetInformationObject(0);
  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP())) {
    double t;
    std::vector<int>::size_type idx;

    t = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    for (idx = 0; idx < internal->time_steps.size(); ++idx) {
      if (t == internal->time_step_values[idx]) {
        break;
      }
    }
    if (idx >= internal->time_steps.size()) {
      idx = 0;
    }

    SetTimeKey(idx);
  }

  return r;
}

int vtkJUPITERReader::RequestData(
  vtkInformation *request, vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *info = outputVector->GetInformationObject(0);
  vtkDataObject *data;
  vtkMultiBlockDataSet *mixed = nullptr;
  vtkMultiPieceDataSet *distr = nullptr;
  vtkRectilinearGrid *fluid = nullptr;
  vtkPolyData *pparticle = nullptr;
  vtkUnstructuredGrid *uparticle = nullptr;
  const int npc = internal->NumberOfPieces();
  const int ipc = internal->PieceNumber();
  bool has_time = false;
  double time = 0.0;
  vtkMTimeType modified;

  if (!internal->input_ready) {
    return 0;
  }

  modified = this->GetMTime();
  LastUpdate = modified;

  data = info->Get(vtkDataObject::DATA_OBJECT());
  mixed = vtkMultiBlockDataSet::SafeDownCast(data);
  distr = vtkMultiPieceDataSet::SafeDownCast(data);
  fluid = vtkRectilinearGrid::SafeDownCast(data);
  pparticle = vtkPolyData::SafeDownCast(data);
  uparticle = vtkUnstructuredGrid::SafeDownCast(data);

  if (mixed)
    mixed->SetNumberOfBlocks(2);

  if (ReadMeshOnly || ProcessFluid) {
    int extent[6];
    int gextent[6];
    vtkMultiPieceDataSet *adist = nullptr;
    vtkRectilinearGrid *agrid = nullptr;
    int r;
    bool alloc_grid = false;

    if (!fluid) {
      agrid = vtkRectilinearGrid::New();
      fluid = agrid;
    }

    internal->GetLocalExtent(extent);
    internal->GetGlobalExtent(gextent);
    fluid->SetExtent(extent);

    if (ReadMeshOnly) {
      r = internal->ConvertFluidMesh(this, fluid);
    } else {
      r = internal->ConvertFluid(this, fluid, TimeKey, InputDirectory);
    }
    if (r)
      return 0;

    vtkDebugMacro(<< *fluid);

    if (mixed) {
      if (internal->NumberOfPieces() > 1) {
        adist = vtkMultiPieceDataSet::New();
        adist->SetNumberOfPieces(internal->NumberOfPieces());
        adist->SetPiece(internal->PieceNumber(), fluid);
        mixed->SetBlock(0, adist);
      } else {
        mixed->SetBlock(0, fluid);
      }
    }
    if (distr) {
      distr->SetNumberOfPieces(internal->NumberOfPieces());
      distr->SetPiece(internal->PieceNumber(), fluid);
    }

    vtkInformation *finfo = fluid->GetInformation();
    finfo->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), npc);
    finfo->Set(vtkDataObject::DATA_PIECE_NUMBER(), ipc);
    finfo->Set(vtkDataObject::DATA_EXTENT(), fluid->GetExtent(), 6);
    finfo->Set(vtkDataObject::ALL_PIECES_EXTENT(), gextent, 6);
    finfo->Set(vtkDataObject::PIECE_EXTENT(), extent, 6);
    finfo->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
    if (!has_time) {
      if (finfo->Has(vtkDataObject::DATA_TIME_STEP())) {
        time = finfo->Get(vtkDataObject::DATA_TIME_STEP());
        has_time = true;
      }
    }
    if (agrid) {
      /* Disown the grid */
      agrid->Delete();
    }
    if (adist) {
      adist->Delete();
    }
  }

  if (!ReadMeshOnly && ProcessParticle) {
    int npt;
    int r;
    vtkDataObject *pdata;
    vtkDataObject *adata = nullptr;

    npt = internal->NumberOfParticle(TimeKey, InputDirectory);
    if (npt < 0) {
      return 0;
    }

    vtkDebugMacro(<< "Number Of Particles for step " << TimeKey << ": " << npt);

    if (uparticle) {
      pdata = uparticle;
    } else if (pparticle) {
      pdata = pparticle;
    } else {
      if (GeneratePolyDataParticle) {
        adata = vtkPolyData::New();
      } else {
        adata = vtkUnstructuredGrid::New();
      }
      pdata = adata;
    }

    r = internal->ConvertParticles(this, pdata, TimeKey, InputDirectory, npt);
    if (r)
      return 0;

    vtkDebugMacro(<< *pdata);

    if (mixed) {
      mixed->SetBlock(1, pdata);
    }

    vtkInformation *pinfo = pdata->GetInformation();
    pinfo->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), npc);
    pinfo->Set(vtkDataObject::DATA_PIECE_NUMBER(), ipc);
    if (!has_time) {
      if (pinfo->Has(vtkDataObject::DATA_TIME_STEP())) {
        time = pinfo->Get(vtkDataObject::DATA_TIME_STEP());
        has_time = true;
      }
    }
    if (adata) {
      /* Disown the grid */
      adata->Delete();
    }
  }

  if (mixed) {
    vtkInformation *binfo;
    binfo = mixed->GetMetaData(0u);
    binfo->Set(vtkMultiBlockDataSet::NAME(), FluidBlockName);

    binfo = mixed->GetMetaData(1u);
    binfo->Set(vtkMultiBlockDataSet::NAME(), ParticleBlockName);

    vtkInformation *minfo;
    minfo = mixed->GetInformation();
    minfo->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), npc);
    minfo->Set(vtkDataObject::DATA_PIECE_NUMBER(), ipc);
    if (has_time) {
      minfo->Set(vtkDataObject::DATA_TIME_STEP(), time);
    }
  }

  return 1;
}

vtkIdType vtkJUPITERReader::GetLocalSize() const
{
  return internal->GetLocalSize();
}

int vtkJUPITERReader::Internal::ReadFile(const char *input, const char *flags,
                                         const char *geom, const char *control)
{
  bool flags_changed, param_changed, geom_changed, control_changed;
  int stat;

  if (!input || !flags) {
    this->input_ready = false;
    return 0;
  }

  if (this->prm) {
    free_parameter(this->prm);
  }
  stat = 0;
  this->prm = set_parameters_file(input, flags, geom, control, &stat);
  if (!this->prm || stat != 0) {
    if (this->prm) {
      free_parameter(this->prm);
      this->prm = nullptr;
    }
    this->input_ready = false;
    return 0;
  }

  this->time_step_values.clear();
  this->time_steps.clear();

  get_global_size(this->prm, this->global_size);
  get_local_size(this->prm, this->local_size);
  get_stm_size(this->prm, this->stm);
  get_stp_size(this->prm, this->stp);
  get_global_start(this->prm, this->global_start);
  this->input_ready = true;

  return 1;
}

bool vtkJUPITERReader::Internal::FindTimeFiles(const char *input_directory)
{
  int nfile;
  int idx;
  double time;
  char *pat;

  std::map<int, double> time_list;

  if (!this->time_steps.empty()) {
    return true;
  }

  this->glob->Reset();
  pat = get_time_file_pattern(this->prm, 0);

  this->glob->SetDirectory(input_directory);
  this->glob->AddFileNames(pat);
  std::free(pat);

  nfile = this->glob->GetNumberOfFileNames();
  this->time_steps.clear();
  this->time_step_values.clear();

  for (int i = 0; i < nfile; ++i) {
    const char *file;
    int r;

    file = this->glob->GetNthFileName(i);
    r = input_time_file_get_index(this->prm, input_directory, file, 0, &time,
                                  &idx);
    if (r != 1)
      continue;

    time_list[idx] = time;
  }

  std::map<int, double>::iterator iter;
  for (iter = time_list.begin(); iter != time_list.end(); ++iter) {
    this->time_steps.push_back(iter->first);
    this->time_step_values.push_back(iter->second);
  }

  if (this->time_step_values.size() == 0)
    return false;
  return true;
}

const char vtkJUPITERReader::FluidBlockName[];
const char vtkJUPITERReader::ParticleBlockName[];

vtkInformationKeyMacro(vtkJUPITERReader, RAW_NAME, String);
vtkInformationKeyMacro(vtkJUPITERReader, COMPONENT_INDEX, Integer);
