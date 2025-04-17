cmake_minimum_required(VERSION 3.9)
project(CPU_CGAL)

set(CMAKE_BUILD_TYPE "Release")

find_package(CGAL REQUIRED)
find_package (Eigen3 REQUIRED NO_MODULE)
find_package(Boost 1.45.0 COMPONENTS filesystem)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

file(GLOB GEOMETRY_SRC_FILES geometry/*.cpp)
add_library(geometry ${GEOMETRY_SRC_FILES})
target_include_directories(geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/geometry)
target_link_libraries(geometry Eigen3::Eigen)
target_link_libraries(geometry CGAL::CGAL)


add_executable(cpu_cgal cpu_cgal.cpp)
target_link_libraries(cpu_cgal geometry)
target_link_libraries(cpu_cgal Eigen3::Eigen)
target_link_libraries(cpu_cgal CGAL::CGAL)


if(Boost_FOUND)
    target_include_directories(cpu_cgal PUBLIC ${Boost_INCLUDE_DIRS}) 
    target_link_libraries(cpu_cgal ${Boost_LIBRARIES})
endif()

