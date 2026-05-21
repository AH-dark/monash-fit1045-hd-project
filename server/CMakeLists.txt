cmake_minimum_required(VERSION 3.27)

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

add_executable(bcmd src/main.cpp)

target_link_libraries(bcmd PRIVATE
    bcmd_domain
)

target_compile_features(bcmd PRIVATE cxx_std_23)
