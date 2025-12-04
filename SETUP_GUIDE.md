# Quick Setup Guide - Distributed OCR System

This guide provides step-by-step instructions for setting up and running the Distributed OCR System.

## Table of Contents
1. [Quick Start (Single Machine)](#quick-start-single-machine)
2. [Two Machine Setup](#two-machine-setup)
3. [VM Setup](#vm-setup)
4. [Troubleshooting](#troubleshooting)

---

## Quick Start (Single Machine)

Perfect for testing on your laptop/desktop.

### Step 1: Install Dependencies

Run the automated setup script:

```bash
cd /Users/alfredvictoria/Dev/stdiscm-ps4
./setup.sh
```

Answer 'y' to both prompts to install dependencies and build the project.

**OR** install manually:

**macOS:**
```bash
brew install cmake qt5 grpc protobuf tesseract leptonica
```

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install -y cmake build-essential qt5-default \
    libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
    libtesseract-dev libleptonica-dev tesseract-ocr-eng
```

### Step 2: Build the Project

```bash
mkdir build && cd build
cmake ..
cmake --build . -j4
```

For macOS, if Qt5 is not found:
```bash
cmake -DCMAKE_PREFIX_PATH=$(brew --prefix qt5) ..
cmake --build . -j4
```

### Step 3: Run the System

**Terminal 1 - Start Server:**
```bash
cd build
./server/ocr_server
```

You should see:
```
OCR Server initialized with 4 worker threads
Server listening on 0.0.0.0:50051
```

**Terminal 2 - Start Client:**
```bash
cd build
./client/ocr_client
```

A GUI window will open.

### Step 4: Test It

1. In the client GUI, the server address should be `localhost:50051`
2. Click "Upload Images"
3. Select some image files containing text (screenshots, photos of documents, etc.)
4. Watch the progress bar and see results appear in real-time!

---

## Two Machine Setup

Use this to demonstrate true distributed computing (client and server on different machines).

### Machine A: Server Setup

**1. Install and Build:**
```bash
# Clone/copy the project to this machine
cd stdiscm-ps4
./setup.sh  # Or install dependencies manually
mkdir build && cd build
cmake ..
cmake --build . -j4
```

**2. Find Server's IP Address:**
```bash
# Linux
ip addr show | grep "inet "

# macOS
ifconfig | grep "inet "

# Look for something like: 192.168.1.100
```

**3. Open Firewall Port:**
```bash
# Ubuntu/Debian
sudo ufw allow 50051

# Fedora/RHEL
sudo firewall-cmd --permanent --add-port=50051/tcp
sudo firewall-cmd --reload

# macOS (if firewall is on)
# System Preferences > Security & Privacy > Firewall > Firewall Options
# Add ocr_server and allow incoming connections
```

**4. Start Server:**
```bash
cd build
./server/ocr_server 0.0.0.0:50051 4
```

Note the IP address (e.g., 192.168.1.100) - you'll need it for the client.

### Machine B: Client Setup

**1. Install and Build:**
```bash
# Clone/copy the project to this machine
cd stdiscm-ps4
./setup.sh  # Or install dependencies manually
mkdir build && cd build
cmake ..
cmake --build . -j4
```

**2. Start Client:**
```bash
cd build
./client/ocr_client
```

**3. Connect to Server:**
1. In the GUI, change "Server Address" from `localhost:50051` to `192.168.1.100:50051` (use your server's actual IP)
2. Click "Upload Images"
3. Select images and watch them get processed on the remote server!

**Verification:**
- You should see "Received image" messages on the server terminal
- Results appear on the client as each completes

---

## VM Setup

Run the server in a virtual machine, client on host.

### Option A: Using VirtualBox

**1. Create Ubuntu VM:**
- Download Ubuntu 20.04+ ISO
- Create new VM with:
  - 2+ CPU cores
  - 2GB+ RAM
  - Bridged network adapter

**2. In VM (Ubuntu Server):**
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install dependencies
sudo apt install -y cmake build-essential qt5-default \
    libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
    libtesseract-dev libleptonica-dev tesseract-ocr-eng

# Copy project files to VM (use shared folder or scp)
# Then build
cd stdiscm-ps4
mkdir build && cd build
cmake ..
cmake --build . -j2

# Find VM's IP
ip addr show

# Open firewall
sudo ufw allow 50051

# Run server
./server/ocr_server 0.0.0.0:50051 4
```

**3. On Host Machine:**
- Build and run client (see Client Setup above)
- Use VM's IP address in client GUI

### Option B: Using VMware

Same steps as VirtualBox, but:
- Set network to "Bridged" mode
- Ensure VM gets its own IP on your network
- Rest of the steps are identical

### Option C: Using Docker (Advanced)

**Server Dockerfile:**
```dockerfile
FROM ubuntu:20.04
RUN apt update && apt install -y cmake build-essential \
    libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
    libtesseract-dev libleptonica-dev tesseract-ocr-eng
COPY . /app
WORKDIR /app
RUN mkdir build && cd build && cmake .. && cmake --build .
CMD ["./build/server/ocr_server", "0.0.0.0:50051", "4"]
EXPOSE 50051
```

Build and run:
```bash
docker build -t ocr-server .
docker run -p 50051:50051 ocr-server
```

---

## Troubleshooting

### Build Issues

**Error: "Could not find Qt5"**

macOS:
```bash
cmake -DCMAKE_PREFIX_PATH=$(brew --prefix qt5) ..
```

Linux:
```bash
sudo apt install qt5-default qtbase5-dev
```

**Error: "grpc not found"**

Ubuntu:
```bash
sudo apt install libgrpc++-dev libgrpc-dev
```

macOS:
```bash
brew install grpc
```

**Error: "tesseract not found"**

Ubuntu:
```bash
sudo apt install libtesseract-dev tesseract-ocr-eng
```

macOS:
```bash
brew install tesseract
```

### Runtime Issues

**Client: "Failed to connect to server"**

1. Check server is running:
   ```bash
   ps aux | grep ocr_server
   ```

2. Test connectivity:
   ```bash
   telnet <server-ip> 50051
   # Or
   nc -zv <server-ip> 50051
   ```

3. Check firewall:
   ```bash
   sudo ufw status  # Ubuntu
   sudo firewall-cmd --list-all  # Fedora
   ```

4. Verify IP address is correct

**Server: "Could not initialize tesseract"**

Install English language data:
```bash
sudo apt install tesseract-ocr-eng  # Ubuntu
brew install tesseract-lang  # macOS
```

Or manually:
```bash
# Download from https://github.com/tesseract-ocr/tessdata
sudo mkdir -p /usr/share/tesseract-ocr/4.00/tessdata
sudo wget -P /usr/share/tesseract-ocr/4.00/tessdata \
    https://github.com/tesseract-ocr/tessdata/raw/main/eng.traineddata
```

**Client: Window doesn't open**

Check X11/display:
```bash
echo $DISPLAY  # Should show :0 or similar
export DISPLAY=:0  # If empty
```

For SSH, use X forwarding:
```bash
ssh -X user@host
```

**Server crashes with "Segmentation fault"**

Check Tesseract installation:
```bash
tesseract --version
```

Re-install if needed:
```bash
sudo apt install --reinstall libtesseract-dev tesseract-ocr
```

### Network Issues

**Can't connect between machines**

1. Ping test:
   ```bash
   ping <server-ip>
   ```

2. Port test:
   ```bash
   telnet <server-ip> 50051
   # Or
   nmap -p 50051 <server-ip>
   ```

3. Temporarily disable firewall to test:
   ```bash
   sudo ufw disable  # Ubuntu (re-enable after: sudo ufw enable)
   ```

4. Check if machines are on same subnet:
   ```bash
   ip route  # Linux
   netstat -rn  # macOS
   ```

**VM can't reach internet**

Change network mode:
- VirtualBox: Settings > Network > Bridged Adapter
- VMware: VM Settings > Network Adapter > Bridged

**Connection timeout**

Increase timeout in `client/ocr_client.cpp` line ~XX:
```cpp
context.set_deadline(std::chrono::system_clock::now() +
                     std::chrono::seconds(120));  // Increase from 60
```

---

## Testing Checklist

Once setup is complete, verify these scenarios:

- [ ] Upload single image → see result immediately
- [ ] Upload 10 images → see them complete one by one
- [ ] Upload 5 images, then 3 more mid-batch → progress bar updates
- [ ] Let batch complete (100%) → next upload clears results
- [ ] Stop server while processing → client shows error
- [ ] Restart server → client can reconnect
- [ ] Test different image formats (PNG, JPG, BMP)
- [ ] Test large image (>5MB)
- [ ] Check server terminal shows "Received image" messages
- [ ] Verify multiple threads are working (server should handle multiple images in parallel)

---

## Performance Tips

1. **More threads for more CPU cores:**
   ```bash
   ./server/ocr_server 0.0.0.0:50051 8
   ```

2. **Use wired ethernet** instead of WiFi for better network performance

3. **Preprocess images** for better OCR:
   - Higher contrast
   - No skew/rotation
   - Clear text

4. **Monitor server resources:**
   ```bash
   htop  # Install: sudo apt install htop
   ```

---

## Quick Reference

**Build commands:**
```bash
mkdir build && cd build
cmake ..  # macOS: cmake -DCMAKE_PREFIX_PATH=$(brew --prefix qt5) ..
cmake --build . -j4
```

**Run server:**
```bash
./server/ocr_server [address:port] [num_threads]
./server/ocr_server 0.0.0.0:50051 4  # Example
```

**Run client:**
```bash
./client/ocr_client
```

**Check server IP:**
```bash
ip addr show  # Linux
ifconfig  # macOS
```

**Test connectivity:**
```bash
ping <server-ip>
telnet <server-ip> 50051
```

**Open firewall port:**
```bash
sudo ufw allow 50051  # Ubuntu
sudo firewall-cmd --permanent --add-port=50051/tcp && sudo firewall-cmd --reload  # Fedora
```

---

## Support

For issues:
1. Check error messages in terminal
2. Review logs
3. Verify all dependencies are installed
4. Check network connectivity
5. Consult README.md for detailed architecture

Good luck with your STDISCM project!
