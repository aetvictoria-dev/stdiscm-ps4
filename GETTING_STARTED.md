# Getting Started - Distributed OCR System

## What You Have

A complete distributed OCR (Optical Character Recognition) system with:

- **Server:** C++ application with multithreading and Tesseract OCR
- **Client:** Qt-based GUI for uploading images and viewing results
- **gRPC Communication:** Efficient client-server protocol
- **Full Documentation:** Setup guides, demonstration guide, and troubleshooting

## Project Files Overview

```
stdiscm-ps4/
├── README.md                    # Main documentation
├── SETUP_GUIDE.md              # Step-by-step setup instructions
├── DEMONSTRATION.md            # Guide for demo video
├── GETTING_STARTED.md          # This file
├── setup.sh                    # Automated setup script
├── test_setup.sh               # Test if dependencies are installed
│
├── proto/                      # gRPC Protocol Buffers
│   └── ocr_service.proto       # Service definition
│
├── server/                     # Server application
│   ├── main.cpp               # Server entry point
│   ├── ocr_server.h           # Server header
│   ├── ocr_server.cpp         # Server implementation
│   └── CMakeLists.txt         # Server build config
│
├── client/                     # Client application
│   ├── main.cpp               # Client entry point
│   ├── ocr_client.h           # Client header
│   ├── ocr_client.cpp         # Client implementation
│   └── CMakeLists.txt         # Client build config
│
└── CMakeLists.txt             # Root build configuration
```

## Quick Start (3 Steps)

### 1. Check Dependencies

```bash
cd /Users/alfredvictoria/Dev/stdiscm-ps4
./test_setup.sh
```

If you see "All dependencies found!", skip to step 2.

If dependencies are missing, run:
```bash
./setup.sh
```

### 2. Build the Project

```bash
mkdir build && cd build
cmake ..
cmake --build . -j4
```

### 3. Run It!

**Terminal 1 (Server):**
```bash
./server/ocr_server
```

**Terminal 2 (Client):**
```bash
./client/ocr_client
```

Upload images and see OCR results!

## Choose Your Setup

### Option A: Single Machine (Easiest)
- Run both client and server on your laptop
- Perfect for initial testing
- Server address: `localhost:50051`
- **See:** SETUP_GUIDE.md - "Quick Start (Single Machine)"

### Option B: Two Machines (Recommended for Demo)
- Server on one computer/VM
- Client on another computer
- Demonstrates true distributed system
- **See:** SETUP_GUIDE.md - "Two Machine Setup"

### Option C: VM Setup
- Server in VirtualBox/VMware
- Client on host machine
- Good compromise if you only have one computer
- **See:** SETUP_GUIDE.md - "VM Setup"

## What to Read Next

1. **First time setup?** → Read `SETUP_GUIDE.md`
2. **Need detailed info?** → Read `README.md`
3. **Preparing demo video?** → Read `DEMONSTRATION.md`
4. **Have problems?** → See "Troubleshooting" in `SETUP_GUIDE.md`

## Common Commands

**Test dependencies:**
```bash
./test_setup.sh
```

**Auto install + build:**
```bash
./setup.sh
```

**Manual build:**
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

**Run server (default):**
```bash
./server/ocr_server
```

**Run server (custom):**
```bash
./server/ocr_server 0.0.0.0:50051 8
# Arguments: [address:port] [num_threads]
```

**Run client:**
```bash
./client/ocr_client
```

**Clean build:**
```bash
rm -rf build
mkdir build && cd build
cmake .. && cmake --build .
```

## System Requirements

### Minimum:
- 2 CPU cores
- 2 GB RAM
- 500 MB disk space
- macOS 10.14+ or Linux (Ubuntu 18.04+, Fedora 30+)

### Recommended:
- 4+ CPU cores (for better multithreading)
- 4 GB RAM
- SSD storage
- Wired network connection (for two-machine setup)

## Testing Your Setup

Once built, test these scenarios:

