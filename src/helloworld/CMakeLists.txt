aux_source_directory(. HELLOWORLD_LIST)

link_directories(/usr/local/lib)

add_executable(helloworld ${HELLOWORLD_LIST})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

target_link_libraries(helloworld avformat avcodec avutil)
