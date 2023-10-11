aux_source_directory(. SRC_LIST)

link_directories(/usr/local/lib)

add_executable(${PROJECT_NAME} ${SRC_LIST})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

target_link_libraries(${PROJECT_NAME} avformat avcodec)