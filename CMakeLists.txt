cmake_minimum_required(VERSION 3.16)

project(MosaicRestored LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

set(HTTP_SERVER_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/extensions/HttpServer)
set(APP_OUTPUT_DIR ${CMAKE_BINARY_DIR}/app)

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
    RUNTIME_OUTPUT_DIRECTORY ${APP_OUTPUT_DIR}
)

add_custom_target(copy_runtime_assets ALL
    COMMAND ${CMAKE_COMMAND} -E remove_directory
        $<TARGET_FILE_DIR:mosaic_server>/wwwroot
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/app/wwwroot
            $<TARGET_FILE_DIR:mosaic_server>/wwwroot
    COMMAND ${CMAKE_COMMAND} -E remove_directory
        $<TARGET_FILE_DIR:mosaic_server>/serving
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/app/serving
            $<TARGET_FILE_DIR:mosaic_server>/serving
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/app/start_services.sh
            $<TARGET_FILE_DIR:mosaic_server>/start_services.sh
    COMMAND chmod +x $<TARGET_FILE_DIR:mosaic_server>/start_services.sh
)
add_dependencies(copy_runtime_assets mosaic_server)
