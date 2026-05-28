cmake_minimum_required(VERSION 3.27)

find_package(gRPC REQUIRED CONFIG)
find_package(CLI11 REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(stduuid REQUIRED CONFIG)

add_library(bcmd_domain STATIC
    src/domain/model/username.cpp
    src/domain/model/channel_name.cpp
    src/domain/model/message_content.cpp
    src/domain/model/channel.cpp
    src/domain/model/client_session.cpp
    src/domain/service/message_router.cpp
)

target_include_directories(bcmd_domain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bcmd_domain PUBLIC
    bcmd_shared
)

target_compile_features(bcmd_domain PUBLIC cxx_std_23)

set_target_properties(bcmd_domain PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(bcmd_application STATIC
    src/application/usecase/join_channel.cpp
    src/application/usecase/leave_channel.cpp
    src/application/usecase/expire_inactive_clients.cpp
    src/application/usecase/internal/remove_member_broadcast.cpp
    src/application/usecase/send_message.cpp
    src/application/usecase/list_channels.cpp
    src/application/usecase/get_recent_messages.cpp
    src/application/usecase/subscribe_to_channel.cpp
    src/application/usecase/create_channel.cpp
)

target_include_directories(bcmd_application PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bcmd_application PUBLIC
    bcmd_domain
)

target_link_libraries(bcmd_application PRIVATE
    spdlog::spdlog
)

target_compile_features(bcmd_application PUBLIC cxx_std_23)

set_target_properties(bcmd_application PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(bcmd_server_adapters STATIC
    src/adapter/persistence/in_memory_channel_repository.cpp
    src/adapter/persistence/in_memory_client_registry.cpp
    src/adapter/persistence/in_memory_message_repository.cpp
    src/adapter/persistence/in_memory_event_log.cpp
    src/adapter/grpc/tls_config.cpp
    src/adapter/grpc/client_publisher.cpp
    src/adapter/grpc/channel_list_publisher.cpp
    src/adapter/grpc/detail/error_to_status.cpp
    src/adapter/grpc/broadcast_service_impl.cpp
    src/adapter/grpc/heartbeat_sweeper.cpp
    src/adapter/grpc/server_runner.cpp
)

target_include_directories(bcmd_server_adapters PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bcmd_server_adapters PUBLIC
    bcmd_application
    bcmd_proto
    gRPC::grpc++
    spdlog::spdlog
)

target_compile_features(bcmd_server_adapters PUBLIC cxx_std_23)

set_target_properties(bcmd_server_adapters PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(bcmd src/main.cpp)

target_link_libraries(bcmd PRIVATE
    bcmd_server_adapters
    CLI11::CLI11
)

target_compile_features(bcmd PRIVATE cxx_std_23)
