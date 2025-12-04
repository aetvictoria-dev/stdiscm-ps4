add_executable(ocr_server
    main.cpp
    ocr_server.cpp
    ocr_server.h
)

target_link_libraries(ocr_server
    PRIVATE
        ocr_proto
        gRPC::grpc++
        protobuf::libprotobuf
        Tesseract::libtesseract
        ${Leptonica_LIBRARIES}
)

target_include_directories(ocr_server
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${Leptonica_INCLUDE_DIRS}
)
