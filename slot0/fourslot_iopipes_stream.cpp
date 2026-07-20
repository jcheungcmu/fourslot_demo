// this kernel reads from an IOpipe and writes to an IOpipe

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
using read_iopipe = ext::intel::kernel_readable_io_pipe<pipe_id<1>, packet_data, 0>;

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
