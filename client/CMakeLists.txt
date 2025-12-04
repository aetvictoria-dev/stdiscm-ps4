add_executable(ocr_client
    main.cpp
    ocr_client.cpp
    ocr_client.h
)

target_link_libraries(ocr_client
    PRIVATE
        ocr_proto
        gRPC::grpc++
        protobuf::libprotobuf
        Qt5::Core
        Qt5::Widgets
)

target_include_directories(ocr_client
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)
