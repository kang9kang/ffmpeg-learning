aux_source_directory(. REMUXING_LIST)

link_directories(/usr/local/lib)

add_executable(remuxing ${REMUXING_LIST})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

target_link_libraries(remuxing avformat avcodec avutil)
