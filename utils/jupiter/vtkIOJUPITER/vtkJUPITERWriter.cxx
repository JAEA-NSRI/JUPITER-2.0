#include "vtkJUPITERWriter.h"

#include "jupiter/func.h"
#include "jupiter/struct.h"

#include <tuple>
#include <vtkAbstractArray.h>
#include <vtkBuffer.h>
#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkCharArray.h>
#include <vtkDataArray.h>
#include <vtkDataObject.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIdTypeArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkIntArray.h>
#include <vtkLongArray.h>
#include <vtkLongLongArray.h>
#include <vtkNew.h>
#include <vtkRectilinearGrid.h>
#include <vtkSetGet.h>
#include <vtkShortArray.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkUnsignedLongArray.h>
#include <vtkUnsignedLongLongArray.h>
#include <vtkUnsignedShortArray.h>
#include <vtksys/SystemTools.hxx>

#ifdef JUPITER_MPI
#include <mpi.h>
#include <vtkMPI.h>
#include <vtkMPICommunicator.h>
#include <vtkMPIController.h>
#endif

vtkJUPITERWriter::vtkJUPITERWriter()
  : Controller(nullptr), Mode(UNIFY_MPI),
  WholeExtent{0, -1, 0, -1, 0, -1}
{
  SetNumberOfInputPorts(1);
}

vtkJUPITERWriter::~vtkJUPITERWriter()
{
  SetController(nullptr);
}

int vtkJUPITERWriter::FillInputPortInformation(int port, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

void vtkJUPITERWriter::WriteArray(const char *filename,
                                  const int data_extent[6],
                                  const int write_extent[6],
                                  vtkDataArray *array)
{
  binary_output_mode mode = BINARY_OUTPUT_INVALID;
  mpi_param lmpi;

  switch (this->Mode) {
  case vtkJUPITERWriter::UNIFY_MPI:
    mode = BINARY_OUTPUT_UNIFY_MPI;
    break;
  case vtkJUPITERWriter::UNIFY_GATHER:
    mode = BINARY_OUTPUT_UNIFY_GATHER;
    break;
  case vtkJUPITERWriter::BYPROCESS:
    mode = BINARY_OUTPUT_BYPROCESS;
    break;
  }
  if (mode == BINARY_OUTPUT_INVALID) {
    SetErrorCode(1);
    return;
  }

  vtkInformation *info;
  const int *whole_extent;
  info = GetInputInformation(0, 0);
  if (info && info->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT())) {
    whole_extent = info->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  } else {
    whole_extent = this->WholeExtent;
  }
  /** @todo whole_extent check */

  int mx = data_extent[1] - data_extent[0];
  int my = data_extent[3] - data_extent[2];
  int mz = data_extent[5] - data_extent[4];
  int stmx = write_extent[0] - data_extent[0];
  int stpx = data_extent[1] - write_extent[1];
  int stmy = write_extent[2] - data_extent[2];
  int stpy = data_extent[3] - write_extent[3];
  int stmz = write_extent[4] - data_extent[4];
  int stpz = data_extent[5] - write_extent[5];

  if (Controller && Controller->GetNumberOfProcesses() > 1) {
#ifdef JUPITER_MPI
    int ir, npex, npey, npez;
    vtkMPICommunicator *vcomm;
    vtkMPIController *mpicontr;
    MPI_Comm comm;
    mpicontr = vtkMPIController::SafeDownCast(Controller);
    if (!mpicontr) {
      vtkErrorMacro(<< "Controller is not vtkMPIController");
      SetErrorCode(1);
      return;
    }
    vcomm = vtkMPICommunicator::SafeDownCast(mpicontr->GetCommunicator());
    comm = *vcomm->GetMPIComm()->GetHandle();

    if (!vtkJUPITERWriter::ComputeStructuredNumberOfPrcoess(write_extent,
                                                            Controller, &npex,
                                                            &npey, &npez,
                                                            nullptr, nullptr,
                                                            nullptr)) {
      vtkErrorMacro(<< "Failed to reduce extents to structured MPI ranking");
      SetErrorCode(1);
      return;
    }

    ir = Controller->GetLocalProcessId();
    lmpi.CommJUPITER = comm;
    int status = OFF;
    set_mpi_for_rank(&lmpi, npex, npey, npez, 0, 0, ir, &status);
    if (status == ON) {
      vtkErrorMacro(<< "Failed to set MPI communication info");
      SetErrorCode(1);
      return;
    }
#else
    vtkErrorMacro(<< "Parallel controller passed, but MPI is not enabled here");
    SetErrorCode(1);
    return;
#endif
  } else {
    set_self_comm_mpi(&lmpi);
  }

  void *data = array->GetVoidPointer(0);
  int unit_size = vtkJUPITERWriter::GetUnitSizeOfArray(array);

  int r;
  r = output_binary_generic(&lmpi, data, stmx, stmy, stmz, stpx, stpy, stpz, mx,
                            my, mz, unit_size, filename, mode);
  if (r != 0) {
    vtkErrorMacro(<< "Failed to write binary file");
    SetErrorCode(1);
    return;
  }
}

