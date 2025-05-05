cmake_minimum_required(VERSION 3.16)
project(vostok VERSION 0.1.0 LANGUAGES CXX C)

option(VOSTOK_ENABLE_VALIDATION "Enable Vulkan validation layers" ON)
option(VOSTOK_TREAT_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(VOSTOK_ENABLE_CLANG_TIDY "Enable clang-tidy checks" ON)
option(VOSTOK_BUILD_EXAMPLES "Build Vostok examples" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
    if(VOSTOK_TREAT_WARNINGS_AS_ERRORS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
    endif()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wcast-qual")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    if(VOSTOK_TREAT_WARNINGS_AS_ERRORS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
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

if (NOT TARGET glfw)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(${THIRD_PARTY_DIR}/glfw EXCLUDE_FROM_ALL)
endif()

if (NOT TARGET spdlog)
    set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
    set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
    set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)

    add_subdirectory(${THIRD_PARTY_DIR}/spdlog EXCLUDE_FROM_ALL)

    if (VOSTOK_ENABLE_CLANG_TIDY)
        set_target_properties(spdlog PROPERTIES CXX_CLANG_TIDY "")
        if (TARGET fmt)
            set_target_properties(fmt PROPERTIES CXX_CLANG_TIDY "")
        endif()
    endif()
endif()

if (TARGET spdlog)
    get_target_property(SPDLOG_INTERFACE_INCLUDES spdlog INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(spdlog PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${SPDLOG_INTERFACE_INCLUDES}")
endif()

if (TARGET glfw)
    get_target_property(GLFW_INTERFACE_INCLUDES glfw INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(glfw PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${GLFW_INTERFACE_INCLUDES}")
endif()

find_package(Vulkan REQUIRED)

set(EXTERNAL_LIBS
    glfw
    spdlog::spdlog
    Vulkan::Vulkan
)

file(GLOB_RECURSE VOSTOK_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/vostok/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/vostok/*.inl"
)

set_source_files_properties(${VOSTOK_HEADERS} PROPERTIES HEADER_FILE_ONLY TRUE)

file(GLOB_RECURSE VOSTOK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(vostok STATIC ${VOSTOK_SOURCES} ${VOSTOK_HEADERS})

target_include_directories(vostok
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
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
)

add_subdirectory(shaders)

if(VOSTOK_BUILD_EXAMPLES)
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