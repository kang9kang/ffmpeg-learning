cmake_minimum_required(VERSION 3.15)

project(main)

set(CMAKE_CXX_STANDARD 17)

set(LINK_PATH /usr/local/lib)

set(LIBAV avcodec avformat avutil)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src)

