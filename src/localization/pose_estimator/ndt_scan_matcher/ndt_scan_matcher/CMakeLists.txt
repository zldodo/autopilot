cmake_minimum_required(VERSION 3.5)
project(ndt_scan_matcher)

add_definitions(-std=c++14)
set(CMAKE_CXX_FLAGS "-std=c++14")
# add_compile_options(-march=native)
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()

ament_auto_add_executable(ndt_scan_matcher
  src/debug.cpp
  src/ndt_scan_matcher_node.cpp
  src/ndt_scan_matcher_core.cpp
  src/util_func.cpp
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_auto_package(
  INSTALL_TO_SHARE
    launch
    config
)
