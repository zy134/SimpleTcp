add_executable(CompressTest ./CompressTest.cpp)
target_include_directories(CompressTest PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(CompressTest SimpleTcp_base)
