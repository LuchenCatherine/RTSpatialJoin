if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(CPU_BASELINE)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)

# CGAL
find_package(CGAL REQUIRED)

# Eigen
find_package(Eigen3 REQUIRED NO_MODULE)

# filesystem
find_package(Boost REQUIRED COMPONENTS filesystem)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(cpu_baseline
    cpu_cgal2.cpp
    mymesh.cpp
  )

target_link_libraries(cpu_baseline CGAL::CGAL)
target_link_libraries(cpu_baseline Eigen3::Eigen)
target_link_libraries(cpu_baseline ${Boost_LIBRARIES})


