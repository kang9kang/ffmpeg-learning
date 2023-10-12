aux_source_directory(. TRANSCODING_LIST)

link_directories(${LINK_PATH})

include_directories(${INCLUDE_PATH})

set(TRANSCODING transcoding)

add_executable(${TRANSCODING} ${TRANSCODING_LIST})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

target_link_libraries(${TRANSCODING} ${LIBAV})
