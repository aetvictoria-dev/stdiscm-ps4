#include "ocr_server.h"
#include <iostream>
#include <fstream>

OCRServiceImpl::OCRServiceImpl(int num_threads)
    : shutdown_(false), num_threads_(num_threads) {

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

grpc::Status OCRServiceImpl::ProcessImage(grpc::ServerContext* context,
                                         const ocrservice::ImageRequest* request,
                                         grpc::ServerWriter<ocrservice::OCRResponse>* writer) {

    std::cout << "Received image: " << request->image_id() << std::endl;

    std::vector<uint8_t> image_data(request->image_data().begin(),
                                    request->image_data().end());

    std::mutex writer_mutex;

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push({request->image_id(), image_data, writer, &writer_mutex});
    }
    queue_cv_.notify_one();

    std::unique_lock<std::mutex> lock(writer_mutex);
    queue_cv_.wait(lock, [this, &request]() {
        std::lock_guard<std::mutex> queue_lock(queue_mutex_);
        bool found = false;
        std::queue<OCRTask> temp_queue = task_queue_;
        while (!temp_queue.empty()) {
            if (temp_queue.front().image_id == request->image_id()) {
                found = true;
                break;
            }
            temp_queue.pop();
        }
        return !found || shutdown_;
    });

    return grpc::Status::OK;
}

void OCRServiceImpl::WorkerThread() {
    tesseract::TessBaseAPI ocr_engine;

    if (ocr_engine.Init(NULL, "eng")) {
        std::cerr << "Could not initialize tesseract in thread "
                  << std::this_thread::get_id() << std::endl;
        return;
    }

    std::cout << "Worker thread " << std::this_thread::get_id()
              << " initialized" << std::endl;

    while (!shutdown_) {
        OCRTask task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this]() {
                return !task_queue_.empty() || shutdown_;
            });

            if (shutdown_ && task_queue_.empty()) {
                break;
            }

            if (!task_queue_.empty()) {
                task = task_queue_.front();
                task_queue_.pop();
            } else {
                continue;
            }
        }

        std::cout << "Processing image: " << task.image_id << std::endl;

        ocrservice::OCRResponse response;
        response.set_image_id(task.image_id);

        try {
            PIX* image = pixReadMem(task.image_data.data(), task.image_data.size());

            if (image == nullptr) {
                response.set_success(false);
                response.set_error_message("Failed to decode image");
            } else {
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

                pixDestroy(&image);
            }
        } catch (const std::exception& e) {
            response.set_success(false);
            response.set_error_message(std::string("Exception: ") + e.what());
        }

        {
            std::lock_guard<std::mutex> lock(*task.writer_mutex);
            task.writer->Write(response);
        }

        std::cout << "Completed image: " << task.image_id << std::endl;
        queue_cv_.notify_all();
    }

    ocr_engine.End();
}

std::string OCRServiceImpl::PerformOCR(const std::vector<uint8_t>& image_data) {
    return "";
}
