// this kernel reads from a buffer and writes to an IOpipe
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

using write_iopipe = ext::intel::kernel_writeable_io_pipe<pipe_id<0>, packet_data, 0>;

extern "C" {
  event slot3_fourslot_iopipes_src(queue &q, buffer<key_value>& a_buf, size_t num_items, uint8_t dest) {

    return q.submit([&](handler &h) {

      // Create an accessor with read permission
      accessor a(a_buf, h, read_only);

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
