aux_source_directory(. BASE_SOURCES)
add_library(SimpleTcp_base STATIC ${BASE_SOURCES})
target_include_directories(SimpleTcp_base PRIVATE ${PROJECT_SOURCE_DIR}/include)
