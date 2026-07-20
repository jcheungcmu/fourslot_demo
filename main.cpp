#include <sycl/sycl.hpp>
#include <array>
#include <iostream>
#include <sycl/ext/intel/fpga_extensions.hpp>

// necessary include for dynamic linking
#include <dlfcn.h>

using namespace sycl;
using namespace std;

// Create an exception handler for asynchronous SYCL exceptions
static auto exception_handler = [](sycl::exception_list e_list) {
  for (std::exception_ptr const &e : e_list) {
    try {
      std::rethrow_exception(e);
    }
    catch (std::exception const &e) {
#if _DEBUG
      std::cout << "Failure" << std::endl;
#endif
      std::terminate();
    }
  }
};

// sample payload type
struct key_value {
  uint8_t key;
  uint8_t value;
};

int main() {

  cout << "Starting...\n";
  auto platforms = sycl::platform::get_platforms();

  cout << "Getting platforms\n";

  for (auto platform : sycl::platform::get_platforms())
  {
      std::cout << "\n\n\n\nPlatform: "
                << platform.get_info<sycl::info::platform::name>()
                << std::endl;

      for (auto device : platform.get_devices())
      {
          std::cout << "\n\n\n\n\t****************Device: "
                    << device.get_info<sycl::info::device::name>()
                    << std::endl;
      }
  }

  // dynamic loading flow for splitting kernels across homogeneous fpgas
  // .so wrap bitstream files
  auto slot0_lib = dlopen("/home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/iopipes_test/slot0/fourslot_iopipes_stream.so", RTLD_NOW);
  auto slot0_kernel = (event (*)(queue&, size_t, uint8_t))dlsym(slot0_lib, "slot0_fourslot_iopipes_stream");

  auto slot1_lib = dlopen("/home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/iopipes_test/slot1/fourslot_iopipes_stream.so", RTLD_NOW);
  auto slot1_kernel = (event (*)(queue&, size_t, uint8_t))dlsym(slot1_lib, "slot1_fourslot_iopipes_stream");

  auto slot2_sink_lib = dlopen("/home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/iopipes_test/slot2/fourslot_iopipes_sink.so", RTLD_NOW);
  auto slot2_sink     = (event (*)(queue&, buffer<key_value>&, size_t))dlsym(slot2_sink_lib, "slot2_fourslot_iopipes_sink");

  auto slot3_src_lib = dlopen("/home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/iopipes_test/slot3/fourslot_iopipes_src.so", RTLD_NOW);
  auto slot3_src     =  (event (*)(queue&, buffer<key_value>&, size_t, uint8_t))dlsym(slot3_src_lib, "slot3_fourslot_iopipes_src");

  // number of packets 
  size_t N = 8;

  // physical slot addresses
  uint8_t addr_s0 = 0;
  uint8_t addr_s1 = 1; 
  uint8_t addr_s2 = 2;
  uint8_t addr_s3 = 3;

  // create vectors
  std::vector<key_value> src_mem(N);
  std::vector<key_value> sink_mem(N);

  // initialize the vectors
  for (uint8_t i = 0; i < N; i++) {
    key_value kv = {i, static_cast<uint8_t>(i*10)};
    src_mem[i] = kv; // fill with some data
    sink_mem[i] = {0, 0}; // initialize sink memory
  }

  // create buffers
  buffer<key_value> buf_src_mem(&src_mem[0], N);
  buffer<key_value> buf_sink_mem(&sink_mem[0], N);

  try {


    // slots appear as two different fpga devices
    cout << "CREATING q0\n";
    queue q0(platforms[1].get_devices()[0], exception_handler);
    cout << "CREATING q1\n";
    queue q1(platforms[1].get_devices()[1], exception_handler);
    cout << "CREATING q2\n";
    queue q2(platforms[1].get_devices()[2], exception_handler);
    cout << "CREATING q3\n";
    queue q3(platforms[1].get_devices()[3], exception_handler);

    // 3 -> 1 -> 0 -> 2
    auto ev2 = slot2_sink(q2, buf_sink_mem, N);
    auto ev0 = slot0_kernel(q0, N, addr_s2);
    auto ev1 = slot1_kernel(q1, N, addr_s0);
    auto ev3 = slot3_src(q3, buf_src_mem, N, addr_s1);

    cout << "waiting for kernels to finish\n";
    ev3.wait();
    ev2.wait();
    ev1.wait();
    ev0.wait();
  } catch (std::exception const &e) {
    cout << "An exception is caught while computing on device.\n";
    terminate();
  }

  host_accessor<key_value> result_sink_mem(buf_sink_mem);

  cout << "*************CHECKING*************\n";
  int fail = 0;
  for (size_t i = 0; i < N; i++) {
    cout << "result_sink_mem[" << i << "] = key: " << (int)result_sink_mem[i].key << ", value: " << (int)result_sink_mem[i].value << "\n";
  }

  if (fail > 0) {
    cout << "Sink memory test failed with " << fail << " errors.\n";
  } else {
    cout << "Sink memory test passed successfully.\n";
  }

  cout << "Successfully completed on device.\n";
  return 0;


}
