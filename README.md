# UCHos

A simple microkernel-based operating system for x86-64 architecture, written in C++17 and Assembly.

This is a hobby project focused on simplicity and minimalism, not intended for production use.

## Overview

UCHos follows a microkernel-like architecture where the kernel provides only essential services:
- Process/Task management
- Memory management (Buddy System + Slab Allocator)
- Inter-process communication (IPC)
- Basic device drivers

## Features

### Kernel
- **Memory Management**: Buddy system for physical pages, Slab allocator for kernel objects
- **Task Management**: Round-robin scheduling with context switching
- **File System**: Custom FAT implementation
- **Device Support**: VirtIO block device, USB keyboard (XHCI)
- **IPC**: Message-passing based communication

### Userland
- Interactive shell
- Basic commands: `ls`, `cat`, `echo`, `cd`, `pwd`, `touch`, `free`, `lspci`
- Simple terminal emulator

## Requirements

- **Host OS**: Linux (tested on Ubuntu 22.04 LTS)
- **Architecture**: x86-64
- **Build Tools**:
  - CMake 3.10+
  - Clang/LLVM
  - NASM
  - QEMU (for testing)
- **UEFI**: OVMF firmware files

## Quick Start

```bash
# Clone the repository
git clone https://github.com/junyaU/uchos.git
cd uchos

# Build and run in QEMU
./run_qemu.sh

# Run code quality checks
./lint.sh
```

## Project Structure

```
uchos/
├── kernel/          # Kernel source code
├── UchLoaderPkg/    # UEFI bootloader
├── userland/        # User space programs
├── libs/            # Shared libraries
├── x86_64-elf/      # Cross-compilation toolchain
└── build/           # Build output
```

## Development

See [CLAUDE.md](CLAUDE.md) for detailed development guidelines and architecture documentation.
