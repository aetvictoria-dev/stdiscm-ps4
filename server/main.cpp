#include "ocr_server.h"
#include <iostream>
#include <string>

void RunServer(const std::string& server_address, int num_threads) {
    OCRServiceImpl service(num_threads);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    std::cout << "Using " << num_threads << " worker threads" << std::endl;

    server->Wait();
}

int main(int argc, char** argv) {
    std::string server_address = "0.0.0.0:50051";
    int num_threads = 4;

    if (argc > 1) {
        server_address = argv[1];
    }

    if (argc > 2) {
        num_threads = std::stoi(argv[2]);
    }

    std::cout << "Starting OCR Server..." << std::endl;
    RunServer(server_address, num_threads);

    return 0;
}