void vtkJUPITERWriter::WriteArray(const char *filename,
                                  const int data_extent[6],
                                  int number_of_ghost_levels,
                                  vtkDataArray *array)
{
  if (number_of_ghost_levels > 0) {
    int write_extent[6] = {
      data_extent[0] + number_of_ghost_levels,
      data_extent[1] - number_of_ghost_levels,
      data_extent[2] + number_of_ghost_levels,
      data_extent[3] - number_of_ghost_levels,
      data_extent[4] + number_of_ghost_levels,
      data_extent[5] - number_of_ghost_levels,
    };
    WriteArray(filename, data_extent, write_extent, array);
  } else {
    WriteArray(filename, data_extent, data_extent, array);
  }
}

void vtkJUPITERWriter::WriteArray(const char *filename, vtkDataObject *object,
                                  vtkDataArray *array)
{
  vtkInformation *info = object->GetInformation();
  int *extent = info->Get(vtkDataObject::DATA_EXTENT());
  if (extent) {
    int ng = info->Get(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS());
    WriteArray(filename, extent, ng, array);
  } else {
    vtkErrorMacro(<< "Input data must be vtkRectilinearGrid or vtkImageData");
  }
}

int vtkJUPITERWriter::ComputeStructuredNumberOfPrcoess(
  vtkIntArray *each_rank_extents, int *npex, int *npey, int *npez)
{
  int np = each_rank_extents->GetNumberOfTuples();
  int lnpex, lnpey, lnpez;
  auto finder = [&](int component_index, int offset) {
    int e = 0;
    int first = 0;
    int ic = 0;
    for (int it = 0; it < np; it += offset, ++ic) {
      int s = each_rank_extents->GetValue(it * 6 + component_index);
      if (it > 0) {
        if (s == first)
          break;
        if (s != e)
          return 0;
      }
      if (it == 0)
        first = s;
      e = each_rank_extents->GetValue(it * 6 + component_index + 1);
    }
    return ic;
  };

  lnpex = finder(0, 1);
  if (!lnpex)
    return 0;

  lnpey = finder(2, lnpex);
  if (!lnpey)
    return 0;

  lnpez = finder(4, lnpex * lnpey);
  if (!lnpez)
    return 0;

  if (lnpex * lnpey * lnpez != np)
    return 0;

  if (npex)
    *npex = lnpex;
  if (npey)
    *npey = lnpey;
  if (npez)
    *npez = lnpez;
  return 1;
}

