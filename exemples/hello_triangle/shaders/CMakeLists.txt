cmake_minimum_required(VERSION 3.25)

set(APP_NAME hello_triangle)

find_program(GLSLC glslc HINTS "$ENV{VULKAN_SDK}/bin" "/usr/bin")
if(NOT GLSLC)
    find_program(GLSLC glslangValidator HINTS "/usr/bin")
    if(NOT GLSLC)
        message(FATAL_ERROR "Could not find glslc or glslangValidator")
    endif()
endif()

set(SHADER_DIR "${CMAKE_BINARY_DIR}/bin/shaders/${APP_NAME}")
file(MAKE_DIRECTORY ${SHADER_DIR})

set(SHADERS triangle.vert triangle.frag)
set(SPIRV_FILES)

foreach(SHADER ${SHADERS})
    set(INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}")
    set(OUTPUT "${SHADER_DIR}/${SHADER}.spv")
    
    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND ${GLSLC} --target-env vulkan1.4 -V ${INPUT} -o ${OUTPUT}
        DEPENDS ${INPUT}
        COMMENT "Compiling ${SHADER}"
    )
    
    list(APPEND SPIRV_FILES ${OUTPUT})
endforeach()

add_custom_target(${APP_NAME}_shaders ALL DEPENDS ${SPIRV_FILES})

add_custom_command(
    TARGET ${APP_NAME}_shaders POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_DIR} $<TARGET_FILE_DIR:${APP_NAME}>/shaders
)
