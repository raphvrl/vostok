cmake_minimum_required(VERSION 3.25)

option(VOSTOK_SHADER_OPTIMIZATION "Enable shader optimization in all builds" OFF)
option(VOSTOK_SHADER_DEBUG_INFO "Include debug info in shader binaries" ON)
option(VOSTOK_SHADER_WARNINGS_AS_ERRORS "Treat shader compiler warnings as errors" ON)

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

set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

set(SPIRV_BINARY_FILES)

foreach(SHADER ${SHADER_FILES})
    get_filename_component(FILENAME ${SHADER} NAME)

    file(RELATIVE_PATH SHADER_RELPATH ${CMAKE_CURRENT_SOURCE_DIR} ${SHADER})
    get_filename_component(SHADER_DIR ${SHADER_RELPATH} DIRECTORY)

    if(SHADER_DIR)
        set(OUTPUT_DIR "${SHADER_BINARY_DIR}/${SHADER_DIR}")
        file(MAKE_DIRECTORY ${OUTPUT_DIR})
        set(OUTPUT "${OUTPUT_DIR}/${FILENAME}.spv")
    else()
        set(OUTPUT "${SHADER_BINARY_DIR}/${FILENAME}.spv")
    endif()

    set(GLSLC_FLAGS "--target-env=vulkan1.3")
    
    if(VOSTOK_SHADER_WARNINGS_AS_ERRORS)
        list(APPEND GLSLC_FLAGS "-Werror")
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR VOSTOK_SHADER_DEBUG_INFO)
        list(APPEND GLSLC_FLAGS "-g")
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR VOSTOK_SHADER_OPTIMIZATION)
        list(APPEND GLSLC_FLAGS "-O")
    endif()

    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND ${GLSLC} ${GLSLC_FLAGS} -o ${OUTPUT} ${SHADER}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${SHADER_RELPATH}"
        VERBATIM
    )

    list(APPEND SPIRV_BINARY_FILES ${OUTPUT})
endforeach()

set_source_files_properties(${SPIRV_BINARY_FILES} PROPERTIES GENERATED TRUE)

add_custom_target(shaders ALL DEPENDS ${SPIRV_BINARY_FILES})

install(
    DIRECTORY ${SHADER_BINARY_DIR}/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/shaders
)

list(LENGTH SHADER_FILES SHADER_COUNT)
message(STATUS "Shader compiler: ${GLSLC}")
message(STATUS "Number of shaders to compile: ${SHADER_COUNT}")
message(STATUS "Shader output directory: ${SHADER_BINARY_DIR}")