1. **Single image:** Upload one image → instant result
2. **Multiple images:** Upload 10 images → see parallel processing
3. **Mid-batch upload:** Add more images while processing
4. **Batch completion:** Let it reach 100%, upload new batch
5. **Error handling:** Stop server while processing → see error messages

## Dependencies List

If you need to install manually:

**Ubuntu/Debian:**
```bash
sudo apt install cmake build-essential qt5-default \
  libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
  libtesseract-dev libleptonica-dev tesseract-ocr-eng
```

**macOS:**
```bash
brew install cmake qt5 grpc protobuf tesseract leptonica
```

**Fedora:**
```bash
sudo dnf install cmake gcc-c++ qt5-qtbase-devel \
  grpc-devel grpc-plugins protobuf-devel \
  tesseract-devel leptonica-devel tesseract-langpack-eng
```

## Troubleshooting Quick Fixes

**Build fails with "Qt5 not found":**
```bash
# macOS
cmake -DCMAKE_PREFIX_PATH=$(brew --prefix qt5) ..

# Linux
sudo apt install qt5-default qtbase5-dev
```

**"Failed to connect to server":**
```bash
# Check server is running
ps aux | grep ocr_server

# Test connection
telnet localhost 50051
```

**"Could not initialize tesseract":**
```bash
sudo apt install tesseract-ocr-eng  # Ubuntu
brew install tesseract-lang         # macOS
```

## Next Steps

1. **Get it running locally** (Option A)
2. **Set up distributed mode** (Option B or C)
3. **Test all scenarios** (see DEMONSTRATION.md)
4. **Prepare demo video** (follow DEMONSTRATION.md)
5. **Create presentation slides** (topics in DEMONSTRATION.md)

## For Your Submission

You need to submit:

1. **Presentation PDF** - explaining architecture, multithreading, etc.
2. **Demo Video (MP4)** - 5-10 minutes showing your system
3. **Source Code** - this entire project

**See DEMONSTRATION.md for detailed submission requirements.**

## Key Features to Highlight

When demonstrating, emphasize:

- **Multithreading:** Server uses thread pool (4+ threads)
- **Synchronization:** Thread-safe task queue with mutex
- **GUI:** Qt interface with progress tracking
- **IPC:** gRPC streaming for real-time results
- **Fault Tolerance:** Handles connection errors gracefully

## Architecture Overview

```
┌─────────────────┐                    ┌─────────────────┐
│     Client      │                    │     Server      │
│                 │                    │                 │
│  ┌───────────┐  │                    │  ┌───────────┐  │
│  │  Qt GUI   │  │                    │  │Task Queue │  │
│  │           │  │      gRPC/         │  │           │  │
│  │ - Upload  │◄─┼──── Protobuf ─────┼─►│  (Mutex)  │  │
│  │ - Display │  │                    │  └─────┬─────┘  │
│  │ - Progress│  │                    │        │        │
│  └───────────┘  │                    │  ┌─────▼─────┐  │
│                 │                    │  │  Workers  │  │
│                 │                    │  │ Thread 1  │  │
│                 │                    │  │ Thread 2  │  │
│  Different      │                    │  │ Thread 3  │  │
│  Machine/VM     │                    │  │ Thread 4  │  │
│                 │                    │  └───────────┘  │
└─────────────────┘                    └─────────────────┘
```

## Help & Support

**For issues:**
1. Check error messages in terminal
2. Run `./test_setup.sh` to verify dependencies
3. See "Troubleshooting" in SETUP_GUIDE.md
4. Check README.md for detailed architecture info

**Quick links:**
- Installation: SETUP_GUIDE.md
- Architecture: README.md
- Demo prep: DEMONSTRATION.md
- This guide: GETTING_STARTED.md

## Summary

You have everything you need to:
1. Build and run the distributed OCR system
2. Test it locally or across machines/VMs
3. Demonstrate all required features
4. Create your submission

**Start with:** `./test_setup.sh` then `./setup.sh`

Good luck with your STDISCM project!
