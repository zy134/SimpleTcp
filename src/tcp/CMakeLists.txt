aux_source_directory(. TCP_SOURCES)
add_library(SimpleTcp_tcp STATIC ${TCP_SOURCES})
target_include_directories(SimpleTcp_tcp PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(SimpleTcp_tcp SimpleTcp_net)
