# PowerPoint Presentation Guide
## Distributed OCR System - Code Snippets to Include

This guide tells you exactly what code to show in your presentation and what each part demonstrates.

---

## **Slide 1: Title Slide**
### Content:
- Title: "DISTRIBUTED OCR SYSTEM"
- Subtitle: "Problem Set 4"
- Your name(s)
- No code needed

---

## **Slide 2: Interprocess Communication - gRPC Protocol Definition**
### Topic: How client and server communicate

### Code to Show: `proto/ocr_service.proto`
```protobuf
service OCRService {
  // Server-side streaming RPC: Client sends one request, server sends multiple responses
  // This allows real-time result delivery as each image is processed
  rpc ProcessImage(ImageRequest) returns (stream OCRResponse);
}

// Request message: Client sends image data to server
message ImageRequest {
  bytes image_data = 1;    // Binary image data (efficient transmission)
  string image_id = 2;     // Filename for logging and tracking
}

// Response message: Server sends OCR results back to client
message OCRResponse {
  string image_id = 1;        // Which image this result is for
  string extracted_text = 2;  // The OCR text extracted from the image
  bool success = 3;           // Whether OCR succeeded
  string error_message = 4;   // Error details if OCR failed
}
```

### What to Say:
- "We use gRPC with Protocol Buffers for efficient communication"
- "The ProcessImage RPC uses **server-side streaming** - client sends one image, server can send multiple responses"
- "Images are sent as binary data (bytes) for fast transmission"
- "Protocol Buffers automatically generate C++ code for both client and server"

---

## **Slide 3: Client Side - GUI Implementation**
### Topic: User interface and batch management

### Code to Show: `client/ocr_client.cpp` (lines 245-253)
```cpp
// GUI IMPLEMENTATION: Create result widgets in 4-column grid layout
int startIndex = imageResults_.size();
for (int i = 0; i < filePaths.size(); ++i) {
    ImageResult* result = new ImageResult(filePaths[i], resultsContainer_);
    int row = (startIndex + i) / 4;  // Calculate grid row
    int col = (startIndex + i) % 4;  // Calculate grid column
    resultsLayout_->addWidget(result, row, col);
    imageResults_.push_back(result);
}
```

### What to Say:
- "We use Qt Framework to create a responsive interface"
- "Results are displayed in a 4-column grid layout"
- "Each ImageResult widget shows the image thumbnail and extracted text side-by-side"
- "Progress bar tracks completed_images / total_images"

---

## **Slide 4: Client Side - Multithreading Setup**
### Topic: Creating worker threads to avoid UI freezing

### Code to Show: `client/ocr_client.cpp` (lines 262-271)
```cpp
// MULTITHREADING: Create and start new worker thread
worker_ = new OCRWorker(serverAddressEdit_->text(), filePaths, this);
worker_->setProperty("startIndex", startIndex);

// SYNCHRONIZATION: Connect signals from worker thread to UI slots
connect(worker_, &OCRWorker::resultReady, this, &MainWindow::onResultReady);
connect(worker_, &OCRWorker::errorOccurred, this, &MainWindow::onErrorOccurred);
connect(worker_, &OCRWorker::progressUpdated, this, &MainWindow::onProgressUpdated);

worker_->start();  // Start background thread
```

### What to Say:
- "OCRWorker inherits from QThread to run in a separate thread"
- "This prevents the UI from freezing during network operations"
- "Signals are connected to slots for thread-safe communication"
- "When user clicks 'Upload Images', a new worker thread starts"

---

## **Slide 5: Client Side - Sending Images via gRPC**
### Topic: How client sends images to server

### Code to Show: `client/ocr_client.cpp` (lines 94-106)
```cpp
// INTERPROCESS COMMUNICATION: Prepare gRPC request with Protocol Buffers
ocrservice::ImageRequest request;
request.set_image_data(imageData.data(), imageData.size());  // Binary image data
QFileInfo fileInfo(imagePath);
request.set_image_id(fileInfo.fileName().toStdString());     // Filename for server logging

// FAULT TOLERANCE: Set 60-second timeout to prevent hanging
grpc::ClientContext context;
context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(60));

// INTERPROCESS COMMUNICATION: Send request and receive streaming responses
std::unique_ptr<grpc::ClientReader<ocrservice::OCRResponse>> reader(
    stub_->ProcessImage(&context, request));
```

### What to Say:
- "Image is read as binary data and packed into a Protocol Buffer message"
- "We set a 60-second timeout to prevent hanging if server is unresponsive"
- "ClientReader allows us to receive streaming responses from the server"
- "Each image is sent with its filename for better server logging"

---

## **Slide 6: Client Side - Synchronization (Signals/Slots)**
### Topic: Thread-safe UI updates

