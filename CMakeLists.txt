cmake_minimum_required(VERSION 3.22)
project(uwbp_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Dependencies ----
find_package(Threads REQUIRED)
find_package(Poco REQUIRED COMPONENTS Net Util Foundation JSON)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Systemd REQUIRED IMPORTED_TARGET libsystemd)
find_package(sdbus-c++ REQUIRED)

# ---- Main server executable ----
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/*/src/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/*/*/src/*.cpp"
)
# Watchdog hat eigene main() -- aus Server-Glob ausschliessen
list(FILTER APP_SOURCES EXCLUDE REGEX ".*/watchdog/.*")

add_executable(uwbp_server ${APP_SOURCES})

target_include_directories(uwbp_server PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/src/main/header"
  "${CMAKE_SOURCE_DIR}/src/server/header"
  "${CMAKE_SOURCE_DIR}/src/server/rest/header"
  "${CMAKE_SOURCE_DIR}/src/server/ws/header"
  "${CMAKE_SOURCE_DIR}/src/net/header"
  "${CMAKE_SOURCE_DIR}/src/common/header"
  "${CMAKE_SOURCE_DIR}/src/uwb/header"
)

target_link_libraries(uwbp_server PRIVATE
  Poco::Net
  Poco::Util
  Poco::Foundation
  Poco::JSON
  SDBusCpp::sdbus-c++
  PkgConfig::Systemd
)

# ---- Watchdog executable ----
add_executable(uwbp_watchdog
  "${CMAKE_SOURCE_DIR}/src/watchdog/src/watchdog_main.cpp"
)

target_include_directories(uwbp_watchdog PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(uwbp_watchdog PRIVATE
  SDBusCpp::sdbus-c++
  PkgConfig::Systemd
)
