cmake_minimum_required(VERSION 3.16)

project(MosaicRestored LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

set(HTTP_SERVER_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/extensions/HttpServer)

file(GLOB_RECURSE HTTP_SERVER_SOURCES CONFIGURE_DEPENDS
    ${HTTP_SERVER_ROOT}/src/*.cpp
)

add_executable(mosaic_server
    app/server.cpp
    ${HTTP_SERVER_SOURCES}
)
target_include_directories(mosaic_server
    PRIVATE
        ${HTTP_SERVER_ROOT}/include
)
target_link_libraries(mosaic_server
    PRIVATE
        Threads::Threads
        CURL::libcurl
)

set_target_properties(mosaic_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET mosaic_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/app/wwwroot
            $<TARGET_FILE_DIR:mosaic_server>/wwwroot
)
