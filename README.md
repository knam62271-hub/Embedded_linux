# Embedded_linux
This repository contains lab exercises, assignments, source code, and testing scripts developed throughout the LOS201 - Linux and Open Source Platform course.
## Development Environment

| Component | Configuration |
|---|---|
| Host Operating System | Ubuntu Linux |
| Host Architecture | x86_64 |
| Target Processor | ARM Cortex-A9 |
| Emulation Platform | QEMU |
| QEMU Machine | `vexpress-a9` |
| Target Memory | 512 MB |
| Linux Kernel | 5.15 |
| U-Boot | 2022.04 |
| BusyBox | 1.35.0 |
| Cross-Compiler | `arm-linux-gnueabihf-` |

## Repository Structure

```text
Embedded_linux/
├── Assignment 1/
│   ├── bao_cao/
│   ├── logs/
│   ├── task1_ioctl/
│   └── task2_misc/
│
├── Assignment 2/
│   ├── Task 1/
│   ├── Task 2/
│   ├── Task 3/
│   ├── Task 4/
│   ├── bao_cao/
│   ├── logs/
│   └── output/
│
├── Lab 1/
│   ├── initramfs/
│   ├── output/
│   └── u-boot-2022.04/
│
├── Lab 2/
│   ├── driver/
│   ├── logs/
│   ├── output/
│   └── rootfs/
│
├── Lab 3/
│   ├── apps/
│   ├── logs/
│   └── output/
│ 
├── .gitignore
└── README.md
```

## Lab Exercises & Assignments Overview

---

### Lab Exercises

#### **Lab 1: Kernel Configuration & Embedded Boot Sequence**
*   **Target Toolchain:** Configured cross-compilation using `arm-linux-gnueabihf-` target for hard-float ABI compatibility on Cortex-A9.
*   **Kernel & Bootloader Build:** Configured and compiled **Linux kernel v5.15** using `vexpress_defconfig` and built **U-Boot v2022.04** bootloader image for `vexpress_ca9x4`.
*   **Root Filesystem:** Constructed a minimalist root filesystem built on **BusyBox v1.35.0** linked statically.
*   **Emulation & Verification:** Booted the fully integrated system image (Kernel + Device Tree Blob + initramfs) under QEMU system emulation (`vexpress-a9`).

#### **Lab 2: Device Driver Writing & File System Management**
*   **Character Device Driver:** Developed a custom kernel character module (`lab2_driver.ko`) implementing safe file operations (`open`, `read`, `write`, `release`) with `mutex` thread protection and dynamic user-space memory synchronization via `copy_to_user` and `copy_from_user`.
*   **Pseudo Filesystems:** Integrated custom statistical and operational diagnostics interfaces exposed dynamically via **procfs** (`/proc/lab2_info`) and **sysfs** attributes.
*   **MTD & JFFS2 Simulation:** Automated raw NAND flash simulation using `nandsim` on host, partitioned memory using the Memory Technology Device (MTD) subsystem, formatted filesystems using `JFFS2`, and conducted read/write persistence benchmarks.
*   **Auto-loading Integration:** Altered the BusyBox initial framework (`/etc/inittab` and `/etc/init.d/rcS`) to trigger automated driver node initialization upon startup execution.

#### **Lab 3: Kernel Debugging & Embedded Performance Profiling**
*   **Remote Debugging:** Established remote debug sessions routing host-side `gdb-multiarch` cross-communication to QEMU target instances running `gdbserver` to trace null-pointer errors, out-of-bound indexes, and overflow bugs.
*   **Memory Leak Analysis:** Utilized `Valgrind` (Memcheck) on architectural logic paths to isolate dynamic allocation leaks (`definitely lost` vs `indirectly lost`) alongside tracking physical RSS footprint patterns via target `/proc/PID/status`.

## Course Assignments

### **Assignment 1: How to Write a Device Driver**
*   **Task 1:** Expanded character driver capabilities (`asgn1_driver`) by implementing a distinct `unlocked_ioctl` architecture supporting operations such as buffer resets, runtime state struct streaming (`GET_STATS`), and active operation filtering modes (`KERNEL` vs `ECHO` mode).
*   **Task 2:** Authored an isolated atomic hardware counter simulator utilizing the simplified Linux `miscdevice` registration layer framework (`misc_register`/`misc_deregister`) enforced by lockless thread-safe `atomic_t` structures.
*   **Task 3 & 4:** Composed rigorous structural system mapping diagrams, compiled cross-comparisons between Character, Block, and Network drivers, and delivered low-level technical reports on MMU virtual-memory boundaries, MTD system behavior, and VFS file offset processing loops.

### **Assignment 2: Write Embedded Application – Sensor Monitoring System**
*   **Application Multi-threading:** Built a concurrent C system monitoring program on target consisting of four specialized POSIX threads (`Sensor`, `Processor`, `Alert`, and `Logger`) orchestrating real-time safe system states.
*   **Inter-Process Communication (IPC):** Handled runtime synchronization dependencies via custom condition-variable thread-safe circular buffers and POSIX message queues (`mq_open`/`mq_receive`) for deterministic asynchronous system exception dispatch.
*   **Hardware Integration:** Streamed dynamically mapped CSV sensors strings out of an isolated kernel module (`sms_sensor.ko`) utilizing kernel-level pseudo-random distributions, adjustable hardware `ioctl` clock pacing steps, and diagnostic procfs trackers.
*   **Real-Time Thread Optimizations:** Configured strict execution deadlines inside critical loops by setting FIFO scheduling bounds (`SCHED_FIFO`), deploying atomic thread timeouts via `clock_nanosleep`, and locking physical page faults via `mlockall`.
*   **Performance Diagnostics:** Profiling of I/O boundaries and runtime computation hotspots by generating granular kernel-space metrics with `strace` summary statistics and hardware performance counter profiling with `perf`.
*   **Real-Time Latency Analysis:** Calculated quantitative latency spikes using `cyclictest` (stress-loaded via `stress-ng`), constructed empirical latency histograms using `gnuplot`, and analyzed the design benefits of standard kernels versus fully preemptible `PREEMPT_RT` kernels.

---
