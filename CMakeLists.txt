cmake_minimum_required(VERSION 3.16)

project(wifisync)

# set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)


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

file(GLOB SRCS src/*.cpp)

add_executable(app 
  src/broadcasts.cpp
  src/difference.cpp
  src/environment.cpp
  src/files_opers.cpp
  src/hash.cpp
  src/init.cpp
  src/http_server.cpp
  src/server.cpp
  src/TCPSocket.cpp
  src/threads.cpp
  src/utils.cpp
  src/dif_work.cpp
  src/transport.cpp
  src/dif_walker.cpp
  src/change_applier.cpp
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
)
