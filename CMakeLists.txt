cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC_FILE CONFIGURE_DEPENDS
  "src/*.cc"
)

project(KazuMyLove)

add_executable(test ${SRC_FILE})

target_link_libraries(test PRIVATE ${CMAKE_SOURCE_DIR}/lib raylib)

target_compile_options(test PRIVATE -O3)
