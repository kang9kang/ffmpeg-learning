aux_source_directory(. REMUX_LIST)

link_directories(${LINK_PATH})

set(REMUX remux)

add_executable(${REMUX} ${REMUX_LIST})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

target_link_libraries(${REMUX} ${LIBAV})
