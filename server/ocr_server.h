#ifndef OCR_SERVER_H
#define OCR_SERVER_H

#include <grpcpp/grpcpp.h>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include "ocr_service.grpc.pb.h"

class OCRServiceImpl final : public ocrservice::OCRService::Service {
public:
    OCRServiceImpl(int num_threads = 4);
    ~OCRServiceImpl();

    grpc::Status ProcessImage(grpc::ServerContext* context,
                             const ocrservice::ImageRequest* request,
                             grpc::ServerWriter<ocrservice::OCRResponse>* writer) override;

private:
    struct OCRTask {
        std::string image_id;
        std::vector<uint8_t> image_data;
        grpc::ServerWriter<ocrservice::OCRResponse>* writer;
        std::shared_ptr<std::mutex> writer_mutex;
        std::shared_ptr<std::condition_variable> done_cv;
        std::shared_ptr<bool> done;
    };

    void WorkerThread();
    std::string PerformOCR(const std::vector<uint8_t>& image_data);

    std::vector<std::thread> worker_threads_;
    std::queue<OCRTask> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> shutdown_;
    int num_threads_;
};

#endif // OCR_SERVER_H
