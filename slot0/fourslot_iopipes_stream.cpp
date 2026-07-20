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

using read_iopipe = ext::intel::kernel_readable_io_pipe<ethernet_pipe_id<1>, packet_data, 0>;

extern "C" {
  event slot0_fourslot_iopipes_stream(queue &q, size_t num_items, uint8_t dest) {

    return q.submit([&](handler &h) {

      h.single_task<class iopipes_stream_test> ([=]() { 

        packet_data write_packet; 
        packet_data read_packet;

        write_packet.dest_addr = dest;
        write_packet.src_addr = 0;
        write_packet.user = 0;

        for (size_t i = 0; i < num_items; i++) {
          read_packet = read_iopipe::read();
          write_packet.payload.key = read_packet.payload.key + 1;
          write_packet.payload.value = read_packet.payload.value + 1;

          write_iopipe::write(write_packet);
        }

      });

    });
  }
  
}
