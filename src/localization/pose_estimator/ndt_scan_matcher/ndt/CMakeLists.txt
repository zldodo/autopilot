cmake_minimum_required(VERSION 3.5)
project(ndt)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
  # add_compile_options(-march=native -Wall -Wextra -Wpedantic )
endif()

# pcl 1.7 causes a segfault when it is built with debug mode
# set(CMAKE_BUILD_TYPE "RELEASE")

# find_package(ament_cmake REQUIRED)
find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()
find_package(PCL REQUIRED COMPONENTS common io registration)
find_package(ndt_omp REQUIRED)
find_package(ndt_pcl_modified REQUIRED)

###########
## Build ##
###########
include_directories(${ndt_omp_INCLUDE_DIRS})
link_directories(${ndt_omp_LIBRARY_DIRS})
ament_auto_add_library(ndt SHARED
  src/base.cpp
  src/pcl_generic.cpp
  src/pcl_modified.cpp
  src/omp.cpp
)
target_link_libraries(ndt 
  ${PCL_LIBRARIES} 
  ${ndt_omp_LIBRARIES}
  ${ndt_pcl_modified_LIBRARIES}
)

#gtest
set(NDT_TEST ndt_gtest)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
  
  ament_add_gtest(${NDT_TEST}
  test/ndt_test.cpp)
  target_link_libraries(${NDT_TEST} ndt)
  target_compile_definitions(${NDT_TEST} PRIVATE -Wno-conversion -Wno-float-conversion -Wno-double-promotion)

endif()

# ament_package()
ament_auto_package()
