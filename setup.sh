#!/bin/bash

set -e

echo "======================================"
echo "Distributed OCR System Setup Script"
echo "======================================"
echo ""

detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -f /etc/debian_version ]; then
            echo "debian"
        elif [ -f /etc/fedora-release ]; then
            echo "fedora"
        else
            echo "linux"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    else
        echo "unknown"
    fi
}

OS=$(detect_os)
echo "Detected OS: $OS"
echo ""

install_dependencies() {
    case $OS in
        debian)
            echo "Installing dependencies for Debian/Ubuntu..."
            sudo apt update
            sudo apt install -y cmake build-essential qt5-default \
                libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
                libtesseract-dev libleptonica-dev tesseract-ocr-eng \
                pkg-config
            ;;
        fedora)
            echo "Installing dependencies for Fedora..."
            sudo dnf install -y cmake gcc-c++ qt5-qtbase-devel \
                grpc-devel grpc-plugins protobuf-devel \
                tesseract-devel leptonica-devel tesseract-langpack-eng
            ;;
        macos)
            echo "Installing dependencies for macOS..."
            if ! command -v brew &> /dev/null; then
                echo "Homebrew not found. Please install Homebrew first:"
                echo "/bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
                exit 1
            fi
            brew install cmake qt5 grpc protobuf tesseract leptonica
            ;;
        *)
            echo "Unsupported OS. Please install dependencies manually."
            echo "Required: cmake, qt5, grpc, protobuf, tesseract, leptonica"
            exit 1
            ;;
    esac
    echo ""
    echo "Dependencies installed successfully!"
    echo ""
}

build_project() {
    echo "Building project..."

    if [ -d "build" ]; then
        echo "Removing old build directory..."
        rm -rf build
    fi

    mkdir build
    cd build

    echo "Running CMake..."
    if [[ "$OS" == "macos" ]]; then
        cmake -DCMAKE_PREFIX_PATH=$(brew --prefix qt5) ..
    else
        cmake ..
    fi

    echo "Compiling..."
    cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

    echo ""
    echo "Build completed successfully!"
    echo ""
}

show_usage() {
    echo "======================================"
    echo "Setup Complete!"
    echo "======================================"
    echo ""
    echo "Executables are located in the 'build' directory:"
    echo "  - Server: build/server/ocr_server"
    echo "  - Client: build/client/ocr_client"
    echo ""
    echo "To run the server:"
    echo "  cd build"
    echo "  ./server/ocr_server [address:port] [num_threads]"
    echo "  Example: ./server/ocr_server 0.0.0.0:50051 4"
    echo ""
    echo "To run the client:"
    echo "  cd build"
    echo "  ./client/ocr_client"
    echo ""
    echo "For detailed instructions, see README.md"
    echo ""
}

read -p "Do you want to install dependencies? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    install_dependencies
fi

read -p "Do you want to build the project? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    build_project
fi

show_usage
