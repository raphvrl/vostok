cmake_minimum_required(VERSION 3.25)

set(APP_NAME hello_triangle)

set(SHADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/triangle.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/triangle.frag"
)

find_program(GLSLC glslc HINTS
    "$ENV{VULKAN_SDK}/bin"
    "$ENV{VULKAN_SDK}/Bin"
    "$ENV{VULKAN_SDK}"
)

if(NOT GLSLC)
    message(FATAL_ERROR "Could not find glslc shader compiler. Make sure Vulkan SDK is installed.")
endif()

set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/bin/shaders")
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

set(APP_SHADER_BINARY_DIR "${SHADER_BINARY_DIR}/${APP_NAME}")
file(MAKE_DIRECTORY ${APP_SHADER_BINARY_DIR})

set(SPIRV_BINARY_FILES)

foreach(SHADER ${SHADER_FILES})
    get_filename_component(FILENAME ${SHADER} NAME)
    set(OUTPUT "${APP_SHADER_BINARY_DIR}/${FILENAME}.spv")
    
    set(GLSLC_FLAGS "--target-env=vulkan1.3")
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND GLSLC_FLAGS "-g")
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        list(APPEND GLSLC_FLAGS "-O")
    endif()

    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND ${GLSLC} ${GLSLC_FLAGS} -o ${OUTPUT} ${SHADER}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${FILENAME}"
        VERBATIM
    )

    list(APPEND SPIRV_BINARY_FILES ${OUTPUT})
endforeach()

add_custom_target(${APP_NAME}_shaders ALL DEPENDS ${SPIRV_BINARY_FILES})

add_custom_command(
    TARGET ${APP_NAME}_shaders POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${APP_SHADER_BINARY_DIR}
        $<TARGET_FILE_DIR:${APP_NAME}>/shaders
    COMMENT "Copying shaders to executable directory"
)

add_custom_command(
    TARGET ${APP_NAME}_shaders POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${APP_SHADER_BINARY_DIR}
        ${SHADER_BINARY_DIR}
    COMMENT "Copying shaders to global shader directory"
)
