# Multislot IO Pipe Demonstration

This repository demonstrates routing data across four independent FPGA slots using Network-on-Chip (NoC) based IO pipes.

---

## Directory Structure

The repository is organized as follows:

*   **`slot0/`**: Contains the source code ([fourslot_iopipes_stream.cpp](file:///home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/old/iopipes_test/slot0/fourslot_iopipes_stream.cpp)) for the FPGA stream kernel running on Slot 0.
*   **`slot1/`**: Contains the source code ([fourslot_iopipes_stream.cpp](file:///home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/old/iopipes_test/slot1/fourslot_iopipes_stream.cpp)) for the FPGA stream kernel running on Slot 1.
*   **`slot2/`**: Contains the source code ([fourslot_iopipes_sink.cpp](file:///home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/old/iopipes_test/slot2/fourslot_iopipes_sink.cpp)) for the FPGA sink kernel running on Slot 2, which writes received data back to host memory.
*   **`slot3/`**: Contains the source code ([fourslot_iopipes_src.cpp](file:///home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/old/iopipes_test/slot3/fourslot_iopipes_src.cpp)) for the FPGA source kernel running on Slot 3, which reads input data from host memory.
*   **`main.cpp`**: The host application that orchestrates the test by dynamically loading the compiled slot kernels, submitting them to the device queues, and validating the processed output.

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

### Compilation Design (Split Kernels)
This project compiles each slot kernel as an independent shared library (`.so`) using the technique described in the Intel oneAPI FPGA Add-on Guide: [Split a Kernel into Multiple FPGA Images](https://www.intel.com/content/www/us/en/docs/oneapi-fpga-add-on/developer-guide/2024-0/split-kernel-into-multiple-fpga-images-linux-only.html).

Normally, the SYCL compilation flow compiles all FPGA kernels into a single device image linked directly inside the host executable. In a multi-slot, Partial Reconfiguration (PR) system, compiling the kernels into independent shared libraries allows us to:
- Compile each slot's kernel independently to target a specific hardware slot partition, avoiding monolithic compiles and saving build time.
- Dynamically load (`dlopen`) and program individual slot kernels onto their respective physical device/slot queues at runtime.
