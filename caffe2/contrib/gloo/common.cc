#include "caffe2/contrib/gloo/common.h"

#include "caffe2/core/logging.h"
#include "caffe2/core/tensor.h"

#if defined(GLOO_USE_MPI) && GLOO_USE_MPI
#include <mutex>
#endif

#if defined(GLOO_USE_MPI) && GLOO_USE_MPI
#include <mpi.h>
#endif

#include <gloo/transport/tcp/device.h>
#if defined(GLOO_USE_IBVERBS) && GLOO_USE_IBVERBS
#include <gloo/transport/ibverbs/device.h>
#endif

namespace caffe2 {
namespace gloo {

void signalFailure(Blob* status_blob, std::exception& /* unused */) {
  auto* res = status_blob->GetMutable<TensorCPU>();
  res->Resize(1);
  res->template mutable_data<int32_t>()[0] = 1;
}

std::shared_ptr<::gloo::transport::Device> createDevice(
    const createDeviceAttr attr) {
  if (attr.transport == "tcp") {
    ::gloo::transport::tcp::attr tcpAttr;
    if (attr.interface.size() > 0) {
      tcpAttr.iface = attr.interface;
    }
    return ::gloo::transport::tcp::CreateDevice(tcpAttr);
  } else if (attr.transport == "ibverbs") {
#if defined(GLOO_USE_IBVERBS) && GLOO_USE_IBVERBS
    ::gloo::transport::ibverbs::attr ibverbsAttr;
    ibverbsAttr.port = 1;
    ibverbsAttr.index = 0;
    if (attr.interface.size() > 0) {
      ibverbsAttr.name = attr.interface;
    }
    return ::gloo::transport::ibverbs::CreateDevice(ibverbsAttr);
#else
    CAFFE_THROW(
      "Gloo was not compiled with ibverbs support. ",
      "Please recompile with -DUSE_IBVERBS=1.");
#endif
  }

  CAFFE_THROW("Invalid transport: ", attr.transport);
}

#if defined(GLOO_USE_MPI) && GLOO_USE_MPI
static std::mutex mpiMutex;
static int mpiRefs = 0;

void mpiInitialize() {
  std::lock_guard<std::mutex> lock(mpiMutex);
  if (mpiRefs++ == 0) {
    auto rv = MPI_Init(nullptr, nullptr);
    CAFFE_ENFORCE_EQ(rv, 0, "MPI_Init() failed");
  }
}

void mpiFinalize() {
  std::lock_guard<std::mutex> lock(mpiMutex);
  if (--mpiRefs == 0) {
    auto rv = MPI_Finalize();
    CAFFE_ENFORCE_EQ(rv, 0, "MPI_Finalize() failed");
  }
}
#endif

} // namespace gloo
} // namespace caffe2