aux_source_directory(. TRANSCODE_LIST)

link_directories(${LINK_PATH})

include_directories(${INCLUDE_PATH})

set(TRANSCODE transcode)

add_executable(${TRANSCODE} ${TRANSCODE_LIST})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

target_link_libraries(${TRANSCODE} ${LIBAV})
