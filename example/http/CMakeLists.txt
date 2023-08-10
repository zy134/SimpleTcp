add_executable(HttpServ HttpServ.cpp)
target_include_directories(HttpServ PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(HttpServ SimpleTcp_http)
