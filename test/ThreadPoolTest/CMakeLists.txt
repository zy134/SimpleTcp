aux_source_directory(. TEST_SRC_FILES)

foreach(TEST_SRC ${TEST_SRC_FILES})
    get_filename_component(TEST_OBJ ${TEST_SRC} NAME_WE)
    add_executable(${TEST_OBJ} ${TEST_SRC})
    target_include_directories(${TEST_OBJ} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(${TEST_OBJ} SimpleTcp_tcp)
endforeach()
