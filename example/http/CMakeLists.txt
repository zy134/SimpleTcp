add_executable(HttpServ HttpServ.cpp)
target_include_directories(HttpServ PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(HttpServ SimpleTcp_http)
# execute_process(COMMAND cp ./index.html ${CMAKE_BINARY_DIR}/example/http
#     WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/example/http
# )
# execute_process(COMMAND cp ./img.jpg ${CMAKE_BINARY_DIR}/example/http
#     WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/example/http
# )

add_compile_definitions(
    HTTP_RESOURCE_PATH="${PROJECT_SOURCE_DIR}/example/http"
)
