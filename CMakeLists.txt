cmake_minimum_required(VERSION 3.25.0)
project(CompSci-NEA VERSION 0.1.0)

set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples")
add_subdirectory(external/raylib)

include_directories(external/asio/asio/include external/sqlite3 external/raylib/src)
add_library(sqlite3 STATIC external/sqlite3/sqlite3.c external/sqlite3/shell.c)

add_definitions(-DASIO_STANDALONE)

file(GLOB_RECURSE CLIENT_SOURCES "src/Client/*.cpp")
add_executable(TCP-Client ${CLIENT_SOURCES})
target_link_libraries(TCP-Client raylib)

file(GLOB_RECURSE SERVER_SOURCES "src/Server/*.cpp")
add_executable(TCP-Server ${SERVER_SOURCES})
target_link_libraries(TCP-Server sqlite3)

file(GLOB_RECURSE GAME_SOURCES "src/MainGame/*.cpp")
add_executable(MainGame ${GAME_SOURCES})
target_link_libraries(MainGame raylib)
