cmake_minimum_required(VERSION 3.15)
project(DistributedOCR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(Tesseract REQUIRED)
find_package(Leptonica REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

set(PROTO_FILES proto/ocr_service.proto)

add_library(ocr_proto ${PROTO_FILES})
target_link_libraries(ocr_proto
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)

protobuf_generate(TARGET ocr_proto LANGUAGE cpp)
protobuf_generate(TARGET ocr_proto LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
)

target_include_directories(ocr_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(server)
add_subdirectory(client)
