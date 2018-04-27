project(dfs_client)

file(GLOB_RECURSE SOURCE_FILES "*.h" "*.cpp")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_LIST_DIR}/../../bin)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../)

link_directories(${CMAKE_CURRENT_LIST_DIR}/../../lib)
#link_directories(${CMAKE_CURRENT_BINARY_DIR}/../)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} simple_uv )
