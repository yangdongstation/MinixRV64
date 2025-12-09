#!/bin/bash

# Minix RV64 Build Script

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
BOARD=${BOARD:-"qemu-virt"}
CROSS_COMPILE=${CROSS_COMPILE:-"riscv64-unknown-elf-"}
CLEAN=0
VERBOSE=0
QEMU=0

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Print usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -b, --board BOARD      Target board (qemu-virt|milkv-duo) [default: qemu-virt]"
    echo "  -c, --clean            Clean before build"
    echo "  -v, --verbose          Verbose build"
    echo "  -q, --qemu             Run in QEMU after successful build"
    echo "  -t, --toolchain CC     Cross compiler prefix"
    echo "  -h, --help             Show this help"
    echo ""
    echo "Examples:"
    echo "  $0                     # Build for QEMU virt"
    echo "  $0 -b milkv-duo        # Build for MilkV Duo"
    echo "  $0 -cv                 # Clean and build with verbose output"
    echo "  $0 -q                  # Build and run in QEMU"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--board)
            BOARD="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -q|--qemu)
            QEMU=1
            shift
            ;;
        -t|--toolchain)
            CROSS_COMPILE="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Validate board
if [[ "$BOARD" != "qemu-virt" && "$BOARD" != "milkv-duo" ]]; then
    print_error "Invalid board: $BOARD"
    usage
    exit 1
fi

# Check toolchain
if ! command -v ${CROSS_COMPILE}gcc &> /dev/null; then
    print_error "Toolchain not found: ${CROSS_COMPILE}gcc"
    print_warning "Install with: ./tools/setup_toolchain.sh"
    exit 1
fi

# Print build configuration
print_status "=== Minix RV64 Build Configuration ==="
echo "  Board: $BOARD"
echo "  Toolchain: $CROSS_COMPILE"
echo "  Clean: $CLEAN"
echo "  Verbose: $VERBOSE"
echo "  Run QEMU: $QEMU"
echo ""

# Clean if requested
if [[ $CLEAN -eq 1 ]]; then
    print_status "Cleaning..."
    make BOARD=$BOARD clean
fi

# Build
print_status "Building kernel..."
export BOARD=$BOARD

if [[ $VERBOSE -eq 1 ]]; then
    make BOARD=$BOARD CROSS_COMPILE=$CROSS_COMPILE all
else
    make BOARD=$BOARD CROSS_COMPILE=$CROSS_COMPILE all > /dev/null 2>&1
fi

if [ $? -eq 0 ]; then
    print_status "✓ Build successful!"
else
    print_error "✗ Build failed!"
    exit 1
fi

# Show build artifacts
echo ""
print_status "Build artifacts:"
ls -la minix-rv64.*

# Show kernel info
if command -v ${CROSS_COMPILE}readelf &> /dev/null; then
    echo ""
    print_status "Kernel ELF info:"
    ${CROSS_COMPILE}readelf -h minix-rv64.elf | grep -E "(Class|Machine|Entry|Type)"
fi

# Run QEMU if requested
if [[ $QEMU -eq 1 ]]; then
    echo ""
    print_status "Starting QEMU..."
    echo "Press Ctrl+A, X to exit QEMU"
    echo ""

    if [[ "$BOARD" == "qemu-virt" ]]; then
        make BOARD=$BOARD qemu
    else
        print_warning "QEMU not supported for $BOARD"
        print_status "You can flash the kernel to your $BOARD device"
    fi
fi

echo ""
print_status "=== Build complete ==="