### Code to Show: `client/ocr_client.cpp` (lines 280-288)
```cpp
// SYNCHRONIZATION: Thread-Safe UI Update Slots
void MainWindow::onResultReady(int index, const QString& text) {
    // SYNCHRONIZATION: Lock mutex before accessing shared data
    QMutexLocker locker(&mutex_);
    int actualIndex = worker_->property("startIndex").toInt() + index;
    if (actualIndex >= 0 && actualIndex < imageResults_.size()) {
        // Update the result widget with OCR text (runs in main/UI thread)
        imageResults_[actualIndex]->setResult(text);
    }
}
```

### What to Say:
- "Background threads **cannot** modify UI widgets directly in Qt"
- "We use Qt's **Signals and Slots** mechanism for thread-safe communication"
- "Worker thread **emits** a signal with data"
- "Main thread **catches** it in a slot and updates the GUI safely"
- "QMutex protects shared state from race conditions"

---

## **Slide 7: Server Side - Thread Pool Initialization**
### Topic: Creating worker threads on server startup

### Code to Show: `server/ocr_server.cpp` (lines 10-19)
```cpp
// MULTITHREADING: Thread Pool Initialization
OCRServiceImpl::OCRServiceImpl(int num_threads)
    : shutdown_(false), num_threads_(num_threads) {

    // MULTITHREADING: Spawn worker threads (default 4)
    for (int i = 0; i < num_threads_; ++i) {
        worker_threads_.emplace_back(&OCRServiceImpl::WorkerThread, this);
    }

    std::cout << "OCR Server initialized with " << num_threads_ << " worker threads" << std::endl;
}
```

### What to Say:
- "Server uses a **thread pool** with configurable size (default 4 threads)"
- "Each worker thread runs independently and processes images concurrently"
- "This allows multiple images to be processed at the same time"
- "Each thread has its own Tesseract OCR instance to avoid conflicts"

---

## **Slide 8: Server Side - Producer-Consumer Pattern**
### Topic: How incoming requests are queued

### Code to Show: `server/ocr_server.cpp` (lines 53-58)
```cpp
// MULTITHREADING: Add task to queue for worker threads (Producer-Consumer pattern)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    task_queue_.push({request->image_id(), image_data, writer, writer_mutex, done_cv, done});
}
queue_cv_.notify_one();  // Wake up one worker thread
```

### What to Say:
- "We implement the **Producer-Consumer pattern**"
- "gRPC handler = **Producer** (adds tasks to queue)"
- "Worker threads = **Consumers** (take tasks from queue)"
- "std::mutex protects the shared queue from race conditions"
- "condition_variable wakes up sleeping worker threads when new tasks arrive"

---

## **Slide 9: Server Side - Worker Thread OCR Processing**
### Topic: How images are processed by worker threads

### Code to Show: `server/ocr_server.cpp` (lines 91-109)
```cpp
// SYNCHRONIZATION: Get task from queue (thread-safe)
{
    std::unique_lock<std::mutex> lock(queue_mutex_);
    // Wait until queue has tasks or shutdown signal
    queue_cv_.wait(lock, [this]() {
        return !task_queue_.empty() || shutdown_;
    });

    if (shutdown_ && task_queue_.empty()) {
        break;  // Exit if shutting down and no more tasks
    }

    if (!task_queue_.empty()) {
        task = task_queue_.front();
        task_queue_.pop();
    } else {
        continue;
    }
}
```

### What to Say:
- "Worker threads continuously wait for tasks from the queue"
- "condition_variable.wait() **blocks** the thread until a task arrives"
- "When notified, worker wakes up, locks mutex, and pops task from queue"
- "This is efficient - threads sleep when idle (no busy-waiting)"

---

## **Slide 10: Server Side - Tesseract OCR Execution**
### Topic: Actual OCR processing

### Code to Show: `server/ocr_server.cpp` (lines 118-138)
```cpp
try {
    // Decode binary image data using Leptonica
    PIX* image = pixReadMem(task.image_data.data(), task.image_data.size());

    if (image == nullptr) {
        response.set_success(false);
        response.set_error_message("Failed to decode image");
    } else {
        // Perform OCR using Tesseract
        ocr_engine.SetImage(image);
        char* text = ocr_engine.GetUTF8Text();

        if (text != nullptr) {
            response.set_extracted_text(text);
            response.set_success(true);
            delete[] text;
        } else {
            response.set_success(false);
            response.set_error_message("Failed to extract text");
        }

        pixDestroy(&image);  // Clean up image memory
    }
}
```

### What to Say:
- "Leptonica library decodes binary image data (supports PNG, JPG, etc.)"
- "Tesseract OCR engine extracts text from the image"
- "Each worker thread has its own Tesseract instance (thread-safe)"
- "Results are packed into a Protocol Buffer response message"

---

## **Slide 11: Server Side - Sending Results Back**
### Topic: gRPC streaming response

