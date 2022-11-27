find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

file(GLOB SRC_FILES_TEST
"${PROJECT_SOURCE_DIR}/src/*.cpp"
"${PROJECT_SOURCE_DIR}/src/*.c"
"${PROJECT_SOURCE_DIR}/src/*.cc"
"${PROJECT_SOURCE_DIR}/include/*.h"
)
# set(SRC_FILES_TEST
# "${PROJECT_SOURCE_DIR}/include/configloader.h"
# "${PROJECT_SOURCE_DIR}/src/configloader.cpp"
# )
list(REMOVE_ITEM SRC_FILES_TEST "${PROJECT_SOURCE_DIR}/src/redbase.cpp")
add_executable(mytest ${SRC_FILES_TEST} ix_test.cpp)
add_executable(mytest1 ${SRC_FILES_TEST} rm_test.cpp)
target_link_libraries(
  mytest
  GTest::gtest_main
)
target_link_libraries(
  mytest1
  GTest::gtest_main
)
add_test(
    NAME mytest
    COMMAND mytest
)
add_test(
    NAME mytest1
    COMMAND mytest1
)
# 添加到测试
gtest_discover_tests(mytest)
gtest_discover_tests(mytest1)