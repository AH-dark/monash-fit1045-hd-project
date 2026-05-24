cmake_minimum_required(VERSION 3.27)

find_package(gRPC REQUIRED CONFIG)
find_package(CLI11 REQUIRED CONFIG)
find_package(ftxui REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(stduuid REQUIRED CONFIG)
find_package(Catch2 REQUIRED CONFIG)

add_library(bcli_lib STATIC
    src/adapter/grpc/detail/event_filter.cpp
    src/adapter/grpc/detail/status_to_error.cpp
    src/adapter/grpc/grpc_server_gateway.cpp
    src/adapter/tui/ftxui_presenter.cpp
    src/adapter/tui/inbox_queue.cpp
    src/application/usecase/connect_to_server.cpp
    src/application/usecase/create_channel_command.cpp
    src/application/usecase/join_channel_command.cpp
    src/application/usecase/send_heartbeat_command.cpp
    src/application/usecase/send_message_command.cpp
    src/application/usecase/subscribe_command.cpp
)

target_include_directories(bcli_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bcli_lib PUBLIC
    bcmd_shared
    bcmd_proto
    gRPC::grpc++
    ftxui::component
)

target_compile_features(bcli_lib PRIVATE cxx_std_23)

set_target_properties(bcli_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(bcli src/main.cpp)

target_link_libraries(bcli PRIVATE
    bcli_lib
    bcmd_proto
    gRPC::grpc++
    CLI11::CLI11
    ftxui::component
    spdlog::spdlog
    stduuid::stduuid
)

target_compile_features(bcli PRIVATE cxx_std_23)
