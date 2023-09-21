# fmtlib
# generate header only library
add_compile_definitions(FMT_HEADER_ONLY)
add_library(fmt INTERFACE)
target_include_directories(fmt INTERFACE ${PROJECT_SOURCE_DIR}/lib/include)
