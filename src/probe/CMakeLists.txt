aux_source_directory(. PROBE_LIST)

link_directories(${LINK_PATH})

set(PROBE probe)

add_executable(${PROBE} ${PROBE_LIST})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

target_link_libraries(${PROBE} ${LIBAV})
