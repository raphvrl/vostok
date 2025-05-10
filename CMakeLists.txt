cmake_minimum_required(VERSION 3.25)
project(vostok VERSION 0.1.0 LANGUAGES CXX C)

option(VOSTOK_ENABLE_VALIDATION "Enable Vulkan validation layers" ON)
option(VOSTOK_TREAT_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(VOSTOK_ENABLE_CLANG_TIDY "Enable clang-tidy checks" ON)
option(VOSTOK_BUILD_EXAMPLES "Build Vostok examples" ON)

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

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address,undefined")
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
    Vulkan::Vulkan
    volk
    VulkanMemoryAllocator
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

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/CMakeLists.txt)
    add_subdirectory(shaders)
else()
    message(WARNING "Shaders directory not found or does not contain CMakeLists.txt")
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