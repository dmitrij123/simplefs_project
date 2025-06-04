# Simple Filesystem Project

This repository contains a simple filesystem module for Linux kernel 6.14.

## Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/dmitrij123/simplefs_project.git
   cd simplefs_project
2. Build the module:
   make clean
   make
3. Load the module:
   sudo insmod simplefs.ko disk_name="/dev/loop19" sb_first_offset=0 sb_second_offset=1024 max_filename_len=256 max_file_size_sectors=10
4. Mount the filesystem:
   sudo mkdir -p /mnt/simplefs
   sudo mount -t simplefs /dev/loop19 /mnt/simplefs
5. Test:
   ls -a /mnt/simplefs
   sudo touch /mnt/simplefs/testfile
   ls -a /mnt/simplefs

## Uninstallation
  sudo umount /mnt/simplefs
  sudo rmmod simplefs

  Notes
Ensure /dev/loop19 is set up (e.g., with losetup).
Check dmesg for logs: sudo dmesg | tail.
undefined
