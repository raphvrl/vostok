cmake_minimum_required(VERSION 3.25)

add_subdirectory(shaders)

add_executable(hello_triangle main.cpp)
target_link_libraries(hello_triangle PRIVATE vostok)

add_dependencies(hello_triangle hello_triangle_shaders)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/textures)
    add_custom_command(TARGET hello_triangle POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/textures
        $<TARGET_FILE_DIR:hello_triangle>/textures
        COMMENT "Copying textures to build directory"
    )
    message(STATUS "Textures will be copied to: $<TARGET_FILE_DIR:hello_triangle>/textures")
else()
    message(WARNING "Textures directory not found: ${CMAKE_CURRENT_SOURCE_DIR}/textures")
endif()

install(TARGETS hello_triangle
    RUNTIME DESTINATION bin
)

message(STATUS "Configured hello_triangle example")
message(STATUS "Shaders will be compiled to: ${CMAKE_BINARY_DIR}/bin/shaders/hello_triangle")
message(STATUS "and copied to: $<TARGET_FILE_DIR:hello_triangle>/shaders")