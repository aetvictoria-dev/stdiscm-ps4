# Demonstration Guide for STDISCM Problem Set 4

This guide helps you prepare a comprehensive demonstration of your Distributed OCR System.

## Pre-Demonstration Checklist

### Before Recording Your Demo Video:

- [ ] Both server and client are built successfully
- [ ] Server is running on a different machine/VM than client
- [ ] You have 10-15 test images with various text content
- [ ] Test images include different formats (PNG, JPG, BMP)
- [ ] Network connection between client and server is stable
- [ ] You've rehearsed the demo flow

## Demo Scenarios to Show

Your 5-10 minute video should demonstrate these scenarios:

### 1. System Startup (30 seconds)

**Show:**
- Start the server on Machine/VM A
  ```bash
  ./server/ocr_server 0.0.0.0:50051 4
  ```
- Server output showing initialization with 4 worker threads
- Start the client on Machine B
  ```bash
  ./client/ocr_client
  ```
- Client GUI window opening

**Explain:**
- Server is running on a separate machine/VM
- Server has 4 worker threads ready for parallel processing
- Client connects via gRPC

### 2. Single Image Upload (30 seconds)

**Show:**
- Upload one image containing clear text
- Result appears almost immediately
- Progress bar shows 1/1

**Explain:**
- Image is sent to server via gRPC
- Server processes using Tesseract OCR
- Result streams back immediately
- Single-threaded execution for one image

### 3. Multiple Image Upload (1-2 minutes)

**Show:**
- Upload 8-10 images at once
- Progress bar shows 0/10, then updates as each completes
- Results appear one by one (not all at once)
- Show server terminal with "Received image" and "Processing" messages

**Explain:**
- **Multithreading:** Multiple images processed in parallel by 4 worker threads
- **Synchronization:** Thread-safe task queue with mutex protection
- **Interprocess Communication:** gRPC streaming delivers results as they complete
- Results display incrementally, showing parallel processing

### 4. Adding Images Mid-Batch (1 minute)

**Show:**
- Upload 5 images
- While they're processing (progress at 2/5 or 3/5)
- Click upload again and add 3 more images
- Progress bar updates to X/8
- All images complete eventually

**Explain:**
- **GUI Implementation:** Progress bar dynamically updates
- **Synchronization:** New tasks safely added to queue while workers are busy
- Batch continues until all images complete

### 5. Batch Completion and Reset (30 seconds)

**Show:**
- Wait for progress bar to reach 100% (all images processed)
- Upload a new image
- Previous results clear
- Progress bar resets to 0/1

**Explain:**
- **GUI Implementation:** Automatic batch management
- Once batch is complete, next upload starts fresh
- UI clears and resets for new batch

### 6. Fault Tolerance - Connection Error (1 minute)

**Show:**
- Upload several images
- Stop the server (Ctrl+C) while client is processing
- Client shows error messages for pending images
- Restart server
- Upload new image successfully

**Explain:**
- **Fault Tolerance:** Client detects connection failure
- Error handling displays clear messages
- System recovers gracefully when server restarts
- No crashes or hangs

### 7. Different Image Formats (30 seconds)

**Show:**
- Upload mix of PNG, JPG, and BMP images
- All process successfully

**Explain:**
- Leptonica library handles multiple image formats
- System is format-agnostic

### 8. Large/Complex Images (30 seconds)

**Show:**
- Upload large, high-resolution image or image with lots of text
- Takes longer to process
- Eventually completes with full text extraction

**Explain:**
- System handles varying image sizes
- OCR processing time depends on image complexity

## Presentation Slide Topics

Your PDF presentation should cover:

### Slide 1: Title
- Project name
- Your name(s)
- Course: STDISCM

### Slide 2: System Architecture
- Diagram showing Client ↔ gRPC ↔ Server
- Client: Qt GUI, Image Upload, Result Display
- Server: Task Queue, Thread Pool, Tesseract OCR

### Slide 3: Multithreading
- Server uses thread pool (default: 4 threads)
- Each thread has own Tesseract instance
- Producer-consumer pattern
- Show code snippet of worker thread initialization:
  ```cpp
  for (int i = 0; i < num_threads_; ++i) {
      worker_threads_.emplace_back(&OCRServiceImpl::WorkerThread, this);
  }
  ```

### Slide 4: Synchronization
- Task queue protected by mutex
- Condition variable for thread coordination
- Thread-safe result streaming
- Code snippet:
  ```cpp
  std::lock_guard<std::mutex> lock(queue_mutex_);
  task_queue_.push(task);
  queue_cv_.notify_one();
  ```

### Slide 5: GUI Implementation
- Qt Framework with signals/slots
- Progress bar tracking
- Grid layout for results
- Separate thread for network I/O
- Code snippet showing signal/slot:
  ```cpp
  connect(worker_, &OCRWorker::resultReady,
          this, &MainWindow::onResultReady);
  ```