int vtkJUPITERWriter::ComputeStructuredRankNumber(int npex, int npey, int npez,
                                                  int rank, int *rank_x,
                                                  int *rank_y, int *rank_z)
{
  int npexy;
  int lrank_x, lrank_y, lrank_z;
  if (npex <= 0 || npey <= 0 || npez <= 0)
    return 0;
  if (rank < 0 || rank > npex * npey * npez)
    return 0;

  npexy = npex * npey;
  lrank_z = rank / npexy;
  rank = rank % npexy;
  lrank_y = rank / npex;
  lrank_x = rank % npex;

  if (rank_x)
    *rank_x = lrank_x;
  if (rank_y)
    *rank_y = lrank_y;
  if (rank_z)
    *rank_z = lrank_z;
  return 1;
}

int vtkJUPITERWriter::ComputeStructuredNumberOfPrcoess(
  const int local_extent[6], vtkMultiProcessController *controller, int *npex,
  int *npey, int *npez, int *rank_x, int *rank_y, int *rank_z)
{
  if (controller && controller->GetNumberOfProcesses() > 1) {
    int lnpex, lnpey, lnpez;
    vtkNew<vtkIntArray> extents;
    vtkNew<vtkIntArray> lextents;
    extents->SetNumberOfComponents(6);
    extents->SetNumberOfTuples(controller->GetNumberOfProcesses());
    lextents->SetNumberOfComponents(6);
    lextents->SetNumberOfTuples(1);
    for (int i = 0; i < 6; ++i)
      lextents->SetValue(i, local_extent[i]);
    if (!controller->AllGather(lextents, extents))
      return 0;

    if (!ComputeStructuredNumberOfPrcoess(extents, &lnpex, &lnpey, &lnpez))
      return 0;

    if (rank_x || rank_y || rank_z) {
      int ir = controller->GetLocalProcessId();
      if (!ComputeStructuredRankNumber(lnpex, lnpey, lnpez, ir, rank_x, rank_y,
                                       rank_z))
        return 0;
    }
    if (npex)
      *npex = lnpex;
    if (npey)
      *npey = lnpey;
    if (npez)
      *npez = lnpez;
    return 1;

  } else {
    if (npex)
      *npex = 1;
    if (npey)
      *npey = 1;
    if (npez)
      *npez = 1;
    if (rank_x)
      *rank_x = 0;
    if (rank_y)
      *rank_y = 0;
    if (rank_z)
      *rank_z = 0;
    return 1;
  }
}

namespace
{
template <typename ArrayType> class get_element_size
{
public:
  int operator()(vtkDataArray *array, size_t &result) const
  {
    ArrayType *t = ArrayType::SafeDownCast(array);
    if (t) {
      result = sizeof(typename ArrayType::ValueType);
      return 1;
    }
    return 0;
  }
};

template <typename T> class get_element_size_tuple;

template <typename T, typename... Ts>
class get_element_size_tuple<std::tuple<T, Ts...>>
{
  using next = get_element_size_tuple<std::tuple<Ts...>>;

public:
  int operator()(vtkDataArray *array, size_t &result) const
  {
    return get_element_size<T>{}(array, result) || next{}(array, result);
  }
};

template <> class get_element_size_tuple<std::tuple<>>
{
public:
  int operator()(vtkDataArray *array, size_t &result) const
  {
    result = 0;
    return 0;
  }
};
} // namespace

int vtkJUPITERWriter::GetUnitSizeOfArray(vtkDataArray *array)
{
  return GetElementSizeOfArray(array) * array->GetNumberOfComponents();
}

size_t vtkJUPITERWriter::GetElementSizeOfArray(vtkDataArray *array)
{
  using types =
    std::tuple<vtkCharArray, vtkIntArray, vtkLongArray, vtkLongLongArray,
               vtkShortArray, vtkUnsignedCharArray, vtkUnsignedIntArray,
               vtkUnsignedLongLongArray, vtkUnsignedShortArray, vtkFloatArray,
               vtkDoubleArray, vtkIdTypeArray>;

  size_t result;
  if (!get_element_size_tuple<types>{}(array, result))
    return 0;
  return result;
}
