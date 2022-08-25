cmake_minimum_required(VERSION 3.2)
project(epoll)

set(ROOT ${PROJECT_SOURCE_DIR})

add_compile_options(-O3 -fPIC)


# include_directories(${ROOT}/src/cpp/thirdparty)
# include_directories(/opt/ros/noetic/include/)

# find_package(pybind11 REQUIRED)

# link_directories(${ROOT})
# link_directories(/opt/ros/noetic/lib/)

# add_library(basics SHARED ${ROOT}/src/to_py/basics.cpp)
# target_link_libraries(basics awm pybind11::module )

add_executable(epoll_ET_LT ${ROOT}/epoll_ET_LT.cpp)
target_link_libraries(epoll_ET_LT pthread)

add_executable(oneshot ${ROOT}/oneshot.cpp)
target_link_libraries(oneshot pthread)