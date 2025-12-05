#include "ocr_server.h"
#include <iostream>
#include <fstream>

// ============================================================================
// MULTITHREADING: Thread Pool Initialization
// ============================================================================
// Constructor creates a pool of worker threads for concurrent image processing.
// Each thread has its own Tesseract instance to avoid conflicts.
OCRServiceImpl::OCRServiceImpl(int num_threads)
    : shutdown_(false), num_threads_(num_threads) {

    // MULTITHREADING: Spawn worker threads (default 4)
    for (int i = 0; i < num_threads_; ++i) {
        worker_threads_.emplace_back(&OCRServiceImpl::WorkerThread, this);
    }

    std::cout << "OCR Server initialized with " << num_threads_ << " worker threads" << std::endl;
}

OCRServiceImpl::~OCRServiceImpl() {
    shutdown_ = true;
    queue_cv_.notify_all();

    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

// ============================================================================
// INTERPROCESS COMMUNICATION: gRPC Request Handler
// ============================================================================
// This function is called when a client sends an image for OCR processing.
// It implements server-side streaming to send results back to the client.
grpc::Status OCRServiceImpl::ProcessImage(grpc::ServerContext* context,
                                         const ocrservice::ImageRequest* request,
                                         grpc::ServerWriter<ocrservice::OCRResponse>* writer) {

    std::cout << "Received image: " << request->image_id() << std::endl;

    // Extract binary image data from Protocol Buffer message
    std::vector<uint8_t> image_data(request->image_data().begin(),
                                    request->image_data().end());

    // SYNCHRONIZATION: Create shared synchronization primitives with proper lifetime
    // Using shared_ptr ensures these objects live until worker thread is done
    auto writer_mutex = std::make_shared<std::mutex>();
    auto done_cv = std::make_shared<std::condition_variable>();
    auto done = std::make_shared<bool>(false);

    // MULTITHREADING: Add task to queue for worker threads (Producer-Consumer pattern)
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push({request->image_id(), image_data, writer, writer_mutex, done_cv, done});
    }
    queue_cv_.notify_one();  // Wake up one worker thread

    // SYNCHRONIZATION: Wait for worker thread to complete processing
    std::unique_lock<std::mutex> lock(*writer_mutex);
    done_cv->wait(lock, [done]() {
        return *done;  // Wait until worker sets done flag
    });

    return grpc::Status::OK;
}

// ============================================================================
// MULTITHREADING: Worker Thread Function (Producer-Consumer)
// ============================================================================
// Each worker thread continuously processes images from the shared queue.
// This implements the Consumer side of the Producer-Consumer pattern.
void OCRServiceImpl::WorkerThread() {
    tesseract::TessBaseAPI ocr_engine;

    // Initialize Tesseract OCR engine (each thread has its own instance)
    if (ocr_engine.Init(NULL, "eng")) {
        std::cerr << "Could not initialize tesseract in thread "
                  << std::this_thread::get_id() << std::endl;
        return;
    }

    std::cout << "Worker thread " << std::this_thread::get_id()
              << " initialized" << std::endl;

    // Main worker loop: continuously process tasks from queue
    while (!shutdown_) {
        OCRTask task;

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

        std::cout << "Processing image: " << task.image_id << std::endl;

        // Prepare response message
        ocrservice::OCRResponse response;
        response.set_image_id(task.image_id);

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
        } catch (const std::exception& e) {
            response.set_success(false);
            response.set_error_message(std::string("Exception: ") + e.what());
        }

        // SYNCHRONIZATION & INTERPROCESS COMMUNICATION: Send response back to client
        {
            std::lock_guard<std::mutex> lock(*task.writer_mutex);
            task.writer->Write(response);  // gRPC streaming write
            *task.done = true;  // Mark task as complete
        }

        std::cout << "Completed image: " << task.image_id << std::endl;
        task.done_cv->notify_one();  // Wake up waiting gRPC handler
    }

    ocr_engine.End();  // Cleanup Tesseract
}

std::string OCRServiceImpl::PerformOCR(const std::vector<uint8_t>& image_data) {
    return "";
}
