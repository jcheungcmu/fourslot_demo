// this kernel reads from an IOpipe and writes to a buffer
// only slot 2 and slot 3 can read/write buffers

#include <sycl/sycl.hpp>
#include <array>
#include <iostream>
#include <sycl/ext/intel/fpga_extensions.hpp>

using namespace sycl;
using namespace std;

// required for IOpipe instantiation
template <unsigned ID>
struct pipe_id {
  static constexpr unsigned id = ID;
};

// sample payload type
struct key_value {
  uint8_t key;
  uint8_t value;
};

// IOpipe type
struct packet_data{
  uint8_t dest_addr; // the first 8 bits of the IOpipe type is used for destination by the NoC
  uint8_t src_addr;
  uint8_t user;
  key_value payload;
};

using read_iopipe = ext::intel::kernel_readable_io_pipe<pipe_id<1>, packet_data, 0>;

extern "C" {
  event slot2_fourslot_iopipes_sink(queue &q, buffer<key_value>& b_buf, size_t num_items) {

    return q.submit([&](handler &h) {

      // Create an accessor with write permission
      accessor b(b_buf, h, write_only, no_init);

      h.single_task<class iopipes_sink_test> ([=]() { 

        packet_data read_data_packet;

        for (size_t i = 0; i < num_items; i++) {

            read_data_packet = read_iopipe::read();
            b[i] = read_data_packet.payload;

        }
      });

    });
  }
  
}
