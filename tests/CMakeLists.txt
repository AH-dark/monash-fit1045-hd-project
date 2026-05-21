cmake_minimum_required(VERSION 3.27)

find_package(Catch2 REQUIRED CONFIG)

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

catch_discover_tests(proto_smoke_test PROPERTIES LABELS "proto")

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

catch_discover_tests(shared_tests PROPERTIES LABELS "shared")

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

catch_discover_tests(server_domain_tests PROPERTIES LABELS "server-domain")

add_executable(server_application_tests
    unit/server/application/join_channel_test.cpp
    unit/server/application/leave_channel_test.cpp
    unit/server/application/send_message_test.cpp
    unit/server/application/list_channels_test.cpp
    unit/server/application/get_recent_messages_test.cpp
)

target_include_directories(server_application_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(server_application_tests PRIVATE
    bcmd_application
    Catch2::Catch2WithMain
)

target_compile_features(server_application_tests PRIVATE cxx_std_23)

catch_discover_tests(server_application_tests PROPERTIES LABELS "server-application")
