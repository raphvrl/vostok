cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE SHADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.comp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.geom"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.tesc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.tese"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.mesh"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.task"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.rgen"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.rchit"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.rmiss"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.glsl"
)

find_program(GLSLC glslc HINTS
    "$ENV{VULKAN_SDK}/bin"
    "$ENV{VULKAN_SDK}/Bin"
    "$ENV{VULKAN_SDK}"
)

if(NOT GLSLC)
    message(FATAL_ERROR "Could not find glslc shader compiler. Make sure Vulkan SDK is installed.")
endif()

set(SPIRV_BINARY_FILES)

set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

foreach(SHADER ${SHADER_FILES})
    get_filename_component(FILENAME ${SHADER} NAME)
    set(OUTPUT "${SHADER_BINARY_DIR}/${FILENAME}.spv")
    
    # Options de compilation
    set(GLSLC_FLAGS "")

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
        COMMENT "Compiling shader ${FILENAME}"
        VERBATIM
    )

    list(APPEND SPIRV_BINARY_FILES ${OUTPUT})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPIRV_BINARY_FILES})

install(
    DIRECTORY ${SHADER_BINARY_DIR}/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/shaders
)

message(STATUS "Shader compiler: ${GLSLC}")
message(STATUS "Number of shaders to compile: ${CMAKE_CONFIGURATION_TYPES}")
message(STATUS "Shader output directory: ${SHADER_BINARY_DIR}")