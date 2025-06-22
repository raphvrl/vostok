cmake_minimum_required(VERSION 3.25)
project(vostok VERSION 0.1.0 LANGUAGES CXX C)

option(VOSTOK_ENABLE_VALIDATION "Enable Vulkan validation layers" ON)
option(VOSTOK_TREAT_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(VOSTOK_ENABLE_CLANG_TIDY "Enable clang-tidy checks" ON)
option(VOSTOK_BUILD_EXAMPLES "Build Vostok examples" ON)
option(VOSTOK_BUILD_TESTS "Build Vostok tests" ON)
option(VOSTOK_ENABLE_COVERAGE "Enable coverage reporting" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(VOSTOK_WARNING_FLAGS "")
set(VOSTOK_OPTIMIZATION_FLAGS "")

if (MSVC)
    list(APPEND VOSTOK_WARNING_FLAGS "/W4")
    set(VOSTOK_OPTIMIZATION_FLAGS "/O2")
    if(VOSTOK_TREAT_WARNINGS_AS_ERRORS)
        list(APPEND VOSTOK_WARNING_FLAGS "/WX")
    endif()
else()
    list(APPEND VOSTOK_WARNING_FLAGS "-Wall" "-Wextra" "-Wpedantic" "-Wconversion" "-Wshadow" "-Wcast-qual")
    set(VOSTOK_OPTIMIZATION_FLAGS "-O3")
    if(VOSTOK_TREAT_WARNINGS_AS_ERRORS)
        list(APPEND VOSTOK_WARNING_FLAGS "-Werror")
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32 AND NOT VOSTOK_ENABLE_COVERAGE)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-fsanitize=address" HAS_ASAN)
    check_cxx_compiler_flag("-fsanitize=undefined" HAS_UBSAN)
    
    if(HAS_ASAN AND HAS_UBSAN)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address,undefined")
        message(STATUS "Address and UndefinedBehavior sanitizers enabled for Debug builds")
    else()
        message(STATUS "Sanitizers not available or incompatible with current setup")
    endif()
else()
    if(WIN32)
        message(STATUS "Sanitizers disabled on Windows")
    elseif(VOSTOK_ENABLE_COVERAGE)
        message(STATUS "Sanitizers disabled when coverage is enabled")
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    link_libraries(stdc++exp)
endif()

if(VOSTOK_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        message(STATUS "Clang-Tidy enabled: ${CLANG_TIDY_EXE}")
        set(CMAKE_CXX_CLANG_TIDY 
            ${CLANG_TIDY_EXE}
            -p=${CMAKE_BINARY_DIR}
            -header-filter=^${CMAKE_CURRENT_SOURCE_DIR}/include/vostok/.*
            --config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy
            -system-headers=false
        )
    else()
        message(WARNING "Clang-Tidy requested but executable not found")
        set(VOSTOK_ENABLE_CLANG_TIDY OFF)
    endif()
endif()

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

function(add_third_party_library NAME)
    set(DIR_NAME ${NAME})
    if (${ARGC} GREATER 1)
        set(DIR_NAME ${ARGV1})
    endif()

    if (NOT EXISTS ${THIRD_PARTY_DIR}/${DIR_NAME}/CMakeLists.txt)
        message(WARNING "Third-party library directory ${THIRD_PARTY_DIR}/${DIR_NAME} does not contain a CMakeLists.txt file")
        return()
    endif()
    
    if (NOT TARGET ${NAME})
        add_subdirectory(${THIRD_PARTY_DIR}/${DIR_NAME} ${CMAKE_BINARY_DIR}/third_party/${DIR_NAME} EXCLUDE_FROM_ALL)
        
        if (VOSTOK_ENABLE_CLANG_TIDY AND TARGET ${NAME})
            set_target_properties(${NAME} PROPERTIES CXX_CLANG_TIDY "")
        endif()
        
        if (TARGET ${NAME})
            get_target_property(INCLUDES ${NAME} INTERFACE_INCLUDE_DIRECTORIES)
            if(INCLUDES)
                set_target_properties(${NAME} PROPERTIES 
                    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${INCLUDES}")
            endif()
        endif()
    endif()
endfunction()

if(EXISTS ${THIRD_PARTY_DIR}/glfw/CMakeLists.txt)
    add_third_party_library(glfw)
    set_target_properties(glfw PROPERTIES
        GLFW_BUILD_DOCS OFF
        GLFW_BUILD_TESTS OFF
        GLFW_BUILD_EXAMPLES OFF
        GLFW_INSTALL OFF
    )
else()
    message(WARNING "GLFW directory not found, please clone glfw in ${THIRD_PARTY_DIR}/glfw")
endif()

if(EXISTS ${THIRD_PARTY_DIR}/spdlog/CMakeLists.txt)
    add_third_party_library(spdlog)
    set_target_properties(spdlog PROPERTIES
        SPDLOG_BUILD_EXAMPLE OFF
        SPDLOG_INSTALL OFF
        SPDLOG_FMT_EXTERNAL OFF
    )
else()
    message(WARNING "spdlog directory not found, please clone spdlog in ${THIRD_PARTY_DIR}/spdlog")
endif()

if(EXISTS ${THIRD_PARTY_DIR}/gtest/CMakeLists.txt)
    add_third_party_library(gtest gtest)
    set_target_properties(gtest PROPERTIES
        GTEST_HAS_PTHREAD OFF
        BUILD_GMOCK ON
        INSTALL_GTEST OFF
    )

    if(TARGET gtest AND VOSTOK_ENABLE_CLANG_TIDY)
        set_target_properties(gtest PROPERTIES CXX_CLANG_TIDY "")
        set_target_properties(gtest_main PROPERTIES CXX_CLANG_TIDY "")
        set_target_properties(gmock PROPERTIES CXX_CLANG_TIDY "")
        set_target_properties(gmock_main PROPERTIES CXX_CLANG_TIDY "")
    endif()
else()
    message(WARNING "Google Test directory not found, please clone googletest in ${THIRD_PARTY_DIR}/gtest")
endif()

find_package(glm CONFIG QUIET)
if(NOT glm_FOUND)
    message(STATUS "GLM not found via find_package, adding manually")
    if(EXISTS ${THIRD_PARTY_DIR}/glm/glm/glm.hpp)
        add_library(glm INTERFACE)
        target_include_directories(glm INTERFACE 
            $<BUILD_INTERFACE:${THIRD_PARTY_DIR}/glm>
            $<INSTALL_INTERFACE:include>
        )
        target_compile_definitions(glm INTERFACE
            GLM_FORCE_DEPTH_ZERO_TO_ONE
            GLM_FORCE_LEFT_HANDED
            GLM_FORCE_RADIANS
        )

        add_library(glm::glm ALIAS glm)
        
        message(STATUS "GLM added manually from third_party: ${THIRD_PARTY_DIR}/glm")
    else()
        message(FATAL_ERROR "GLM headers not found in ${THIRD_PARTY_DIR}/glm/glm/glm.hpp")
    endif()
else()
    message(STATUS "GLM found via find_package")
endif()

find_package(Vulkan REQUIRED)

if(EXISTS ${THIRD_PARTY_DIR}/volk/CMakeLists.txt)
    add_third_party_library(volk)
    set_target_properties(volk PROPERTIES
        VOLK_STATIC_DEFINES VK_NO_PROTOTYPES
        VOLK_PULL_IN_VULKAN OFF
    )
else()
    message(WARNING "volk directory not found, please clone volk in ${THIRD_PARTY_DIR}/volk")
endif()

if(EXISTS ${THIRD_PARTY_DIR}/vma/CMakeLists.txt)
    add_third_party_library(VulkanMemoryAllocator vma)
    set_target_properties(VulkanMemoryAllocator PROPERTIES
        VMA_STATIC_VULKAN_FUNCTIONS OFF
        VMA_DYNAMIC_VULKAN_FUNCTIONS ON
    )
else()
    message(WARNING "VulkanMemoryAllocator directory not found, please clone VMA in ${THIRD_PARTY_DIR}/vma")
endif()

set(EXTERNAL_LIBS
    glfw
    spdlog::spdlog
    volk
    VulkanMemoryAllocator
    glm::glm
)

set(VOSTOK_SOURCES "")
set(VOSTOK_HEADERS "")

function(add_vostok_module MODULE_NAME)
    file(GLOB_RECURSE MODULE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/${MODULE_NAME}/*.cpp")
    file(GLOB_RECURSE MODULE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/vostok/${MODULE_NAME}/*.hpp")
    
    set(VOSTOK_SOURCES ${VOSTOK_SOURCES} ${MODULE_SOURCES} PARENT_SCOPE)
    set(VOSTOK_HEADERS ${VOSTOK_HEADERS} ${MODULE_HEADERS} PARENT_SCOPE)
endfunction()

add_vostok_module(core)
add_vostok_module(window)
add_vostok_module(graphics)

add_library(vostok STATIC "")
target_sources(vostok 
    PRIVATE 
        ${VOSTOK_SOURCES}
    PUBLIC
        ${VOSTOK_HEADERS}
)

target_compile_options(vostok PRIVATE 
    $<$<CONFIG:Release>:${VOSTOK_OPTIMIZATION_FLAGS}> 
    ${VOSTOK_WARNING_FLAGS}
)

target_include_directories(vostok
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include/vostok
)

target_link_libraries(vostok
    PUBLIC
        ${EXTERNAL_LIBS}
)

target_compile_definitions(vostok
    PRIVATE
        $<$<CONFIG:Debug>:VOSTOK_DEBUG>
        $<$<BOOL:${VOSTOK_ENABLE_VALIDATION}>:VOSTOK_ENABLE_VALIDATION>
        VK_NO_PROTOTYPES
        VOSTOK_HAS_GLFW
        VOSTOK_HAS_VULKAN
)

if(VOSTOK_BUILD_TESTS AND TARGET gtest)
    enable_testing()

    if(VOSTOK_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(vostok PRIVATE --coverage)
        target_link_libraries(vostok PRIVATE --coverage)
    endif()

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
        add_subdirectory(tests)
    else()
        message(WARNING "Tests directory not found or does not contain CMakeLists.txt")
    endif()
endif()

if(VOSTOK_BUILD_EXAMPLES AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/exemples/CMakeLists.txt)
    add_subdirectory(exemples)
endif()

include(GNUInstallDirs)
install(TARGETS vostok
    EXPORT vostok-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})