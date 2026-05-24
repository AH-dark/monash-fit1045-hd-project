cmake_minimum_required(VERSION 3.27)

find_package(Catch2 REQUIRED CONFIG)
find_package(gRPC REQUIRED CONFIG)

include(CTest)
include(Catch)

add_executable(proto_smoke_test
    unit/server/domain/proto_smoke_test.cpp
)

target_link_libraries(proto_smoke_test PRIVATE
    bcmd_proto
    Catch2::Catch2WithMain
)

target_compile_features(proto_smoke_test PRIVATE cxx_std_23)

catch_discover_tests(proto_smoke_test PROPERTIES LABELS "unit")

add_executable(shared_tests
    unit/shared/ids_test.cpp
    unit/shared/result_test.cpp
    unit/shared/logging_test.cpp
)

target_link_libraries(shared_tests PRIVATE
    bcmd_shared
    Catch2::Catch2WithMain
)

target_compile_features(shared_tests PRIVATE cxx_std_23)

catch_discover_tests(shared_tests PROPERTIES LABELS "unit")

add_executable(server_domain_tests
    unit/server/domain/value_objects_test.cpp
    unit/server/domain/channel_test.cpp
    unit/server/domain/message_test.cpp
    unit/server/domain/message_router_test.cpp
)

target_link_libraries(server_domain_tests PRIVATE
    bcmd_domain
    Catch2::Catch2WithMain
)

target_compile_features(server_domain_tests PRIVATE cxx_std_23)

catch_discover_tests(server_domain_tests PROPERTIES LABELS "unit")

add_executable(server_application_tests
    unit/server/application/join_channel_test.cpp
    unit/server/application/leave_channel_test.cpp
    unit/server/application/send_message_test.cpp
    unit/server/application/list_channels_test.cpp
    unit/server/application/get_recent_messages_test.cpp
    unit/server/application/expire_inactive_clients_test.cpp
)

target_include_directories(server_application_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(server_application_tests PRIVATE
    bcmd_application
    Catch2::Catch2WithMain
)

target_compile_features(server_application_tests PRIVATE cxx_std_23)

catch_discover_tests(server_application_tests PROPERTIES LABELS "unit")

add_executable(server_adapter_persistence_tests
    unit/server/adapter/persistence/in_memory_client_registry_heartbeat_test.cpp
)

target_include_directories(server_adapter_persistence_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(server_adapter_persistence_tests PRIVATE
    bcmd_server_adapters
    Catch2::Catch2WithMain
)

target_compile_features(server_adapter_persistence_tests PRIVATE cxx_std_23)

catch_discover_tests(server_adapter_persistence_tests PROPERTIES LABELS "unit")

add_executable(server_adapter_grpc_tests
    unit/server/adapter/grpc/error_to_status_test.cpp
)

target_link_libraries(server_adapter_grpc_tests PRIVATE
    bcmd_server_adapters
    Catch2::Catch2WithMain
)

target_compile_features(server_adapter_grpc_tests PRIVATE cxx_std_23)

catch_discover_tests(server_adapter_grpc_tests PROPERTIES LABELS "unit")

add_executable(client_application_tests
    unit/client/application/send_message_command_test.cpp
)

target_include_directories(client_application_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(client_application_tests PRIVATE
    bcli_lib
    Catch2::Catch2WithMain
)

target_compile_features(client_application_tests PRIVATE cxx_std_23)

catch_discover_tests(client_application_tests PROPERTIES LABELS "unit")

add_executable(client_adapter_tests
    unit/client/adapter/cli/command_parser_test.cpp
    unit/client/adapter/tui/channel_list_test.cpp
    unit/client/adapter/tui/scroll_math_test.cpp
    unit/client/adapter/tui/presenter_scroll_test.cpp
)

target_include_directories(client_adapter_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(client_adapter_tests PRIVATE
    BCMD_CLIENT_TUI_TESTING
)

target_link_libraries(client_adapter_tests PRIVATE
    bcli_lib
    Catch2::Catch2WithMain
)

target_compile_features(client_adapter_tests PRIVATE cxx_std_23)

catch_discover_tests(client_adapter_tests TEST_PREFIX "client_adapter_tests." PROPERTIES LABELS "unit")

add_executable(client_adapter_grpc_tests
    unit/client/adapter/grpc/event_filter_test.cpp
    unit/client/adapter/grpc/status_to_error_test.cpp
)

target_link_libraries(client_adapter_grpc_tests PRIVATE
    bcli_lib
    Catch2::Catch2WithMain
)

target_compile_features(client_adapter_grpc_tests PRIVATE cxx_std_23)

catch_discover_tests(client_adapter_grpc_tests PROPERTIES LABELS "unit")

function(add_bcmd_integration_test target source)
    add_executable(${target}
        ${source}
    )

    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(${target} PRIVATE
        bcmd_server_adapters
        bcmd_application
        bcmd_domain
        bcmd_shared
        bcmd_proto
        gRPC::grpc++
        Catch2::Catch2WithMain
    )

    target_compile_features(${target} PRIVATE cxx_std_23)
    catch_discover_tests(${target} PROPERTIES LABELS "integration")
endfunction()

add_bcmd_integration_test(integration_grpc_round_trip_test
    integration/grpc_round_trip_test.cpp
)
add_bcmd_integration_test(integration_reconnect_test
    integration/reconnect_test.cpp
)
add_bcmd_integration_test(integration_channel_lifecycle_test
    integration/channel_lifecycle_test.cpp
)
add_bcmd_integration_test(integration_history_replay_test
    integration/history_replay_test.cpp
)
add_bcmd_integration_test(integration_tls_enforcement_test
    integration/tls_enforcement_test.cpp
)
add_bcmd_integration_test(integration_heartbeat_expiry_test
    integration/heartbeat_expiry_test.cpp
)
add_bcmd_integration_test(integration_disconnect_broadcasts_member_left_test
    integration/disconnect_broadcasts_member_left_test.cpp
)
add_bcmd_integration_test(integration_cross_channel_leave_isolation_test
    integration/cross_channel_leave_isolation_test.cpp
)
add_bcmd_integration_test(integration_server_rejects_slash_prefix_test
    integration/server_rejects_slash_prefix_test.cpp
)
