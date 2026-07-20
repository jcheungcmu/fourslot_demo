//==============================================================
// Iota is the equivalent of a Hello, World! sample for data parallel programs.
// Building and running the sample verifies that your development environment
// is setup correctly and demonstrates the use of the core features of SYCL.
// This sample runs on both CPU and GPU (or FPGA). When run, it computes on both
// the CPU and offload device, then compares results. If the code executes on
// both CPU and the offload device, the name of the offload device and a success
// message are displayed. And, your development environment is setup correctly!
//
// For comprehensive instructions regarding SYCL Programming, go to
// https://software.intel.com/en-us/oneapi-programming-guide and search based on
// relevant terms noted in the comments.
//
// SYCL material used in the code sample:
// •	A one dimensional array of data.
// •	A device queue, buffer, accessor, and kernel.
//==============================================================
// Copyright © 2020 Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#include <sycl/sycl.hpp>
#include <array>
#include <iostream>

// #if FPGA_HARDWARE || FPGA_EMULATOR || FPGA_SIMULATOR
#include <sycl/ext/intel/fpga_extensions.hpp>
// #endif

using namespace sycl;
using namespace std;

//************************************
// Iota in SYCL on device.
//************************************

template <unsigned ID>
struct ethernet_pipe_id {
  static constexpr unsigned id = ID;
};

struct key_value {
  uint8_t key;
  uint8_t value;
};

struct packet_data{
  uint8_t dest_addr; // position of dest_addr is important for extraction to noc
  uint8_t src_addr;
  uint8_t user;
  key_value payload;
};

using write_iopipe = ext::intel::kernel_writeable_io_pipe<ethernet_pipe_id<0>, packet_data, 0>;

extern "C" {
  event slot3_fourslot_iopipes_src(queue &q, buffer<key_value>& a_buf, size_t num_items, uint8_t dest) {

    return q.submit([&](handler &h) {
      // Create an accessor with read permission.
      accessor a(a_buf, h, read_only);
      // accessor b(b_buf, h, write_only, no_init);

      h.single_task<class iopipes_src_test> ([=]() { 

        packet_data write_packet;

        write_packet.dest_addr = dest;
        write_packet.src_addr = 3;
        write_packet.user = 0;
        
        for (size_t i = 0; i < num_items; i++) {

          write_packet.payload = a[i];
          write_iopipe::write(write_packet);

        }
      });

    });
  }
  
}
