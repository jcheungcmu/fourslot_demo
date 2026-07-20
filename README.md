# Multislot IO Pipe Demonstration (`iopipes_test`)

This directory contains a SYCL/oneAPI demonstration and test suite for Intel's Open FPGA Stack (OFS) targeting a multi-slot FPGA system (such as the Intel Dev Kit with USM and NoC support: `ofs_iseries-dk_usm_noc`).

It demonstrates routing packetized data across four independent FPGA slots using Network-on-Chip (NoC) based IO pipes.

---

## Directory Structure

The repository is organized as follows:

*   **`slot0/`**: Contains the source code ([fourslot_iopipes_stream.cpp](file:///home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/old/iopipes_test/slot0/fourslot_iopipes_stream.cpp)) for the FPGA stream kernel running on Slot 0.
*   **`slot1/`**: Contains the source code ([fourslot_iopipes_stream.cpp](file:///home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/old/iopipes_test/slot1/fourslot_iopipes_stream.cpp)) for the FPGA stream kernel running on Slot 1.
*   **`slot2/`**: Contains the source code ([fourslot_iopipes_sink.cpp](file:///home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/old/iopipes_test/slot2/fourslot_iopipes_sink.cpp)) for the FPGA sink kernel running on Slot 2, which writes received data back to host memory.
*   **`slot3/`**: Contains the source code ([fourslot_iopipes_src.cpp](file:///home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/old/iopipes_test/slot3/fourslot_iopipes_src.cpp)) for the FPGA source kernel running on Slot 3, which reads input data from host memory.
*   **`main.cpp`**: The host application that orchestrates the test by dynamically loading the compiled slot kernels, submitting them to the device queues, and validating the processed output.
*   **`Makefile`**: Compilation rules for individual slots (`s0`, `s1`, `s2`, `s3`), early linking (`s0_early` etc.), and the host application.
*   **`build.sh`**: A shell script that sequentially builds the kernel libraries for all slots. It automatically updates the slot-specific hardware build configurations before running each compilation.

---

## Data Flow Topology

Data starts on the host, flows sequentially through all four FPGA slots via the NoC, and is written back to the host:

```
[ Host Input Buffer ]
        │
        ▼ (SYCL Accessor)
┌──────────────────────────────────────┐
│  Slot 3: Source (iopipes_src)       │
└──────────────────────────────────────┘
        │
        ▼ (NoC IO Pipe: dest_addr = 1)
┌──────────────────────────────────────┐
│  Slot 1: Stream (iopipes_stream)     │ <-- Increments key & value by 1
└──────────────────────────────────────┘
        │
        ▼ (NoC IO Pipe: dest_addr = 0)
┌──────────────────────────────────────┐
│  Slot 0: Stream (iopipes_stream)     │ <-- Increments key & value by 1
└──────────────────────────────────────┘
        │
        ▼ (NoC IO Pipe: dest_addr = 2)
┌──────────────────────────────────────┐
│  Slot 2: Sink (iopipes_sink)         │
└──────────────────────────────────────┘
        │
        ▼ (SYCL Accessor)
[ Host Output Buffer ]
```

### Component Details
1. **Slot 3 (Source)**: Reads input packets from the host SYCL buffer `a_buf` and writes them to the NoC IO pipe, targeting Slot 1.
2. **Slot 1 (Stream)**: Reads packets from the NoC IO pipe, increments both the payload `key` and `value` by 1, and writes them to the next NoC IO pipe targeting Slot 0.
3. **Slot 0 (Stream)**: Reads packets, increments the payload `key` and `value` by 1 again, and writes them targeting Slot 2.
4. **Slot 2 (Sink)**: Reads packets from the NoC IO pipe and writes the payload back to the host SYCL buffer `b_buf`.

*Note: Only Slot 2 and Slot 3 can read/write host buffers directly.*

---

## Build System

To compile the kernels and the host executable:

### Prerequisites
- The environment variable `OFS_ASP_ROOT` must be set to the root of the OFS Board Support Package (BSP).
- Intel oneAPI compiler `icpx` must be available in the system PATH.

### Building
You can run the build script to compile the shared objects for all slots:
```bash
./build.sh
```

Alternatively, you can compile specific targets via the [Makefile](file:///home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/old/iopipes_test/Makefile):
- **Full Hardware/Simulation shared libraries**:
  - `make s0` / `make s1` / `make s2` / `make s3`
- **Early Device/Simulation link files** (for quick compilation/checks):
  - `make s0_early` / `make s1_early` / `make s2_early` / `make s3_early` / `make all_early`
- **Host Application**:
  - `make main`

---

## Execution & Verification

1. Compile the host program:
   ```bash
   make main
   ```
2. Run the host executable:
   ```bash
   ./main
   ```

### How the Host Verification Works
1. The [main.cpp](file:///home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/old/iopipes_test/main.cpp) host program loads the four compiled kernel libraries (`.so` wrappers around FPGA bitstreams) dynamically using `dlopen`/`dlsym`.
2. It discovers the FPGA platforms and slots, mapping them to 4 distinct SYCL device queues (`q0` to `q3`).
3. It initializes input data in a host vector (`src_mem`), then runs the four kernels asynchronously on their respective slot queues.
4. Finally, it checks if the results in `sink_mem` reflect the expected operations (i.e., data passing successfully through all hops and accumulating the operations performed by the stream kernels).