### Code to Show: `server/ocr_server.cpp` (lines 145-153)
```cpp
// SYNCHRONIZATION & INTERPROCESS COMMUNICATION: Send response back to client
{
    std::lock_guard<std::mutex> lock(*task.writer_mutex);
    task.writer->Write(response);  // gRPC streaming write
    *task.done = true;  // Mark task as complete
}

std::cout << "Completed image: " << task.image_id << std::endl;
task.done_cv->notify_one();  // Wake up waiting gRPC handler
```

### What to Say:
- "Worker locks the writer mutex before sending response (thread-safe)"
- "gRPC writer->Write() sends the result back to the client immediately"
- "Client receives results in **real-time** as each image completes"
- "Done flag signals the gRPC handler that processing is complete"

---

## **Slide 12: Synchronization - Shared Pointer Lifetime Management**
### Topic: Preventing race conditions with proper object lifetime

### Code to Show: `server/ocr_server.cpp` (lines 47-51)
```cpp
// SYNCHRONIZATION: Create shared synchronization primitives with proper lifetime
// Using shared_ptr ensures these objects live until worker thread is done
auto writer_mutex = std::make_shared<std::mutex>();
auto done_cv = std::make_shared<std::condition_variable>();
auto done = std::make_shared<bool>(false);
```

### What to Say:
- "Early bug: mutex was stack-allocated and went out of scope too early"
- "This caused crashes when worker threads tried to lock destroyed mutexes"
- "**Solution**: Use std::shared_ptr to extend lifetime"
- "Objects are kept alive until both gRPC handler AND worker are done"
- "This fixed the 'pthread_mutex_lock assertion failed' error"

---

## **Slide 13: Fault Tolerance**
### Topic: Error handling and timeouts

### Code to Show: `client/ocr_client.cpp` (lines 121-125)
```cpp
// FAULT TOLERANCE: Check for connection errors
grpc::Status status = reader->Finish();
if (!status.ok() && !received) {
    emit errorOccurred(i, QString::fromStdString("Connection error: " + status.error_message()));
}
```

### What to Say:
- "Client sets 60-second timeout per image"
- "If server crashes or is unreachable, error is caught and displayed"
- "User sees friendly error message instead of application crash"
- "Other images continue processing even if one fails (graceful degradation)"

---

## **Slide 14: System Architecture Diagram**
### Content: (Draw or create a diagram showing):
```
┌─────────────────┐         gRPC          ┌─────────────────┐
│   CLIENT        │   (Port 50051)        │   SERVER        │
│   (Mac/Linux)   │◄─────────────────────►│   (Ubuntu VM)   │
├─────────────────┤                       ├─────────────────┤
│ Qt GUI          │                       │ gRPC Handler    │
│ OCRWorker Thread│   Image (bytes)       │ Task Queue      │
│ Signal/Slot     │  ──────────────────►  │ Worker Thread 1 │
│ QMutex          │                       │ Worker Thread 2 │
│                 │   OCR Result          │ Worker Thread 3 │
│                 │  ◄──────────────────  │ Worker Thread 4 │
└─────────────────┘   (streaming)         │ Tesseract OCR   │
                                          └─────────────────┘
```

---

## **Slide 15: Key Takeaways**
### Content (Bullet points):
- ✓ **Multithreading**: Client uses QThread, Server uses thread pool
- ✓ **Synchronization**: Qt Signals/Slots (client), mutex + condition_variable (server)
- ✓ **IPC**: gRPC with Protocol Buffers for efficient binary communication
- ✓ **Fault Tolerance**: Timeouts, error handling, graceful degradation
- ✓ **Real-time Results**: Server streaming allows progressive result display
- ✓ **Thread Safety**: Proper use of mutexes, shared_ptr lifetime management

---

## **Slide 16: Demo/Screenshots**
- Screenshot of your working GUI showing:
  - Multiple images uploaded
  - Progress bar showing completion
  - Grid layout with images and extracted text
  - Maybe one showing an error message (fault tolerance)

- Screenshot of server terminal showing:
  - "Received image: filename.png"
  - "Processing image: filename.png"
  - "Completed image: filename.png"

---

## **Slide 17: Thank You**
- "THANK YOU FOR LISTENING"
- Your name(s) / group name
- "Questions?"

---

## **Tips for Presenting:**

1. **Don't read the code line-by-line**. Explain the **concept** it demonstrates.

2. **Use the comment tags** as your talking points:
   - "MULTITHREADING" → explain concurrency
   - "SYNCHRONIZATION" → explain thread safety
   - "INTERPROCESS COMMUNICATION" → explain gRPC/Protocol Buffers
   - "FAULT TOLERANCE" → explain error handling

3. **Highlight key lines** in your PowerPoint (use arrows, boxes, or colors)

4. **Show the flow**:
   - Client uploads → Worker thread sends via gRPC → Server queues → Worker processes → Results stream back → UI updates

5. **Mention the bug you fixed**: The mutex lifetime issue shows problem-solving skills!

6. **End with a live demo** if possible (much more impressive than screenshots)
