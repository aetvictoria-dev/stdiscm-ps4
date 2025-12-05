cmake_minimum_required(VERSION 3.15)
project(DistributedOCR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find packages
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

# Find Tesseract and Leptonica using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(TESSERACT REQUIRED tesseract)
pkg_check_modules(LEPTONICA REQUIRED lept)

# Set proto files
set(PROTO_FILES proto/ocr_service.proto)

# Generate protobuf files
set(PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/ocr_service.pb.cc")
set(PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/ocr_service.pb.h")
set(GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/ocr_service.grpc.pb.cc")
set(GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/ocr_service.grpc.pb.h")

add_custom_command(
    OUTPUT "${PROTO_SRCS}" "${PROTO_HDRS}" "${GRPC_SRCS}" "${GRPC_HDRS}"
    COMMAND protobuf::protoc
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         -I "${CMAKE_CURRENT_SOURCE_DIR}/proto"
         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/ocr_service.proto"
    DEPENDS "${PROTO_FILES}")

add_library(ocr_proto ${PROTO_SRCS} ${GRPC_SRCS})
target_link_libraries(ocr_proto
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
)
target_include_directories(ocr_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(server)
add_subdirectory(client)
