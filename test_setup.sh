#!/bin/bash

echo "======================================"
echo "Distributed OCR System - Setup Test"
echo "======================================"
echo ""

ERRORS=0

echo "Checking dependencies..."
echo ""

check_command() {
    if command -v $1 &> /dev/null; then
        echo "✓ $1 found: $(command -v $1)"
        return 0
    else
        echo "✗ $1 NOT found"
        ERRORS=$((ERRORS + 1))
        return 1
    fi
}

check_library() {
    if pkg-config --exists $1 2>/dev/null; then
        echo "✓ $1 found: $(pkg-config --modversion $1)"
        return 0
    else
        echo "✗ $1 NOT found"
        ERRORS=$((ERRORS + 1))
        return 1
    fi
}

echo "Build tools:"
check_command cmake
check_command make
check_command g++
echo ""

echo "Qt5:"
if command -v qmake &> /dev/null || pkg-config --exists Qt5 2>/dev/null; then
    echo "✓ Qt5 found"
else
    echo "✗ Qt5 NOT found"
    ERRORS=$((ERRORS + 1))
fi
echo ""

echo "gRPC and Protobuf:"
check_command protoc
if ldconfig -p 2>/dev/null | grep -q libgrpc++ || [ -f /usr/local/lib/libgrpc++.dylib ] || [ -f /opt/homebrew/lib/libgrpc++.dylib ]; then
    echo "✓ gRPC found"
else
    echo "✗ gRPC NOT found"
    ERRORS=$((ERRORS + 1))
fi
echo ""

echo "Tesseract:"
check_command tesseract
if [ -d /usr/share/tesseract-ocr/*/tessdata ] || [ -d /usr/local/share/tessdata ] || [ -d /opt/homebrew/share/tessdata ]; then
    echo "✓ Tesseract language data found"
else
    echo "⚠ Tesseract language data might not be installed"
fi
echo ""

echo "======================================"
if [ $ERRORS -eq 0 ]; then
    echo "✓ All dependencies found!"
    echo ""
    echo "You can proceed to build the project:"
    echo "  mkdir build && cd build"
    echo "  cmake .."
    echo "  cmake --build ."
else
    echo "✗ Missing $ERRORS dependencies"
    echo ""
    echo "Please install missing dependencies."
    echo "Run ./setup.sh or see SETUP_GUIDE.md for instructions."
fi
echo "======================================"

exit $ERRORS
