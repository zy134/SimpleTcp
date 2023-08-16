add_executable(StringHelperTest ./StringHelperTest.cpp)
target_include_directories(StringHelperTest PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(StringHelperTest SimpleTcp_base)
