# GateOS

A ground-up OS built from scratch in C and x86 assembly by Tadhg Hallahan.

## Requirements
- Docker
- QEMU (`qemu-system-x86`)

## Build & Run
```bash
make docker-build   # one-time, builds the cross-compiler image (~20 min)
make build          # compiles the kernel and produces a bootable ISO
make run            # boots the ISO in QEMU
```
