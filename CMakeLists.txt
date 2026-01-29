cmake_minimum_required(VERSION 3.22)
project(uwbp_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/*/src/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/*/*/src/*.cpp"
)

add_executable(uwbp_server ${APP_SOURCES})

target_include_directories(uwbp_server PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/src/main/header"
  "${CMAKE_SOURCE_DIR}/src/server/header"
  "${CMAKE_SOURCE_DIR}/src/server/rest/header"
  "${CMAKE_SOURCE_DIR}/src/server/ws/header"
  "${CMAKE_SOURCE_DIR}/src/net/header"
  "${CMAKE_SOURCE_DIR}/src/common/header"
)

find_package(Threads REQUIRED)

find_package(Poco REQUIRED COMPONENTS Net Util Foundation)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Systemd REQUIRED IMPORTED_TARGET libsystemd)

find_package(sdbus-c++ REQUIRED)

target_link_libraries(uwbp_server PRIVATE
  Poco::Net
  Poco::Util
  Poco::Foundation
  SDBusCpp::sdbus-c++
  PkgConfig::Systemd
)
