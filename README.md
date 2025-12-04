# Distributed OCR System

A client-server distributed system for Optical Character Recognition (OCR) using C++, Qt, Tesseract, and gRPC.

## Project Structure

```
stdiscm-ps4/
├── proto/                  # Protocol Buffer definitions
│   └── ocr_service.proto  # gRPC service definition
├── server/                # Server application
│   ├── main.cpp
│   ├── ocr_server.h
│   ├── ocr_server.cpp
│   └── CMakeLists.txt
├── client/                # Client application
│   ├── main.cpp
│   ├── ocr_client.h
│   ├── ocr_client.cpp
│   └── CMakeLists.txt
└── CMakeLists.txt         # Root CMake configuration
```

## Features

### Client Features
- Qt-based graphical user interface
- Upload multiple images simultaneously
- Real-time progress tracking with progress bar
- Display OCR results as they complete (streaming results)
- Grid layout showing images and their extracted text
- Batch processing with automatic batch management
- Connection error handling and retry logic
- Support for multiple image formats (PNG, JPG, JPEG, BMP, GIF, TIFF)

### Server Features
- Multi-threaded OCR processing with configurable thread pool
- gRPC-based communication with streaming responses
- Tesseract OCR engine integration
- Thread-safe task queue with mutex synchronization
- Fault tolerance with connection error handling
- Support for running on separate machines or VMs
- Concurrent processing of multiple images

## System Requirements

### Dependencies
- **CMake** (>= 3.15)
- **C++ Compiler** with C++17 support (GCC, Clang, or MSVC)
- **Qt 5** (Core and Widgets modules)
- **gRPC** and **Protocol Buffers**
- **Tesseract OCR** (>= 4.0)
- **Leptonica** (image processing library)

### Operating Systems
- Linux (Ubuntu, Debian, Fedora, etc.)
- macOS (10.14+)
- Windows (with appropriate build tools)

## Installation Instructions

### Option 1: Single Machine Testing (Localhost)

If you want to test on a single machine, you can run both client and server on `localhost`.

#### Step 1: Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install -y cmake build-essential qt5-default \
    libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
    libtesseract-dev libleptonica-dev tesseract-ocr-eng
```

**macOS (using Homebrew):**
```bash
brew install cmake qt5 grpc protobuf tesseract leptonica
```

**Fedora:**
```bash
sudo dnf install -y cmake gcc-c++ qt5-qtbase-devel \
    grpc-devel grpc-plugins protobuf-devel \
    tesseract-devel leptonica-devel tesseract-langpack-eng
```

#### Step 2: Build the Project

```bash
cd /Users/alfredvictoria/Dev/stdiscm-ps4

# Create build directory
mkdir build && cd build

# Configure CMake
cmake ..

# Build the project
cmake --build . -j$(nproc)
```

#### Step 3: Run the Server

```bash
# From the build directory
./server/ocr_server 0.0.0.0:50051 4
```

Arguments:
- `0.0.0.0:50051` - Server address and port (default if not specified)
- `4` - Number of worker threads (default is 4)

#### Step 4: Run the Client

Open a new terminal:

```bash
# From the build directory
./client/ocr_client
```

In the client GUI:
1. Keep the default server address as `localhost:50051`
2. Click "Upload Images" to select image files
3. Watch as OCR results appear in real-time

### Option 2: Two-Machine Setup (Distributed System)

This is the recommended setup to demonstrate true distributed computing.

#### Machine 1: Server (Can be a VM or separate computer)

1. Install dependencies (see Step 1 above)

2. Build the project:
```bash
cd /path/to/stdiscm-ps4
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

3. Find the server's IP address:
```bash
# Linux/macOS
ip addr show  # or: ifconfig

# Look for inet address, e.g., 192.168.1.100
```

4. Run the server:
```bash
./server/ocr_server 0.0.0.0:50051 4
```

**Important:** Make sure firewall allows port 50051:
```bash
# Ubuntu/Debian
sudo ufw allow 50051

# Fedora/CentOS
sudo firewall-cmd --permanent --add-port=50051/tcp
sudo firewall-cmd --reload
```

#### Machine 2: Client (Your main computer)

1. Install dependencies (see Step 1 above)

