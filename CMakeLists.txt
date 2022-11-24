cmake_minimum_required(VERSION 3.20)

project(RedBase)
file(GLOB SRC_FILES
"${PROJECT_SOURCE_DIR}/src/*.cpp"
"${PROJECT_SOURCE_DIR}/src/*.c"
"${PROJECT_SOURCE_DIR}/src/*.cc"
"${PROJECT_SOURCE_DIR}/include/*.h"
)
# include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)
list(REMOVE_ITEM SRC_FILES "{PROJECT_SOURCE_DIR}/src/redbase.cpp")
set(CMAKE_CXX_STANDARD 17)

enable_testing()

add_executable(${CMAKE_PROJECT_NAME} ${SRC_FILES} src/redbase.cpp)

# add_library(RedBase_lib STATIC ${SRC_FILES})

add_subdirectory(test)