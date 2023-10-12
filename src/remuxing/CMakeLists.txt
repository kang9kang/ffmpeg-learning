aux_source_directory(. REMUXING_LIST)

link_directories(${LINK_PATH})

set(REMUXING remuxing)

add_executable(${REMUXING} ${REMUXING_LIST})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

target_link_libraries(${REMUXING} ${LIBAV})
