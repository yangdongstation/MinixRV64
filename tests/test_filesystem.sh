#!/bin/bash
# Test filesystem functionality in MinixRV64

echo "Testing MinixRV64 Filesystem Support"
echo "====================================="
echo ""
echo "This script will run QEMU and test filesystem operations."
echo "Commands to try:"
echo ""
echo "1. ls /          - List root directory"
echo "2. mkdir /test   - Create a directory"
echo "3. ls /          - List again to see new directory"
echo "4. write /hello.txt Hello from MinixRV64 - Create a file with content"
echo "5. cat /hello.txt - Read the file"
echo "6. ls /          - List to see the file"
echo ""
echo "Press Enter to start QEMU..."
read

make qemu