2. Build the project:
```bash
cd /path/to/stdiscm-ps4
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

3. Run the client:
```bash
./client/ocr_client
```

4. In the client GUI:
   - Change server address to your server's IP: `192.168.1.100:50051`
   - Click "Upload Images" and select images
   - OCR results will stream back from the remote server

### Option 3: Using Virtual Machines

You can run the server in a VM (VirtualBox, VMware, etc.) and the client on your host machine.

#### VM Setup (Server):

1. Install Ubuntu/Debian in VM
2. Set network adapter to "Bridged" mode (to get its own IP on your network)
3. Follow "Machine 1: Server" instructions above
4. Note the VM's IP address

#### Host Machine (Client):

1. Follow "Machine 2: Client" instructions above
2. Use VM's IP address as server address

## Usage Guide

### Starting a New Session

1. **Start the server** first on the server machine
2. **Start the client** on the client machine
3. **Enter server address** in the client (e.g., `192.168.1.100:50051`)
4. **Click "Upload Images"** and select one or more image files
5. **Watch progress bar** - it shows X/Y images processed
6. **Results appear immediately** as each image completes (not all at once)

### Batch Processing Behavior

- While progress bar is < 100%, you can upload more images
- New images are added to current batch and progress bar updates
- Once progress reaches 100%, the batch is complete
- Next upload starts a new batch and clears previous results

### Testing Different Scenarios

1. **Single Image:** Upload one image, see instant result
2. **Multiple Images:** Upload 10+ images, watch them complete one by one
3. **Add More Mid-Batch:** Upload 5 images, then add 3 more while processing
4. **Connection Error:** Stop server while client is processing (test fault tolerance)
5. **Large Images:** Test with high-resolution images
6. **Different Formats:** Try PNG, JPG, BMP, TIFF files

## Architecture Details

### Multithreading
- **Server:** Thread pool with configurable size (default: 4 threads)
- **Client:** Separate thread for network I/O to keep UI responsive
- Each worker thread has its own Tesseract instance to avoid conflicts

### Synchronization
- **Server:** Uses mutex + condition variable for task queue
- Thread-safe writer access for streaming results
- Atomic flags for clean shutdown

### GUI Implementation
- **Qt Framework** with Model-View architecture
- Signal-slot mechanism for thread-safe UI updates
- Progress bar tracks completion percentage
- Grid layout automatically arranges results

### Interprocess Communication
- **gRPC** with Protocol Buffers for efficient serialization
- Server streaming for real-time result delivery
- Binary image data transfer
- Timeout handling (60 seconds per image)

### Fault Tolerance
- Connection timeout detection
- Graceful error display in UI
- Server crash recovery (client shows error, can retry)
- Network interruption handling

## Troubleshooting

### Build Errors

**CMake can't find Qt5:**
```bash
# Set Qt5 path manually
cmake -DCMAKE_PREFIX_PATH=/usr/local/opt/qt5 ..
```

**gRPC not found:**
```bash
# Install gRPC from source or use package manager
# Ubuntu: sudo apt install libgrpc++-dev
# macOS: brew install grpc
```

**Tesseract not found:**
```bash
# Ubuntu: sudo apt install libtesseract-dev tesseract-ocr-eng
# macOS: brew install tesseract
```

### Runtime Errors

**"Failed to connect to server":**
- Check server is running: `ps aux | grep ocr_server`
- Verify server address and port are correct
- Check firewall settings
- Test connectivity: `telnet <server-ip> 50051`

**"Could not initialize tesseract":**
- Install Tesseract language data: `sudo apt install tesseract-ocr-eng`
- Or download from: https://github.com/tesseract-ocr/tessdata

**"Connection timeout":**
- Large images may take longer than 60s
- Increase timeout in client/ocr_client.cpp:
  ```cpp
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(120));
  ```

### Network Issues Between Machines

**Can't connect from client to server:**
1. Ping server: `ping <server-ip>`
2. Check port is open: `telnet <server-ip> 50051` or `nc -zv <server-ip> 50051`
3. Disable firewall temporarily to test
4. Ensure machines are on same network

## Performance Tips

1. **Adjust thread count** based on CPU cores:
   ```bash
   ./server/ocr_server 0.0.0.0:50051 8  # 8 threads
   ```

2. **Use SSD** for faster image loading

3. **Optimize images** - smaller images process faster

4. **Network bandwidth** - local network is faster than internet

## Testing the System

### Test Checklist

- [ ] Single image upload and result display
- [ ] Multiple images upload (5-10 images)
- [ ] Add images while batch is processing
- [ ] Progress bar updates correctly
- [ ] Results appear one by one (not all at once)
- [ ] New batch clears previous results
- [ ] Server handles concurrent requests
- [ ] Connection error shows proper message
- [ ] Different image formats work
- [ ] Large images process successfully

### Sample Test Images

You can create test images with text:
- Take screenshots of text documents
- Use online OCR test images
- Generate images with text using image editors
- Download from: https://github.com/tesseract-ocr/tesseract/tree/main/testing

## Implementation Highlights

### Key Technologies
- **C++17** for modern language features
- **Qt 5** for cross-platform GUI
- **gRPC** for efficient RPC communication
- **Tesseract 4+** for state-of-the-art OCR
- **Protocol Buffers** for data serialization

### Design Patterns
- Producer-Consumer pattern (task queue)
- Observer pattern (Qt signals/slots)
- Client-Server architecture
- Thread pool pattern

## Contributing

This is an academic project for STDISCM. Modifications should focus on:
- Improving OCR accuracy
- Adding more image preprocessing
- Enhancing UI/UX
- Better error handling
- Performance optimization

## License

Academic project - STDISCM Problem Set 4

## Authors

Developed for STDISCM course requirements.
