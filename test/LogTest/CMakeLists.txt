add_executable(LogBench ./LogBench.cpp)
target_include_directories(LogBench PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(LogBench SimpleTcp_base)

add_executable(LogTest ./LogTest.cpp)
target_include_directories(LogTest PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(LogTest SimpleTcp_base)
