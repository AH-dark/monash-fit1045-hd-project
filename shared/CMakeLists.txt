cmake_minimum_required(VERSION 3.27)

find_package(spdlog REQUIRED CONFIG)
find_package(stduuid REQUIRED CONFIG)

add_library(bcmd_shared STATIC
    src/result.cpp
    src/logging.cpp
    src/string_utils.cpp
)

target_include_directories(bcmd_shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bcmd_shared PUBLIC
    spdlog::spdlog
    stduuid::stduuid
    bcmd_proto
)

target_compile_features(bcmd_shared PUBLIC cxx_std_23)

set_target_properties(bcmd_shared PROPERTIES POSITION_INDEPENDENT_CODE ON)
