cmake_minimum_required(VERSION 3.16)
project(nerulink LANGUAGES CXX)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
 
find_package(Threads REQUIRED)
 
set(NERULINK_SOURCES
    src/main.cpp
    src/discovery/discovery_common.cpp
)
 
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AVAHI REQUIRED IMPORTED_TARGET avahi-client)
    list(APPEND NERULINK_SOURCES src/discovery/discovery_linux.cpp)
else()
    message(FATAL_ERROR "Solo hay backend de descubrimiento para Linux (Avahi).")
endif()
 
add_executable(nerulink ${NERULINK_SOURCES})
 
target_include_directories(nerulink PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(nerulink PRIVATE -Wall -Wextra)
target_link_libraries(nerulink PRIVATE Threads::Threads PkgConfig::AVAHI)