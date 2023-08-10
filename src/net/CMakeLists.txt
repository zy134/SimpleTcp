aux_source_directory(. NET_SOURCES)
add_library(SimpleTcp_net STATIC ${NET_SOURCES})
target_include_directories(SimpleTcp_net PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(SimpleTcp_net SimpleTcp_base)
