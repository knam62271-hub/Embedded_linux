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
