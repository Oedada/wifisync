cmake_minimum_required(VERSION 3.16)

project(wifisync VERSION 0.1.0)

# set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_subdirectory(external)
include(FetchContent)

fetchcontent_declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.3
)

fetchcontent_makeavailable(httplib)

fetchcontent_declare(
  stduuid
  GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
  GIT_TAG v1.2.3
)

fetchcontent_makeavailable(stduuid)

add_compile_options(
    -Wall
    -Wextra
    -pthread
)

add_executable(app 
  src/main.cpp
  src/threads.cpp
  src/core/difference.cpp
  src/core/dif_work.cpp
  src/core/dif_walker.cpp
  src/core/transport.cpp
  src/core/change_applier.cpp
  src/network/broadcasts.cpp
  src/network/TCPSocket.cpp
  src/network/http_server.cpp
  src/network/server.cpp
  src/network/websocket.cpp
  src/crypto/ed25519.cpp
  src/crypto/x25519.cpp
  src/crypto/hash.cpp
  src/fs/environment.cpp
  src/fs/files_opers.cpp
  src/util/init.cpp
  src/util/utils.cpp
)

target_include_directories(app PRIVATE
    include
)

find_package(OpenSSL REQUIRED)

find_package(SDL2 REQUIRED)

target_link_libraries(app PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    SDL2::SDL2
    httplib
    stduuid
    uWebSockets
)