### Slide 6: Interprocess Communication
- gRPC with Protocol Buffers
- Server streaming RPC
- Binary image transfer
- Message structure:
  ```protobuf
  message ImageRequest {
    bytes image_data = 1;
    string image_id = 2;
  }
  ```

### Slide 7: Fault Tolerance
- Connection timeout (60 seconds)
- Error detection and display
- Graceful server shutdown
- Client reconnection capability
- Code snippet:
  ```cpp
  context.set_deadline(
      std::chrono::system_clock::now() +
      std::chrono::seconds(60));
  ```

### Slide 8: Technologies Used
- C++17
- Qt 5 (GUI)
- gRPC (Communication)
- Tesseract (OCR)
- Leptonica (Image Processing)
- CMake (Build System)

### Slide 9: Testing & Results
- Successfully processes multiple image formats
- Parallel processing demonstrated
- Real-time result streaming
- Fault tolerance verified

### Slide 10: Challenges & Solutions
- Challenge: Thread-safe Tesseract usage
  - Solution: Each thread has own instance
- Challenge: UI responsiveness during network I/O
  - Solution: Separate worker thread
- Challenge: Progress tracking with dynamic uploads
  - Solution: Atomic counters and mutex protection

## Video Recording Tips

### Setup:
1. Use screen recording software (OBS, QuickTime, etc.)
2. Record both server and client screens if possible
3. Use picture-in-picture or split screen
4. Ensure good audio quality

### Script Flow:
1. **Introduction (30s):** "Hello, I'm demonstrating a distributed OCR system..."
2. **Architecture (30s):** Explain client-server setup
3. **Demo Scenarios (6-7 min):** Follow scenarios above
4. **Code Walkthrough (1-2 min):** Show key code sections
5. **Conclusion (30s):** Summarize features

### Recording Tips:
- Speak clearly and not too fast
- Point out what you're clicking
- Explain what's happening on screen
- Highlight server terminal messages
- Show code files briefly to prove implementation

## Code to Highlight in Video

### Server - Thread Pool Initialization:
File: `server/ocr_server.cpp`
```cpp
OCRServiceImpl::OCRServiceImpl(int num_threads)
    : shutdown_(false), num_threads_(num_threads) {
    for (int i = 0; i < num_threads_; ++i) {
        worker_threads_.emplace_back(&OCRServiceImpl::WorkerThread, this);
    }
}
```

### Server - Worker Thread Function:
```cpp
void OCRServiceImpl::WorkerThread() {
    tesseract::TessBaseAPI ocr_engine;
    ocr_engine.Init(NULL, "eng");

    while (!shutdown_) {
        // Wait for task
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this]() {
            return !task_queue_.empty() || shutdown_;
        });
        // Process OCR...
    }
}
```

### Client - Result Handling:
File: `client/ocr_client.cpp`
```cpp
void MainWindow::onResultReady(int index, const QString& text) {
    QMutexLocker locker(&mutex_);
    imageResults_[index]->setResult(text);
}
```

### gRPC Protocol:
File: `proto/ocr_service.proto`
```protobuf
service OCRService {
  rpc ProcessImage(ImageRequest) returns (stream OCRResponse);
}
```

## Grading Criteria (Reference)

Based on the problem set, you'll be graded on:

1. **Multithreading** - Server uses multiple threads for OCR
2. **Synchronization** - Thread-safe queue and result delivery
3. **GUI Implementation** - Qt interface with progress tracking
4. **Interprocess Communication** - gRPC client-server communication
5. **Fault Tolerance** - Error handling and recovery

Make sure your video demonstrates ALL of these!

## Sample Test Images

Create test images with:
- Book pages (complex text)
- Screenshots of code
- Receipts or invoices
- Handwritten text (challenges OCR)
- Different languages (if Tesseract data installed)
- Mixed text and images
- Low quality scans

## Post-Demo Checklist

- [ ] Video is 5-10 minutes long
- [ ] All required scenarios demonstrated
- [ ] Presentation slides exported to PDF
- [ ] Source code is clean and commented
- [ ] README.md is complete
- [ ] All files uploaded to Google Drive
- [ ] Drive link shared with proper permissions

## Google Drive Submission Structure

```
STDISCM_PS4_YourNames/
├── presentation.pdf
├── demo_video.mp4
├── source_code/
│   ├── client/
│   ├── server/
│   ├── proto/
│   ├── CMakeLists.txt
│   └── README.md
└── README.txt (contains build/run instructions)
```

## Final Tips

1. **Practice your demo** multiple times before recording
2. **Check audio levels** before final recording
3. **Show server terminal** to prove remote execution
4. **Explain while showing** - don't just demonstrate silently
5. **Highlight key code** - prove you wrote it
6. **Show network setup** - demonstrate it's truly distributed
7. **Test error scenarios** - show fault tolerance
8. **Be enthusiastic** - show you understand what you built!

Good luck with your demonstration!
