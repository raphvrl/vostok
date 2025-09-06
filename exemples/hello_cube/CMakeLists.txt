cmake_minimum_required(VERSION 3.25)

add_subdirectory(shaders)

add_executable(hello_cube 
    main.cpp 
    app.cpp
    systems/cube_render_system.cpp
)
target_link_libraries(hello_cube PRIVATE vostok)

if(VOSTOK_ENABLE_TRACY)
    target_compile_definitions(hello_cube PRIVATE TRACY_ENABLE)
endif()

add_dependencies(hello_cube hello_cube_shaders)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/textures)
    add_custom_command(TARGET hello_cube POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/textures
        $<TARGET_FILE_DIR:hello_cube>/textures
        COMMENT "Copying textures to build directory"
    )
    message(STATUS "Textures will be copied to: $<TARGET_FILE_DIR:hello_cube>/textures")
else()
    message(WARNING "Textures directory not found: ${CMAKE_CURRENT_SOURCE_DIR}/textures")
endif()

install(TARGETS hello_cube
    RUNTIME DESTINATION bin
)

message(STATUS "Configured hello_cube example")
message(STATUS "Shaders will be compiled to: ${CMAKE_BINARY_DIR}/bin/shaders/hello_cube")
message(STATUS "and copied to: $<TARGET_FILE_DIR:hello_cube>/shaders")