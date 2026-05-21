cmake_minimum_required(VERSION 3.27)

find_package(Protobuf REQUIRED CONFIG)
find_package(gRPC REQUIRED CONFIG)

set(PROTO_FILES
    bcmd/v1/common.proto
    bcmd/v1/broadcast.proto
)

set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_DIR ${PROTO_FILE} DIRECTORY)

    set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_DIR}/${PROTO_NAME}.pb.cc")
    set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_DIR}/${PROTO_NAME}.pb.h")
    set(GRPC_SRC  "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_DIR}/${PROTO_NAME}.grpc.pb.cc")
    set(GRPC_HDR  "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_DIR}/${PROTO_NAME}.grpc.pb.h")

    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${PROTO_HDR})
    list(APPEND GRPC_SRCS  ${GRPC_SRC})
    list(APPEND GRPC_HDRS  ${GRPC_HDR})

    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_DIR}"
        COMMAND $<TARGET_FILE:protobuf::protoc>
            --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
            --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
            --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
            --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
            ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
            protobuf::protoc
            gRPC::grpc_cpp_plugin
        COMMENT "Generating protobuf/gRPC for ${PROTO_FILE}"
        VERBATIM
    )
endforeach()

add_library(bcmd_proto STATIC
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_include_directories(bcmd_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(bcmd_proto PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)

target_compile_features(bcmd_proto PUBLIC cxx_std_23)

set_target_properties(bcmd_proto PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(NOT MSVC)
    target_compile_options(bcmd_proto PRIVATE -Wno-error)
endif()
