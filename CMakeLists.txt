cmake_minimum_required(VERSION 3.20)

project(RedBase)

file(GLOB SRC_FILES
"${PROJECT_SOURCE_DIR}/src/*.cpp"
"${PROJECT_SOURCE_DIR}/src/*.c"
"${PROJECT_SOURCE_DIR}/src/*.cc"
"${PROJECT_SOURCE_DIR}/include/*.h"
)
include_directories(include)
set(CMAKE_CXX_STANDARD 17)
add_executable(${CMAKE_PROJECT_NAME} ${SRC_FILES})
